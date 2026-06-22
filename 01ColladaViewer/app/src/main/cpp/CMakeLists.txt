cmake_minimum_required(VERSION 3.10.2)
project(collada)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_library(native SHARED Native.cpp DeltaClock.cpp)
target_include_directories(native PRIVATE DeltaClock.h)
target_link_libraries(native core animation tiny stb_image android GLESv3 log)

add_subdirectory(core)
add_subdirectory(animation)
add_subdirectory(stb)
add_subdirectory(TinyXml)