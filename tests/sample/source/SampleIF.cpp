// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/sample/SampleIF.hpp"
#include <assert.h>

namespace ridehal
{
namespace sample
{

std::map<std::string, Sample_CreateFunction_t> SampleIF::s_SampleMap;

SampleIF *SampleIF::Create( std::string name )
{
    SampleIF *sample = nullptr;

    auto it = s_SampleMap.find( name );
    if ( it != s_SampleMap.end() )
    {
        Sample_CreateFunction_t createFnc = it->second;
        sample = createFnc();
    }

    return sample;
}

void SampleIF::RegisterSample( std::string name, Sample_CreateFunction_t createFnc )
{
    auto it = s_SampleMap.find( name );
    if ( it == s_SampleMap.end() )
    {
        s_SampleMap[name] = createFnc;
        printf( "register sample type %s\n", name.c_str() );
    }
    else
    {
        printf( "sample %s is already registered\n", name.c_str() );
        assert( 0 );
    }
}

RideHalError_e SampleIF::Init( std::string name )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_name = name;
    ret = RIDEHAL_LOGGER_INIT( name.c_str(), LOGGER_LEVEL_INFO );

    return ret;
}

const char *SampleIF::GetName()
{
    return m_name.c_str();
}

std::string SampleIF::Get( SampleConfig_t &config, std::string key, std::string defaultV )
{
    std::string ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = it->second;
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), ret.c_str() );

    return ret;
}

int32_t SampleIF::Get( SampleConfig_t &config, std::string key, int32_t defaultV )
{
    int32_t ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = std::stoi( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

uint32_t SampleIF::Get( SampleConfig_t &config, std::string key, uint32_t defaultV )
{
    uint32_t ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = (uint32_t) std::stoi( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %u\n", key.c_str(), ret );

    return ret;
}

float SampleIF::Get( SampleConfig_t &config, std::string key, float defaultV )
{
    float ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = std::stof( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %f\n", key.c_str(), ret );

    return ret;
}

RideHal_ImageFormat_e SampleIF::Get( SampleConfig_t &config, std::string key,
                                     RideHal_ImageFormat_e defaultV )
{
    RideHal_ImageFormat_e ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string format = it->second;
        if ( "rgb" == format )
        {
            ret = RIDE_HAL_IMAGE_FORMAT_RGB888;
        }
        else if ( "bgr" == format )
        {
            ret = RIDE_HAL_IMAGE_FORMAT_BGR888;
        }
        else if ( "uyvy" == format )
        {
            ret = RIDE_HAL_IMAGE_FORMAT_UYVY;
        }
        else if ( "nv12" == format )
        {
            ret = RIDE_HAL_IMAGE_FORMAT_NV12;
        }
        else if ( "p010" == format )
        {
            ret = RIDE_HAL_IMAGE_FORMAT_P010;
        }
        else
        {
            ret = RIDE_HAL_IMAGE_FORMAT_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %f\n", key.c_str(), ret );
    return ret;
}

RideHal_ProcessorType_e SampleIF::Get( SampleConfig_t &config, std::string key,
                                       RideHal_ProcessorType_e defaultV )
{
    RideHal_ProcessorType_e ret = defaultV;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string processor = it->second;
        if ( "dsp0" == processor )
        {
            ret = RIDE_HAL_PROCESSOR_HTP0;
        }
        else if ( "dsp1" == processor )
        {
            ret = RIDE_HAL_PROCESSOR_HTP1;
        }
        else if ( "cpu" == processor )
        {
            ret = RIDE_HAL_PROCESSOR_CPU;
        }
        else if ( "gpu" == processor )
        {
            ret = RIDE_HAL_PROCESSOR_GPU;
        }
        else
        {
            ret = RIDE_HAL_PROCESSOR_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

}   // namespace sample
}   // namespace ridehal