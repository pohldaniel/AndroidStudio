cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(WEBGPU_SOURCES WgpTexture.cpp WgpBuffer.cpp WgpMesh.cpp WgpModel.cpp WgpBatchRenderer.cpp WgpBatchRenderer.cpp WgpContext.cpp)
add_library(webgpu STATIC ${WEBGPU_SOURCES})

set(WEBGPU_HEADERS WgpTexture.h WgpBuffer.h WgpMesh.h WgpModel.h WgpBatchRenderer.h WgpBatchRenderer.h WgpData.h WgpContext.h)
target_include_directories(webgpu PRIVATE ${WEBGPU_HEADERS})