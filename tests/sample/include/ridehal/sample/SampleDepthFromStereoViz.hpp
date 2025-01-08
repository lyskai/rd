// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
#define RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP

#include "OpenclIface.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::OpenclIface;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleDepthFromStereoViz
///
/// SampleDepthFromStereoViz that to demonstate how to decoding the output of the RideHal component
/// DepthFromStereo
class SampleDepthFromStereoViz : public SampleIF
{
public:
    SampleDepthFromStereoViz();
    ~SampleDepthFromStereoViz();

    /// @brief Initialize the DepthFromStereo Viz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    RideHalError_e ConvertToRgbCPU( RideHal_SharedBuffer_t *pDisparity,
                                    RideHal_SharedBuffer_t *pConf, RideHal_SharedBuffer_t *pRGB );
    RideHalError_e ConvertToRgbGPU( RideHal_SharedBuffer_t *pDisparity,
                                    RideHal_SharedBuffer_t *pConf, RideHal_SharedBuffer_t *pRGB );

private:
    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_rgbPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    uint16_t m_disparityMax = 1008;

    uint8_t m_nConfMapThreshold = 0;
    uint8_t m_nTransparency = 0xFF;

    RideHal_ProcessorType_e m_processor;
    OpenclSrv m_openclSrvObj;
    cl_kernel m_kernel;
};   // class SampleDepthFromStereoViz

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
