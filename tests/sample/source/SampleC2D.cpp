// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleC2D.hpp"


namespace ridehal
{
namespace sample
{

SampleC2D::SampleC2D() {}
SampleC2D::~SampleC2D() {}

RideHalError_e SampleC2D::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_config.outputResolution.width = Get( config, "output_width", 1928 );
    if ( 0 == m_config.outputResolution.width )
    {
        RIDEHAL_ERROR( "invalid output_width\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputResolution.height = Get( config, "output_height", 1208 );
    if ( 0 == m_config.outputResolution.height )
    {
        RIDEHAL_ERROR( "invalid output_height\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputFormat = Get( config, "output_format", RIDE_HAL_IMAGE_FORMAT_UYVY );
    if ( RIDE_HAL_IMAGE_FORMAT_MAX == m_config.outputFormat )
    {
        RIDEHAL_ERROR( "invalid output_format\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numOfInputs = Get( config, "batch_size", 1 );
    if ( 0 == m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "invalid batch_size\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    m_config.numOfOutputs = 1;
    m_config.batchSize = m_config.numOfInputs;

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputConfigs[i].inputResolution.width =
                Get( config, "input_width" + std::to_string( i ), 1928 );
        if ( 0 == m_config.inputConfigs[i].inputResolution.width )
        {
            RIDEHAL_ERROR( "invalid input_width%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputResolution.height =
                Get( config, "input_height" + std::to_string( i ), 1208 );
        if ( 0 == m_config.inputConfigs[i].inputResolution.height )
        {
            RIDEHAL_ERROR( "invalid input_height%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputFormat =
                Get( config, "input_format" + std::to_string( i ), RIDE_HAL_IMAGE_FORMAT_NV12 );
        if ( RIDE_HAL_IMAGE_FORMAT_MAX == m_config.inputConfigs[i].inputFormat )
        {
            RIDEHAL_ERROR( "invalid input_format%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.topX = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.topX >= m_config.inputConfigs[i].inputResolution.width )
        {
            RIDEHAL_ERROR( "invalid roi_x%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.topY = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.topY >= m_config.inputConfigs[i].inputResolution.height )
        {
            RIDEHAL_ERROR( "invalid roi_y%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.width = Get( config, "roi_width" + std::to_string( i ),
                                                  m_config.inputConfigs[i].inputResolution.width );
        if ( 0 == m_config.inputConfigs[i].ROI.width )
        {
            RIDEHAL_ERROR( "invalid roi_width%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.height =
                Get( config, "roi_height" + std::to_string( i ),
                     m_config.inputConfigs[i].inputResolution.height );
        if ( 0 == m_config.inputConfigs[i].ROI.height )
        {
            RIDEHAL_ERROR( "invalid roi_height%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
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

    return ret;
}

RideHalError_e SampleC2D::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, m_config.numOfInputs,
                                m_config.outputResolution.width, m_config.outputResolution.height,
                                m_config.outputFormat, RIDE_HAL_BUFFER_USAGE_GPU );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_c2d.Init( name.c_str(), &m_config );
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

RideHalError_e SampleC2D::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_c2d.Start();
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleC2D::ThreadMain, this );
    }

    return ret;
}

void SampleC2D::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.frames[0].frameId, frames.frames[0].timestamp );
            std::shared_ptr<SharedBuffer_t> buffer = m_imagePool.Get();
            if ( nullptr != buffer )
            {
                std::vector<RideHal_SharedBuffer_t> inputs;
                for ( auto &frame : frames.frames )
                {
                    inputs.push_back( frame.buffer->sharedBuffer );
                }
                PROFILER_BEGIN();
                ret = m_c2d.Execute( inputs.data(), inputs.size(), &buffer->sharedBuffer, 1 );
                if ( RIDE_HAL_ERROR_NONE == ret )
                {
                    PROFILER_END();
                    CamFrames_t outFrames;
                    CamFrame_t frame;
                    frame.buffer = buffer;
                    frame.frameId = frames.frames[0].frameId;
                    frame.timestamp = frames.frames[0].timestamp;
                    outFrames.frames.push_back( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    RIDEHAL_ERROR( "c2d execute failed for %" PRIu64 " : %d",
                                   frames.frames[0].frameId, ret );
                }
            }
        }
    }
}

RideHalError_e SampleC2D::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    ret = m_c2d.Stop();

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleC2D::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_c2d.Deinit();

    return ret;
}

REGISTER_SAMPLE( C2D, SampleC2D );

}   // namespace sample
}   // namespace ridehal
