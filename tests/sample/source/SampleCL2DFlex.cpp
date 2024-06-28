// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleCL2DFlex.hpp"


namespace ridehal
{
namespace sample
{

SampleCL2DFlex::SampleCL2DFlex() {}
SampleCL2DFlex::~SampleCL2DFlex() {}

RideHalError_e SampleCL2DFlex::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_config.inputWidth = Get( config, "input_width", 1920 );
    if ( 0 == m_config.inputWidth )
    {
        RIDEHAL_ERROR( "invalid input_width\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.inputHeight = Get( config, "input_height", 1024 );
    if ( 0 == m_config.inputHeight )
    {
        RIDEHAL_ERROR( "invalid input_height\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.inputFormat = Get( config, "input_format", RIDEHAL_IMAGE_FORMAT_NV12 );
    if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.inputFormat )
    {
        RIDEHAL_ERROR( "invalid input_format\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputFormat = Get( config, "output_format", RIDEHAL_IMAGE_FORMAT_RGB888 );
    if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.outputFormat )
    {
        RIDEHAL_ERROR( "invalid output_format\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputWidth = m_config.inputWidth;
    m_inputHeight = m_config.inputHeight;
    m_inputFormat = m_config.inputFormat;
    m_outputFormat = m_config.outputFormat;

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
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

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleCL2DFlex::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_ImageProps_t imgProp;
        imgProp.format = m_config.outputFormat;
        imgProp.batchSize = 1;
        imgProp.width = m_config.inputWidth;
        imgProp.height = m_config.inputHeight;
        imgProp.stride[0] = m_config.inputWidth * 3;
        imgProp.actualHeight[0] = m_config.inputHeight;
        imgProp.numPlanes = 1;
        imgProp.extraPadding = 0;

        ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_CL2DFlex.Init( name.c_str(), &m_config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleCL2DFlex::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_CL2DFlex.Start();
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleCL2DFlex::ThreadMain, this );
    }

    return ret;
}

void SampleCL2DFlex::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> buffer = m_imagePool.Get();
            if ( nullptr != buffer )
            {
                std::vector<RideHal_SharedBuffer_t> inputs;
                for ( auto &frame : frames.frames )
                {
                    inputs.push_back( frame.buffer->sharedBuffer );
                }

                PROFILER_BEGIN();
                ret = m_CL2DFlex.Execute( inputs.data(), &buffer->sharedBuffer );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    PROFILER_END();
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = buffer;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    RIDEHAL_ERROR( "CL2D execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                   ret );
                }
            }
        }
    }
}

RideHalError_e SampleCL2DFlex::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    ret = m_CL2DFlex.Stop();

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleCL2DFlex::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_CL2DFlex.Deinit();

    return ret;
}

REGISTER_SAMPLE( CL2DFlex, SampleCL2DFlex );

}   // namespace sample
}   // namespace ridehal
