// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleCamera.hpp"


namespace ridehal
{
namespace sample
{

SampleCamera::SampleCamera() {}
SampleCamera ::~SampleCamera() {}

RideHalError_e SampleCamera::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_inputId = Get( config, "input_id", -1 );
        if ( -1 == m_inputId )
        {
            RIDEHAL_ERROR( "invalid input id = %d\n", m_inputId );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

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
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // TODO: call camera Init
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleCamera::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

RideHalError_e SampleCamera::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

RideHalError_e SampleCamera::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace ridehal
