// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_VIDEO_CODEC_IF_HPP_
#define _RIDE_HAL_VIDEO_CODEC_IF_HPP_

#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{

typedef struct
{
    RideHal_SharedBuffer_t *pSharedBuffer;
    /* the timestamp must be set for the SubmitInput */
    uint64_t timestamp;
} RideHal_VideoCodecFrame_t;

typedef void ( *VideoCodec_FrameCallback_t )( const RideHal_VideoCodecFrame_t *pVideoCodecFrame );

/// @brief ride::hal::VideoCodecIF
///
/// VideoCodec Interface
class VideoCodecIF : public ComponentIF
{
public:
    VideoCodecIF() = default;
    ~VideoCodecIF() = default;

    /// @brief Start the video codec
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the video codec
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the video codec
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;

    /// @brief submit a video codec frame
    /// @param pVideoCodecFrame pointer to hold the video codec frame information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e SubmitInput( const RideHal_VideoCodecFrame_t *pVideoCodecFrame ) = 0;

    /// @brief release a video codec frame
    /// @param pVideoCodecFrame pointer to the video codec frame information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e ReleaseOutput( const RideHal_VideoCodecFrame_t *pVideoCodecFrame ) = 0;

protected:
    VideoCodec_FrameCallback_t m_FrameCallback = nullptr;

};   // class VideoCodecIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_VIDEO_CODEC_IF_HPP_
