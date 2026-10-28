#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <chrono>
#include <ctime>

#include "RenderThread.h"
#include "AssetIO.h"
#include "InputTouch.h"
#include "Globals.h"

#include <WebGPU/WgpContext.h>
#include <WebGPU/WgpTexture.h>

#include <States/StateMachine.h>
#include <States/Collada.h>
#include <States/Wireframe.h>
#include <States/DeferredRendering.h>
#include <States/ComputeParticleLogo.h>
#include <States/VolumeRendering.h>
#include <States/AudioDecode.h>
#include <States/VideoDecode.h>
#include <States/Cubes.h>
#include <States/Isometric.h>
#include <States/Controller.h>

#include <core/Event.h>
#include <core/GamePad.h>

#include <Sound/SoundDevice.h>
#include <Sound/AudioDecoder.h>
#include <Sound/OpenALPlayer.h>

#include "DeltaClock.h"
#include "Logging.h"


DeltaClock DeltaClock;
RenderThread* renderThread = nullptr;
StateMachine* stateMachine= nullptr;
std::unique_ptr<Physics> Globals::physics = nullptr;
States currentState = States::COLLADA;

State* recoverState(States crrntStt){
    switch(crrntStt){
        case States::COLLADA:
            return new Collada(*stateMachine);
        case WIREFRAME:
            return new Wireframe(*stateMachine);
        case DEFERRED_RENDERING:
            return new DeferredRendering(*stateMachine);
        case COMPUTE_PARTICLE_LOGO:
            return new ComputeParticleLogo(*stateMachine);
        case VOLUME_RENDERING:
            return new VolumeRendering(*stateMachine);
        case States::AUDIO_DECODE:
            return new AudioDecode(*stateMachine);
        case States::VIDEO_DECODE:
            return new VideoDecode(*stateMachine);
        case States::CUBES:
            return new Cubes(*stateMachine);
        case States::ISOMETRIC:
            return new Isometric(*stateMachine);
        case States::CONTROLLER:
            return new Controller(*stateMachine);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpInit(JNIEnv* env, jclass clazz, jobject assetManager) {
    AssetIO::Init(AAssetManager_fromJava(env, assetManager));
    SoundDevice::Init();
    AndroidGamepad::Initialize(env);
    Globals::physics = std::make_unique<Physics>();

    wgpInit();
    stateMachine = new StateMachine( DeltaClock.ReadDelta(), DeltaClock.ReadFixedDelta());
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_initStates(JNIEnv* env, jclass clazz){
    if(!stateMachine->isRunning())
        stateMachine->addStateAtTop(recoverState(currentState));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpConfigureSurface(JNIEnv* env, jclass clazz, jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    wgpConfigureSurface(static_cast<void*>(window));
    ANativeWindow_release(window);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_resize(JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    wgpResize((void*)ANativeWindow_fromSurface(env, surface), width, height);
    if(stateMachine->isRunning())
        stateMachine->getStates().top()->resize(0, 0);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_onResize(JNIEnv* env, jclass clazz, jint width, jint height) {
    renderThread->pause();
    wgpResizeSilent(width, height);
    if(stateMachine->isRunning())
        stateMachine->getStates().top()->resize(0, 0);
    renderThread->resume();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_start(JNIEnv* env, jclass clazz, jobject surface) {
    if (renderThread == nullptr) {
        DeltaClock.SetMaxDelta(0.05f);
        DeltaClock.Reset();
        renderThread = new RenderThread(DeltaClock, *stateMachine);
        renderThread->start();
    }
    renderThread->setWindow(ANativeWindow_fromSurface(env, surface));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_stop(JNIEnv *env, jclass clazz) {
    if (renderThread != nullptr) {
        ANativeWindow* window = renderThread->getWindow();

        renderThread->setWindow(nullptr);
        renderThread->stop();

       delete renderThread;
       renderThread = nullptr;

       ANativeWindow_release(window);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_destroy(JNIEnv *env, jclass clazz) {
    wgpShutDown();
    SoundDevice::ShutDown();
    AndroidGamepad::Shutdown();
    delete stateMachine;
    stateMachine = nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_OnButton(JNIEnv *env, jclass clazz, jint button) {
    renderThread->pause();
    if(stateMachine->isRunning()) {
        Event event;
        event.type = Event::MOUSEBUTTONDOWN;
        event.data.mouseButton.x = 0;
        event.data.mouseButton.y = 0;
        event.data.mouseButton.button = button == 0 ? Event::MouseButtonEvent::MouseButton::BUTTON_LEFT  :
                                        button == 1 ?  Event::MouseButtonEvent::MouseButton::BUTTON_MIDDLE :
                                        Event::MouseButtonEvent::MouseButton::BUTTON_RIGHT;

        stateMachine->getStates().top()->OnButton(event.data.mouseButton);
        stateMachine->popState();
    }
    renderThread->resume();
    currentState = stateMachine->getCurrentState();
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_webgpu_NativeLibrary_OnTouchEvent(JNIEnv* env, jclass clazz, jint pointer_id, jfloat x, jfloat y, jint action_type) {

    touchStates[pointer_id].touchX = x;
    touchStates[pointer_id].touchY = y;

    if (action_type == 0 || action_type == 1) {
        touchStates[pointer_id].touchPressed = !touchStates[pointer_id].touchActive;
        touchStates[pointer_id].touchActive = true;
    } else if (action_type == 2) {
        touchStates[pointer_id].touchActive = false;
        touchStates[pointer_id].touchPressed = false;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_webgpu_NativeLibrary_OnKey(JNIEnv* env, jclass clazz, jint keyCode, jint action, jint source, jint deviceId) {
    for (int i = 0; i < 4; ++i) {
        AndroidGamepad* player = AndroidGamepad::s_Players[i];
        if (player->getDeviceId() == deviceId && player->isConnected()) {
            GamepadButton btn = AndroidGamepad::MapKeyCode(keyCode);
            player->setButtonState(btn, (action == 0));
            break;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_webgpu_NativeLibrary_OnGenericMotionEvent(JNIEnv* env, jclass clazz,
                                                           jfloat axisX, jfloat axisY, jfloat axisZ,jfloat axisRX, jfloat axisRY, jfloat axisRZ,
                                                           jfloat axisBrake, jfloat axisGas, jfloat hatX, jfloat hatY, jint source, jint deviceId) {
    for (int i = 0; i < 4; ++i) {
        AndroidGamepad* player = AndroidGamepad::s_Players[i];
        if (player->getDeviceId() == deviceId && player->isConnected()) {
            float normRightX = 0.0f;
            float normRightY = 0.0f;
            AndroidGamepad::ApplyRadialDeadzone(axisX, -axisY, normRightX, normRightY);

            player->setRightStickX(normRightX);
            player->setRightStickY(normRightY);

            float rawLeftX = (player->getLayoutType() == GamepadLayout::DINPUT) ? axisZ  : axisRX;
            float rawLeftY = (player->getLayoutType() == GamepadLayout::DINPUT) ? axisRZ : axisRY;
            float normLeftX = 0.0f;
            float normLeftY = 0.0f;
            AndroidGamepad::ApplyRadialDeadzone(rawLeftX, -rawLeftY, normLeftX, normLeftY);

            player->setLeftStickX(normLeftX);
            player->setLeftStickY(normLeftY);

            float axisLT = player->getLayoutType() == GamepadLayout::DINPUT ? axisBrake : axisZ;
            float axisRT = player->getLayoutType() == GamepadLayout::DINPUT ? axisGas : axisRZ;
            player->setLeftTrigger(axisLT);
            player->setRightTrigger(axisRT);
            player->setButtonState(GamepadButton::GAMEPAD_LEFT_TRIGGER, (axisLT == 1.0f));
            player->setButtonState(GamepadButton::GAMEPAD_RIGHT_TRIGGER, (axisRT == 1.0f));

            bool up    = (hatX < -0.5f);
            bool down  = (hatX > 0.5f);
            bool left  = (hatY < -0.5f);
            bool right = (hatY > 0.5f);
            player->setButtonState(GamepadButton::DPAD_LEFT, up);
            player->setButtonState(GamepadButton::DPAD_RIGHT, down);
            player->setButtonState(GamepadButton::DPAD_UP, left);
            player->setButtonState(GamepadButton::DPAD_DOWN, right);

            player->setButtonState(GamepadButton::DPAD_UP_RIGHT, left && down);
            player->setButtonState(GamepadButton::DPAD_DOWN_RIGHT, right && down);
            player->setButtonState(GamepadButton::DPAD_UP_LEFT, left && up);
            player->setButtonState(GamepadButton::DPAD_DOWN_LEFT, right && up);
            break;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_webgpu_NativeLibrary_OnInputDeviceAdded(JNIEnv* env, jclass clazz, jint deviceId, jint layoutType) {
    for (int i = 0; i < 4; ++i) {
        if (AndroidGamepad::s_Players[i]->getDeviceId() == deviceId) return;
    }

    for (int i = 0; i < 4; ++i) {
        if (!AndroidGamepad::s_Players[i]->isConnected()) {
            AndroidGamepad::s_Players[i]->setDeviceId(deviceId);
            AndroidGamepad::s_Players[i]->setConnected(true);
            AndroidGamepad::s_Players[i]->setLayoutType(static_cast<GamepadLayout>(layoutType));
            break;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_android_webgpu_NativeLibrary_OnInputDeviceRemoved(JNIEnv* env, jclass clazz, jint deviceId) {
    for (int i = 0; i < 4; ++i) {
        if (AndroidGamepad::s_Players[i]->getDeviceId() == deviceId) {
            AndroidGamepad::s_Players[i]->reset();
            break;
        }
    }
}