// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_CAM_HPP_
#define _RIDEHAL_CAM_HPP_

#include "qcarcam.h"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace component
{
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer; /* Shared buffer associated with the image */
    uint64_t timestamp;                  /* Hardware timestamp (in nanoseconds) */
    uint64_t timestampQGPTP; /* Generic Precision Time Protocol (GPTP) timestamp in nanoseconds */
    uint32_t frameIndex;     /* Index of the camera frame */
    uint32_t flags;          /* Flag to indicate error state of the buffer */
} CameraFrame_t;

typedef struct
{
    QCarCamInput_t *pCameraInputs; /* pointer to the list of qcarcam inputs info */
    uint32_t numInputs;            /* num of qcarcam inputs */
} CameraInputs_t;

/// @brief callback for camera frame done
typedef void ( *RideHal_CamFrameCallback_t )( CameraFrame_t *pFrame, void *pPrivData );

// @brief callback for camera event
typedef void ( *RideHal_CamEventCallback_t )( const uint32_t eventId, const void *pPayload,
                                              void *pPrivData );

// @brief camera configuration
typedef struct Camera_Config
{
    bool bAllocator;              /* Flag to indicate if component is buffer allocator*/
    bool bRequestMode;            /* Flag to set request buffer mode */
    uint32_t streamId;            /* Camera steam id */
    uint32_t inputId;             /* Camera input id */
    uint32_t ispUserCase;         /* ISP user case defined by qcarcam */
    uint32_t width;               /* Frame width */
    uint32_t height;              /* Frame height */
    uint32_t fps;                 /* Frames per second */
    uint32_t bufCnt;              /* Buffer count set to camera */
    uint32_t camFrameDropPat;     /* Frame drop patten defined by qcarcam */
    uint32_t opMode;              /* Operation mode defined by qcarcam */
    RideHal_ImageFormat_e format; /* Camera frame format */
} Camera_Config_t;

/// Camera Interface
class Camera : public ComponentIF
{
public:
    /// @brief Construct a new Qcarcam object
    Camera();

    /// @brief Destroy the Qcarcam object
    ~Camera();

    /// @brief init the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( char *pName, const Camera_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Start the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start() final;

    /// @brief Stop the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop() final;

    /// @brief deinit the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit() final;

    /// @brief Pause the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Pause();

    /// @brief Resume the Camera object
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Resume();

    /// @brief release a camera frame
    /// @param pFrame the camera frame to be released
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e ReleaseFrame( CameraFrame_t *pFrame );

    /// @brief resuest a new camera frame
    /// @param pFrame the frame to request from camera
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e RequestFrame( CameraFrame_t *pFrame );

    /// @brief resuest a new camera frame
    /// @param pBuffer a list of buffers to be set to camera
    /// @param numBuffers number of buffers to be set
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e SetBuffers( const RideHal_SharedBuffer_t *pBuffer, uint32_t numBuffers );

    /// @brief register callback
    /// @param frameCallback frame callback function
    /// @param eventCallback event callback function
    /// @param pAppPriv app private data
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterCallback( RideHal_CamFrameCallback_t frameCallback,
                                     RideHal_CamEventCallback_t eventCallback, void *pAppPriv );

    /// @brief get camera inputs info
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e GetInputsInfo( CameraInputs_t *pCamInputs );

private:
    QCarCamColorFmt_e GetQcarCamFormat( RideHal_ImageFormat_e colorFormat );

    RideHalError_e AllocateBuffer();

    RideHalError_e FreeBuffer();

    CameraFrame_t *GetFrame( const QCarCamFrameInfo_t *pFrameInfo );

    static QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                        const QCarCamEventPayload_t *pPayload, void *pPrivateData );
    QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                 const QCarCamEventPayload_t *pPayload );

    RideHalError_e QueryInputs();

    bool m_bIsAllocator;
    bool m_bRequestMode;
    uint32_t m_nStreamId;
    uint32_t m_nBufCnt;
    uint32_t m_nWidth;
    uint32_t m_nHeight;
    uint32_t m_nInputId;
    uint32_t m_nCurrentBufIdx;
    uint32_t m_nRequestId;
    RideHal_ImageFormat_e m_colorFormat;
    void *m_pAppPriv = nullptr;
    RideHal_CamEventCallback_t m_EventCallback = nullptr;
    RideHal_CamFrameCallback_t m_FrameCallback = nullptr;
    CameraFrame_t *m_pCameraFrames = nullptr;
    QCarCamBuffer_t *m_pQcarcamBuffer = nullptr;
    QCarCamBufferList_t m_qcarcamBuffers;
    QCarCamHndl_t m_QcarCamHndl;
    CameraInputs_t m_sCameraInputsInfo;
};   // class Camera

}   // namespace component
}   // namespace ridehal

#endif   // _RIDEHAL_RIDEHALCAM_HPP_
