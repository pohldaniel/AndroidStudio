cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(THREAD_SOURCES LoadingManager.cpp Thread.cpp)
add_library(thread STATIC ${THREAD_SOURCES})

set(THREAD_HEADERS ThreadPool.h ILoadingListener.h Thread.h LoadingManager.h)
target_include_directories(thread PRIVATE ${THREAD_HEADERS})