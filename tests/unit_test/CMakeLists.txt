# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

find_package(GTest REQUIRED)

# Enable GTest
enable_testing()
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
set(GTEST_TIMEOUT 600)
include(GoogleTest)

add_subdirectory(buffer_manager)
add_subdirectory(components)
