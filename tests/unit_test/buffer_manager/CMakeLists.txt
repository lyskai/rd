# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_BufferManager gtest_BufferManager.cpp )
target_link_libraries( gtest_BufferManager GTest::Main Ride::Hal::BufferManager )
install(TARGETS gtest_BufferManager DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_BufferManager.cpp
     PARENT_SCOPE )
