// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_TEST_MD5_UTILS
#define RIDEHAL_TEST_MD5_UTILS

#include <stdint.h>
#include <string>

namespace ridehal
{
namespace test
{
namespace utils
{

std::string MD5Sum( const void *data, uint32_t length );

}   // namespace utils
}   // namespace test
}   // namespace ridehal
#endif /* RIDEHAL_TEST_MD5_UTILS */