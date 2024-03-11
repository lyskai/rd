# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
find_package(vidc)
add_executable( gtest_VideoEncoder gtest_VideoEncoder.cpp )
target_link_libraries( 
     gtest_VideoEncoder
     GTest::Main
     Ride::Hal::Component::Common
     Ride::Hal::Component::VideoEncoder
     Ride::Hal::BufferManager
     vidc
)

install(TARGETS gtest_VideoEncoder DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_VideoEncoder.cpp
     PARENT_SCOPE )

set( GTEST_RIDEHAL_LIBS ${GTEST_RIDEHAL_LIBS} 
     Ride::Hal::Component::VideoEncoder
     PARENT_SCOPE )

