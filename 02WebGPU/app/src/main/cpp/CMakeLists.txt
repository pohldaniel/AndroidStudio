cmake_minimum_required(VERSION 3.10.2)
project(webgpu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_library(wgpu STATIC IMPORTED)
set_target_properties(wgpu PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libwgpu.a")
set_target_properties(wgpu PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/SDK/include")

add_library(webgpu STATIC WebGPU/WgpContext.cpp)
target_include_directories(webgpu PUBLIC WebGPU/WgpContext.h)
target_link_libraries(webgpu wgpu)

add_library(native SHARED Native.cpp DeltaClock.cpp)
target_include_directories(native PRIVATE DeltaClock.h)
target_link_libraries(native core stb_image android GLESv3 animation tiny webgpu)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/SDK/include")

add_subdirectory(core)
add_subdirectory(animation)
add_subdirectory(stb)
add_subdirectory(TinyXml)