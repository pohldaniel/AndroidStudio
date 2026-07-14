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

#include <WebGPU/WgpContext.h>
#include <WebGpu/WgpTexture.h>

#include <States/StateMachine.h>
#include <States/Collada.h>
#include <States/Wireframe.h>
#include <States/DeferredRendering.h>
#include <States/ComputeParticleLogo.h>
#include <States/VolumeRendering.h>
#include <States/BowSimulation.h>

#include <core/Event.h>

#include "DeltaClock.h"
#include "Logging.h"

DeltaClock DeltaClock;
RenderThread* renderThread = nullptr;
StateMachine* stateMachine= nullptr;
States currentState = States::COLLADA;

#define MAX_TOUCH_POINTERS 5

float c_touch_x[MAX_TOUCH_POINTERS] = {0.0f};
float c_touch_y[MAX_TOUCH_POINTERS] = {0.0f};
bool c_touch_active[MAX_TOUCH_POINTERS] = {false};

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
        case States::BOW_SIMULATION:
            return new BowSimulation(*stateMachine);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpInit(JNIEnv* env, jclass clazz, jobject assetManager) {
    AssetIO::Init(AAssetManager_fromJava(env, assetManager));
    wgpInit();
    float fdt = 0.0f;
    stateMachine = new StateMachine( DeltaClock.ReadDelta(), fdt);
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
Java_com_android_webgpu_NativeLibrary_nativeSendTouch(JNIEnv* env, jclass clazz, jint pointer_id, jfloat x, jfloat y, jint action_type) {

    c_touch_x[pointer_id] = x;
    c_touch_y[pointer_id] = y;

    if (action_type == 0 || action_type == 1) {
        c_touch_active[pointer_id] = true;
    } else if (action_type == 2) {
        c_touch_active[pointer_id] = false;
    }
}