// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "CL2DFlex.cl.h"
#include "md5_utils.hpp"
#include "ridehal/component/CL2DFlex.hpp"

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

void AccuracyTest( RideHal_ImageFormat_e inputFormatTest, RideHal_ImageFormat_e outputFormatTest,
                   uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
                   uint32_t outputHeightTest, std::string pathTest, std::string goldenPath,
                   bool saveOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.inputWidth = inputWidthTest;
    CL2DFlexConfig.inputHeight = inputHeightTest;
    CL2DFlexConfig.inputFormat = inputFormatTest;
    CL2DFlexConfig.outputWidth = outputWidthTest;
    CL2DFlexConfig.outputHeight = outputHeightTest;
    CL2DFlexConfig.outputFormat = outputFormatTest;

    RideHal_ImageProps_t imgProp1;
    imgProp1.batchSize = 1;
    imgProp1.width = CL2DFlexConfig.inputWidth;
    imgProp1.height = CL2DFlexConfig.inputHeight;
    imgProp1.format = CL2DFlexConfig.inputFormat;
    if ( RIDEHAL_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormat )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidth;
        imgProp1.stride[1] = CL2DFlexConfig.inputWidth;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeight;
        imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeight / 2;
        imgProp1.extraPadding = 0;
        imgProp1.numPlanes = 2;
    }
    else if ( RIDEHAL_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormat )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidth * 2;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeight;
        imgProp1.extraPadding = 0;
        imgProp1.numPlanes = 1;
    }

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
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( RIDEHAL_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.extraPadding = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( RIDEHAL_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.extraPadding = 0;
        imgProp2.numPlanes = 2;
    }

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    memset( output.data(), 0, output.size );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Execute( &input, &output );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( true == saveOutput )
    {
        uint8_t *ptr = (uint8_t *) output.data();
        FILE *fp = fopen( goldenPath.c_str(), "wb" );
        if ( nullptr != fp )
        {
            fwrite( ptr, output.size, 1, fp );
            fclose( fp );
        }
    }

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

void PerformanceTest( RideHal_ImageFormat_e inputFormatTest, RideHal_ImageFormat_e outputFormatTest,
                      uint32_t inputWidthTest, uint32_t inputHeightTest, uint32_t outputWidthTest,
                      uint32_t outputHeightTest, uint32_t times )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";

    CL2DFlexConfig.inputWidth = inputWidthTest;
    CL2DFlexConfig.inputHeight = inputHeightTest;
    CL2DFlexConfig.inputFormat = inputFormatTest;
    CL2DFlexConfig.outputWidth = outputWidthTest;
    CL2DFlexConfig.outputHeight = outputHeightTest;
    CL2DFlexConfig.outputFormat = outputFormatTest;

    RideHal_ImageProps_t imgProp1;
    imgProp1.batchSize = 1;
    imgProp1.width = CL2DFlexConfig.inputWidth;
    imgProp1.height = CL2DFlexConfig.inputHeight;
    imgProp1.format = CL2DFlexConfig.inputFormat;
    if ( RIDEHAL_IMAGE_FORMAT_NV12 == CL2DFlexConfig.inputFormat )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidth;
        imgProp1.stride[1] = CL2DFlexConfig.inputWidth;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeight;
        imgProp1.actualHeight[1] = CL2DFlexConfig.inputHeight / 2;
        imgProp1.extraPadding = 0;
        imgProp1.numPlanes = 2;
    }
    else if ( RIDEHAL_IMAGE_FORMAT_UYVY == CL2DFlexConfig.inputFormat )
    {
        imgProp1.stride[0] = CL2DFlexConfig.inputWidth * 2;
        imgProp1.actualHeight[0] = CL2DFlexConfig.inputHeight;
        imgProp1.extraPadding = 0;
        imgProp1.numPlanes = 1;
    }

    RideHal_SharedBuffer_t input;
    ret = input.Allocate( &imgProp1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_ImageProps_t imgProp2;
    imgProp2.batchSize = 1;
    imgProp2.width = CL2DFlexConfig.outputWidth;
    imgProp2.height = CL2DFlexConfig.outputHeight;
    imgProp2.format = CL2DFlexConfig.outputFormat;
    if ( RIDEHAL_IMAGE_FORMAT_RGB888 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth * 3;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.extraPadding = 0;
        imgProp2.numPlanes = 1;
    }
    else if ( RIDEHAL_IMAGE_FORMAT_NV12 == CL2DFlexConfig.outputFormat )
    {
        imgProp2.stride[0] = CL2DFlexConfig.outputWidth;
        imgProp2.stride[1] = CL2DFlexConfig.outputWidth;
        imgProp2.actualHeight[0] = CL2DFlexConfig.outputHeight;
        imgProp2.actualHeight[1] = CL2DFlexConfig.outputHeight / 2;
        imgProp2.extraPadding = 0;
        imgProp2.numPlanes = 2;
    }

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( &imgProp2 );
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
    printf( "%d*%d to %d*%d, execute time = %f ms\n", inputWidthTest, inputHeightTest,
            outputWidthTest, outputHeightTest, (float) duration_ms / (float) times );
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

    CL2DFlexConfig.inputWidth = 128;
    CL2DFlexConfig.inputHeight = 128;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputWidth = 128;
    CL2DFlexConfig.outputHeight = 128;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    RideHal_SharedBuffer_t input;

    ret = input.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                          CL2DFlexConfig.inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( CL2DFlexConfig.outputWidth, CL2DFlexConfig.outputHeight,
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

void CoverageTest()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";
    CL2DFlexConfig.inputWidth = 128;
    CL2DFlexConfig.inputHeight = 128;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputWidth = 128;
    CL2DFlexConfig.outputHeight = 128;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    RideHal_SharedBuffer_t input;
    RideHal_SharedBuffer_t output;

    ret = CL2DFlexObj.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // start before init

    ret = CL2DFlexObj.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // stop before init

    ret = CL2DFlexObj.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );   // deinit before init

    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );   // register buffer before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );   // deregister buffer before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = CL2DFlexObj.Execute( &input, &output );   // execute before init
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // success init
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // init twice, wrong status
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = CL2DFlexObj.Deinit();   // success deinit
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Init( pName, nullptr );   // null pointer for CL2DFlex configuration
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    CL2DFlexConfig.inputWidth = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input width
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputWidth = 128;

    CL2DFlexConfig.inputHeight = 1;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input height
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputHeight = 128;

    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_MAX;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong input format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;

    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_MAX;
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // wrong output format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig,
                            LOGGER_LEVEL_MAX );   // success init with invalid logger level
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( nullptr, 1 );   // null pointer for buffer to be register
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( nullptr, 1 );   // null pointer for buffer to be deregister
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Execute( nullptr, &output );   // null pointer for input buffer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Execute( &input, nullptr );   // null pointer for output buffer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = CL2DFlexObj.Deinit();   // success deinit
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );   // success init
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                          CL2DFlexConfig.inputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = output.Allocate( CL2DFlexConfig.outputWidth, CL2DFlexConfig.outputHeight,
                           CL2DFlexConfig.outputFormat );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );   // register twice
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );   // deregister twice
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    input.type = RIDEHAL_BUFFER_TYPE_TENSOR;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong input buffer type
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    input.type = RIDEHAL_BUFFER_TYPE_IMAGE;

    output.type = RIDEHAL_BUFFER_TYPE_TENSOR;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong output buffer type
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    output.type = RIDEHAL_BUFFER_TYPE_IMAGE;

    input.imgProps.format = RIDEHAL_IMAGE_FORMAT_RGB888;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong input image format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    input.imgProps.format = RIDEHAL_IMAGE_FORMAT_NV12;

    input.imgProps.width = input.imgProps.width + 1;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong input image width
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    input.imgProps.width = input.imgProps.width - 1;

    input.imgProps.height = input.imgProps.height + 1;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong input image height
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    input.imgProps.height = input.imgProps.height - 1;

    output.imgProps.format = RIDEHAL_IMAGE_FORMAT_NV12;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong output image format
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    output.imgProps.format = RIDEHAL_IMAGE_FORMAT_RGB888;

    output.imgProps.width = output.imgProps.width + 1;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong output image width
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    output.imgProps.width = output.imgProps.width - 1;

    output.imgProps.height = output.imgProps.height + 1;
    ret = CL2DFlexObj.Execute( &input, &output );   // execute with wrong output image height
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    output.imgProps.height = output.imgProps.height - 1;

    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = CL2DFlexObj.Deinit();   // success deinit with registered buffer
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = input.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = output.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    OpenclSrv OpenclSrvObj;

    ret = OpenclSrvObj.Init( pName, LOGGER_LEVEL_ERROR );   // success init OpenclSrv
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = OpenclSrvObj.LoadFromSource( "", "" );   // create kernel with null source
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = OpenclSrvObj.LoadFromSource( s_pSourceConvertNV12ToRGB,
                                       "" );   // create kernel with null kernel
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = OpenclSrvObj.LoadFromBinary( (const unsigned char *) "",
                                       "" );   // create kernel with null binary
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = OpenclSrvObj.RegBuf( nullptr, 0, 0, nullptr );   // register with null host pointer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = OpenclSrvObj.DeregBuf( nullptr );   // deregister with null pointer
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    OpenclIfcae_Arg_t OpenclArg;
    OpenclArg.pArg = nullptr;
    OpenclArg.argSize = 0;
    OpenclIface_WorkParams_t OpenclWorkParams;
    ret = OpenclSrvObj.Execute( &OpenclArg, 1, &OpenclWorkParams );   // execute with null args
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    int arg = 1;
    OpenclArg.pArg = &arg;
    OpenclArg.argSize = sizeof( cl_int );
    OpenclWorkParams.workDim = 0;
    OpenclWorkParams.pGlobalWorkSize = nullptr;
    OpenclWorkParams.pGlobalWorkOffset = nullptr;
    OpenclWorkParams.pLocalWorkSize = nullptr;
    ret = OpenclSrvObj.Execute( &OpenclArg, 1,
                                &OpenclWorkParams );   // execute with null work params
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = OpenclSrvObj.Deinit();   // success deinit
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    cl_mem *clMem;
    ret = OpenclSrvObj.RegBuf( (void *) &input, 1, 0, clMem );   // register without init
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    return;
}

