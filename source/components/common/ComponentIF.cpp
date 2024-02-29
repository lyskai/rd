// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{

RideHalError_e ComponentIF::Init( const char *pName, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_INITIAL != m_State )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else
    {
        m_Name = pName;
        if ( nullptr == pLogger )
        {
            ret = m_DefaultLogger.Init( pName );
            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                m_pLogger = &m_DefaultLogger;
            }
        }
        else
        {
            m_pLogger = pLogger;
        }
    }

    return ret;
}

void ComponentIF::Log( Logger_Level_e level, const char *pFormat, ... )
{
    va_list args;

    if ( nullptr != m_pLogger )
    {
        va_start( args, pFormat );
        m_pLogger->Log( level, pFormat, args );
        va_end( args );
    }
}

RideHal_ComponentState_t ComponentIF::GetState()
{
    return m_State;
}

}   // namespace hal
}   // namespace ride
