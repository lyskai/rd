// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleVideoEncoder.hpp"


namespace ridehal
{
namespace sample
{

SampleVideoEncoder::SampleVideoEncoder() {}
SampleVideoEncoder ::~SampleVideoEncoder() {}


void SampleVideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame )
{
    uint64_t frameId = ( uint64_t )(uintptr_t) pInputFrame->pAppMarkData;

    RIDEHAL_DEBUG( "InFrameCallback for frameId %" PRIu64, frameId );

    std::lock_guard<std::mutex> l( m_lock );
    auto it = m_camFrameMap.find( frameId );
    if ( it != m_camFrameMap.end() )
    { /* release the input camera frame */
        m_camFrameMap.erase( frameId );
    }
    else
    {
        RIDEHAL_ERROR( "InFrameCallback with invalid frameId %" PRIu64, frameId );
    }
}

void SampleVideoEncoder::OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame )
{
    CamFrames_t frames;
    CamFrame_t frame;
    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;

    pSharedBuffer->sharedBuffer = pOutputFrame->sharedBuffer;
    pSharedBuffer->pubHandle = 0;

    PROFILER_BEGIN();
    PROFILER_END();
    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        VideoEncoder_OutputFrame_t outFrame;
        outFrame.sharedBuffer = pSharedBuffer->sharedBuffer;
        outFrame.pAppMarkData = nullptr;
        m_encoder.SubmitOutputFrame( &outFrame );
        delete pSharedBuffer;
    } );

    if ( false == m_frameInfoQueue.empty() )
    {
        FrameInfo info;
        {
            std::lock_guard<std::mutex> l( m_lock );
            info = m_frameInfoQueue.front();
            m_frameInfoQueue.pop();
        }
        frame.frameId = info.frameId;
        frame.buffer = buffer;
        frame.timestamp = info.timestamp;
        frames.frames.push_back( frame );
        m_pub.Publish( frames );
        RIDEHAL_DEBUG( "OutFrameCallback for frameId %" PRIu64 " type %d size %" PRIu32,
                       info.frameId, pOutputFrame->frameType, pOutputFrame->sharedBuffer.size );
    }
    else
    {
        frame.frameId = ( uint64_t )(uintptr_t) pOutputFrame->pAppMarkData;
        frame.buffer = buffer;
        frame.timestamp = pOutputFrame->timestampNs;
        frames.frames.push_back( frame );
        m_pub.Publish( frames );
        RIDEHAL_DEBUG( "frame info queue is empty!" );
    }
}

void SampleVideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId,
                                        const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );
}

void SampleVideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame,
                                          void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->InFrameCallback( pInputFrame );
}

void SampleVideoEncoder::OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                           void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->OutFrameCallback( pOutputFrame );
}

void SampleVideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId,
                                        const void *pPayload, void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->EventCallback( eventId, pPayload );
}

RideHalError_e SampleVideoEncoder::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_config.width = Get( config, "width", 0 );
    if ( 0 == m_config.width )
    {
        RIDEHAL_ERROR( "invalid width = %u\n", m_config.width );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.height = Get( config, "height", 0 );
    if ( 0 == m_config.height )
    {
        RIDEHAL_ERROR( "invalid height = %u\n", m_config.height );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numInputBufferReq = Get( config, "pool_size", 4 );
    if ( 0 == m_config.numInputBufferReq )
    {
        RIDEHAL_ERROR( "invalid pool_size = %u\n", m_config.numInputBufferReq );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    m_config.numOutputBufferReq = m_config.numInputBufferReq;

    m_config.bitRate = Get( config, "bitrate", 8000000 );
    if ( 0 == m_config.bitRate )
    {
        RIDEHAL_ERROR( "invalid bitrate = %u\n", m_config.bitRate );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.frameRate = Get( config, "fps", 30 );
    if ( 0 == m_config.frameRate )
    {
        RIDEHAL_ERROR( "invalid fps = %u\n", m_config.frameRate );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.gop = 20;
    m_config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    m_config.inFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    m_config.outFormat = RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265;
    m_config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    m_config.bInputDynamicMode = true;
    m_config.bOutputDynamicMode = false;

    return ret;
}

RideHalError_e SampleVideoEncoder::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_encoder.Init( (char *) name.c_str(), &m_config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_encoder.RegisterCallback( SampleVideoEncoder::InFrameCallback,
                                          SampleVideoEncoder::OutFrameCallback,
                                          SampleVideoEncoder::EventCallback, (void *) this );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleVideoEncoder::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_encoder.Start();

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoEncoder::ThreadMain, this );
    }

    return ret;
}


void SampleVideoEncoder::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        CamFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            std::string camName = "CAM0";
            frame = frames.frames[0];
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ", frame.frameId,
                           frame.timestamp );

            VideoEncoder_InputFrame_t inputFrame;
            inputFrame.sharedBuffer = frame.buffer->sharedBuffer;
            inputFrame.timestampNs = frame.timestamp;
            inputFrame.pAppMarkData = (void *) frame.frameId;
            inputFrame.pOnTheFlyCmd = nullptr;

            {
                std::lock_guard<std::mutex> l( m_lock );
                m_camFrameMap[frame.frameId] = frame;
            }

            ret = m_encoder.SubmitInputFrame( &inputFrame );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "failed to submit input frameId %" PRIu64, frame.frameId );
                std::lock_guard<std::mutex> l( m_lock );
                m_camFrameMap.erase( frame.frameId );
            }
            else
            {
                FrameInfo info = { frame.frameId, frame.timestamp };
                std::lock_guard<std::mutex> l( m_lock );
                m_frameInfoQueue.push( info );
            }
        }
    }
}

RideHalError_e SampleVideoEncoder::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    ret = m_encoder.Stop();

    return ret;
}

RideHalError_e SampleVideoEncoder::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_encoder.Deinit();

    return ret;
}

REGISTER_SAMPLE( VideoEncoder, SampleVideoEncoder );

}   // namespace sample
}   // namespace ridehal
