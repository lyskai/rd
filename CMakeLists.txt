# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

cmake_minimum_required(VERSION 3.15.0)

project(ridehal LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_FLAGS "-Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(source)
add_subdirectory(tests)
