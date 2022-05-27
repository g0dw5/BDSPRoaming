cmake_minimum_required(VERSION 3.8)
project(RoamingID)

set(CMAKE_CXX_STANDARD 11)

FILE(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)
add_executable(RoamingID ${SOURCE_FILES})

target_link_libraries(RoamingID pthread)
