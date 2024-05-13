// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleDataReader.hpp"
#include <chrono>
#include <time.h>

namespace ridehal
{
namespace sample
{

static std::string s_rideHalFormatToStr[RIDEHAL_IMAGE_FORMAT_MAX] = {
        ".rgb",  /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        ".bgr",  /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        ".uyvy", /* RIDEHAL_IMAGE_FORMAT_UYVY */
        ".nv12", /* RIDEHAL_IMAGE_FORMAT_NV12 */
        ".p010"  /* RIDEHAL_IMAGE_FORMAT_P010 */
};

SampleDataReader::SampleDataReader() {}
SampleDataReader::~SampleDataReader() {}

RideHalError_e SampleDataReader::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_numOfDataReaders = Get( config, "number", 1 );
    if ( 0 == m_numOfDataReaders )
    {
        RIDEHAL_ERROR( "invalid number\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        m_configs.reserve( m_numOfDataReaders );
    }

    for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
    {
        DataReaderConfig_t cfg;

        cfg.format = Get( config, "format" + std::to_string( i ), RIDEHAL_IMAGE_FORMAT_NV12 );
        if ( RIDEHAL_IMAGE_FORMAT_MAX == cfg.format )
        {
            RIDEHAL_ERROR( "invalid format%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        cfg.width = Get( config, "width" + std::to_string( i ), 1920 );
        if ( 0 == cfg.width )
        {
            RIDEHAL_ERROR( "invalid width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        cfg.height = Get( config, "height" + std::to_string( i ), 1024 );
        if ( 0 == cfg.height )
        {
            RIDEHAL_ERROR( "invalid height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        cfg.dataPath = Get( config, "data_path" + std::to_string( i ), "" );
        if ( "" == cfg.dataPath )
        {
            RIDEHAL_INFO( "using dummy camera frame for %u\n", i );
        }

        m_configs.push_back( cfg );
    }

    m_fps = Get( config, "fps", 30 );
    if ( 0 == m_fps )
    {
        RIDEHAL_ERROR( "invalid fps = %d\n", m_fps );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleDataReader::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_imagePools.resize( m_numOfDataReaders );
        for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            ret = m_imagePools[i].Init( name + std::to_string( i ), LOGGER_LEVEL_INFO, m_poolSize,
                                        m_configs[i].width, m_configs[i].height,
                                        m_configs[i].format, RIDEHAL_BUFFER_USAGE_CAMERA,
                                        m_bufferFlags );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleDataReader::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    m_stop = false;
    m_thread = std::thread( &SampleDataReader::ThreadMain, this );
    return ret;
}

RideHalError_e SampleDataReader::LoadImage( std::shared_ptr<SharedBuffer_t> image,
                                            std::string path )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    FILE *file = nullptr;
    size_t length = 0;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        RIDEHAL_ERROR( "Failed to open file %s", path.c_str() );
        ret = RIDEHAL_ERROR_ALREADY;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( image->sharedBuffer.size < length )
        {
            RIDEHAL_ERROR( "Invalid image file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( image->sharedBuffer.data(), 1, length, file );
        if ( length != r )
        {
            RIDEHAL_ERROR( "failed to read image file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    RIDEHAL_DEBUG( "Loading Image %s %s", path.c_str(),
                   ( RIDEHAL_ERROR_NONE == ret ) ? "OK" : "FAIL" );

    return ret;
}

void SampleDataReader::ThreadMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t index = 0;
    uint64_t frameId = 0;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        ret = RIDEHAL_ERROR_NONE;
        auto start = std::chrono::high_resolution_clock::now();
        PROFILER_BEGIN();
        for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            std::shared_ptr<SharedBuffer_t> buffer = m_imagePools[i].Get();
            if ( nullptr != buffer )
            {
                if ( m_configs[i].dataPath != "" )
                {
                    std::string path = m_configs[i].dataPath + "/" + std::to_string( index ) +
                                       s_rideHalFormatToStr[m_configs[i].format];
                    ret = LoadImage( buffer, path );
                }
                else
                {
                    // skip loading, using dummy data
                }
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    CamFrame_t frame;
                    struct timespec ts;
                    clock_gettime( CLOCK_MONOTONIC, &ts );
                    frame.buffer = buffer;
                    frame.frameId = frameId++;
                    frame.timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                    frames.frames.push_back( frame );
                }
                else
                {
                    ret = RIDEHAL_ERROR_ALREADY;
                }
            }
            else
            {
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            PROFILER_END();
            m_pub.Publish( frames );
            index++;
        }
        else if ( RIDEHAL_ERROR_ALREADY == ret )
        {
            index = 0;
            continue; /* retry load from beginning */
        }
        else if ( RIDEHAL_ERROR_NOMEM == ret )
        { /* sleep to wait buffer resource ready */
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        else
        {
            /* OK */
        }
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
        RIDEHAL_DEBUG( "Loading frame %" PRIu64 " cost %" PRIu64 "ms", frameId, elapsedMs );
        if ( ( 1000 / m_fps ) > elapsedMs )
        {
            std::this_thread::sleep_for(
                    std::chrono::milliseconds( ( 1000 / m_fps ) - elapsedMs ) );
        }
    }
}

RideHalError_e SampleDataReader::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleDataReader::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    return ret;
}

REGISTER_SAMPLE( DataReader, SampleDataReader );

}   // namespace sample
}   // namespace ridehal
