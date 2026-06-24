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

#include "WebGPU/WgpContext.h"
#include "WebGPU/WgpRenderer.h"

#include "States/StateMachine.h"
#include "States/Wireframe.h"

#include "DeltaClock.h"

DeltaClock mFrameClock;
WgpRenderer* renderer = nullptr;
StateMachine* Machine= nullptr;

void renderFrame(){
    float deltaT = mFrameClock.ReadDelta();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_initStates(JNIEnv* env, jclass clazz){
    float dt = 0.0f; float fdt = 0.0f;
    Machine = new StateMachine(dt, fdt);
    Machine->addStateAtTop(new Wireframe(*Machine));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpInit(JNIEnv* env, jclass clazz) {
    wgpInit();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpConfigureSurface(JNIEnv* env, jclass clazz, jobject surface) {
    wgpConfigureSurface((void*)ANativeWindow_fromSurface(env, surface));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpResize(JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    wgpResize((void*)ANativeWindow_fromSurface(env, surface), width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_start(JNIEnv* env, jclass clazz, jobject surface) {
    if (renderer == nullptr) {
        renderer = new WgpRenderer();
        renderer->start();
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    renderer->setWindow(window);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_stop(JNIEnv *env, jclass clazz) {
    if (renderer != nullptr) {
        renderer->setWindow(nullptr);
    }
}