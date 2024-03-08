# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_ComponentC2D gtest_ComponentC2D.cpp )
target_link_libraries( gtest_ComponentC2D GTest::Main Ride::Hal::Component::C2D )
install(TARGETS gtest_ComponentC2D DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_ComponentC2D.cpp
     PARENT_SCOPE )
