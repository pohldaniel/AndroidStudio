cmake_minimum_required(VERSION 4.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

set(SOUND_SOURCES SoundDevice.cpp AudioDecoder.cpp OpenALPlayer.cpp OboePlayer.cpp)
add_library(sound STATIC ${SOUND_SOURCES})

set(SOUND_HEADERS SoundDevice.h AudioDecoder.h OpenALPlayer.h OboePlayer.h AudioRingBuffer.h IAudioOutput.h)
target_include_directories(sound PRIVATE ${SOUND_HEADERS})