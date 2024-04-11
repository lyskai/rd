# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

add_executable( gtest_Camera gtest_Camera.cpp )
target_include_directories( gtest_Camera PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_Camera gtest RideHalCamera )
install(TARGETS gtest_Camera DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_Camera.cpp
     PARENT_SCOPE )