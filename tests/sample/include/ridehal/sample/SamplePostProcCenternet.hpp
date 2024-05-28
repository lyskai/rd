// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_
#define _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_

#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SamplePostProcCenternet
///
/// SamplePostProcCenternet that to demonstate how to do QNN output post processing for Centernet
class SamplePostProcCenternet : public SampleIF
{
public:
    SamplePostProcCenternet();
    ~SamplePostProcCenternet();

    /// @brief Initialize the PostProc
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    void ProcessUint8( DataFrames_t &inputsFrame );
    void NMS( std::vector<Road2DObject_t> &boxes, float thres );
    float ComputeIou( const Road2DObject_t &box1, const Road2DObject_t &box2 );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;

    uint32_t m_roiX = 0;
    uint32_t m_roiY = 0;
    uint32_t m_camWidth = 1920;
    uint32_t m_camHeight = 1024;

    // the score and NMS threshold value used for the NMS post processing
    float m_scoreThreshold = 0.6;
    float m_NMSThreshold = 0.6;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<Road2DObjects_t> m_pub;
};   // class SamplePostProcCenternet

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_
