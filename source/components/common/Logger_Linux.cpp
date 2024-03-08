// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Logger.hpp"
#include <syslog.h>

namespace ride
{
namespace hal
{

static int s_rideHalLoggerLevelToJournalPriotity[] = {
        LOG_DEBUG,   /* LOGGER_LEVEL_VERBOSE */
        LOG_INFO,    /* LOGGER_LEVEL_DEBUG */
        LOG_NOTICE,  /* LOGGER_LEVEL_INFO */
        LOG_WARNING, /* LOGGER_LEVEL_WARN */
        LOG_ERR      /* LOGGER_LEVEL_ERROR */
};

void Logger::Logger_DefaultCallback( const void *pPriv, Logger_Level_e level, const char *pFormat,
                                     va_list args )
{
    std::string strFmt;
    int priority = s_rideHalLoggerLevelToJournalPriotity[level];
    const char *pName = (const char *) pPriv;
    strFmt = std::string( pName ) + " : " + std::string( pFormat );
    vsyslog( priority, strFmt.c_str(), args );
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