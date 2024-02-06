# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

add_executable( gtest_ComponentIF gtest_ComponentIF.cpp )
target_link_libraries( gtest_ComponentIF GTest::Main Ride::Hal::Component::Common )
install(TARGETS gtest_ComponentIF DESTINATION bin)
