cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(CORE_SOURCES Camera.cpp Transform.cpp)
add_library(core STATIC ${CORE_SOURCES})

set(CORE_HEADERS Camera.h Transform.h TrackBall.h)
target_include_directories(core PRIVATE ${CORE_HEADERS})