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

    m_winW = Get( config, "winW", 1920 );
    if ( 0 == m_winW )
    {
        RIDEHAL_ERROR( "invalid winW = %d\n", m_winW );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_winH = Get( config, "winH", 1080 );
    if ( 0 == m_winH )
    {
        RIDEHAL_ERROR( "invalid winH = %d\n", m_winH );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_width = Get( config, "width", 1920 );
    if ( 0 == m_width )
    {
        RIDEHAL_ERROR( "invalid width = %d\n", m_width );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_height = Get( config, "height", 1024 );
    if ( 0 == m_height )
    {
        RIDEHAL_ERROR( "invalid height = %d\n", m_height );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    auto fstr = Get( config, "format", "nv12" );
    if ( fstr == "nv12" )
    {
        m_format = QRide::Stack::TinyVizIF::PixelFormat::NV12;
    }
    else if ( fstr == "uyvy" )
    {
        m_format = QRide::Stack::TinyVizIF::PixelFormat::UYVY;
    }
    else if ( fstr == "rgb" )
    {
        m_format = QRide::Stack::TinyVizIF::PixelFormat::RGB;
    }
    else
    {
        RIDEHAL_ERROR( "invalid format %s\n", fstr.c_str() );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_objTopicName = Get( config, "obj_topic", "" );
    if ( "" == m_objTopicName )
    {
        RIDEHAL_ERROR( "no obj topic\n" );
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
        bool bOK = m_tinyViz.init( m_format, m_winW, m_winH );
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

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_objSub.Init( name, m_objTopicName );
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
        m_objThread = std::thread( &SampleTinyViz::ObjThreadMain, this );
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
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ", frame.frameId,
                           frame.timestamp );
            m_tinyViz.addData( camName, frame );
        }
    }
}

void SampleTinyViz::ObjThreadMain()
{
    int ret;
    while ( false == m_stop )
    {
        Road2DObjects_t objs;
        ret = m_objSub.Receive( objs );
        if ( 0 == ret )
        {
            std::string camName = "CAM0";
            RIDEHAL_DEBUG( "receive objects for frameId %" PRIu64 ", timestamp %" PRIu64 "\n ",
                           objs.frameId, objs.timestamp );
            m_tinyViz.addData( camName, objs );
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

REGISTER_SAMPLE( TinyViz, SampleTinyViz );

}   // namespace sample
}   // namespace ridehal
