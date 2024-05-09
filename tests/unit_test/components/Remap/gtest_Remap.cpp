// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <chrono>
#include <stdio.h>

#include "ridehal/component/Remap.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

void SetCommonParam( Remap_Config_t *pRemapConfig )
{
    pRemapConfig->processor = RIDEHAL_PROCESSOR_HTP0;
    pRemapConfig->numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < pRemapConfig->numOfInputs; inputId++ )
    {
        pRemapConfig->inputConfigs[inputId].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        pRemapConfig->inputConfigs[inputId].inputWidth = 512;
        pRemapConfig->inputConfigs[inputId].inputHeight = 512;
        pRemapConfig->inputConfigs[inputId].mapWidth = 256;
        pRemapConfig->inputConfigs[inputId].mapHeight = 256;
        pRemapConfig->inputConfigs[inputId].ROI.x = 0;
        pRemapConfig->inputConfigs[inputId].ROI.y = 0;
        pRemapConfig->inputConfigs[inputId].ROI.width = 256;
        pRemapConfig->inputConfigs[inputId].ROI.height = 256;
    }
    pRemapConfig->outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    pRemapConfig->outputWidth = 256;
    pRemapConfig->outputHeight = 256;
    pRemapConfig->bEnableUndistortion = false;
    pRemapConfig->bEnableNormalize = false;
    return;
}

void FailTest1()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    RideHal_SharedBuffer_t inputs[1];
    RideHal_SharedBuffer_t output;

    ret = RemapObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // start before init

    ret = RemapObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // stop before init

    ret = RemapObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // deinit before init

    ret = RemapObj.RegBuf( &output, 1, FADAS_BUF_TYPE_OUT );   // register buffer before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = RemapObj.DeregBuf( &output, 1 );   // deregister buffer before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = RemapObj.Execute( inputs, 1, &output );   // execute before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    SetCommonParam( &RemapConfig );
    ret = RemapObj.Init( pName, &RemapConfig );   // success init
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.Init( pName, &RemapConfig );   // init twice, wrong status
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    return;
}

void FailTest2()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    RideHal_SharedBuffer_t inputs[1];
    RideHal_SharedBuffer_t output;

    ret = RemapObj.Init( pName, nullptr );   // null pointer for remap configuration
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.processor = RIDEHAL_PROCESSOR_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong processor type
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.numOfInputs = RIDEHAL_MAX_INPUTS + 1;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong number of inputs
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong output format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.inputConfigs[0].inputFormat = RIDEHAL_IMAGE_FORMAT_MAX;
    ret = RemapObj.Init( pName, &RemapConfig );   // wrong input format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    RemapConfig.bEnableUndistortion = true;
    RemapConfig.inputConfigs[0].remapTable.pMapX = nullptr;
    RemapConfig.inputConfigs[0].remapTable.pMapY = nullptr;
    ret = RemapObj.Init( pName, &RemapConfig );   // null pointer for map table
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    SetCommonParam( &RemapConfig );
    ret = RemapObj.Init( pName, &RemapConfig );   // success init
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.RegBuf( nullptr, 1,
                           FADAS_BUF_TYPE_OUT );   // null pointer for buffer to be register
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = RemapObj.DeregBuf( nullptr, 1 );   // null pointer for buffer to be deregister
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( nullptr, RemapConfig.numOfInputs,
                            &output );   // null pointer for input buffer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs,
                            nullptr );   // null pointer for output buffer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs + 1,
                            &output );   // wrong input buffer number
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    return;
}

void SuccessTest( RideHal_ProcessorType_e processorTest, RideHal_ImageFormat_e inputFormatTest,
                  RideHal_ImageFormat_e outputFormatTest, bool bEnableUndistortionTest,
                  bool bEnableNormalizeTest, bool bCheckAccuracyTest, bool bCheckPerformanceTest )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    Remap RemapObj;
    Remap_Config_t RemapConfig;
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
            uint32_t mapSize = mapWidth * mapHeight * sizeof( float );
            RideHal_SharedBuffer_t mapXBuffer;
            RideHal_SharedBuffer_t mapYBuffer;
            ret = mapXBuffer.Allocate( mapSize );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
            ret = mapYBuffer.Allocate( mapSize );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
            float *mapX = (float *) mapXBuffer.data();
            float *mapY = (float *) mapYBuffer.data();
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
            else if ( RemapConfig.inputConfigs[inputId].inputFormat == RIDEHAL_IMAGE_FORMAT_RGB888 )
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
            for ( int i = 0; i < 6; i++ )
            {
                printf( "inputId = %d, i = %d, data = %d \n", inputId, i, inputData[i] );
            }
        }
    }

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = RemapObj.Init( pName, &RemapConfig );
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
            for ( int i = 0; i < 6; i++ )
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

TEST( Remap, DSPSuccessPipeline1Test )   // general success test on DSP for UYVY/RGB to RGB, with
                                         // and without normalization, no undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, true, false, false );
}

TEST( Remap, DSPSuccessPipeline2Test )   // general success test on DSP for UYVY/RGB to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, true, false, false );
}

TEST( Remap, DSPSuccessPipeline3Test )   // general success test on CPU for NV12 to BGR, with
                                         // and without undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_BGR888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_BGR888,
                 true, false, false, false );
}

TEST( Remap, CPUSuccessPipeline1Test )   // general success test on CPU for UYVY/RGB to RGB, with
                                         // and without normalization, no undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, true, false, false );
}

TEST( Remap, CPUSuccessPipeline2Test )   // general success test on CPU for UYVY/RGB to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, true, false, false );
}

TEST( Remap, CPUSuccessPipeline3Test )   // general success test on CPU for NV12 to BGR, with
                                         // and without undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_BGR888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_BGR888,
                 true, false, false, false );
}

TEST( Remap, CPUSuccessPipeline4Test )   // general success test on CPU for NV12 to RGB, with
                                         // and without normalization, undistortion
{
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, false, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, true, false, false );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888,
                 true, true, false, false );
}

TEST( Remap, GeneralAccuracyTest )   // general accuracy test for DSP&CPU backend, RGB to RGB
                                     // pipeline, no undistortion and no renormalization
{
    printf( "DSP general accuracy test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, true, false );
    printf( "CPU general accuracy test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, true, false );
}

TEST( Remap, GeneralPerformanceTest )   // general performance test for DSP&CPU backend, RGB to
                                        // RGB pipeline, no undistortion and no renormalization
{
    printf( "DSP general performance test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_HTP0, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, true );
    printf( "CPU general performance test\n" );
    SuccessTest( RIDEHAL_PROCESSOR_CPU, RIDEHAL_IMAGE_FORMAT_RGB888, RIDEHAL_IMAGE_FORMAT_RGB888,
                 false, false, false, true );
}

TEST( Remap, FailTest )   // fail path tests
{
    FailTest1();   // bad status error
    FailTest2();   // bad arguments error
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif