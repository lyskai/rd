// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_CAMERA_HPP_
#define _RIDE_HAL_SAMPLE_CAMERA_HPP_

#include "ridehal/component/Camera.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleCamera
///
/// SampleCamera that to demonstate how to use the RideHal component Camera
class SampleCamera : public SampleIF
{
public:
    SampleCamera();
    ~SampleCamera();

    /// @brief Initialize the camera
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    void FrameCallBack( CameraFrame_t *pFrame, bool RequestMode );
    void EventCallBack( const uint32_t eventId, const void *pPayload );
    static void FrameCallBack( CameraFrame_t *pFrame, void *pPrivData );
    static void EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData );

private:
    Camera m_camera;
    Camera_Config_t m_camConfig;

    std::string m_topicName;

    DataPublisher<CamFrames_t> m_pub;
    uint64_t m_frameId = 0;
};   // class SampleCamera

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_CAMERA_HPP_
