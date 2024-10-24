// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_C2C_EXAMPLE_HPP
#define RIDEHAL_SAMPLE_C2C_EXAMPLE_HPP

#include "ridehal/sample/SampleIF.hpp"
extern "C"
{
#include <c2c.h>
}
using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleC2C
///
/// SampleC2C that to demonstate how to chip to chip communicaiton over PCIe to pass camera frame
class SampleC2C : public SampleIF
{
public:
    SampleC2C();
    ~SampleC2C();

    /// @brief Initialize the C2C
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the C2C
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the C2C
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the C2C
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadPubMain();
    void ThreadSubMain();

private:
    struct DmaInfo
    {
        void *pData;
        uint64_t dmaBufId;
        int dmaFd;
    };

private:
    std::string m_topicName;
    std::string m_buffersName;

    std::thread m_thread;
    bool m_stop;

    bool m_bIsPub = true;
    uint32_t m_queueDepth = 2;

    int m_c2cFd = -1;

    uint32_t m_width;
    uint32_t m_height;
    RideHal_ImageFormat_e m_format;

    uint32_t m_poolSize = 4;
    uint32_t m_channleId;

    SharedBufferPool m_imagePool;

    std::vector<RideHal_SharedBuffer_t> m_buffers;
    std::vector<DmaInfo> m_dmaInfos;
    std::map<uint64_t, uint32_t> m_dmaIndexMap;


    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;
};   // class SampleC2C

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_C2C_EXAMPLE_HPP
