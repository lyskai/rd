// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_SAMPLE_PLRPRE_HPP_
#define _RIDEHAL_SAMPLE_PLRPRE_HPP_

#include "ridehal/component/PointPillarPreProc.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SamplePlrPre
///
/// SamplePlrPre that to demonstate how to use the RideHal component PointPillarPreProc
class SamplePlrPre : public SampleIF
{
public:
    SamplePlrPre();
    ~SamplePlrPre();

    /// @brief Initialize the PointPillarPreProc
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the PointPillarPreProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the PointPillarPreProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the PointPillarPreProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    PointPillarPreProc_Config_t m_config = { RIDEHAL_PROCESSOR_HTP0, 0 };
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_coordsPool;
    SharedBufferPool m_featuresPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    PointPillarPreProc m_plrPre;
};   // class SamplePlrPre

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_PLRPRE_HPP_
