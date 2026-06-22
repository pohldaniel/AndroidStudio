cmake_minimum_required(VERSION 3.10.2)
project(Animation)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(CORE_SOURCES MeshCube.cpp Vector.cpp Camera.cpp ModelMatrix.cpp Texture.cpp Shader.cpp)
add_library(core STATIC ${CORE_SOURCES})

set(CORE_HEADERS MeshCube.h Vector.h Camera.h ModelMatrix.h Texture.h Shader.h)
target_include_directories(core PRIVATE ${CORE_HEADERS})