// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleTinyViz.hpp"


namespace ridehal
{
namespace sample
{

SampleTinyViz::SampleTinyViz() {}
SampleTinyViz::~SampleTinyViz() {}

RideHalError_e SampleTinyViz::ParseConfig( SampleConfig_t &config )
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

    return ret;
}

RideHalError_e SampleTinyViz::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        bool bOK = m_tinyViz.init( QRide::Stack::TinyVizIF::PixelFormat::NV12 );
        if ( false == bOK )
        {
            RIDEHAL_ERROR( "init tinyviz failed\n" );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            m_tinyViz.addCamera( "CAM0", m_width, m_height );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleTinyViz::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    if ( !m_tinyViz.start() )
    {
        RIDEHAL_ERROR( "start tinyviz failed\n" );
        ret = RIDE_HAL_ERROR_FAIL;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleTinyViz::ThreadMain, this );
    }

    return ret;
}

void SampleTinyViz::ThreadMain()
{
    int ret;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        CamFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            std::string camName = "CAM0";
            frame = frames.frames[0];
            RIDEHAL_DEBUG( "receive frameId %llu, timestamp %llu\n", frame.frameId,
                           frame.timestamp );
            m_tinyViz.addData( camName, frame );
        }
    }
}

RideHalError_e SampleTinyViz::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    m_tinyViz.stop();

    return ret;
}

RideHalError_e SampleTinyViz::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

}   // namespace sample
}   // namespace ridehal
