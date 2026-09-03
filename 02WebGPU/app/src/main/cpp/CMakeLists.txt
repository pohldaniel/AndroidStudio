cmake_minimum_required(VERSION 4.1.0)
project(webgpu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SDK/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SDK/include/ffmpeg")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_library(wgpu STATIC IMPORTED)
set_target_properties(wgpu PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libwgpu.a")

add_library(assimp STATIC IMPORTED)
set_target_properties(assimp PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libassimp.a")

add_library(freeimage STATIC IMPORTED)
set_target_properties(freeimage PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libfreeimage.a")

add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libzlib.a")

add_library(oboe STATIC IMPORTED)
set_target_properties(oboe PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/liboboe.a")

add_library(openal STATIC IMPORTED)
set_target_properties(openal PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libopenal.a")

add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libavformat.a")

add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libavcodec.a")

add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libswresample.a")

add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libavutil.a")

add_subdirectory(core)
add_subdirectory(Shape)
add_subdirectory(WebGPU)
add_subdirectory(States)
add_subdirectory(Nuklear)
add_subdirectory(Sound)

add_library(native SHARED bullet_store.cpp Native.cpp InputTouch.cpp DeltaClock.cpp RenderThread.cpp AssetIO.cpp)
target_include_directories(native PRIVATE bullet_store.h InputTouch.h DeltaClock.h RenderThread.h AssetIO.h)
target_link_libraries(native core shape states webgpu nuklear sound avformat avcodec swresample avutil freeimage assimp openal oboe zlib wgpu android log)

target_include_directories(webgpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shape")
target_include_directories(states PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/WebGPU")
target_include_directories(states PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Sound")