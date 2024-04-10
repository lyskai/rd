// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <condition_variable>
#include <malloc.h>
#include <stdio.h>

#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoEncoder.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

static std::mutex s_inMutex;
static std::condition_variable s_InCondVar;
static std::mutex s_OutMutex;
static std::condition_variable s_OutCondVar;

static VideoEncoder_InputFrame_t s_sharedInputFrame;
static VideoEncoder_OutputFrame_t s_sharedOutputFrame;

static uint64_t g_timestamp = 0;

void OnInputDoneCb( const VideoEncoder_InputFrame_t *pInputFrame, void *pPrivData )
{
    s_sharedInputFrame = *pInputFrame;
    // sent signal
    s_InCondVar.notify_one();
}

void OnOutputDoneCb( const VideoEncoder_OutputFrame_t *pOutputFrame, void *pPrivData )
{
    s_sharedOutputFrame = *pOutputFrame;
    // sent signal
    s_OutCondVar.notify_one();
}

void EventCb( const VideoEncoder_EventType_e eventId, const void *pEvent, void *pPrivData )
{
    printf( "EventCb return \n" );
    switch ( eventId )
    {
        case VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
        case VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
        case VIDEO_ENCODER_EVENT_ERROR:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
    }
}

TEST( VideoEncoder, SANITY_VideoEncoder_Dynamic )
{
    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 20;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    config.outFormat = RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = true;
    config.bOutputDynamicMode = true;
    config.inputBufferList = nullptr;
    config.outputBufferList = nullptr;

    RideHalError_e ret;
    RideHal_SharedBuffer_t *sharedBuffer = nullptr;

    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderDynamic", &config );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_READY, veTest.GetState() );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = veTest.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_RUNNING, veTest.GetState() );

    VideoEncoder_OnTheFlyCmd_t onTheFlyCmd;
    onTheFlyCmd.propID = VIDEO_ENCODER_PROP_FRAME_RATE;
    onTheFlyCmd.pValue = 15;
    ret = veTest.Configure( &onTheFlyCmd );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    VideoEncoder_InputFrame_t inputFrame;
    sharedBuffer = &inputFrame.sharedBuffer;
    ret = sharedBuffer->Allocate( config.width, config.height, config.inFormat );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    VideoEncoder_OutputFrame_t *outputFrame =
            new VideoEncoder_OutputFrame_t[config.numOutputBufferReq];

    for ( int i = 0; i < config.numOutputBufferReq; i++ )
    {
        sharedBuffer = &outputFrame[i].sharedBuffer;
        RideHal_ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = config.width;
        imgProps.height = config.height;
        imgProps.compressedSize = 118784;
        imgProps.format = config.outFormat;
        ret = sharedBuffer->Allocate( &imgProps );
        // ret = sharedBuffer->Allocate( 118784 );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
        ret = veTest.SubmitOutputFrame( &outputFrame[i] );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    inputFrame.timestampNs = 0;
    inputFrame.appMarkData = nullptr;
    onTheFlyCmd.propID = VIDEO_ENCODER_PROP_BITRATE;
    onTheFlyCmd.pValue = 32000;
    inputFrame.numCmd = 2;
    VideoEncoder_OnTheFlyCmd_t *onTheFlyCmds = new VideoEncoder_OnTheFlyCmd_t[2];
    onTheFlyCmds[0].propID = VIDEO_ENCODER_PROP_BITRATE;
    onTheFlyCmds[0].pValue = 32000;
    onTheFlyCmds[1].propID = VIDEO_ENCODER_PROP_FRAME_RATE;
    onTheFlyCmds[1].pValue = 20;
    inputFrame.onTheFlyCmd = onTheFlyCmds;

    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );

    // compare input buffer
    auto rc = memcmp( &inputFrame.sharedBuffer, &s_sharedInputFrame.sharedBuffer,
                      sizeof( RideHal_SharedBuffer_t ) );
    ASSERT_EQ( 0, rc );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );

    ret = veTest.SubmitOutputFrame( &s_sharedOutputFrame );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = veTest.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_INITIAL, veTest.GetState() );
}

TEST( VideoEncoder, SANITY_VideoEncoder_NonDynamic )
{
    RideHalError_e ret;
    uint32_t i = 0;

    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    config.outFormat = RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
    config.inputBufferList = nullptr;
    config.outputBufferList = nullptr;
    /*
    RideHal_SharedBuffer_t *outBufferList = new RideHal_SharedBuffer_t[config.numOutputBufferReq];
    for ( i = 0; i < config.numOutputBufferReq; i++ )
    {
        RideHal_ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = config.width;
        imgProps.height = config.height;
        imgProps.compressedSize = 118784;
        imgProps.format = config.outFormat;
        ret = outBufferList[i].Allocate( &imgProps );
        // ret = sharedBuffer->Allocate( 118784 );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }
    config.outputBufferList = outBufferList;*/

    RideHal_SharedBuffer_t *sharedBuffer = nullptr;

    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderNormal", &config );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_READY, veTest.GetState() );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = veTest.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDE_HAL_COMPONENT_STATE_RUNNING, veTest.GetState() );

    RideHal_SharedBuffer_t *inputList = new RideHal_SharedBuffer_t[config.numInputBufferReq];

    ret = veTest.GetInputBuffers( inputList, config.numInputBufferReq );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        VideoEncoder_InputFrame_t inputFrame;
        inputFrame.sharedBuffer = inputList[i];
        inputFrame.timestampNs = g_timestamp;
        inputFrame.appMarkData = nullptr;
        ret = veTest.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
        g_timestamp += 33333;
    }

    RideHal_SharedBuffer_t *outputList = new RideHal_SharedBuffer_t[config.numOutputBufferReq];
    ret = veTest.GetOutputBuffers( outputList, config.numOutputBufferReq );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    for ( i = 0; i < config.numOutputBufferReq; i++ )
    {
        printf( "outputList[%d].data(): 0x%x\n", i, outputList[i].data() );
    }

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );

    ret = veTest.SubmitOutputFrame( &s_sharedOutputFrame );
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