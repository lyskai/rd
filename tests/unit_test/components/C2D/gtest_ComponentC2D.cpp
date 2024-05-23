// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/C2D.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void C2DTestNormal( C2D_Config_t *c2dConfig, RideHal_ImageFormat_e outputFormat,
                    uint32_t outputWidth, uint32_t outputHeight )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    char pName[5] = "C2D";
    uint32_t numInputs = c2dConfig->numOfInputs;
    RideHal_SharedBuffer_t inputs[numInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( c2dConfig->inputConfigs[i].inputResolution.width,
                                  c2dConfig->inputConfigs[i].inputResolution.height,
                                  c2dConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( outputWidth, outputHeight, outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, c2dConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoRGB )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

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

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoYUVY )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

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

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoNV12 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

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

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertNV12toUYVY )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 300;
        pC2DConfig->inputConfigs[i].ROI.topY = 300;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoNV12 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoBGR )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_BGR888;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_ConvertNV12toP010 )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_P010;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, BOUND_C2D_ROI )
{
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;

    pC2DConfig->numOfInputs = 1;
    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 0;
        pC2DConfig->inputConfigs[i].ROI.topY = 0;
        pC2DConfig->inputConfigs[i].ROI.width = 0;
        pC2DConfig->inputConfigs[i].ROI.height = 0;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    C2DTestNormal( pC2DConfig, outputFormat, outputWidth, outputHeight );
}

TEST( C2D, SANITY_C2D_RegDeregBuffer )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    RideHal_SharedBuffer_t inputs[pC2DConfig->numOfInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
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

TEST( C2D, FAILURE_C2D_TopXBadArgs )
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
        C2DConfig.inputConfigs[i].ROI.topX = 800;
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
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_TopYBadArgs )
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
        C2DConfig.inputConfigs[i].ROI.topY = 650;
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
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_ROIWidthBadArgs )
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
        C2DConfig.inputConfigs[i].ROI.width = 1000;
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
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_ROIHeightBadArgs )
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
        C2DConfig.inputConfigs[i].ROI.height = 1000;
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
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( C2D, FAILURE_C2D_InputNumError )
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

    ret = C2DObj.Execute( inputs, 2, &output );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_GetSourceSurf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.numOfInputs = 2;
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

    ret = C2DObj.Execute( inputs + 2, C2DConfig.numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_RegInputBufferFormat )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    RideHal_SharedBuffer_t inputs[pC2DConfig->numOfInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  RIDEHAL_IMAGE_FORMAT_RGB888 );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_RegInputBufferRes )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    RideHal_SharedBuffer_t inputs[pC2DConfig->numOfInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( 1000, 1000, pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_DeRegInputBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    RideHal_SharedBuffer_t inputs[pC2DConfig->numOfInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum + 1 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( C2D, FAILURE_C2D_DeRegOutputBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    pC2DConfig->numOfInputs = 1;
    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;
    uint32_t inputBufferNum = 1;
    uint32_t outputBufferNum = 1;

    RideHal_SharedBuffer_t inputs[pC2DConfig->numOfInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        pC2DConfig->inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pC2DConfig->inputConfigs[i].inputResolution.width = 1920;
        pC2DConfig->inputConfigs[i].inputResolution.height = 1080;
        pC2DConfig->inputConfigs[i].ROI.topX = 100;
        pC2DConfig->inputConfigs[i].ROI.topY = 100;
        pC2DConfig->inputConfigs[i].ROI.width = 1080;
        pC2DConfig->inputConfigs[i].ROI.height = 720;

        ret = inputs[i].Allocate( pC2DConfig->inputConfigs[i].inputResolution.width,
                                  pC2DConfig->inputConfigs[i].inputResolution.height,
                                  pC2DConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, outputBufferNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Execute( inputs, pC2DConfig->numOfInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( size_t i = 0; i < pC2DConfig->numOfInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], inputBufferNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = C2DObj.DeregisterOutputBuffers( &output, outputBufferNum + 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

