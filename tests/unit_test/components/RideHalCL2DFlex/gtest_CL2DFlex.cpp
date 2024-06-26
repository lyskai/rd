// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "md5_utils.hpp"
#include "ridehal/component/CL2DFlex.hpp"

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

void AccuracyTest( std::string pathTest, std::string goldenPath )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.inputWidth = 1920;
    CL2DFlexConfig.inputHeight = 1024;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    RideHal_ImageProps_t imgProp1;
    imgProp1.batchSize = 1;
    imgProp1.width = CL2DFlexConfig.inputWidth;
    imgProp1.height = CL2DFlexConfig.inputHeight;
    imgProp1.format = CL2DFlexConfig.inputFormat;
    imgProp1.stride[0] = CL2DFlexConfig.inputWidth;
    imgProp1.stride[1] = CL2DFlexConfig.inputWidth;
    imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeight;
    imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeight / 2;
    imgProp1.extraPadding = 0;
    imgProp1.numPlanes = 2;

    RideHal_SharedBuffer_t input;
    ret = input.Allocate( &imgProp1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    memset( input.data(), 0, input.size );

    FILE *file1 = nullptr;
    size_t length1 = 0;
    file1 = fopen( pathTest.c_str(), "rb" );
    if ( nullptr == file1 )
    {
        printf( "could not open image file %s\n", pathTest.c_str() );
    }
    else
    {
        fseek( file1, 0, SEEK_END );
        length1 = (size_t) ftell( file1 );
        if ( input.size != length1 )
        {
            printf( "image file %s size not match, need %d but got %d\n", pathTest.c_str(),
                    (int) input.size, (int) length1 );
        }
        else
        {
            fseek( file1, 0, SEEK_SET );
            auto r = fread( input.data(), 1, length1, file1 );
            if ( length1 != r )
            {
                printf( "failed to read image file %s, need %d but read %d\n", pathTest.c_str(),
                        (int) length1, (int) r );
            }
        }
        fclose( file1 );
    }

    RideHal_ImageProps_t imgProp2;
    imgProp2.batchSize = 1;
    imgProp2.width = CL2DFlexConfig.inputWidth;
    imgProp2.height = CL2DFlexConfig.inputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    imgProp2.stride[0] = CL2DFlexConfig.inputWidth * 3;
    imgProp2.actualHeight[0] = CL2DFlexConfig.inputHeight;
    imgProp2.extraPadding = 0;
    imgProp2.numPlanes = 1;

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    memset( output.data(), 0, output.size );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Execute( &input, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t golden;
    ret = golden.Allocate( &imgProp2 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    FILE *file2 = nullptr;
    size_t length2 = 0;
    file2 = fopen( goldenPath.c_str(), "rb" );
    if ( nullptr == file2 )
    {
        printf( "could not open golden file %s\n", goldenPath.c_str() );
    }
    else
    {
        fseek( file2, 0, SEEK_END );
        length2 = (size_t) ftell( file2 );
        if ( golden.size != length2 )
        {
            printf( "golden file %s size not match, need %d but got %d\n", goldenPath.c_str(),
                    (int) golden.size, (int) length2 );
        }
        else
        {
            fseek( file2, 0, SEEK_SET );
            auto r = fread( golden.data(), 1, length2, file2 );
            if ( length2 != r )
            {
                printf( "failed to read golden file %s, need %d but read %d\n", pathTest.c_str(),
                        (int) length2, (int) r );
            }
        }
        fclose( file2 );
    }

    std::string md5Output = MD5Sum( output.data(), output.size );
    printf( "output md5 = %s\n", md5Output.c_str() );
    std::string md5Golden = MD5Sum( golden.data(), output.size );
    printf( "golden md5 = %s\n", md5Golden.c_str() );

    if ( md5Output != md5Golden )   // check cosine similarity if md5 not match
    {
        size_t outputSize = output.size;
        uint8_t *outputData = (uint8_t *) output.data();
        uint8_t *goldenData = (uint8_t *) golden.data();
        float dot = 0.0;
        float norm1 = 1e-10;
        float norm2 = 1e-10;
        int miss = 0;
        for ( int i = 0; i < outputSize; i++ )
        {
            if ( outputData[i] != goldenData[i] )
            {
                miss++;
                if ( miss < 10 )
                {
                    printf( "data not match at i=%d, output=%d, golden=%d\n", i, outputData[i],
                            goldenData[i] );
                }
            }
            dot = dot + outputData[i] * goldenData[i];
            norm1 = norm1 + outputData[i] * outputData[i];
            norm2 = norm2 + goldenData[i] * goldenData[i];
        }
        float cos = dot / sqrt( norm1 * norm2 );
        printf( "cosine similarity = %f\n", cos );
        printf( "miss data number = %d\n", miss );
    }
    ASSERT_EQ( md5Output, md5Golden );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = output.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    return;
}

void PerformanceTest( uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t times )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.inputWidth = inputWidthTest;
    CL2DFlexConfig.inputHeight = inputHeightTest;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    RideHal_SharedBuffer_t input;

    ret = input.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                          CL2DFlexConfig.inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                           CL2DFlexConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    auto start = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < times; i++ )
    {
        ret = CL2DFlexObj.Execute( &input, &output );
    }
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
    printf( "execute time = %f ms\n", (float) duration_ms / (float) times );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = output.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    return;
}

void SanityTest()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.inputWidth = 256;
    CL2DFlexConfig.inputHeight = 256;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    RideHal_SharedBuffer_t input;

    ret = input.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                          CL2DFlexConfig.inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                           CL2DFlexConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Execute( &input, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = output.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    return;
}

TEST( CL2DFlex, SanityTest )
{
    SanityTest();
}

TEST( CL2DFlex, AccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of golden.rgb is 318450304ff3a55fc65b5a4bb1a641d5
    AccuracyTest( "./data/test/CL2DFlex/0.nv12", "./data/test/CL2DFlex/golden.rgb" );
}

TEST( CL2DFlex, PerformanceTest )
{
    PerformanceTest( 128, 128, 100 );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif