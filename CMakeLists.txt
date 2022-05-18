cmake_minimum_required(VERSION 3.22)
project(RoamingID)

set(CMAKE_CXX_STANDARD 14)

add_executable(RoamingID main.cpp Xoroshiro128Plus8b.h roaming_finder.cpp roaming_finder.h definitions.h)
