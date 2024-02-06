// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_CAMERA_IF_HPP_
#define _RIDE_HAL_CAMERA_IF_HPP_

#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{

typedef void ( *Camera_FrameCallback_t )( RideHal_SharedBuffer_t *pBuffer, uint64_t handle );


/// @brief ride::hal::CameraIF
///
/// Camera Interface
class CameraIF
{
public:
    CameraIF() = default;
    ~CameraIF() = default;

    /// @brief Start the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;

    /// @brief get a camera frame
    /// @param frameDesc the frame descriptor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e GetFrame( RideHal_SharedBuffer_t *pBuffer, uint64_t *pHandle ) = 0;

    /// @brief release a camera frame
    /// @param frameDesc the frame descriptor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e ReleaseFrame( uint64_t handle ) = 0;


    /// @brief Get the shared buffers used by the camera stream
    /// @param streamID [in] the camera stream ID
    /// @param pSharedBuffers [out] shared buffers pointers to return buffer information
    /// @param pNumSharedBuffers [inout] input and capability of the pSharedBuffers
    /// and return the number of the shared buffers actually used
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e GetSharedBuffers( uint32_t streamID,
                                             RideHal_SharedBuffer_t *pSharedBuffers,
                                             uint32_t *pNumSharedBuffers ) = 0;

protected:
    Camera_FrameCallback_t m_FrameCallback = nullptr;

};   // class CameraIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_CAMERA_IF_HPP_
