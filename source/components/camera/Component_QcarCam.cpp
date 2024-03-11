// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Component_QcarCam.hpp"


namespace ride
{
namespace hal
{

static uint32_t GetNumBitsOfInteger( uint32_t nInteger )
{
    uint32_t nTmp = nInteger;
    uint32_t n = 0;

    while ( 0 != nTmp )
    {
        nTmp = ( nTmp >> 1U );
        ++n;
    }

    return n;
}

QCarCamColorFmt_e RideHalCam::GetQcarCamFormat( RideHal_ImageFormat_e colorFormat )
{
    QCarCamColorFmt_e qcarcamFormat = QCARCAM_FMT_MAX;

    switch ( colorFormat )
    {
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
        {
            qcarcamFormat = QCARCAM_FMT_UYVY_8;
            break;
        }
        case RIDE_HAL_IMAGE_FORMAT_NV12:
        {
            qcarcamFormat = QCARCAM_FMT_NV12;
            break;
        }
        default:
        {
            RIDEHAL_ERROR( "Unsupport corlor sormat: %d", colorFormat );
            break;
        }
    }

    return qcarcamFormat;
}

QCarCamRet_e RideHalCam::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                         const QCarCamEventPayload_t *pPayload, void *pPrivateData )
{
    if ( nullptr == pPrivateData )
    {
        // RIDEHAL_ERROR("invalid pPrivateData");
    }
    else
    {
        RideHalCam *pCamContext = (RideHalCam *) pPrivateData;
        RideHal_CameraFrame_t *pCameraFrame = nullptr;

        switch ( eventId )
        {
            case QCARCAM_EVENT_FRAME_READY:
            {
                pCameraFrame = pCamContext->GetFrame();
                void *pAppPriv = pCamContext->m_pAppPriv;
                pCamContext->m_FrameCallback( pCameraFrame, pAppPriv );

                break;
            }
            case QCARCAM_EVENT_INPUT_SIGNAL:
            {
                // TODO
                break;
            }

            case QCARCAM_EVENT_ERROR:
            {
                // TODO
                break;
            }
            default:
            {
                // RIDEHAL_ERROR("event_cb Received unsupported event %d", eventId);
                break;
            }
        }
    }

    return QCARCAM_RET_OK;
}

/// RideHalCam Interface
/// @brief Construct a new Qcarcam object
RideHalCam::RideHalCam() {}

/// @brief Destroy the Qcarcam object
RideHalCam::~RideHalCam() {}

/// @brief init the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Init( char *pName, RideHalCamConfig_t config, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t param = 0;
    QCarCamInit_t qcarcamInit = { 0 };
    qcarcamInit.apiVersion = QCARCAM_VERSION;
    // TODO param checks
    m_bIsAllocator = config.isAllocator;
    m_nInputId = config.inputId;
    m_nWidth = config.width;
    m_nHeight = config.height;
    m_nBufCnt = config.bufCnt;
    m_colorFormat = config.format;


    ret = ComponentIF::Init( pName, pLogger );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "ComponentIF::Init failed" );
    }
    else
    {
        RIDEHAL_DEBUG( "RideHalCam::Init input id: %d, width: %d, height: %d", m_nInputId, m_nWidth,
                       m_nHeight );

        status = QCarCamInitialize( (const QCarCamInit_t *) &qcarcamInit );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCarCamInitialize failed  with ret %d", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCarCamInitialize success" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        QCarCamInputStream_t inputParams = {};
        inputParams.inputId = config.inputId;
        inputParams.srcId = 0;
        inputParams.inputMode = 0;

        QCarCamOpen_t openParams = {};
        openParams.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        openParams.numInputs = 1;
        openParams.inputs[0] = inputParams;

        status = QCarCamOpen( &openParams, &m_QcarCamHndl );
        if ( !m_QcarCamHndl )
        {
            RIDEHAL_ERROR( "QCarCamOpen failed" );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCarCamOpen Success" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // setup params
        status = QCarCamRegisterEventCallback( m_QcarCamHndl, &QcarcamEventCb, this );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_EVENT_CB failed ret %d", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_EVENT_CB Success" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        param = QCARCAM_EVENT_FRAME_READY | QCARCAM_EVENT_INPUT_SIGNAL | QCARCAM_EVENT_ERROR;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_EVENT_MASK, &param,
                                  sizeof( param ) );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_EVENT_MASK failed ret %d", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_EVENT_MASK Success" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // setup isp settings
        // ToDo: add configurable isp settings
        QCarCamIspUsecaseConfig_t ispConfig;

        ispConfig.id = 0;
        ispConfig.cameraId = 0;
        ispConfig.usecaseId = (QCarCamIspUsecase_e) 3;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE,
                                  &ispConfig, sizeof( ispConfig ) );

        if ( status != QCARCAM_RET_OK )
        {
            RIDEHAL_ERROR( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE failed ret %d", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE successg" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // setup frame rate params
        QCarCamFrameDropConfig_t frameDropConfig;
        frameDropConfig.frameDropPeriod = GetNumBitsOfInteger( config.camFrameDropPat );
        frameDropConfig.frameDropPattern = config.camFrameDropPat;
        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_FRAME_DROP_CONTROL,
                                  &frameDropConfig, sizeof( frameDropConfig ) );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_FRAME_RATE failed ret %d", ret );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_FRAME_RATE Success" );
        }
    }

    if ( ( RIDE_HAL_ERROR_NONE == ret ) && ( true == m_bIsAllocator ) )
    {
        ret = AllocateBuffer();
        if ( RIDE_HAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "AllocateBuffer failed" );
        }
    }

    return ret;
}

/// @brief Start the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    status = QCarCamReserve( m_QcarCamHndl );
    if ( QCARCAM_RET_OK != status )
    {
        RIDEHAL_ERROR( "QCarCamReserve failed with ret %d, exit", status );
        ret = RIDE_HAL_ERROR_FAIL;
    }
    else
    {
        status = QCarCamStart( m_QcarCamHndl );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCarCamStart failed with ret %d , exit", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "QCarCamStart success" );
        }
    }

    return ret;
}


/// @brief Stop the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    status = QCarCamStop( m_QcarCamHndl );
    if ( QCARCAM_RET_OK == status )
    {
        RIDEHAL_INFO( "Qcarcam Stop call success" );
    }
    else
    {
        RIDEHAL_ERROR( "Qcarcam could not be stopped" );
        ret = RIDE_HAL_ERROR_FAIL;
    }

    return ret;
}

/// @brief deinit the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( m_bIsAllocator )
    {
        ret = FreeBuffer();
        if ( RIDE_HAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Error in free buffer" );
        }
    }

    if ( 0 == m_QcarCamHndl )
    {
        RIDEHAL_ERROR( "Qcarcam null handle" );
        ret = RIDE_HAL_ERROR_FAIL;
    }
    else if ( QCARCAM_RET_OK != ( status = QCarCamRelease( m_QcarCamHndl ) ) )
    {
        RIDEHAL_ERROR( "Qcarcam context release failed %d", status );
        ret = RIDE_HAL_ERROR_FAIL;
    }
    else
    {
        status = QCarCamClose( m_QcarCamHndl );

        if ( QCARCAM_RET_OK == status )
        {
            RIDEHAL_INFO( "Qcarcamt closed now" );
        }
        else
        {
            RIDEHAL_ERROR( "Qcarcam close failed %d", status );
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    return ret;
}

/// @brief Pause the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Pause()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    // TODO
    return ret;
}

/// @brief Resume the RideHalCam object
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::Resume()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    // TODO
    return ret;
}

/// @brief release a camera frame
/// @param streamId camera stream index
/// @param frameIndex index of the frame
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::ReleaseFrame( uint32_t streamId, uint32_t frameIndex )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    status = QCarCamReleaseFrame( m_QcarCamHndl, streamId, frameIndex );
    if ( QCARCAM_RET_OK == status )
    {
        RIDEHAL_INFO( "QCarCamReleaseFrame success for index: %d", frameIndex );
    }
    else
    {
        RIDEHAL_ERROR( "QCarCamReleaseFrame fail for index: %d", frameIndex );
        ret = RIDE_HAL_ERROR_FAIL;
    }

    return ret;
}

/// @brief get a ready frame from camera
/// @return pointer to camera frame
RideHal_CameraFrame_t *RideHalCam::GetFrame()
{
    int32_t frameIndex = -1;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamFrameInfo_t frameInformation;
    RideHal_CameraFrame_t *pCameraFrame = nullptr;
    uint64_t timeout = 0;

    status = QCarCamGetFrame( m_QcarCamHndl, &frameInformation, timeout, 0 );

    if ( QCARCAM_RET_OK == status )
    {
        frameIndex = frameInformation.bufferIndex;
        pCameraFrame = &m_pCameraFrames[frameIndex];
        pCameraFrame->timestamp = frameInformation.timestamp;
        pCameraFrame->timestampQGPTP = frameInformation.sofTimestamp.timestampGPTP;
        RIDEHAL_DEBUG( "GetFrame index: %d ptr: %p buffer: %p, size: %d", frameIndex, pCameraFrame,
                       pCameraFrame->sharedBuffer.data(), pCameraFrame->sharedBuffer.size );
    }
    else
    {
        RIDEHAL_ERROR( "QCarCamGetFrame failed" );
    }

    return pCameraFrame;
}

