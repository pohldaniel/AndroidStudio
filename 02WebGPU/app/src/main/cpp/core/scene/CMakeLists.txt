cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(SCENE_SOURCES Node.cpp BaseNode.cpp SceneNode.cpp CollisionNode.cpp)
add_library(scene STATIC ${SCENE_SOURCES})

set(SCENE_HEADERS Node.h BaseNode.h SceneNode.h CollisionNode.h)
target_include_directories(scene PRIVATE ${SCENE_HEADERS})