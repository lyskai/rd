// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/OpticalFlow.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace ridehal::common;
using namespace ridehal::component;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

static void Eva_SanityOpticalFlow( OpticalFlow_Config_t &config )
{
    OpticalFlow ofl;
    RideHalError_e ret;
    RideHal_SharedBuffer_t refImg;
    RideHal_SharedBuffer_t curImg;
    RideHal_SharedBuffer_t mvFwdMap;
    RideHal_SharedBuffer_t mvConf;

    uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
    uint32_t height = ( config.height >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;

    RideHal_TensorProps_t mvFwdMapTsProp = {
            RIDEHAL_TENSOR_TYPE_UINT_16,
            { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
            4 };
    RideHal_TensorProps_t mvConfTsProp = { RIDEHAL_TENSOR_TYPE_UINT_8,
                                           { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                                           4 };


    ret = refImg.Allocate( 1920, 1024, RIDEHAL_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = curImg.Allocate( 1920, 1024, RIDEHAL_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvFwdMap.Allocate( &mvFwdMapTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvConf.Allocate( &mvConfTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Init( "OFL0", &config );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.RegisterBuffers( &refImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &curImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Execute( &curImg, &refImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.DeRegisterBuffers( &refImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &curImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = refImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = curImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvFwdMap.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvConf.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( EVA, SANITY_OpticalFlowCPU )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_CPU;
    Eva_SanityOpticalFlow( config );
}

TEST( EVA, SANITY_OpticalFlowDSP )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_DSP;
    Eva_SanityOpticalFlow( config );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif