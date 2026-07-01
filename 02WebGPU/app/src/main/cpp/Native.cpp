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

#include "WebGPU/WgpContext.h"
#include "WebGpu/WgpTexture.h"

#include "States/StateMachine.h"
#include "States/Wireframe.h"
#include "States/Collada.h"

#include "AssimpModel.h"
#include "ObjModel.h"

#include "DeltaClock.h"
#include "Logging.h"

DeltaClock mFrameClock;
RenderThread* renderThread = nullptr;
StateMachine* Machine= nullptr;

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpInit(JNIEnv* env, jclass clazz, jobject assetManager) {
    AssetIO::Init(AAssetManager_fromJava(env, assetManager));
    wgpInit();
    float fdt = 0.0f;
    Machine = new StateMachine( mFrameClock.ReadDelta(), fdt);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_initStates(JNIEnv* env, jclass clazz){
    if(!Machine->isRunning())
        Machine->addStateAtTop(new Collada(*Machine));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpConfigureSurface(JNIEnv* env, jclass clazz, jobject surface) {
    wgpConfigureSurface((void*)ANativeWindow_fromSurface(env, surface));
    if(Machine->isRunning())
        Machine->getStates().top()->resize(0, 0);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_resize(JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    wgpResize((void*)ANativeWindow_fromSurface(env, surface), width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_start(JNIEnv* env, jclass clazz, jobject surface) {
    if (renderThread == nullptr) {
        mFrameClock.SetMaxDelta(0.05f);
        mFrameClock.Reset();
        renderThread = new RenderThread(mFrameClock, *Machine);
        renderThread->start();
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    renderThread->setWindow(window);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_stop(JNIEnv *env, jclass clazz) {
    if (renderThread != nullptr) {
        renderThread->setWindow(nullptr);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_destroy(JNIEnv *env, jclass clazz) {
    wgpShutDown();
    delete Machine;
    delete renderThread;
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_OnButton(JNIEnv *env, jclass clazz) {
    renderThread->pause();
    if(Machine->isRunning()) {
        Machine->getStates().top()->OnButton();
        Machine->popState();
    }

    renderThread->resume();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_OnAction(JNIEnv *env, jclass clazz) {
    StateMachine::ToggleWireframe();
}