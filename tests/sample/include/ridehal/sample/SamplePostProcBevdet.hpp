// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_
#define _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_

#include "ridehal/sample/SampleIF.hpp"
#include <array>

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SamplePostProcBevdet
///
/// SamplePostProcBevdet that to demonstate how to do QNN output post processing for Centernet
class SamplePostProcBevdet : public SampleIF
{
public:
    SamplePostProcBevdet();
    ~SamplePostProcBevdet();

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
    struct Object
    {
        int classId;
        float prob;
        // 3D information
        float x;
        float y;
        float z;
        float dx;
        float dy;
        float dz;
        float yaw;
        float vel[2];
    };
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    void ProcessUint8( DataFrames_t &inputsFrame );
    void NMS( std::vector<Object> &boxes, float thres );
    float ComputeIou( const Object &box1, const Object &box2 );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;

    // the score and NMS threshold value used for the NMS post processing
    float m_scoreThreshold = 0.49;
    float m_NMSThreshold = 0.6;

    std::vector<float> m_voxelSize;
    std::vector<float> m_pointCloudRange;
    float m_outSizeFactor = 8.0;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<Road2DObjects_t> m_pub;
};   // class SamplePostProcBevdet

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_
