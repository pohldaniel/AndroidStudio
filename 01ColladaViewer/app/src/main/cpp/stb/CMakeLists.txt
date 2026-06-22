cmake_minimum_required(VERSION 3.10.2)
project(stb_image)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(STB_IMAGE_SOURCES stb_image.cpp)
add_library(stb_image STATIC ${STB_IMAGE_SOURCES})

set(STB_IMAGE_HEADERS stb_image.h)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_HEADERS})
