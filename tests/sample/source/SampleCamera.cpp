// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/sample/SampleCamera.hpp"
#include <time.h>

namespace ridehal
{
namespace sample
{

SampleCamera::SampleCamera() {}
SampleCamera ::~SampleCamera() {}

void SampleCamera::FrameCallBack( CameraFrame_t *pFrame )
{
    CamFrames_t frames;
    CamFrame_t frame;
    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
    pSharedBuffer->sharedBuffer = pFrame->sharedBuffer;
    pSharedBuffer->pubHandle = ( (uint64_t) pFrame->streamId << 32 ) + pFrame->frameIndex;

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        uint32_t streamId = ( pSharedBuffer->pubHandle >> 32 ) & 0xFFFFFFFFul;
        uint32_t frameIndex = pSharedBuffer->pubHandle & 0xFFFFFFFFul;
        m_camera.ReleaseFrame( streamId, frameIndex );
        delete pSharedBuffer;
    } );

    frame.frameId = m_frameId++;
    frame.buffer = buffer;
    frame.timestamp = pFrame->timestamp;
    if ( 0 == frame.timestamp )
    {
        struct timespec ts;
        clock_gettime( CLOCK_MONOTONIC, &ts );
        frame.timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
    }
    frames.frames.push_back( frame );
    m_pub.Publish( frames );
}

void SampleCamera::EventCallBack( const uint32_t eventId, const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );
}

void SampleCamera::FrameCallBack( CameraFrame_t *pFrame, void *pPrivData )
{
    SampleCamera *self = (SampleCamera *) pPrivData;
    self->FrameCallBack( pFrame );
}

void SampleCamera::EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    SampleCamera *self = (SampleCamera *) pPrivData;
    self->EventCallBack( eventId, pPayload );
}

RideHalError_e SampleCamera::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_camConfig.inputId = Get( config, "input_id", -1 );
        if ( -1 == m_camConfig.inputId )
        {
            RIDEHAL_ERROR( "invalid input id = %d\n", m_camConfig.inputId );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.width = Get( config, "width", 0 );
        if ( 0 == m_camConfig.width )
        {
            RIDEHAL_ERROR( "invalid width = %d\n", m_camConfig.width );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.height = Get( config, "height", 0 );
        if ( 0 == m_camConfig.height )
        {
            RIDEHAL_ERROR( "invalid height = %d\n", m_camConfig.height );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.isAllocator = true;
        m_camConfig.requestMode = false;
        m_camConfig.bufCnt = 4;
        m_camConfig.format = RIDE_HAL_IMAGE_FORMAT_NV12;

        m_topicName = Get( config, "topic", "" );
        if ( "" == m_topicName )
        {
            RIDEHAL_ERROR( "no topic\n" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_camera.Init( (char *) name.c_str(), m_camConfig );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_camera.RegisterCallback( SampleCamera::FrameCallBack, SampleCamera::EventCallBack,
                                         (void *) this );
    }


    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleCamera::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_camera.Start();

    return ret;
}

RideHalError_e SampleCamera::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_camera.Stop();

    return ret;
}

RideHalError_e SampleCamera::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_camera.Deinit();

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace ridehal
