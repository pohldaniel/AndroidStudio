cmake_minimum_required(VERSION 3.10.2)
project(Animation)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(ANIMATION_SOURCES Render/AnimationShader.cpp ColladaLoader/ColladaLoader.cpp Animator/Animation.cpp Animator/Animator.cpp AnimatedModel/AnimatedMesh.cpp AnimatedModel/AnimatedModel.cpp)
add_library(animation STATIC ${ANIMATION_SOURCES})

set(ANIMATION_HEADERS Render/AnimationShader.h ColladaLoader/ColladaLoader.h Animator/Animation.h Animator/Animator.h AnimatedModel/AnimatedMesh.h AnimatedModel/AnimatedModel.h)
target_include_directories(animation PRIVATE ${ANIMATION_HEADERS})