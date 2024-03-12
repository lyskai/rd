// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleDataReader.hpp"
#include <chrono>
#include <time.h>

namespace ridehal
{
namespace sample
{

SampleDataReader::SampleDataReader() {}
SampleDataReader::~SampleDataReader() {}

RideHalError_e SampleDataReader::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_width = Get( config, "width", 0 );
    if ( 0 == m_width )
    {
        RIDEHAL_ERROR( "invalid width = %d\n", m_width );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_height = Get( config, "height", 0 );
    if ( 0 == m_height )
    {
        RIDEHAL_ERROR( "invalid height = %d\n", m_height );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_dataPath = Get( config, "data_path", "" );
    if ( "" == m_dataPath )
    {
        RIDEHAL_ERROR( "no data path\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleDataReader::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_imagePool.Init( name, GetLogger(), m_poolSize, m_width, m_height,
                                RIDE_HAL_IMAGE_FORMAT_NV12, RIDE_HAL_BUFFER_USAGE_CAMERA );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleDataReader::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    m_stop = false;
    m_thread = std::thread( &SampleDataReader::ThreadMain, this );
    return ret;
}

RideHalError_e SampleDataReader::LoadImage( std::shared_ptr<SharedBuffer_t> image,
                                            std::string path )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    FILE *file = nullptr;
    size_t length = 0;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        RIDEHAL_ERROR( "Failed to open file %s", path.c_str() );
        ret = RIDE_HAL_ERROR_EXISTS;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( image->sharedBuffer.size < length )
        {
            RIDEHAL_ERROR( "Invalid image file %s", path.c_str() );
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( image->sharedBuffer.data(), 1, length, file );
        fclose( file );
        if ( length != r )
        {
            RIDEHAL_ERROR( "failed to read image file %s", path.c_str() );
            ret = RIDE_HAL_ERROR_ACCES;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    RIDEHAL_DEBUG( "Loading Image %s %s", path.c_str(),
                   ( RIDE_HAL_ERROR_NONE == ret ) ? "OK" : "FAIL" );

    return ret;
}

void SampleDataReader::ThreadMain()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    uint32_t index = 0;
    uint64_t frameId = 0;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        CamFrame_t frame;
        auto now = std::chrono::high_resolution_clock::now();
        std::shared_ptr<SharedBuffer_t> buffer = m_imagePool.Get();
        if ( nullptr != buffer )
        {
            std::string path = m_dataPath + "/" + std::to_string( index ) + ".nv12";
            ret = LoadImage( buffer, path );
            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                struct timespec ts;
                clock_gettime( CLOCK_MONOTONIC, &ts );
                frame.buffer = buffer;
                frame.frameId = frameId++;
                frame.timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                frames.frames.push_back( frame );
                m_pub.Publish( frames );
                index++;
            }
            else
            {
                index = 0;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>( now - end ).count();
        if ( 33 > elapsedMs )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 33 - elapsedMs ) );
        }
    }
}

RideHalError_e SampleDataReader::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    return ret;
}

RideHalError_e SampleDataReader::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

}   // namespace sample
}   // namespace ridehal
