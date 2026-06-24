cmake_minimum_required(VERSION 4.1.0)
project(Animation)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(SHAPE_SOURCES Capsule.cpp Cube.cpp Cylinder.cpp Quad.cpp Segment.cpp Sphere.cpp Spiral.cpp Torus.cpp TorusKnot.cpp Shape.cpp)
add_library(shape STATIC ${SHAPE_SOURCES})

set(SHAPE_HEADERS Capsule.h Cube.h Cylinder.h Quad.h Segment.h Sphere.h Spiral.h Torus.h TorusKnot.hp Shape.h)
target_include_directories(shape PRIVATE ${SHAPE_HEADERS})