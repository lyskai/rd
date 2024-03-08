// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_LOGGER_HPP_
#define _RIDE_HAL_LOGGER_HPP_

#include "ride/hal/Types.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string>

namespace ride
{
namespace hal
{

/* The following macros are provided to be used by RideHal components and utils only,
 * It's not for the application and any other usage */
#ifndef DISABLE_RIDEHAL_LOG
#define RIDEHAL_VERBOSE( format, ... ) Log( LOGGER_LEVEL_VERBOSE, format, ##__VA_ARGS__ )
#define RIDEHAL_DEBUG( format, ... ) Log( LOGGER_LEVEL_DEBUG, format, ##__VA_ARGS__ )
#define RIDEHAL_INFO( format, ... ) Log( LOGGER_LEVEL_INFO, format, ##__VA_ARGS__ )
#define RIDEHAL_WARN( format, ... ) Log( LOGGER_LEVEL_WARN, format, ##__VA_ARGS__ )
#define RIDEHAL_ERROR( format, ... ) Log( LOGGER_LEVEL_ERROR, format, ##__VA_ARGS__ )
#else
#define RIDEHAL_VERBOSE( format, ... )
#define RIDEHAL_DEBUG( format, ... )
#define RIDEHAL_INFO( format, ... )
#define RIDEHAL_WARN( format, ... )
#define RIDEHAL_ERROR( format, ... )
#endif

/// @brief The message log level
typedef enum
{
    LOGGER_LEVEL_VERBOSE,   /// The level for the verbose message
    LOGGER_LEVEL_DEBUG,     /// The level for the debug message
    LOGGER_LEVEL_INFO,      /// The level for the information message
    LOGGER_LEVEL_WARN,      /// The level for the warning message
    LOGGER_LEVEL_ERROR      /// The level for the error message
} Logger_Level_e;

typedef void ( *Logger_Callback_t )( const void *pPriv, Logger_Level_e level, const char *pFormat,
                                     va_list args );

/// @brief ride::hal::Logger
///
/// Logger
class Logger
{
public:
    Logger();
    ~Logger();

    /// @brief Initialize the logger with the default callback implemented
    /// @param pName the name of the logger
    /// @param level the message log level
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Initialize the logger with a customized callback and a private parameter
    /// @param callback the logger callback
    /// @param pPriv the private parameter when invoke the callback
    /// @param level the message log level
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( Logger_Callback_t callback, const void *pPriv,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param ... variable arguments
    /// @return void
    void Log( Logger_Level_e level, const char *pFormat, ... );

    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param args variable arguments
    /// @return void
    void Log( Logger_Level_e level, const char *pFormat, va_list args );

private:
    Logger_Level_e DecideLoggerLevel( std::string name, Logger_Level_e level );
    static void Logger_DefaultCallback( const void *pPriv, Logger_Level_e level,
                                        const char *pFormat, va_list args );

private:
    std::string m_Name; /* used to store the name of the logger when use API Init(name) */
    const void *m_pPriv = nullptr;
    Logger_Callback_t m_callback = nullptr;
    Logger_Level_e m_level = LOGGER_LEVEL_ERROR;
};   // class ComponentIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_LOGGER_HPP_
