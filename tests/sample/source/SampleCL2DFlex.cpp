// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


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

    m_config.outputWidth = Get( config, "output_width", 1920 );
    if ( 0 == m_config.outputWidth )
    {
        RIDEHAL_ERROR( "invalid output_width\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputHeight = Get( config, "output_height", 1024 );
    if ( 0 == m_config.outputHeight )
    {
        RIDEHAL_ERROR( "invalid output_height\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputFormat = Get( config, "output_format", RIDEHAL_IMAGE_FORMAT_RGB888 );
    if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.outputFormat )
    {
        RIDEHAL_ERROR( "invalid output_format\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numOfInputs = Get( config, "batch_size", 1 );
    if ( 0 == m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "invalid batch_size\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputWidths[i] = Get( config, "input_width" + std::to_string( i ), 1920 );
        if ( 0 == m_config.inputWidths[i] )
        {
            RIDEHAL_ERROR( "invalid input_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputHeights[i] = Get( config, "input_height" + std::to_string( i ), 1024 );
        if ( 0 == m_config.inputHeights[i] )
        {
            RIDEHAL_ERROR( "invalid input_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputFormats[i] =
                Get( config, "input_format" + std::to_string( i ), RIDEHAL_IMAGE_FORMAT_NV12 );
        if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.inputFormats[i] )
        {
            RIDEHAL_ERROR( "invalid input_format%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

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
        TRACE_ON( GPU );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat )
        {
            RideHal_ImageProps_t imgProp;
            imgProp.format = RIDEHAL_IMAGE_FORMAT_RGB888;
            imgProp.batchSize = m_config.numOfInputs;
            imgProp.width = m_config.outputWidth;
            imgProp.height = m_config.outputHeight;
            imgProp.stride[0] = m_config.outputWidth * 3;
            imgProp.actualHeight[0] = m_config.outputHeight;
            imgProp.numPlanes = 1;
            imgProp.planeBufSize[0] = 0;

            ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                    RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
        }
        else
        {
            ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, m_config.outputWidth,
                                    m_config.outputHeight, m_config.outputFormat,
                                    RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_CL2DFlex.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
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

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_CL2DFlex.Start();
    TRACE_END( SYSTRACE_TASK_START );
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
                TRACE_BEGIN( frames.FrameId( 0 ) );
                ret = m_CL2DFlex.Execute( inputs.data(), inputs.size(), &buffer->sharedBuffer );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
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

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_CL2DFlex.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleCL2DFlex::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_CL2DFlex.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( CL2DFlex, SampleCL2DFlex );

}   // namespace sample
}   // namespace ridehal
