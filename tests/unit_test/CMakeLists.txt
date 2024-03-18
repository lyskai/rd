# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

find_package(GTest REQUIRED)

# Enable GTest
enable_testing()
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
set(GTEST_TIMEOUT 600)
include(GoogleTest)

set( GTEST_RIDEHAL_SOURCES gtest_RideHal.cpp )
set( GTEST_RIDEHAL_INCLUDES ${PROJECT_SOURCE_DIR}/include )
set( GTEST_RIDEHAL_LIBS GTest::Main RideHal )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    add_link_options("-lregex") # gtest main need this
endif()

add_subdirectory(buffer)
add_subdirectory(components)

add_executable( gtest_RideHal ${GTEST_RIDEHAL_SOURCES} )
target_link_libraries( gtest_RideHal ${GTEST_RIDEHAL_LIBS} )
target_compile_definitions( gtest_RideHal PUBLIC GTEST_RIDEHAL )
target_include_directories( gtest_RideHal PUBLIC ${GTEST_RIDEHAL_INCLUDES} )

install(TARGETS gtest_RideHal DESTINATION bin)
