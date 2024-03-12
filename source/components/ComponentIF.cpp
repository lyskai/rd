// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/component/ComponentIF.hpp"

namespace ridehal
{
namespace component
{

RideHalError_e ComponentIF::Init( const char *pName, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_INITIAL != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else
    {
        ret = LoggerIF::Init( pName, pLogger );
    }

    return ret;
}

RideHal_ComponentState_t ComponentIF::GetState()
{
    return m_state;
}

}   // namespace component
}   // namespace ridehal
