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
set( GTEST_RIDEHAL_LIBS GTest::Main Ride::Hal::BufferManager Ride::Hal::Component::Common Ride::Hal::Component::C2D Ride::Hal::Component::Camera)

add_subdirectory(buffer_manager)
add_subdirectory(components)

add_executable( gtest_RideHal ${GTEST_RIDEHAL_SOURCES} )
target_link_libraries( gtest_RideHal ${GTEST_RIDEHAL_LIBS} )
target_compile_definitions( gtest_RideHal PUBLIC GTEST_RIDEHAL )
target_include_directories( gtest_RideHal PUBLIC ${GTEST_RIDEHAL_INCLUDES} )

install(TARGETS gtest_RideHal DESTINATION bin)
