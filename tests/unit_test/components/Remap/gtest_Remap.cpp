// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <chrono>
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

    RemapConfig.processor = RIDE_HAL_PROCESSOR_HTP0;
    RemapConfig.numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        RemapConfig.inputConfigs[inputId].inputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
        RemapConfig.inputConfigs[inputId].inputWidth = 3840;
        RemapConfig.inputConfigs[inputId].inputHeight = 2160;
        RemapConfig.inputConfigs[inputId].mapWidth = 1024;
        RemapConfig.inputConfigs[inputId].mapHeight = 768;
        RemapConfig.inputConfigs[inputId].ROI.x = 0;
        RemapConfig.inputConfigs[inputId].ROI.y = 0;
        RemapConfig.inputConfigs[inputId].ROI.width = 1024;
        RemapConfig.inputConfigs[inputId].ROI.height = 768;
    }
    RemapConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    RemapConfig.outputWidth = 1024;
    RemapConfig.outputHeight = 768;
    RemapConfig.bEnableUndistortion = false;
    RemapConfig.bEnableNormalize = false;
    RemapConfig.normlzR.sub = 0.0;
    RemapConfig.normlzR.mul = 1.0;
    RemapConfig.normlzR.add = 0.0;
    RemapConfig.normlzG.sub = 0.0;
    RemapConfig.normlzG.mul = 1.0;
    RemapConfig.normlzG.add = 0.0;
    RemapConfig.normlzB.sub = 0.0;
    RemapConfig.normlzB.mul = 1.0;
    RemapConfig.normlzB.add = 0.0;

    RideHal_SharedBuffer_t inputs[RemapConfig.numOfInputs];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    size_t inputSize[RIDE_HAL_MAX_INPUTS];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        if ( RemapConfig.inputConfigs[inputId].inputFormat == RIDE_HAL_IMAGE_FORMAT_UYVY )
        {
            inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                 RemapConfig.inputConfigs[inputId].inputHeight * 2;
        }
        else if ( RemapConfig.inputConfigs[inputId].inputFormat == RIDE_HAL_IMAGE_FORMAT_RGB888 )
        {
            inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                 RemapConfig.inputConfigs[inputId].inputHeight * 3;
        }
    }

    printf( "inputData is: \n" );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        uint8_t *inputData = (uint8_t *) inputs[inputId].data();
        for ( int i = 0; i < inputSize[inputId]; i++ )
        {
            inputData[i] = i % 256;
        }
        for ( int i = 0; i < 10; i++ )
        {
            printf( "inputId = %d, i = %d, data = %d \n", inputId, i, inputData[i] );
        }
    }


    RideHal_SharedBuffer_t output;
    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    size_t outputSize = RemapConfig.outputWidth * RemapConfig.outputHeight * 3;
    uint8_t *outputData = (uint8_t *) output.data();

    ret = RemapObj.Init( pName, pRemapConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.RegBuf( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_IN );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.RegBuf( &output, 1, FADAS_BUF_TYPE_OUT );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    uint32_t times = 1;
    auto start = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < times; i++ )
    {
        ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
    }
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
    printf( "execute time = %f\n", (float) duration_ms / (float) times );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.DeregBuf( inputs, RemapConfig.numOfInputs );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = RemapObj.DeregBuf( &output, 1 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    printf( "outputData is: \n" );
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        for ( int i = 0; i < 10; i++ )
        {
            printf( "inputId = %d, i = %d, data = %d \n", inputId, i,
                    outputData[inputId * outputSize + i] );
        }
    }

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