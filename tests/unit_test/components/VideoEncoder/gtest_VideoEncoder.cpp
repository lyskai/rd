// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <condition_variable>
#include <malloc.h>
#include <stdio.h>
#include <vidc_ioctl.h>


#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoEncoder.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

static std::mutex s_inMutex;
static std::condition_variable s_InCondVar;
static std::mutex s_OutMutex;
static std::condition_variable s_OutCondVar;

static RideHal_SharedBuffer_t s_sharedInputBuffer;
static RideHal_SharedBuffer_t s_sharedOutputBuffer;

void OnInputBufferDoneCb( const RideHal_SharedBuffer_t *pSharedBuffer )
{
    s_sharedInputBuffer = *pSharedBuffer;
    // sent signal
    s_InCondVar.notify_one();
}

void OnOutputBufferDoneCb( const RideHal_SharedBuffer_t *pSharedBuffer )
{
    s_sharedOutputBuffer = *pSharedBuffer;
    // sent signal
    s_OutCondVar.notify_one();
}

TEST( VideoEncoder, SANITY_VideoEncoder )
{
    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 25;
    config.numInputBufferReq = 8;
    config.numOutputBufferReq = 8;
    config.frameRate = 30;
    config.rateControlMode = RIDE_HAL_CBR_CFR;
    config.format = RIDE_HAL_IMAGE_FORMAT_NV12;
    config.inputBufferDoneCb = OnInputBufferDoneCb;
    config.outputBufferDoneCb = OnOutputBufferDoneCb;
    config.bDynamicMode = true;

    RideHalError_e ret;

    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "testVideoEncoder", &config );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_READY, veTest.GetState() );

    ret = veTest.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_RUNNING, veTest.GetState() );

    RideHal_SharedBuffer_t sharedBuffer;
    ret = sharedBuffer.Allocate( 176, 144, RIDE_HAL_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ret = veTest.SubmitInputBuffer( &sharedBuffer, 0 );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );
    // compare buffer
    auto rc = memcmp( &sharedBuffer, &s_sharedInputBuffer, sizeof( RideHal_SharedBuffer_t ) );
    ASSERT_EQ( 0, rc );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );
    ret = veTest.SubmitOutputBuffer( &s_sharedOutputBuffer );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = veTest.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_INITIAL, veTest.GetState() );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif