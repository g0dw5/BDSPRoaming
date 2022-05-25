cmake_minimum_required(VERSION 3.22)
project(RoamingID)

set(CMAKE_CXX_STANDARD 14)

add_executable(RoamingID
        main.cpp
        roaming_finder.cpp roaming_finder.h
        Xoroshiro128Plus8b.h
        definitions.h
        thread_pool.h scope_guard.h
        )
