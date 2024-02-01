# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_BufferManager gtest_BufferManager.cpp )
target_link_libraries( gtest_BufferManager GTest::Main Ride::Hal::BufferManager )
