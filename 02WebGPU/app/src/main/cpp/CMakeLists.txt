cmake_minimum_required(VERSION 4.1.0)
project(webgpu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SDK/include")
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

add_subdirectory(core)
add_subdirectory(Shape)
add_subdirectory(WebGPU)
add_subdirectory(States)

add_library(native SHARED Native.cpp DeltaClock.cpp RenderThread.cpp AssetIO.cpp)
target_include_directories(native PRIVATE DeltaClock.h RenderThread.h AssetIO.h)
target_link_libraries(native core shape states webgpu freeimage assimp zlib wgpu android log)

target_include_directories(webgpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shape")
target_include_directories(states PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/WebGPU")