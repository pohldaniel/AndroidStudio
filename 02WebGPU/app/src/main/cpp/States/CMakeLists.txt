cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(STATES_SOURCES StateMachine.cpp Isometric.cpp BowSimulation.cpp AudioDecode.cpp VolumeRendering.cpp ComputeParticleLogo.cpp DeferredRendering.cpp Wireframe.cpp Collada.cpp)
add_library(states STATIC ${STATES_SOURCES})

set(STATES_HEADERS IStateMachine.h StateMachine.h Isometric.h BowSimulation.h AudioDecode.h VolumeRendering.h ComputeParticleLogo.h DeferredRendering.h Wireframe.h Collada.h)
target_include_directories(states PRIVATE ${STATES_HEADERS})