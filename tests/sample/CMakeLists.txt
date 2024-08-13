
cmake_minimum_required(VERSION 3.15.0)

project(ridehal-sample LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_CXX_FLAGS "-Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/ridehal")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=/ridehal")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs)
add_subdirectory(source)

add_executable( RideHalSampleApp main.cpp )
target_link_libraries( RideHalSampleApp RideHalSample )
install(TARGETS RideHalSampleApp DESTINATION bin)

