// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{

RideHalError_e ComponentIF::Init( std::string name, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_INITIAL != m_State )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else
    {
        m_Name = name;
        m_pLogger = pLogger;
    }

    return ret;
}

void ComponentIF::Log( Logger::Level_e level, const char *pFormat, ... )
{
    va_list args;

    va_start( args, pFormat );
    // TODO:
    vprintf( pFormat, args );
    va_end( args );
}

RideHal_ComponentState_t ComponentIF::GetState()
{
    return m_State;
}

}   // namespace hal
}   // namespace ride
