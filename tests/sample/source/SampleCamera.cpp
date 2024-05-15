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
    pSharedBuffer->pubHandle = (uint64_t) pFrame->frameIndex;

    PROFILER_BEGIN();
    PROFILER_END();

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        uint32_t frameIndex = pSharedBuffer->pubHandle & 0xFFFFFFFFul;
        CameraFrame_t camFrame;
        camFrame.sharedBuffer = pSharedBuffer->sharedBuffer;
        camFrame.frameIndex = frameIndex;
        if ( false == m_camConfig.bRequestMode )
        {
            m_camera.ReleaseFrame( &camFrame );
        }
        else
        {

            m_camera.RequestFrame( &camFrame );
        }
        delete pSharedBuffer;
    } );

    frame.frameId = m_frameId++;
    frame.buffer = buffer;
    frame.timestamp = pFrame->timestamp;
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
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_camConfig.inputId = Get( config, "input_id", -1 );
        if ( -1 == m_camConfig.inputId )
        {
            RIDEHAL_ERROR( "invalid input id = %d\n", m_camConfig.inputId );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.width = Get( config, "width", 0 );
        if ( 0 == m_camConfig.width )
        {
            RIDEHAL_ERROR( "invalid width = %d\n", m_camConfig.width );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.height = Get( config, "height", 0 );
        if ( 0 == m_camConfig.height )
        {
            RIDEHAL_ERROR( "invalid height = %d\n", m_camConfig.height );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.bRequestMode = Get( config, "request_mode", false );
        m_camConfig.streamId = Get( config, "stream_id", 0 );

        m_camConfig.bAllocator = true;
        m_camConfig.ispUserCase = Get( config, "isp_use_case", 3 );
        m_camConfig.bufCnt = Get( config, "pool_size", 4 );
        if ( 0 == m_camConfig.bufCnt )
        {
            RIDEHAL_ERROR( "invalid pool_size \n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        m_camConfig.format = Get( config, "format", RIDEHAL_IMAGE_FORMAT_NV12 );
        if ( RIDEHAL_IMAGE_FORMAT_MAX == m_camConfig.format )
        {
            RIDEHAL_ERROR( "invalid format\n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.camFrameDropPat = Get( config, "frame_drop_patten", 0 );

        m_camConfig.opMode = Get( config, "op_mode", (uint32_t) QCARCAM_OPMODE_OFFLINE_ISP );

        m_topicName = Get( config, "topic", "" );
        if ( "" == m_topicName )
        {
            RIDEHAL_ERROR( "no topic\n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_bIgnoreError = Get( config, "ignore_error", false );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_camera.Init( (char *) name.c_str(), &m_camConfig );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = m_camera.RegisterCallback( SampleCamera::FrameCallBack,
                                             SampleCamera::EventCallBack, (void *) this );
        }
        else
        {
            if ( m_bIgnoreError )
            {
                RIDEHAL_ERROR( "Init failed: %d, ignore it\n" );
                ret = RIDEHAL_ERROR_NONE;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleCamera::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_camera.Start();

    if ( m_bIgnoreError )
    {
        RIDEHAL_ERROR( "Start failed: %d, ignore it\n" );
        ret = RIDEHAL_ERROR_NONE;
    }

    return ret;
}

RideHalError_e SampleCamera::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_camera.Stop();

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleCamera::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_camera.Deinit();

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace ridehal
