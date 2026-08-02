cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(NUKLEAR_SOURCES NkContext.cpp NkStyle.cpp NkJoystick.cpp)
add_library(nuklear STATIC ${NUKLEAR_SOURCES})

set(NUKLEAR_HEADERS NkContext.h StateMachine.h NkStyle.h NkJoystick.h)
target_include_directories(nuklear PRIVATE ${NUKLEAR_HEADERS})