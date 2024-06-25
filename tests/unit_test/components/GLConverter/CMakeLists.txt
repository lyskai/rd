# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

add_executable( gtest_ComponentGLConverter gtest_ComponentGLConverter.cpp )
target_link_libraries( gtest_ComponentGLConverter gtest RideHalGLConverter )
install(TARGETS gtest_ComponentGLConverter DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_ComponentGLConverter.cpp
     PARENT_SCOPE )
