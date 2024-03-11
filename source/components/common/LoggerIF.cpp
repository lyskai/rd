// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/LoggerIF.hpp"

namespace ride
{
namespace hal
{

RideHalError_e LoggerIF::Init( const char *pName, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_name = pName;
    if ( nullptr == pLogger )
    {
        ret = m_defaultLogger.Init( pName );
        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            m_pLogger = &m_defaultLogger;
        }
    }
    else
    {
        m_pLogger = pLogger;
    }

    return ret;
}

void LoggerIF::Log( Logger_Level_e level, const char *pFormat, ... )
{
    va_list args;

    if ( nullptr != m_pLogger )
    {
        va_start( args, pFormat );
        m_pLogger->Log( level, pFormat, args );
        va_end( args );
    }
}

void LoggerIF::Log( Logger_Level_e level, const char *pFormat, va_list args )
{
    if ( nullptr != m_pLogger )
    {
        m_pLogger->Log( level, pFormat, args );
    }
}

const char *LoggerIF::GetName()
{
    return m_name.c_str();
}

Logger *LoggerIF::GetLogger()
{
    return m_pLogger;
}

RideHalError_e LoggerIF::SetLogger( Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == pLogger )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        m_pLogger = pLogger;
    }

    return ret;
}

}   // namespace hal
}   // namespace ride
