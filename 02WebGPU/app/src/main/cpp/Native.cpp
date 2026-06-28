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

#include "AssimpModel.h"
#include "ObjModel.h"

#include "DeltaClock.h"

DeltaClock mFrameClock;
RenderThread* renderThread = nullptr;
StateMachine* Machine= nullptr;

void renderFrame(){
    float deltaT = mFrameClock.ReadDelta();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpInit(JNIEnv* env, jclass clazz, jobject assetManager) {
    AssetIO::Init(AAssetManager_fromJava(env, assetManager));

    /*MemoryIOSystem* memoryFS = new MemoryIOSystem();

    uint8_t* data; uint32_t size;
    AssetIO::LoadAsset("models/dragon/dragon.obj", data, size);
    memoryFS->AddFile("models/dragon/dragon.obj", std::vector<char>{data, data + size});
    AssetIO::Free(data);

    AssetIO::LoadAsset("models/dragon/dragon.mtl", data, size);
    memoryFS->AddFile("models/dragon/dragon.mtl", std::vector<char>{data, data + size});
    AssetIO::Free(data);

    AssimpModel m_assimpModel;
    m_assimpModel.loadModel(memoryFS, "models/dragon/dragon.obj", glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, glm::vec3(0.0f, -1.0f, 0.0f), 0.1f, false, false, false, false, true);

    ObjModel m_objModel;
    m_objModel.loadModel("models/dragon/dragon.obj", glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, glm::vec3(0.0f, -1.0f, 0.0f), 0.1f, false, false, false, false, false, true);*/

    wgpInit();
    float dt = 0.0f; float fdt = 0.0f;
    Machine = new StateMachine(dt, fdt);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_initStates(JNIEnv* env, jclass clazz){
    if(!Machine->isRunning())
        Machine->addStateAtTop(new Wireframe(*Machine));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpConfigureSurface(JNIEnv* env, jclass clazz, jobject surface) {
    wgpConfigureSurface((void*)ANativeWindow_fromSurface(env, surface));
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_wgpResize(JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    wgpResize((void*)ANativeWindow_fromSurface(env, surface), width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_android_webgpu_NativeLibrary_start(JNIEnv* env, jclass clazz, jobject surface) {
    if (renderThread == nullptr) {
        renderThread = new RenderThread();
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