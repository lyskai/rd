// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_QNN_HPP_
#define _RIDE_HAL_SAMPLE_QNN_HPP_

#include "ridehal/component/QnnRuntime.hpp"
#include "ridehal/sample/SampleIF.hpp"
using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleQnn
///
/// SampleQnn that to demonstate how to use the RideHal component QnnRuntime
class SampleQnn : public SampleIF
{
public:
    SampleQnn();
    ~SampleQnn();

    /// @brief Initialize the QnnRuntime
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the QnnRuntime
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the QnnRuntime
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the QnnRuntime
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    QnnRuntime_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::string m_modelPath;
    std::thread m_thread;
    bool m_stop;

    std::vector<QnnRuntime_TensorInfo_t> m_inputInfos;
    std::vector<QnnRuntime_TensorInfo_t> m_outputInfos;

    std::vector<SharedBufferPool> m_tensorPools;

    DataSubscriber<CamFrames_t> m_sub;
    DataPublisher<Tensors_t> m_pub;

    QnnRuntime m_qnn;
};   // class SampleQnn

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_QNN_HPP_
