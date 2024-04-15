// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_DATAREADER_HPP_
#define _RIDE_HAL_SAMPLE_DATAREADER_HPP_

#include "ridehal/sample/SampleIF.hpp"
#include <mutex>

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleDataReader
///
/// SampleDataReader that to demonstate how to use the RideHal component Remap
class SampleDataReader : public SampleIF
{
public:
    SampleDataReader();
    ~SampleDataReader();

    /// @brief Initialize the remap
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the remap
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the remap
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the remap
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    RideHalError_e LoadImage( std::shared_ptr<SharedBuffer_t> image, std::string path );

private:
    typedef struct
    {
        RideHal_ImageFormat_e format;
        uint32_t width;
        uint32_t height;
        std::string dataPath;
    } DataReaderConfig_t;

    uint32_t m_fps;
    std::vector<DataReaderConfig_t> m_configs;
    uint32_t m_numOfDataReaders;

    uint32_t m_poolSize = 4;
    std::string m_topicName;

    std::thread m_thread;
    std::vector<SharedBufferPool> m_imagePools;
    bool m_stop;

    DataPublisher<CamFrames_t> m_pub;

    static std::mutex s_lock;

};   // class SampleDataReader

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_DATAREADER_HPP_
