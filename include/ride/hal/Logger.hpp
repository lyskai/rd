// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_LOGGER_HPP_
#define _RIDE_HAL_LOGGER_HPP_

#include <stdarg.h>
#include <stdio.h>

namespace ride
{
namespace hal
{

typedef void ( *Logger_Callback_t )( const void *pPriv, const char *pFormat, va_list args );

/// @brief ride::hal::Logger
///
/// Logger
class Logger
{
public:
    typedef enum
    {
        VERBOSE,
        DEBUG,
        INFO,
        WARN,
        ERROR
    } Level_e;

public:
    Logger( Logger_Callback_t callback, const void *pPriv );
    ~Logger() = default;

    void Log( Logger::Level_e level, const char *pFormat, ... );
    void Log( Logger::Level_e level, const char *pFormat, va_list args );

private:
    void *m_pPriv = nullptr;
    Logger_Callback_t m_callback = nullptr;
};   // class ComponentIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_LOGGER_HPP_
