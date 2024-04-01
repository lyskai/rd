// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_TINYVIZ_HPP_
#define _RIDE_HAL_SAMPLE_TINYVIZ_HPP_
#include "TinyViz.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace QRide::Stack;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleTinyViz
///
/// SampleTinyViz that to demonstate how to use the RideHal component TinyViz
class SampleTinyViz : public SampleIF
{
public:
    SampleTinyViz();
    ~SampleTinyViz();

    /// @brief Initialize the tinyviz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the tinyviz
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the tinyviz
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the tinyviz
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    uint32_t m_width;
    uint32_t m_height;
    TinyVizIF::PixelFormat m_format;

    uint32_t m_winW;
    uint32_t m_winH;

    std::string m_topicName;

    std::thread m_thread;
    bool m_stop;

    DataSubscriber<CamFrames_t> m_sub;

    TinyViz m_tinyViz;

};   // class SampleTinyViz

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_TINYVIZ_HPP_
