// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "md5_utils.hpp"
#include "ridehal/component/DepthFromStereo.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

static void LoadRaw( void *pData, uint32_t length, std::string path )
{
    printf( "  load raw from %s\n", path.c_str() );
    FILE *pFile = fopen( path.c_str(), "rb" );
    ASSERT_NE( nullptr, pFile );
    fseek( pFile, 0, SEEK_END );
    int size = ftell( pFile );
    ASSERT_LE( size, length );
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, size, pFile );
    ASSERT_EQ( r, size );
    fclose( pFile );
}

static void SaveRaw( std::string path, void *pData, size_t size )
{
    FILE *pFile = fopen( path.c_str(), "wb" );
    if ( nullptr != pFile )
    {
        fwrite( pData, 1, size, pFile );
        fclose( pFile );
        printf( "  save raw %s\n", path.c_str() );
    }
}

static void Eva_DepthFromStereoRun( std::string name, DepthFromStereo_Config_t &config,
                                    std::string img1 = "", std::string img2 = "",
                                    std::string goldenDispMap = "", std::string goldenConfMap = "" )
{
    DepthFromStereo dfs;
    RideHalError_e ret;
    RideHal_SharedBuffer_t priImg;
    RideHal_SharedBuffer_t auxImg;
    RideHal_SharedBuffer_t dispMap;
    RideHal_SharedBuffer_t confMap;
    RideHal_SharedBuffer_t dispMapG;
    RideHal_SharedBuffer_t confMapG;

    uint32_t width = config.width;
    uint32_t height = config.height;

    RideHal_TensorProps_t dispMapTsProp = { RIDEHAL_TENSOR_TYPE_UINT_16,
                                            { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                            4 };
    RideHal_TensorProps_t confMapTsProp = { RIDEHAL_TENSOR_TYPE_UINT_8,
                                            { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                            4 };


    printf( "-- Test for %s\n", name.c_str() );

    ret = priImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = auxImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dispMap.Allocate( &dispMapTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = confMap.Allocate( &confMapTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( false == img1.empty() )
    {
        LoadRaw( priImg.data(), priImg.size, img1 );
    }

    if ( false == img2.empty() )
    {
        LoadRaw( auxImg.data(), auxImg.size, img2 );
    }

    if ( ( false == goldenDispMap.empty() ) && ( false == goldenConfMap.empty() ) )
    { /* clear output buffer for accuracy test */
        memset( dispMap.data(), 0, dispMap.size );
        memset( confMap.data(), 0, confMap.size );
    }

    ret = dfs.Init( name.c_str(), &config, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.RegisterBuffers( &priImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.RegisterBuffers( &auxImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.RegisterBuffers( &dispMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.RegisterBuffers( &confMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.Execute( &priImg, &auxImg, &dispMap, &confMap );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( ( false == goldenDispMap.empty() ) && ( false == goldenConfMap.empty() ) )
    {
        // for the first run, with below to generate the golden
        SaveRaw( goldenDispMap, dispMap.data(), dispMap.size );
        SaveRaw( goldenConfMap, confMap.data(), confMap.size );

        ret = dispMapG.Allocate( &dispMapTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = confMapG.Allocate( &confMapTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        LoadRaw( dispMapG.data(), dispMapG.size, goldenDispMap );
        LoadRaw( confMapG.data(), confMapG.size, goldenConfMap );

        std::string md5Output = MD5Sum( dispMap.data(), dispMap.size );
        std::string md5Golden = MD5Sum( dispMapG.data(), dispMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( confMap.data(), confMap.size );
        md5Golden = MD5Sum( confMapG.data(), confMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        dispMapG.Free();
        confMapG.Free();
    }

    ret = dfs.Execute( &auxImg, &priImg, &dispMap, &confMap );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.DeRegisterBuffers( &priImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.DeRegisterBuffers( &auxImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.DeRegisterBuffers( &dispMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dfs.DeRegisterBuffers( &confMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = dfs.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = priImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = auxImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = dispMap.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = confMap.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( EVA, SANITY_DepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 400;
    Eva_DepthFromStereoRun( "DFS0", config );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif