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
    Point2D_t ProjectToImage( Point2D_t &pt, Point2D_t &center, float yaw );
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
    float m_minX = 8.0;
    float m_maxY = 8.0;
    float m_offsetX = 8.0;
    float m_offsetY = 8.0;
    float m_ratioW = 8.0;
    float m_ratioH = 8.0;
    std::vector<uint32_t> m_indexs = { 5, 0, 1, 2, 3, 4 };

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<Road2DObjects_t> m_pub;
};   // class SamplePostProcBevdet

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_
