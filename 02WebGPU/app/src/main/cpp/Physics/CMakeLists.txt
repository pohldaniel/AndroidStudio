cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(PHYSICS_SOURCES Physics.cpp DebugDrawer.cpp)
add_library(physics STATIC ${PHYSICS_SOURCES})

set(PHYSICS_HEADERS Physics.h DebugDrawer.h)
target_include_directories(physics PRIVATE ${PHYSICS_HEADERS})