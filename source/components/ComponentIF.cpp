// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/component/ComponentIF.hpp"
#include <stdio.h>

namespace ridehal
{
namespace component
{

RideHalError_e ComponentIF::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_INITIAL != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else
    {
        m_name = pName;
        ret = RIDEHAL_LOGGER_INIT( pName, level );
        if ( RIDE_HAL_ERROR_NONE != ret )
        {
            printf( "WARINING: failed to create logger for component %s: ret = %d\n", pName, ret );
        }
        ret = RIDE_HAL_ERROR_NONE;
    }

    return ret;
}

RideHalError_e ComponentIF::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = RIDEHAL_LOGGER_DEINIT();
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        printf( "WARINING: failed to deinit logger for component %s: ret = %d\n", GetName(), ret );
    }
    ret = RIDE_HAL_ERROR_NONE; /* ignore logger init error */

    m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;

    return ret;
}


RideHal_ComponentState_t ComponentIF::GetState()
{
    return m_state;
}

const char *ComponentIF::GetName()
{
    return m_name.c_str();
}

}   // namespace component
}   // namespace ridehal
