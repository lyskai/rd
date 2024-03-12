// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_IF_HPP_
#define _RIDE_HAL_SAMPLE_IF_HPP_

#include <map>
#include <string>
#include <thread>

#include "ridehal/common/LoggerIF.hpp"
#include "ridehal/common/Types.hpp"
#include "ridehal/sample/DataBroker.hpp"
#include "ridehal/sample/DataTypes.hpp"


using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

typedef std::map<std::string, std::string> SampleConfig_t;

/// @brief ridehal::sample::SampleIF
///
/// Sample Interface that to demonstate how to use the RideHal component
class SampleIF : public LoggerIF
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

protected:
    RideHalError_e Init( std::string name );
    std::string Get( SampleConfig_t &config, std::string key, std::string defaultV );
    int Get( SampleConfig_t &config, std::string key, int defaultV );
    uint32_t Get( SampleConfig_t &config, std::string key, uint32_t defaultV );
    float Get( SampleConfig_t &config, std::string key, float defaultV );
    RideHal_ImageFormat_e Get( SampleConfig_t &config, std::string key,
                               RideHal_ImageFormat_e defaultV );
};   // class SampleIF

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_IF_HPP_
