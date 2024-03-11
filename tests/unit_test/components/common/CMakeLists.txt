# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_ComponentIF gtest_ComponentIF.cpp )
target_link_libraries( gtest_ComponentIF GTest::Main Ride::Hal::Component::Common )
install(TARGETS gtest_ComponentIF DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_ComponentIF.cpp
     PARENT_SCOPE )
