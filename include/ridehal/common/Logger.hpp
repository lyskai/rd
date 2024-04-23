#ifndef _RIDEHAL_LOGGER_HPP_
#define _RIDEHAL_LOGGER_HPP_

#include "ridehal/common/Types.hpp"
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <string>

namespace ridehal
{
namespace common
{

#ifndef DISABLE_RIDEHAL_LOG
#define RIDEHAL_DECLARE_LOGGER() Logger m_logger

#define RIDEHAL_LOGGER_INIT( pName, level ) m_logger.Init( pName, level )
#define RIDEHAL_LOGGER_DEINIT() m_logger.Deinit()

#define RIDEHAL_LOGGER_LOG( logger, level, format, ... )                                           \
    ( logger ).Log( level, "%s:%d " format, __FILE__, __LINE__, ##__VA_ARGS__ )

#define RIDEHAL_VERBOSE( format, ... )                                                             \
    RIDEHAL_LOGGER_LOG( m_logger, LOGGER_LEVEL_VERBOSE, "VERBOSE: " format, ##__VA_ARGS__ )
#define RIDEHAL_DEBUG( format, ... )                                                               \
    RIDEHAL_LOGGER_LOG( m_logger, LOGGER_LEVEL_DEBUG, "DEBUG: " format, ##__VA_ARGS__ )
#define RIDEHAL_INFO( format, ... )                                                                \
    RIDEHAL_LOGGER_LOG( m_logger, LOGGER_LEVEL_INFO, "INFO: " format, ##__VA_ARGS__ )
#define RIDEHAL_WARN( format, ... )                                                                \
    RIDEHAL_LOGGER_LOG( m_logger, LOGGER_LEVEL_WARN, "WARN: " format, ##__VA_ARGS__ )
#define RIDEHAL_ERROR( format, ... )                                                               \
    RIDEHAL_LOGGER_LOG( m_logger, LOGGER_LEVEL_ERROR, "ERROR: " format, ##__VA_ARGS__ )

#define RIDEHAL_LOG_VERBOSE( format, ... )                                                         \
    RIDEHAL_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_VERBOSE, "VERBOSE: " format,            \
                        ##__VA_ARGS__ )
#define RIDEHAL_LOG_DEBUG( format, ... )                                                           \
    RIDEHAL_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_DEBUG, "DEBUG: " format, ##__VA_ARGS__ )
#define RIDEHAL_LOG_INFO( format, ... )                                                            \
    RIDEHAL_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_INFO, "INFO: " format, ##__VA_ARGS__ )
#define RIDEHAL_LOG_WARN( format, ... )                                                            \
    RIDEHAL_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_WARN, "WARN: " format, ##__VA_ARGS__ )
#define RIDEHAL_LOG_ERROR( format, ... )                                                           \
    RIDEHAL_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_ERROR, "ERROR: " format, ##__VA_ARGS__ )
#else
#define RIDEHAL_DECLARE_LOGGER()

#define RIDEHAL_LOGGER_INIT( pName, level ) RIDEHAL_ERROR_NONE
#define RIDEHAL_LOGGER_DEINIT() RIDEHAL_ERROR_NONE

#define RIDEHAL_LOGGER_LOG( logger, level, format, ... )

#define RIDEHAL_VERBOSE( format, ... )
#define RIDEHAL_DEBUG( format, ... )
#define RIDEHAL_INFO( format, ... )
#define RIDEHAL_WARN( format, ... )
#define RIDEHAL_ERROR( format, ... )

#define RIDEHAL_LOG_VERBOSE( format, ... )
#define RIDEHAL_LOG_DEBUG( format, ... )
#define RIDEHAL_LOG_INFO( format, ... )
#define RIDEHAL_LOG_WARN( format, ... )
#define RIDEHAL_LOG_ERROR( format, ... )
#endif

/** @brief The message log level */
typedef enum
{
    LOGGER_LEVEL_VERBOSE, /**< The level for the verbose message */
    LOGGER_LEVEL_DEBUG,   /**< The level for the debug message */
    LOGGER_LEVEL_INFO,    /**< The level for the information message */
    LOGGER_LEVEL_WARN,    /**< The level for the warning message */
    LOGGER_LEVEL_ERROR    /**< The level for the error message */
} Logger_Level_e;


/** @brief The handle to the log system  */
typedef void *Logger_Handle_t;

/**
 * @brief A callback to log a message
 * @param[in] hHandle the customer used log system handle
 * @param[in] level the message log level
 * @param[in] pFormat the message format
 * @param[in] args variable arguments
 * @return void
 */
typedef void ( *Logger_Log_t )( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                                va_list args );

/**
 * @brief A callback to create the customer used log system handle
 * @param[in] pName the name of the logger
 * @param[in] level the message log level
 * @param[out] pHandle [out] the pointer to return the created customer used log system handle
 * @return RIDEHAL_ERROR_NONE on success, others on failure
 */
typedef RideHalError_e ( *Logger_Create_t )( const char *pName, Logger_Level_e level,
                                             Logger_Handle_t *pHandle );

/**
 * @brief A callback to destroy the customer used log system handle
 * @param[in] hHandle the customer used log system handle
 * @return void
 */
typedef void ( *Logger_Destroy_t )( Logger_Handle_t hHandle );

/**
 * @brief ridehal::Logger
 *
 * Logger
 */
class Logger
{
public:
    Logger();
    ~Logger();

    /**
     * @brief Initialize the logger
     * @param[in] pName the name of the logger
     * @param[in] level the message log level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief deinitialize the logger
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @brief Logger a message
     * @param[in] level the message log level
     * @param[in] pFormat the message format
     * @param[in] ... variable arguments
     * @return void
     */
    void Log( Logger_Level_e level, const char *pFormat, ... );

    /**
     * @brief Logger a message
     * @param[in] level the message log level
     * @param[in] pFormat the message format
     * @param[in] args variable arguments
     * @return void
     */
    void Log( Logger_Level_e level, const char *pFormat, va_list args );
    /**
     * @brief Setup the logger backend fuction pointers
     * @param[in] logFnc the function pointer that do log
     * @param[in] createFnc the function pointer that do create the implementation related handle
     * @param[in] destoryFnc the function pointer that do destroy the implementation related handle
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */

    static RideHalError_e Setup( Logger_Log_t logFnc, Logger_Create_t createFnc,
                                 Logger_Destroy_t destoryFnc );
    /**
     * @brief Get a default RideHal logger
     * @return void
     */
    static Logger &GetDefault();

private:
    /**
     * @brief Decide the logger actual level used
     * @param[in] pName the name of the logger
     * @param[in] level the message log level
     * @return The actual level used
     */
    Logger_Level_e DecideLoggerLevel( std::string name, Logger_Level_e level );

    static void DefaultLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                            va_list args );
    static RideHalError_e DefaultCreate( const char *pName, Logger_Level_e level,
                                         Logger_Handle_t *pHandle );
    static void DefaultDestory( Logger_Handle_t hHandle );

private:
    Logger_Handle_t m_hHandle = nullptr;
    Logger_Level_e m_level = LOGGER_LEVEL_ERROR;

    static Logger_Log_t s_logFnc;
    static Logger_Create_t s_createFnc;
    static Logger_Destroy_t s_destroyFnc;

    static std::mutex s_lock;
    static Logger s_defaultLogger;

};   // class Logger

}   // namespace common
}   // namespace ridehal

#endif   // _RIDEHAL_LOGGER_HPP_