# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_CL2DFlex gtest_CL2DFlex.cpp )
target_include_directories( gtest_CL2DFlex PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_CL2DFlex gtest RideHalComponentCommon RideHalCL2DFlex RideHalTestUtils)
install(TARGETS gtest_CL2DFlex DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_CL2DFlex.cpp)
