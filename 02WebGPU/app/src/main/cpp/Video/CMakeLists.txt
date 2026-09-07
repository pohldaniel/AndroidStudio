cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(VIDEO_SOURCES VideoDecoder.cpp RGBADecoder.cpp YUVDecoder.cpp)
add_library(video STATIC ${VIDEO_SOURCES})

set(VIDEO_HEADERS IVideoDecoder.h VideoDecoder.h RGBADecoder.h YUVDecoder.h)
target_include_directories(video PRIVATE ${VIDEO_HEADERS})