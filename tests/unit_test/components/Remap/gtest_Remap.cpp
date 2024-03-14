// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/Remap.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

TEST( Remap, SANITY_RemapGeneral )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    Remap_Config_t *pRemapConfig = &RemapConfig;
    char pName[10] = "Remap";

    RemapConfig.processor = REMAP_PROCESSOR_CPU;
    RemapConfig.numOfInputs = 1;
    RemapConfig.inputConfigs[0].inputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    RemapConfig.inputConfigs[0].inputWidth = 200;
    RemapConfig.inputConfigs[0].inputHeight = 200;
    RemapConfig.inputConfigs[0].ROI.x = 0;
    RemapConfig.inputConfigs[0].ROI.y = 0;
    RemapConfig.inputConfigs[0].ROI.width = 200;
    RemapConfig.inputConfigs[0].ROI.height = 200;
    RemapConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    RemapConfig.outputWidth = 100;
    RemapConfig.outputHeight = 100;
    RemapConfig.bEnableUndistortion = false;
    RemapConfig.bEnableNormalize = false;

    RideHal_SharedBuffer_t inputs[RemapConfig.numOfInputs];
    ret = inputs[0].Allocate( RemapConfig.inputConfigs[0].inputWidth,
                              RemapConfig.inputConfigs[0].inputHeight,
                              RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    size_t inputSize =
            RemapConfig.inputConfigs[0].inputWidth * RemapConfig.inputConfigs[0].inputHeight;
    uint8_t *inputData = (uint8_t *) inputs[0].data();
    for ( int i = 0; i < inputSize; i++ )
    {
        inputData[i] = i;
    }

    RideHal_SharedBuffer_t outputs[1];
    ret = outputs[0].Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                               RemapConfig.outputHeight, RIDE_HAL_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    size_t outputSize =
            RemapConfig.outputWidth * RemapConfig.outputHeight * RemapConfig.numOfInputs;
    uint8_t *outputData = (uint8_t *) outputs[0].data();

    ret = RemapObj.Init( pName, pRemapConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, outputs, 1 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif