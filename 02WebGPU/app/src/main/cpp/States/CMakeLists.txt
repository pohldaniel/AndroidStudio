cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(STATES_SOURCES StateMachine.cpp DefferedRendering.cpp Wireframe.cpp Collada.cpp)
add_library(states STATIC ${STATES_SOURCES})

set(STATES_HEADERS IStateMachine.h StateMachine.h DefferedRendering.h Wireframe.h Collada.h)
target_include_directories(states PRIVATE ${STATES_HEADERS})