// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "md5_utils.hpp"
#include "ridehal/component/ColorConvertor.hpp"

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

void SanityTest()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ColorConvertor ColorConvertorObj;
    ColorConvertor_Config_t ColorConvertorConfig;
    char pName[20] = "ColorConvertor";

    ColorConvertorConfig.inputWidth = 256;
    ColorConvertorConfig.inputHeight = 256;
    ColorConvertorConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    ColorConvertorConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    RideHal_SharedBuffer_t input;

    ret = input.Allocate( ColorConvertorConfig.inputWidth, ColorConvertorConfig.inputHeight,
                          ColorConvertorConfig.inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( ColorConvertorConfig.inputWidth, ColorConvertorConfig.inputHeight,
                           ColorConvertorConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.Init( pName, &ColorConvertorConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.Execute( &input, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ColorConvertorObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = output.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    return;
}


TEST( ColorConvertor, SanityTest )
{
    SanityTest();
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif