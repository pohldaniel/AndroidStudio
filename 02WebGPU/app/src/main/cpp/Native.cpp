
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

#include "core/Camera.h"
#include "core/Shader.h"
#include "Animation/AnimatedModel/AnimatedModel.h"
#include "DeltaClock.h"


Camera camera;
AnimatedModel dragon;
DeltaClock mFrameClock;

bool setupGraphics(int width, int height, AAssetManager* assetManager){

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glViewport(0, 0, width, height);

    Vector3f camPos{ 0, 5, 25.0f };Vector3f target(0.0, 5.0, 0.0);Vector3f up(0.0, 1.0, 0.0);
    camera = Camera(camPos, target, up);
    camera.perspective(25.0f, (GLfloat)width / (GLfloat)height, 1.0f, 2000.0f);

    dragon.loadModel("cowboy.dae", "cowboy.png", assetManager);
    dragon.getAnimator()->startAnimation("");

    mFrameClock.SetMaxDelta(0.05f);
    mFrameClock.Reset();
    return true;
}

void renderFrame(){

    float deltaT = mFrameClock.ReadDelta();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    dragon.update(deltaT);
    dragon.draw(camera);
}

void nativeInit(ANativeWindow* nativeWindow){
    wgpInit((void*)nativeWindow);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_init(JNIEnv * env, jclass clazz,  jint width, jint height, jobject _assetManager){
    setupGraphics(width, height, AAssetManager_fromJava(env, _assetManager));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_step( JNIEnv * env, jclass obj){
    renderFrame();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_initWindow(JNIEnv* env, jclass clazz, jobject surface) {
    nativeInit(ANativeWindow_fromSurface(env, surface));
}