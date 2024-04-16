// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleRecorder.hpp"


namespace ridehal
{
namespace sample
{

SampleRecorder::SampleRecorder() {}
SampleRecorder::~SampleRecorder() {}

RideHalError_e SampleRecorder::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_maxImages = Get( config, "max", 1000 );
    if ( 0 == m_maxImages )
    {
        RIDEHAL_ERROR( "invalid max = %d\n", m_maxImages );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleRecorder::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_topicName );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        std::string path = "/tmp/" + name + ".raw";
        m_file = fopen( path.c_str(), "wb" );
        if ( nullptr == m_file )
        {
            RIDEHAL_ERROR( "can't create file %s", path.c_str() );
            ret = RIDE_HAL_ERROR_ACCES;
        }
    }

    return ret;
}

RideHalError_e SampleRecorder::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = false;
    m_thread = std::thread( &SampleRecorder::ThreadMain, this );

    return ret;
}

void SampleRecorder::ThreadMain()
{
    RideHalError_e ret;
    uint32_t num = 0;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        CamFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            frame = frames.frames[0];
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frame.frameId,
                           frame.timestamp );
            if ( num < m_maxImages )
            {
                auto &buffer = frame.buffer->sharedBuffer;
                if ( buffer.imgProps.format < RIDE_HAL_IMAGE_FORMAT_MAX )
                {
                    uint32_t sizeOne = buffer.size / buffer.imgProps.batchSize;
                    for ( uint32_t i = 0; i < buffer.imgProps.batchSize; i++ )
                    {
                        std::string path = "/tmp/" + m_name + "_" + std::to_string( num ) + "_" +
                                           std::to_string( i ) + ".raw";
                        uint8_t *ptr = (uint8_t *) buffer.data() + sizeOne * i;
                        FILE *fp = fopen( path.c_str(), "wb" );
                        if ( nullptr != fp )
                        {
                            fwrite( ptr, sizeOne, 1, fp );
                            fclose( fp );
                        }
                        else
                        {
                            RIDEHAL_ERROR( "failed to create file: %s", path.c_str() );
                        }
                    }
                    fprintf( m_file,
                             "%u: frameId %" PRIu64 " timestamp %" PRIu64
                             ": batch=%u resolution=%ux%u stride=%u actual_height=%u format=%d\n",
                             num, frame.frameId, frame.timestamp, buffer.imgProps.batchSize,
                             buffer.imgProps.width, buffer.imgProps.height,
                             buffer.imgProps.stride[0], buffer.imgProps.actualHeight[0],
                             buffer.imgProps.format );
                }
                else
                { /* compressed image */
                    fwrite( buffer.data(), buffer.size, 1, m_file );
                }
                num++;
            }
            else if ( nullptr != m_file )
            {
                fclose( m_file );
                m_file = nullptr;
                RIDEHAL_INFO( "recording done!" );
                printf( "recording done!\n" );
            }
            else
            {
            }
        }
    }
}

RideHalError_e SampleRecorder::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    return ret;
}

RideHalError_e SampleRecorder::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

REGISTER_SAMPLE( Recorder, SampleRecorder );

}   // namespace sample
}   // namespace ridehal
