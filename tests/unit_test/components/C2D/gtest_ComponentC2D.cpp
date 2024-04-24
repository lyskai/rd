// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/C2D.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

TEST( C2D, SANITY_C2D_ConvertUYVYtoRGB )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoYUVY )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
        C2DConfig.inputConfigs[i].inputResolution.width = 1000;
        C2DConfig.inputConfigs[i].inputResolution.height = 1000;
        C2DConfig.inputConfigs[i].ROI.topX = 150;
        C2DConfig.inputConfigs[i].ROI.topY = 150;
        C2DConfig.inputConfigs[i].ROI.width = 600;
        C2DConfig.inputConfigs[i].ROI.height = 600;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoNV12 )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
        C2DConfig.inputConfigs[i].inputResolution.width = 1920;
        C2DConfig.inputConfigs[i].inputResolution.height = 1080;
        C2DConfig.inputConfigs[i].ROI.topX = 200;
        C2DConfig.inputConfigs[i].ROI.topY = 200;
        C2DConfig.inputConfigs[i].ROI.width = 1080;
        C2DConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertNV12toUYVY )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        C2DConfig.inputConfigs[i].inputResolution.width = 1920;
        C2DConfig.inputConfigs[i].inputResolution.height = 1080;
        C2DConfig.inputConfigs[i].ROI.topX = 300;
        C2DConfig.inputConfigs[i].ROI.topY = 300;
        C2DConfig.inputConfigs[i].ROI.width = 1080;
        C2DConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoNV12 )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 1920;
        C2DConfig.inputConfigs[i].inputResolution.height = 1080;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 1080;
        C2DConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_UnInitialized )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        C2DConfig.inputConfigs[i].inputResolution.width = 600;
        C2DConfig.inputConfigs[i].inputResolution.height = 600;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 100;
        C2DConfig.inputConfigs[i].ROI.height = 100;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t C2DOutputWidth = 600;
    uint32_t C2DOutputHeight = 600;

    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                              C2DConfig.inputConfigs[0].inputResolution.height,
                              C2DConfig.inputConfigs[0].inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

