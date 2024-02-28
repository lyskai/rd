// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Logger.hpp"

namespace ride
{
namespace hal
{

void Logger::Logger_DefaultCallback( const void *pPriv, Logger_Level_e level, const char *pFormat,
                                     va_list args )
{
    const char *pName = (const char *) pPriv;

    printf( "%s: ", pName );
    vprintf( pFormat, args );
}

RideHalError_e Logger::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == pName )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        m_Name = pName;
        m_callback = Logger_DefaultCallback;
        m_pPriv = (void *) m_Name.c_str();
        m_level = DecideLoggerLevel( pName, level );
    }

    return ret;
}

}   // namespace hal
}   // namespace ride