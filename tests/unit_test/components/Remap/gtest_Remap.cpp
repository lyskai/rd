// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <chrono>
#include <stdio.h>

#include "ridehal/component/Remap.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void SuccessTest( RideHal_ProcessorType_e processorTest, RideHal_ImageFormat_e inputFormatTest,
                  RideHal_ImageFormat_e outputFormatTest, bool bEnableUndistortionTest,
                  bool bEnableNormalizeTest, bool bCheckAccuracyTest, bool bCheckPerformanceTest )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    Remap_Config_t *pRemapConfig = &RemapConfig;
    char pName[10] = "Remap";

    RemapConfig.processor = processorTest;
    RemapConfig.numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        RemapConfig.inputConfigs[inputId].inputFormat = inputFormatTest;
        RemapConfig.inputConfigs[inputId].inputWidth = 512;
        RemapConfig.inputConfigs[inputId].inputHeight = 512;
        RemapConfig.inputConfigs[inputId].mapWidth = 256;
        RemapConfig.inputConfigs[inputId].mapHeight = 256;
        RemapConfig.inputConfigs[inputId].ROI.x = 0;
        RemapConfig.inputConfigs[inputId].ROI.y = 0;
        RemapConfig.inputConfigs[inputId].ROI.width = 256;
        RemapConfig.inputConfigs[inputId].ROI.height = 256;
    }
    RemapConfig.outputFormat = outputFormatTest;
    RemapConfig.outputWidth = 256;
    RemapConfig.outputHeight = 256;
    RemapConfig.bEnableUndistortion = bEnableUndistortionTest;
    RemapConfig.bEnableNormalize = bEnableNormalizeTest;

    if ( bEnableNormalizeTest == true )
    {
        RemapConfig.normlzR.sub = 0.0;
        RemapConfig.normlzR.mul = 1.0;
        RemapConfig.normlzR.add = 0.0;
        RemapConfig.normlzG.sub = 0.0;
        RemapConfig.normlzG.mul = 1.0;
        RemapConfig.normlzG.add = 0.0;
        RemapConfig.normlzB.sub = 0.0;
        RemapConfig.normlzB.mul = 1.0;
        RemapConfig.normlzB.add = 0.0;
    }

    if ( bEnableUndistortionTest == true )
    {
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            uint32_t mapWidth = RemapConfig.inputConfigs[inputId].mapWidth;
            uint32_t mapHeight = RemapConfig.inputConfigs[inputId].mapHeight;
            uint32_t inputWidth = RemapConfig.inputConfigs[inputId].inputWidth;
            uint32_t inputHeight = RemapConfig.inputConfigs[inputId].inputHeight;
            uint32_t mapSize = mapWidth * mapHeight;
            float mapX[mapSize];
            float mapY[mapSize];
            for ( int i = 0; i < mapHeight; i++ )
            {
                for ( int j = 0; j < mapWidth; j++ )
                {
                    mapX[i * mapWidth + j] = j / mapWidth * inputWidth;
                    mapY[i * mapWidth + j] = i / mapHeight * inputHeight;
                }
            }

            RemapConfig.inputConfigs[inputId].remapTable.pMapX = mapX;
            RemapConfig.inputConfigs[inputId].remapTable.pMapY = mapY;
        }
    }

    RideHal_SharedBuffer_t inputs[RemapConfig.numOfInputs];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    if ( bCheckAccuracyTest == true )
    {
        size_t inputSize[RIDEHAL_MAX_INPUTS];
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            if ( RemapConfig.inputConfigs[inputId].inputFormat == RIDEHAL_IMAGE_FORMAT_UYVY )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 2;
            }
            else if ( RemapConfig.inputConfigs[inputId].inputFormat ==
                      RIDEHAL_IMAGE_FORMAT_RGB888 )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 3;
            }
            else if ( RemapConfig.inputConfigs[inputId].inputFormat == RIDEHAL_IMAGE_FORMAT_NV12 )
            {
                inputSize[inputId] = RemapConfig.inputConfigs[inputId].inputWidth *
                                     RemapConfig.inputConfigs[inputId].inputHeight * 1.5;
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
    }

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.Init( pName, pRemapConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.RegBuf( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_IN );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.RegBuf( &output, 1, FADAS_BUF_TYPE_OUT );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( bCheckPerformanceTest == true )
    {
        uint32_t times = 100;
        auto start = std::chrono::high_resolution_clock::now();
        for ( int i = 0; i < times; i++ )
        {
            ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
        }
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
        printf( "execute time = %f\n", (float) duration_ms / (float) times );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    else
    {
        ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = RemapObj.DeregBuf( inputs, RemapConfig.numOfInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.DeregBuf( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( bCheckAccuracyTest == true )
    {
        printf( "outputData is: \n" );
        size_t outputSize = RemapConfig.outputWidth * RemapConfig.outputHeight * 3;
        uint8_t *outputData = (uint8_t *) output.data();
        for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
        {
            for ( int i = 0; i < 10; i++ )
            {
                printf( "inputId = %d, i = %d, data = %d \n", inputId, i,
                        outputData[inputId * outputSize + i] );
            }
        }
    }

    ret = RemapObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    return;
}

TEST( Remap, DSPSuccessPipelineTest )   // general success test on DSP for various pipeline,
                                        // including multiple input formats, output formats,
                                        // undisortion or not, normalization or not
{
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888,
                 RIDEHAL_IMAGE_FORMAT_RGB888, false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, true, false, false );
}

TEST( Remap, CPUSuccessPipelineTest )   // general success test on CPU for various pipeline,
                                        // including multiple input formats, output formats,
                                        // undisortion or not, normalization or not
{
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, true, false, false );
}

TEST( Remap, GeneralAccuracyTest )   // general accuracy test for DSP&CPU backend, RGB to RGB
                                     // pipeline, no undistortion and no renormalization
{
    printf( "DSP general accuracy test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888,
                 RIDEHAL_IMAGE_FORMAT_RGB888, false, false, true, false );
    printf( "CPU general accuracy test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, true, false );
}

TEST( Remap, GeneralPerformanceTest )   // general performance test for DSP&CPU backend, RGB to
                                        // RGB pipeline, no undistortion and no renormalization
{
    printf( "DSP general performance test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888,
                 RIDEHAL_IMAGE_FORMAT_RGB888, false, false, false, true );
    printf( "CPU general performance test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, true );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif