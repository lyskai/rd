// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_IF_HPP_
#define _RIDE_HAL_SAMPLE_IF_HPP_

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/Types.hpp"
#include "ridehal/sample/DataBroker.hpp"
#include "ridehal/sample/DataTypes.hpp"

#include <map>
#include <mutex>
#include <string>
#include <thread>

#if defined( WITH_RSM_V2 )
#include <rsm_client_v2.h>
#endif

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

typedef std::map<std::string, std::string> SampleConfig_t;

class SampleIF;

typedef SampleIF *( *Sample_CreateFunction_t )();

#define REGISTER_SAMPLE( name, class_name )                                                        \
    static SampleIF *CreatSample##name() { return new class_name(); }                              \
    class Register##class_name                                                                     \
    {                                                                                              \
    public:                                                                                        \
        Register##class_name() { SampleIF::RegisterSample( #name, CreatSample##name ); }           \
    };                                                                                             \
    const Register##class_name g_register##name;

/// @brief ridehal::sample::SampleIF
///
/// Sample Interface that to demonstate how to use the RideHal component
class SampleIF
{
public:
    SampleIF() = default;
    ~SampleIF() = default;

    /// @brief Initialize the sample
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Init( std::string name, SampleConfig_t &config ) = 0;

    /// @brief Start the sample
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the sample
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the sample
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;

    const char *GetName();

    /// @brief create a RideHal sample from type name
    /// @param name the sample type name
    /// @return the sample pointer on success, nullptr on failure
    static SampleIF *Create( std::string name );

    /// @brief register a RideHal sample
    /// @param name the sample type name
    /// @param createFnc the function to create the sample
    static void RegisterSample( std::string name, Sample_CreateFunction_t createFnc );

protected:
    RideHalError_e Init( std::string name );

    RideHalError_e Init( RideHal_ProcessorType_e processor );
    RideHalError_e Lock();
    RideHalError_e Unlock();

    std::string Get( SampleConfig_t &config, std::string key, std::string defaultV );
    int32_t Get( SampleConfig_t &config, std::string key, int32_t defaultV );
    uint32_t Get( SampleConfig_t &config, std::string key, uint32_t defaultV );
    float Get( SampleConfig_t &config, std::string key, float defaultV );
    RideHal_ImageFormat_e Get( SampleConfig_t &config, std::string key,
                               RideHal_ImageFormat_e defaultV );
    RideHal_ProcessorType_e Get( SampleConfig_t &config, std::string key,
                                 RideHal_ProcessorType_e defaultV );


protected:
    std::string m_name;
    RIDEHAL_DECLARE_LOGGER();

private:
#if defined( WITH_RSM_V2 )
    rsm_acquire_cmd_v2 m_acquireCmdV2;
    rsm_acquire_rsp_v2 m_acquireRspV2;
    rsm_handle m_handle = 0;
#endif

    RideHal_ProcessorType_e m_processor = RIDE_HAL_PROCESSOR_MAX;

    static std::mutex s_locks[RIDE_HAL_PROCESSOR_MAX];

private:
    static std::map<std::string, Sample_CreateFunction_t> s_SampleMap;
};   // class SampleIF

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_IF_HPP_
