# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_Logger gtest_Logger.cpp )
target_link_libraries( gtest_Logger GTest::Main RideHalCommon )
install(TARGETS gtest_Logger DESTINATION bin)
