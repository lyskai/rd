// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/GL2DFlex.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void GL2DFlexTestNormal( GL2DFlex_Config_t *pConfig, RideHal_ImageFormat_e outputFormat,
                         uint32_t outputWidth, uint32_t outputHeight )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    GL2DFlex GL2DFlexObj;
    char pName[12] = "GL2DFlex";
    uint32_t numInputs = pConfig->numOfInputs;
    RideHal_SharedBuffer_t inputs[numInputs];
    RideHal_SharedBuffer_t output;

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( pConfig->inputConfigs[i].inputResolution.width,
                                  pConfig->inputConfigs[i].inputResolution.height,
                                  pConfig->inputConfigs[i].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    pConfig->outputFormat = outputFormat;
    pConfig->outputResolution.width = outputWidth;
    pConfig->outputResolution.height = outputHeight;

    ret = output.Allocate( outputWidth, outputHeight, pConfig->outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Init( pName, pConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( GL2DFlex, SANITY_ConvertNV12toRGB )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertRGBtoNV12 )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertUYVYtoNV12 )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GL2DFlex, SANITY_ConvertUYVYtoRGB )
{
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 200;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
