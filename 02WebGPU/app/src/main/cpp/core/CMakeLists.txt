cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(CORE_SOURCES BinaryIO.cpp AssimpModel.cpp ObjModel.cpp Model.cpp Mesh.cpp Material.cpp Camera.cpp Transform.cpp)
add_library(core STATIC ${CORE_SOURCES})

set(CORE_HEADERS BinaryIO.h AssimpModel.h ObjModel.h Model.h Mesh.h Material.h Camera.h Transform.h TrackBall.h)
target_include_directories(core PRIVATE ${CORE_HEADERS})

target_link_libraries(core animation)

add_subdirectory(animation)