/// @brief resuest a new camera frame
/// @param pFrame the frame to request from camera
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::RequestFrame( RideHal_CameraFrame_t *pFrame )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    return ret;
}

/// @brief register callback
/// @param frameCallback frame callback function
/// @param eventCallback event callback function
/// @param pAppPriv app private data
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::RegisterCallback( RideHal_CamFrameCallback_t frameCallback,
                                             RideHal_CamEventCallback_t eventCallback,
                                             void *pAppPriv )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_FrameCallback = frameCallback;
    m_EventCallback = eventCallback;
    m_pAppPriv = pAppPriv;

    return ret;
}

/// @brief allocate buffers for camera
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::AllocateBuffer()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( m_nBufCnt > 0 )
    {
        m_pCameraFrames = new RideHal_CameraFrame_t[m_nBufCnt];
        m_pQcarcamBuffer = new QCarCamBuffer_t[m_nBufCnt];
        m_qcarcamBuffers.id = m_nInputId;
        m_qcarcamBuffers.nBuffers = m_nBufCnt;
        m_qcarcamBuffers.pBuffers = m_pQcarcamBuffer;
        m_qcarcamBuffers.colorFmt = GetQcarCamFormat( m_colorFormat );


        m_qcarcamBuffers.flags = QCARCAM_BUFFER_FLAG_OS_HNDL;

        for ( uint32_t i = 0; i < m_nBufCnt; i++ )
        {
            if ( RIDE_HAL_ERROR_NONE !=
                 m_pCameraFrames[i].sharedBuffer.Allocate( m_nWidth, m_nHeight, m_colorFormat ) )
            {
                RIDEHAL_ERROR( "Buffer allocation failed" );
                ret = RIDE_HAL_ERROR_FAIL;
                break;
            }
            else
            {

                m_pQcarcamBuffer[i].numPlanes = 1;
                m_pQcarcamBuffer[i].planes[0].memHndl =
                        m_pCameraFrames[i].sharedBuffer.buffer.dmaHandle;
                m_pQcarcamBuffer[i].planes[0].width =
                        m_pCameraFrames[i].sharedBuffer.imgProps.width;
                m_pQcarcamBuffer[i].planes[0].height =
                        m_pCameraFrames[i].sharedBuffer.imgProps.height;
                m_pQcarcamBuffer[i].planes[0].stride =
                        m_pCameraFrames[i].sharedBuffer.imgProps.stride[0];
                m_pQcarcamBuffer[i].planes[0].size = m_pCameraFrames[i].sharedBuffer.size;
                m_pCameraFrames[i].streamId = 0;
                m_pCameraFrames[i].frameIndex = i;
                RIDEHAL_DEBUG(
                        "register buffer index %d ptr: %p memHndl: %x va: %p width: %d, "
                        "height: %d, steide: %d size: %d",
                        i, &m_pCameraFrames[i], m_pQcarcamBuffer[i].planes[0].memHndl,
                        m_pCameraFrames[i].sharedBuffer.data(), m_pQcarcamBuffer[i].planes[0].width,
                        m_pQcarcamBuffer[i].planes[0].height, m_pQcarcamBuffer[i].planes[0].stride,
                        m_pQcarcamBuffer[i].planes[0].size );
            }
        }

        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            // setup buffers
            if ( QCARCAM_RET_OK !=
                 ( status = QCarCamSetBuffers( m_QcarCamHndl,
                                               (const QCarCamBufferList_t *) &m_qcarcamBuffers ) ) )
            {
                RIDEHAL_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status,
                               m_QcarCamHndl );
                ret = RIDE_HAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "QCarCamSetBuffers successful" );
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "QCARCAM_PARAM_FRAME_RATE failed ret %d", ret );
        ret = RIDE_HAL_ERROR_FAIL;
    }

    return ret;
}

/// @brief free camera buffer
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::FreeBuffer()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( m_pCameraFrames )
    {
        for ( uint32_t i = 0; i < m_nBufCnt; i++ )
        {
            if ( RIDE_HAL_ERROR_NONE != m_pCameraFrames[i].sharedBuffer.Free() )
            {
                RIDEHAL_ERROR( "Free buffer failed index: %d", i );
            }
        }
    }

    if ( m_pCameraFrames )
    {
        delete m_pCameraFrames;
        m_pCameraFrames = nullptr;
    }

    if ( m_pQcarcamBuffer )
    {
        delete m_pQcarcamBuffer;
        m_pQcarcamBuffer = nullptr;
    }

    return ret;
}

/// @brief resuest a new camera frame
/// @param pBuffer a list of buffers to be set to camera
/// @param numBuffers number of buffers to be set
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHalCam::SetBuffer( const RideHal_SharedBuffer_t *pBuffer, uint32_t numBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    return ret;
}

}   // namespace hal
}   // namespace ride