TEST( CL2DFlex, SanityTest )
{
    SanityTest();
}

TEST( CL2DFlex, CoverageTest )
{
    CoverageTest();
}

TEST( CL2DFlex, ConvertAccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
    // md5 of golden1.rgb is 4b528d54b7c5d5164f66719a89f988be
    // md5 of golden2.rgb is f94a6aeae302add0424821660bcd2684
    // md5 of golden3.nv12 is 91ed68589443b87bcfff8ae7e69b03b2
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024,
                  "./data/test/CL2DFlex/0.nv12", "./data/test/CL2DFlex/golden1.rgb", false );
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024,
                  "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden2.rgb", false );
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_NV12, 1920, 1024, 1920, 1024,
                  "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden3.nv12", false );
}

TEST( CL2DFlex, ResizeAccuracyTest )
{
    // md5 of 0.nv12 is a1591f4b8c196a47628f0ef6bc3a721c
    // md5 of 0.uyvy is 5b1ae2203a9d97aeafe65e997f3beebc
    // md5 of golden4.rgb is 9382129ca960b8ff22e3c135e3eb12b7
    // md5 of golden5.rgb is 520d1039f96107ef4db669e9644250fd
    // md5 of golden6.nv12 is 91cdd0def0f40ce3c0fec070c2bccd01
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1152, 800,
                  "./data/test/CL2DFlex/0.nv12", "./data/test/CL2DFlex/golden4.rgb", false );
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1152, 800,
                  "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden5.rgb", false );
    AccuracyTest( RIDEHAL_IMAGE_FORMAT_UYVY, RIDEHAL_IMAGE_FORMAT_NV12, 1920, 1024, 1152, 800,
                  "./data/test/CL2DFlex/0.uyvy", "./data/test/CL2DFlex/golden6.nv12", false );
}

TEST( CL2DFlex, PerformanceTest )
{
    PerformanceTest( RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1920, 1024,
                     100 );
    PerformanceTest( RIDEHAL_IMAGE_FORMAT_NV12, RIDEHAL_IMAGE_FORMAT_RGB888, 1920, 1024, 1152, 800,
                     100 );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif