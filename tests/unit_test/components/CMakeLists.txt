# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_subdirectory(C2D)
add_subdirectory(Camera)
add_subdirectory(common)
add_subdirectory(PointPillar)
add_subdirectory(VideoEncoder)
add_subdirectory(Remap)
add_subdirectory(QnnRuntime)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES} PARENT_SCOPE )
set( GTEST_RIDEHAL_INCLUDES ${GTEST_RIDEHAL_INCLUDES} PARENT_SCOPE )
set( GTEST_RIDEHAL_LIBS ${GTEST_RIDEHAL_LIBS} PARENT_SCOPE )
