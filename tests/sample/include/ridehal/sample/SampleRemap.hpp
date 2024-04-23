// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_SAMPLE_REMAP_HPP_
#define _RIDEHAL_SAMPLE_REMAP_HPP_

#include "ridehal/component/Remap.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleRemap
///
/// SampleRemap that to demonstate how to use the RideHal component Remap
class SampleRemap : public SampleIF
{
public:
    SampleRemap();
    ~SampleRemap();

    /// @brief Initialize the remap
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    Remap_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<CamFrames_t> m_sub;
    DataPublisher<CamFrames_t> m_pub;

    Remap m_remap;
};   // class SampleRemap

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_REMAP_HPP_
