// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technolgtest_Bufferogies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/common/Logger.hpp"

using namespace ridehal::common;

typedef struct
{
    std::string name;
} Logger_HandleContextUser_t;

static char s_LoggerMsg[1024] = { 0 };

#define MSG_PREFIX( name, b )                                                                      \
    std::string( #name ) + " " + std::string( __FILE__ ) + ":" + std::to_string( __LINE__ - b ) +  \
            " "

static void UserLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                     va_list args )
{
    int len = 0;
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    len = snprintf( s_LoggerMsg, sizeof( s_LoggerMsg ), "%s ", pContext->name.c_str() );
    (void) vsnprintf( &s_LoggerMsg[len], sizeof( s_LoggerMsg ) - len, pFormat, args );
}

static RideHalError_e UserLoggerHandleCreate( const char *pName, Logger_Level_e level,
                                              Logger_Handle_t *pHandle )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        Logger_HandleContextUser_t *pContext = new Logger_HandleContextUser_t;
        if ( nullptr != pContext )
        {
            pContext->name = pName;
            (void) level;
            *pHandle = (Logger_Handle_t) pContext;
        }
        else
        {
            ret = RIDEHAL_ERROR_NOMEM;
        }
    }

    return ret;
}

static void UserLoggerHandleDestroy( Logger_Handle_t hHandle )
{
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    if ( nullptr != pContext )
    {
        delete pContext;
    }
}


static void TestDefaultLogger()
{
    std::string rst;
    /* default logger level is ERROR */
    RIDEHAL_LOG_WARN( "A warn messgae is ignored", (uint32_t) 1234, 1.23431 );
    ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
    s_LoggerMsg[0] = '\0';

    RIDEHAL_LOG_ERROR( "A Fatal error: %.2f", 1.2345 );
    rst = MSG_PREFIX( RIHDEHAL, 1 ) + std::string( "ERROR: A Fatal error: 1.23" );
    ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
    s_LoggerMsg[0] = '\0';
}

class LoggerUser
{
public:
    LoggerUser() {}
    ~LoggerUser() {}

    RideHalError_e Init( const char *pName, Logger_Level_e level )
    {
        return RIDEHAL_LOGGER_INIT( pName, level );
    }

    RideHalError_e Deinit() { return RIDEHAL_LOGGER_DEINIT(); }

    void TestLoggerVerbose()
    {
        std::string rst;
        RIDEHAL_VERBOSE( "a=%d", 1234 );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "VERBOSE: a=1234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_VERBOSE( "str=%s a=%d", "hello world", 1234 );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "VERBOSE: str=hello world a=1234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_INFO( "a=%u c=%.3f", (uint32_t) 1234, 1.23431 );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "INFO: a=1234 c=1.234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_ERROR( "A Fatal error: 0x%x", 0xdeadbeef );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "ERROR: A Fatal error: 0xdeadbeef" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';
    }

    void TestLoggerInfo()
    {
        std::string rst;
        RIDEHAL_DEBUG( "a debug message is ignored" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_INFO( "a=%u c=%.3f", (uint32_t) 1234, 1.23431 );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "INFO: a=1234 c=1.234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_ERROR( "A Fatal error: 0x%x", 0xdeadbeef );
        rst = MSG_PREFIX( Test, 1 ) + std::string( "ERROR: A Fatal error: 0xdeadbeef" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';
    }

private:
    RIDEHAL_DECLARE_LOGGER();
};

TEST( Logger, SANITY_Logger )
{
    RideHalError_e ret;
    LoggerUser loggerUser;

    ret = Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /* only alow to setup once, the second setup will fail */
    ret = Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    ret = loggerUser.Init( "Test", LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    loggerUser.TestLoggerVerbose();
    ret = loggerUser.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = loggerUser.Init( "Test", LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    loggerUser.TestLoggerInfo();
    ret = loggerUser.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    TestDefaultLogger();
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
