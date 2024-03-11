// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Logger.hpp"
#include <sys/slog2.h>

namespace ride
{
namespace hal
{

extern "C" char *__progname;

static uint8_t s_rideHalLoggerLevelToSlog2Level[] = {
        SLOG2_DEBUG2,  /* LOGGER_LEVEL_VERBOSE */
        SLOG2_DEBUG1,  /* LOGGER_LEVEL_DEBUG */
        SLOG2_INFO,    /* LOGGER_LEVEL_INFO */
        SLOG2_WARNING, /* LOGGER_LEVEL_WARN */
        SLOG2_ERROR    /* LOGGER_LEVEL_ERROR */
};

void Logger::Logger_DefaultCallback( const void *pPriv, Logger_Level_e level, const char *pFormat,
                                     va_list args )
{
    char msg[256];
    slog2_buffer_t hBuffer = (slog2_buffer_t) pPriv;

    vsnprintf( msg, sizeof( msg ), pFormat, args );
    (void) slog2c( hBuffer, 0, s_rideHalLoggerLevelToSlog2Level[level], msg );
}

RideHalError_e Logger::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    slog2_buffer_t hBuffer = nullptr;
    slog2_buffer_set_config_t bufferConfig;

    if ( nullptr == pName )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        /* register slog2 buffer */
        bufferConfig.num_buffers = 1;
        bufferConfig.verbosity_level = SLOG2_DEBUG2;

        bufferConfig.buffer_config[0].buffer_name = pName;
        bufferConfig.buffer_set_name = __progname;
        bufferConfig.buffer_config[0].num_pages = 32;

        int rv = slog2_register( &bufferConfig, &hBuffer, SLOG2_TRY_REUSE_BUFFER_SET );
        if ( 0 == rv )
        {
            m_name = pName;
            m_callback = Logger_DefaultCallback;
            m_pPriv = (void *) hBuffer;
            m_level = DecideLoggerLevel( pName, level );
            (void) slog2f( hBuffer, 0, SLOG2_DEBUG2, "ridehal logger for %s online with level %d",
                           pName, m_level );
        }
        else
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    return ret;
}

}   // namespace hal
}   // namespace ride