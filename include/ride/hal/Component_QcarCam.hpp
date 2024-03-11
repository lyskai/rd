// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_CAM_HPP_
#define _RIDEHAL_CAM_HPP_

#include "qcarcam.h"
#include "ride/hal/ComponentIF.hpp"
#include "ride/hal/Types.hpp"

namespace ride
{
namespace hal
{

typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer; /* Shared buffer associated with the image */
    uint64_t timestamp;                  /* Hardware timestamp (in nanoseconds) */
    uint64_t timestampQGPTP; /* Generic Precision Time Protocol (GPTP) timestamp in nanoseconds */
    uint32_t streamId;
    uint32_t frameIndex;
} RideHal_CameraFrame_t;

/// @brief callback for camera frame done
typedef void ( *RideHal_CamFrameCallback_t )( RideHal_CameraFrame_t *pFrame, void *pPrivData );

// @brief callback for camera event
typedef void ( *RideHal_CamEventCallback_t )( const uint32_t eventId, const void *pPayload,
                                              void *pPrivData );

// @brief camera configuration
typedef struct RideHalCamConfig
{
    bool isAllocator;
    bool requestMode;
    uint32_t inputId;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bufCnt;
    uint32_t camFrameDropPat;
    RideHal_ImageFormat_e format;
} RideHalCamConfig_t;

// TODO check qcarcam open multiple stream with same input id

/// RideHalCam Interface
class RideHalCam : public ComponentIF
{
public:
    /// @brief Construct a new Qcarcam object
    RideHalCam();

    /// @brief Destroy the Qcarcam object
    ~RideHalCam();

    /// @brief init the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( char *pName, RideHalCamConfig_t config, Logger *pLogger = nullptr );

    /// @brief Start the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start() final;

    /// @brief Stop the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop() final;

    /// @brief deinit the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit() final;

    // TODO: check if necessary?
    /// @brief Pause the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Pause();

    /// @brief Resume the RideHalCam object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Resume();

    /// @brief release a camera frame
    /// @param streamId camera stream index
    /// @param frameIndex index of the frame
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e ReleaseFrame( uint32_t streamId, uint32_t frameIndex );

    // TODO
    /// @brief resuest a new camera frame
    /// @param pFrame the frame to request from camera
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RequestFrame( RideHal_CameraFrame_t *pFrame );

    /// @brief resuest a new camera frame
    /// @param pBuffer a list of buffers to be set to camera
    /// @param numBuffers number of buffers to be set
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SetBuffer( const RideHal_SharedBuffer_t *pBuffer, uint32_t numBuffers );

    /// @brief register callback
    /// @param frameCallback frame callback function
    /// @param eventCallback event callback function
    /// @param pAppPriv app private data
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterCallback( RideHal_CamFrameCallback_t frameCallback,
                                     RideHal_CamEventCallback_t eventCallback, void *pAppPriv );


private:
    QCarCamColorFmt_e GetQcarCamFormat( RideHal_ImageFormat_e colorFormat );

    RideHalError_e AllocateBuffer();

    RideHalError_e FreeBuffer();

    RideHal_CameraFrame_t *GetFrame();

    static QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                        const QCarCamEventPayload_t *pPayload, void *pPrivateData );

    bool m_bIsAllocator;
    uint32_t m_nBufCnt;
    uint32_t m_nWidth;
    uint32_t m_nHeight;
    uint32_t m_nInputId;
    uint32_t m_nCurrentBufIdx;
    RideHal_ImageFormat_e m_colorFormat;
    void *m_pAppPriv = nullptr;
    RideHal_CamEventCallback_t m_EventCallback = nullptr;
    RideHal_CamFrameCallback_t m_FrameCallback = nullptr;
    RideHal_CameraFrame_t *m_pCameraFrames = nullptr;
    QCarCamBuffer_t *m_pQcarcamBuffer = nullptr;
    QCarCamBufferList_t m_qcarcamBuffers;
    QCarCamHndl_t m_QcarCamHndl;
    RideHalCamConfig_t m_Config = { 0 };
};   // class RideHalCam

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_RIDEHALCAM_HPP_
