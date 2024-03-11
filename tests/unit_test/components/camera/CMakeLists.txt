# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

add_executable( gtest_QcarCam gtest_QcarCam.cpp )
target_include_directories( gtest_QcarCam PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_QcarCam GTest::Main RideHalCamera )
install(TARGETS gtest_QcarCam DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_QcarCam.cpp
     PARENT_SCOPE )

set( GTEST_RIDEHAL_LIBS ${GTEST_RIDEHAL_LIBS}
     RideHalCamera
     PARENT_SCOPE )