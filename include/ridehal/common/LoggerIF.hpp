// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_LOGGER_IF_HPP_
#define _RIDE_HAL_LOGGER_IF_HPP_

#include <string>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/Types.hpp"


namespace ridehal
{
namespace common
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

/// @brief ridehal::LoggerIF
///
/// Logger Interface
class LoggerIF
{
public:
    LoggerIF() = default;
    ~LoggerIF() = default;

    /// @brief Get the name of the logger interface
    /// @return the name of the logger interface
    const char *GetName();

    /// @brief Get the logger pointer of the logger interface
    /// @return the logger pointer of the logger interface
    Logger *GetLogger();

    /// @brief Change to use a customized logger
    /// @param pLogger the customized logger to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SetLogger( Logger *pLogger );

protected:
    /// @brief Initialize the logger interface
    /// @param pName the logger unique instance name
    /// @param pLogger the logger used by the logger to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, Logger *pLogger = nullptr );

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
    std::string m_name;
    Logger m_defaultLogger;
    Logger *m_pLogger = nullptr;
};   // class LoggerIF

}   // namespace common
}   // namespace ridehal

#endif   // _RIDE_HAL_LOGGER_IF_HPP_
