// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Logger.hpp"
#include <stdlib.h>

namespace ride
{
namespace hal
{

Logger::Logger() {}

Logger::~Logger() {}

RideHalError_e Logger::Init( Logger_Callback_t callback, const void *pPriv, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == callback )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        m_callback = callback;
        m_pPriv = pPriv;
        m_level = level;
    }

    return ret;
}

void Logger::Log( Logger_Level_e level, const char *pFormat, ... )
{
    va_list args;

    if ( ( level >= m_level ) && ( nullptr != m_callback ) )
    {
        va_start( args, pFormat );
        m_callback( m_pPriv, level, pFormat, args );
        va_end( args );
    }
}

void Logger::Log( Logger_Level_e level, const char *pFormat, va_list args )
{
    if ( ( level >= m_level ) && ( nullptr != m_callback ) )
    {
        m_callback( m_pPriv, level, pFormat, args );
    }
}

Logger_Level_e Logger::DecideLoggerLevel( std::string name, Logger_Level_e level )
{
    Logger_Level_e loggerLevel = level;
    std::string envName = name + "_RIDEHAL_LOG_LEVEL";
    char *envValue = getenv( envName.c_str() );

    if ( envValue != NULL )
    {
        std::string strEnv = envValue;
        if ( strEnv == "VERBOSE" )
        {
            loggerLevel = LOGGER_LEVEL_VERBOSE;
        }
        else if ( strEnv == "DEBUG" )
        {
            loggerLevel = LOGGER_LEVEL_DEBUG;
        }
        else if ( strEnv == "INFO" )
        {
            loggerLevel = LOGGER_LEVEL_INFO;
        }
        else if ( strEnv == "WARN" )
        {
            loggerLevel = LOGGER_LEVEL_WARN;
        }
        else if ( strEnv == "ERROR" )
        {
            loggerLevel = LOGGER_LEVEL_ERROR;
        }
        else
        {
        }
    }

    return loggerLevel;
}

}   // namespace hal
}   // namespace ride