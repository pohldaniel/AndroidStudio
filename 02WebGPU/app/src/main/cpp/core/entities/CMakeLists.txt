cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(ENTITIES_SOURCES Entity.cpp CollisionEntity.cpp Enemy.cpp Player.cpp)
add_library(entities STATIC ${ENTITIES_SOURCES})

set(ENTITIES_HEADERS Entity.h CollisionEntity.h Enemy.h Player.h)
target_include_directories(entities PRIVATE ${ENTITIES_HEADERS})