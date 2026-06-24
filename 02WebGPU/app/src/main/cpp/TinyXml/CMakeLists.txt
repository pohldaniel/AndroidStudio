cmake_minimum_required(VERSION 4.1.0)
project(TinyXml)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(TINY_SOURCES tinyxml.cpp tinystr.cpp tinyxmlerror.cpp tinyxmlparser.cpp) # Setup the list of sources here.
add_library(tiny STATIC ${TINY_SOURCES})

set(TINY_HEADERS tinystr.h tinyxml.h)
target_include_directories(tiny PRIVATE ${TINY_HEADERS})