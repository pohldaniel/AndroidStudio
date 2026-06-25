cmake_minimum_required(VERSION 4.1.0)
project(webgpu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_library(wgpu STATIC IMPORTED)
set_target_properties(wgpu PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDK/jniLibs/${ANDROID_ABI}/libwgpu.a")
set_target_properties(wgpu PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/SDK/include")

include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/SDK/include"
)

add_subdirectory(core)
add_subdirectory(stb)
add_subdirectory(TinyXml)
add_subdirectory(Shape)
add_subdirectory(WebGPU)
add_subdirectory(States)

add_library(native SHARED Native.cpp DeltaClock.cpp)
target_include_directories(native PRIVATE DeltaClock.h)
target_link_libraries(native core stb_image tiny webgpu states android GLESv3 wgpu log)

target_include_directories(webgpu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shape")
target_include_directories(states PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/WebGPU")