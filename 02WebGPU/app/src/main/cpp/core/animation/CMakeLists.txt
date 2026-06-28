cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(ANIMATION_SOURCES AnimatedModel.cpp Animation.cpp AnimationState.cpp Bone.cpp BoneDescription.cpp)
add_library(animation STATIC ${ANIMATION_SOURCES})

set(ANIMATION_HEADERS AnimatedModel.h Animation.h AnimationState.h Bone.h BoneDescription.h)
target_include_directories(animation PRIVATE ${ANIMATION_HEADERS})