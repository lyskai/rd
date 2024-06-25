// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/GLConverter.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void GLConverterTestNormal( GLConverter_Config_t *pConfig, RideHal_ImageFormat_e outputFormat,
                            uint32_t outputWidth, uint32_t outputHeight )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    GLConverter GLConverterObj;
    char pName[12] = "GLConverter";
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

    ret = output.Allocate( outputWidth, outputHeight, outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GLConverterObj.Init( pName, pConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GLConverterObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GLConverterObj.Execute( inputs, numInputs, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GLConverterObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = GLConverterObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( GLConverter, SANITY_ConvertUYVYtoRGB )
{
    GLConverter_Config_t GLConverterConfig;
    GLConverter_Config_t *pConfig = &GLConverterConfig;

    GLConverterConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GLConverterConfig.numOfInputs; i++ )
    {
        GLConverterConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GLConverterConfig.inputConfigs[i].inputResolution.width = 600;
        GLConverterConfig.inputConfigs[i].inputResolution.height = 600;
        GLConverterConfig.inputConfigs[i].ROI.topX = 100;
        GLConverterConfig.inputConfigs[i].ROI.topY = 100;
        GLConverterConfig.inputConfigs[i].ROI.width = 100;
        GLConverterConfig.inputConfigs[i].ROI.height = 100;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}

TEST( GLConverter, SANITY_ConvertRGBtoYUVY )
{
    GLConverter_Config_t GLConverterConfig;
    GLConverter_Config_t *pConfig = &GLConverterConfig;

    GLConverterConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GLConverterConfig.numOfInputs; i++ )
    {
        GLConverterConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
        GLConverterConfig.inputConfigs[i].inputResolution.width = 1000;
        GLConverterConfig.inputConfigs[i].inputResolution.height = 1000;
        GLConverterConfig.inputConfigs[i].ROI.topX = 150;
        GLConverterConfig.inputConfigs[i].ROI.topY = 150;
        GLConverterConfig.inputConfigs[i].ROI.width = 600;
        GLConverterConfig.inputConfigs[i].ROI.height = 600;
    }

    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t outputWidth = 600;
    uint32_t outputHeight = 600;

    C2DTestNormal( pConfig, outputFormat, outputWidth, outputHeight );
}
