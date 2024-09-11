// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_
#define _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_

#include "OpenclIface.hpp"
#include "ridehal/sample/SampleIF.hpp"
#include <array>

using namespace ridehal::common;
using namespace ridehal::libs::OpenclIface;

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
    RideHalError_e RegisterInputBuffers( DataFrames_t &tensors );
    RideHalError_e RegisterOutputBuffers();
    void PostProcCPU( DataFrames_t &inputsFrame );
    RideHalError_e PostProcCL( DataFrames_t &tensors );
    void SetCLParams();
    void NMS( std::vector<Object> &boxes, float thres );
    float ComputeIou( const Object &box1, const Object &box2 );

private:
    Point2D_t ProjectToImage( Point2D_t &pt, Point2D_t &center, float yaw );
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;
    RideHal_ProcessorType_e m_processor;

    // the score and NMS threshold value used for the NMS post processing
    float m_scoreThreshold = 0.49;
    float m_NMSThreshold = 0.6;

    // max object num
    const uint32_t MAX_OBJ_NUM = 1000;

    std::vector<float> m_voxelSize;
    std::vector<float> m_pointCloudRange;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_hmScale = 0;
    int32_t m_hmOffset = 0;
    float m_regScale = 0;
    int32_t m_regOffset = 0;
    float m_heightScale = 0;
    int32_t m_heightOffset = 0;
    float m_dimScale = 0;
    int32_t m_dimOffset = 0;
    float m_rotScale = 0;
    int32_t m_rotOffset = 0;
    float m_velScale = 0;
    int32_t m_velOffset = 0;
    float m_outSizeFactor = 8.0;
    float m_minX = 8.0;
    float m_maxY = 8.0;
    float m_offsetX = 8.0;
    float m_offsetY = 8.0;
    float m_ratioW = 8.0;
    float m_ratioH = 8.0;
    uint32_t m_classNum = 0;
    uint32_t m_maxObjNum = MAX_OBJ_NUM;
    std::vector<uint32_t> m_indexs = { 5, 0, 1, 2, 3, 4 };

    RideHal_SharedBuffer_t m_outputClsIdBuf;
    RideHal_SharedBuffer_t m_outputDetObjBuf;

    cl_mem m_clInputHMBuf;
    cl_mem m_clInputRegBuf;
    cl_mem m_clInputHeightBuf;
    cl_mem m_clInputDimBuf;
    cl_mem m_clInputRotBuf;
    cl_mem m_clInputVelBuf;
    cl_mem m_clOutputClsIdBuf;
    cl_mem m_clOutputDetObjBuf;

    OpenclSrv m_OpenclSrvObj;
    OpenclIfcae_Arg_t m_openclArgs[30];

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<Road2DObjects_t> m_pub;
};   // class SamplePostProcBevdet

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_POST_PROC_BEVDET_HPP_

