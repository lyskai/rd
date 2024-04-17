// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/sample/SampleCamera.hpp"
#include "ridehal/sample/SampleDataReader.hpp"
#include "ridehal/sample/SampleRemap.hpp"
#ifdef WITH_TINYVIZ
#include "ridehal/sample/SampleTinyViz.hpp"
#endif

#include <chrono>
#include <iostream>
#include <thread>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace ridehal::sample;

typedef struct
{
    std::string name;
} Logger_HandleContextUser_t;

std::condition_variable CV;

static void UserLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                     va_list args )
{
    char msg[256];
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    (void) vsnprintf( msg, sizeof( msg ), pFormat, args );
    printf( "%s: %s\n", pContext->name.c_str(), msg );
}

static RideHalError_e UserLoggerHandleCreate( const char *pName, Logger_Level_e level,
                                              Logger_Handle_t *pHandle )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
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
            ret = RIDE_HAL_ERROR_NORES;
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

void SignalHandler( int signal )
{
    std::cout << "Caught signal " << signal << std::endl;
    CV.notify_one();
}

typedef struct
{
    std::string name;
    SampleConfig_t config;
    std::string type;
} PipelineConfig_t;

int Usage( const char *program, int error )
{
    printf( "Usage: %s -n name -t type -k key -v value [-h]\n"
            "examples:\n"
            "%s -n CAM0 -t camera -k input_id -v 0 -k width -v 1920 -k height -v 1024 \\\n"
            "    -k topic -v /sensor/camera/CAM0/raw \\\n"
            "  -n CAM0_REMAP -t remap -k width -v 1152 -k height -v 768 \\\n"
            "    -k input_topic -v /sensor/camera/CAM0/raw \\\n"
            "    -k output_topic -v /sensor/camera/CAM0/rgb\n",
            program, program );
    return error;
}

int main( int argc, char *argv[] )
{
    RideHalError_e ret;
    std::vector<SampleIF *> samples;

    signal( SIGINT, SignalHandler );
    signal( SIGTERM, SignalHandler );

    std::vector<PipelineConfig_t> pipelineConfigs;
    PipelineConfig_t cameraConfig;
    std::string key;
    int flags, opt;
    while ( ( opt = getopt( argc, argv, "dn:t:k:v:h" ) ) != -1 )
    {
        switch ( opt )
        {
            case 'n':
            {
                PipelineConfig_t config;
                config.name = optarg;
                pipelineConfigs.push_back( config );
                break;
            }
            case 't':
            {
                PipelineConfig_t &config = pipelineConfigs.back();
                config.type = optarg;
                break;
            }
            case 'k':
            {
                key = optarg;
                break;
            }
            case 'v':
            {
                PipelineConfig_t &config = pipelineConfigs.back();
                config.config[key] = optarg;
                break;
            }
            case 'd':
                (void) Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
                break;
            case 'h':
                return Usage( argv[0], 0 );
                break;
            default:
                return Usage( argv[0], -1 );
                break;
        }
    }

    if ( 0 == pipelineConfigs.size() )
    {
        return Usage( argv[0], -1 );
    }

    for ( auto &config : pipelineConfigs )
    {
        SampleIF *pSample = SampleIF::Create( config.type );
        if ( nullptr != pSample )
        {
            ret = pSample->Init( config.name, config.config );
            if ( ret != RIDE_HAL_ERROR_NONE )
            {
                printf( "Init %s failed: ret = %d\n", config.name.c_str(), ret );
                return -1;
            }
            else
            {
                printf( "Init %s OK\n", config.name.c_str() );
                samples.push_back( pSample );
            }
        }
        else
        {
            printf( "Create %s with type %s failed\n", config.name.c_str(), config.type.c_str() );
            return -1;
        }
    }

    for ( auto sample : samples )
    {
        ret = sample->Start();
        if ( ret != RIDE_HAL_ERROR_NONE )
        {
            printf( "Start %s failed: ret = %d\n", sample->GetName(), ret );
            return -1;
        }
        else
        {
            printf( "Start %s OK\n", sample->GetName() );
        }
    }

    {
        // wait for signal
        std::mutex m;
        std::unique_lock<std::mutex> lock( m );
        CV.wait( lock );
    }

    for ( auto sample : samples )
    {
        ret = sample->Stop();
        if ( ret != RIDE_HAL_ERROR_NONE )
        {
            printf( "Stop %s failed: ret = %d\n", sample->GetName(), ret );
            return -1;
        }
        else
        {
            printf( "Stop %s OK\n", sample->GetName() );
        }
    }

    for ( auto sample : samples )
    {
        ret = sample->Deinit();
        if ( ret != RIDE_HAL_ERROR_NONE )
        {
            printf( "Deinit %s failed: ret = %d\n", sample->GetName(), ret );
            return -1;
        }
        else
        {
            printf( "Deinit %s OK\n", sample->GetName() );
        }
    }
    return 0;
}