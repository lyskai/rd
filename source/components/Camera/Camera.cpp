// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <atomic>
#include "ridehal/component/Camera.hpp"

namespace ridehal
{
namespace component
{

static int g_nCamInitRefCount = 0;
static std::mutex g_camInitMutex;

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

QCarCamColorFmt_e Camera::GetQcarCamFormat( RideHal_ImageFormat_e colorFormat )
{
    QCarCamColorFmt_e qcarcamFormat = QCARCAM_FMT_MAX;

    switch ( colorFormat )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
        {
            qcarcamFormat = QCARCAM_FMT_UYVY_8;
            break;
        }
        case RIDEHAL_IMAGE_FORMAT_NV12:
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

QCarCamRet_e Camera::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                     const QCarCamEventPayload_t *pPayload, void *pPrivateData )
{
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( nullptr == pPrivateData )
    {
         RIDEHAL_LOG_ERROR("invalid pPrivateData");
    }
    else
    {
        Camera *self = (Camera *) pPrivateData;
        status = self->QcarcamEventCb(hndl, eventId, pPayload);
    }

    return status;
}

QCarCamRet_e Camera::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                     const QCarCamEventPayload_t *pPayload )
{
    CameraFrame_t *pCameraFrame = nullptr;

    RIDEHAL_INFO("QcarcamEventCb eventId: %d", eventId);
    switch ( eventId )
    {
        case QCARCAM_EVENT_FRAME_READY:
            {
                pCameraFrame = GetFrame(&pPayload->frameInfo);
                if (nullptr != pCameraFrame)
                {
                    m_FrameCallback( pCameraFrame, m_pAppPriv );
                }
                else
                {
                    RIDEHAL_ERROR("GetFrame failed, returning nullptr");
                }

                break;
            }
        case QCARCAM_EVENT_INPUT_SIGNAL:
            {
                RIDEHAL_ERROR("QCARCAM received new input signal");
                break;
            }
        case QCARCAM_EVENT_ERROR:
            {
                RIDEHAL_ERROR("QCARCAM_EVENT_ERROR");
                m_EventCallback( eventId, (void *)pPayload, m_pAppPriv );
                break;
            }
        default:
            {
                RIDEHAL_ERROR("event_cb Received unsupported event %d", eventId);
                break;
            }
    }

    return QCARCAM_RET_OK;
}

/// Camera Interface
/// @brief Construct a new Qcarcam object
Camera::Camera()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamInit_t qcarcamInit = { 0 };
    qcarcamInit.apiVersion = QCARCAM_VERSION;
    QCarCamRet_e status = QCARCAM_RET_OK;
    m_QcarCamHndl = 0;

    std::lock_guard<std::mutex> guard(g_camInitMutex);
    if (0 == g_nCamInitRefCount)
    {
        status = QCarCamInitialize( (const QCarCamInit_t *) &qcarcamInit );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_LOG_ERROR( "QCarCamInitialize failed", status );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_LOG_INFO( "QCarCamInitialize success" );
            m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            g_nCamInitRefCount ++;
        }
    }
    else
    {
        g_nCamInitRefCount ++;
    }

    m_sCameraInputsInfo.pCameraInputs = nullptr;
    m_sCameraInputsInfo.numInputs = 0;

    ret = QueryInputs();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_LOG_ERROR( "QueryInputs failed: %d", ret );
    }
}

/// @brief Destroy the Qcarcam object
Camera::~Camera()
{
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( nullptr != m_sCameraInputsInfo.pCameraInputs )
    {
        delete (m_sCameraInputsInfo.pCameraInputs);
        m_sCameraInputsInfo.pCameraInputs = nullptr;
    }

    std::lock_guard<std::mutex> guard(g_camInitMutex);
    if (0 < g_nCamInitRefCount)
    {
        g_nCamInitRefCount --;

        if ( 0 == g_nCamInitRefCount )
        {
            status = QCarCamUninitialize();
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_LOG_ERROR( "QCarCamUninitialize failed: %d", status );
            }
        }
        else
        {
            RIDEHAL_LOG_INFO( "Skip QCarCamUninitialize" );
        }
    }
    else
    {
        RIDEHAL_LOG_ERROR( "g_nCamInitRefCount not greater than 0, unexpected" );
    }
}

/// @brief init the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Init( char *pName, const Camera_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t param = 0;

    if (nullptr != pConfig)
    {
        m_bIsAllocator = pConfig->isAllocator;
        m_nInputId = pConfig->inputId;
        m_nWidth = pConfig->width;
        m_nHeight = pConfig->height;
        m_nBufCnt = pConfig->bufCnt;
        m_colorFormat = pConfig->format;
        m_bRequestMode = pConfig->requestMode;
        m_nStreamId = pConfig->streamId;
    }
    else
    {
        RIDEHAL_ERROR( "pConfig is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ComponentIF::Init( pName, level );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "ComponentIF::Init failed" );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else if ( RIDEHAL_COMPONENT_STATE_INITIAL == m_state )
        {
            m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;

            RIDEHAL_DEBUG( "Camera::Init input id: %d, width: %d, height: %d, format: %d, buffer count: %d",
                    m_nInputId, m_nWidth, m_nHeight, m_colorFormat, m_nBufCnt );
        }
        else
        {
            RIDEHAL_ERROR( "Camera not in initial state: %d", m_state );
            ret = RIDEHAL_ERROR_BAD_STATE;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QCarCamInputStream_t inputParams = {};
        inputParams.inputId = pConfig->inputId;
        inputParams.srcId = 0;
        inputParams.inputMode = 0;

        QCarCamOpen_t openParams = {};
        openParams.opMode = (QCarCamOpmode_e) pConfig->opMode;
        openParams.numInputs = 1;
        openParams.inputs[0] = inputParams;

        if (m_bRequestMode)
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_REQUEST_MODE;
        }

        status = QCarCamOpen( &openParams, &m_QcarCamHndl );
        if ( (QCARCAM_RET_OK != status) || ( !m_QcarCamHndl ) )
        {
            RIDEHAL_ERROR( "QCarCamOpen failed: %d", status );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_INFO( "QCarCamOpen Success handle: %lu", m_QcarCamHndl );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // setup params
        status = QCarCamRegisterEventCallback( m_QcarCamHndl, &QcarcamEventCb, this );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_EVENT_CB failed ret %d", status );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_EVENT_CB Success" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        param = QCARCAM_EVENT_FRAME_READY | QCARCAM_EVENT_INPUT_SIGNAL | QCARCAM_EVENT_ERROR;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_EVENT_MASK, &param,
                                  sizeof( param ) );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_EVENT_MASK failed ret %d", status );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_EVENT_MASK Success" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // setup isp settings
        QCarCamIspUsecaseConfig_t ispConfig;

        ispConfig.id = 0;
        ispConfig.cameraId = 0;
        ispConfig.usecaseId = (QCarCamIspUsecase_e) pConfig->ispUserCase;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE,
                                  &ispConfig, sizeof( ispConfig ) );

        if ( status != QCARCAM_RET_OK )
        {
            RIDEHAL_ERROR( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE failed ret %d", status );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE successg" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // setup frame rate params
        QCarCamFrameDropConfig_t frameDropConfig;
        frameDropConfig.frameDropPeriod = GetNumBitsOfInteger( pConfig->camFrameDropPat );
        frameDropConfig.frameDropPattern = pConfig->camFrameDropPat;
        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_FRAME_DROP_CONTROL,
                                  &frameDropConfig, sizeof( frameDropConfig ) );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCARCAM_PARAM_FRAME_RATE failed ret %d", ret );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            RIDEHAL_INFO( "QCARCAM_PARAM_FRAME_RATE Success" );
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == m_bIsAllocator ) )
    {
        ret = AllocateBuffer();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "AllocateBuffer failed" );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else if (!m_QcarCamHndl)
    {
        RIDEHAL_ERROR( "Error happens in Init" );
        (void )QCarCamRelease( m_QcarCamHndl );
        (void )QCarCamClose( m_QcarCamHndl );
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
        m_QcarCamHndl = 0;
    }
    else
    {
        /* Nothing to do */
    }

    return ret;
}

/// @brief Start the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if (RIDEHAL_COMPONENT_STATE_READY == m_state)
    {
        m_state = RIDEHAL_COMPONENT_STATE_STATING;
        status = QCarCamReserve( m_QcarCamHndl );
        if ( QCARCAM_RET_OK != status )
        {
            RIDEHAL_ERROR( "QCarCamReserve failed with ret %d, exit", status );
            ret = RIDEHAL_ERROR_FAIL;
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            status = QCarCamStart( m_QcarCamHndl );
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "QCarCamStart failed with ret %d , exit", status );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "QCarCamStart success" );
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            }

            if (m_bRequestMode)
            {
                m_nRequestId = 0;

                for ( uint32_t i = 0; i < m_nBufCnt; i++ )
                {
                    ret = RequestFrame(&m_pCameraFrames[i]);
                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "RequestFrame failed, index: %d", i );
                        break;
                    }
                }
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in ready state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}


/// @brief Stop the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STOPING;

        status = QCarCamStop( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
            RIDEHAL_INFO( "Qcarcam Stop call success" );
        }
        else
        {
            RIDEHAL_ERROR( "Qcarcam could not be stopped" );
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in running state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief deinit the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        if ( 0 == m_QcarCamHndl )
        {
            RIDEHAL_ERROR( "Qcarcam null handle" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            status = QCarCamRelease( m_QcarCamHndl );
            if ( QCARCAM_RET_OK !=  status )
            {
                RIDEHAL_ERROR( "QCarCamRelease failed %d", status );
            }

            status = QCarCamClose( m_QcarCamHndl );
            if ( QCARCAM_RET_OK !=  status )
            {
                RIDEHAL_ERROR( "QCarCamClose failed %d", status );
            }

            if ( m_bIsAllocator )
            {
                ret = FreeBuffer();
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Error in free buffer" );
                }
            }
        }

        if ( m_pCameraFrames )
        {
            delete [] m_pCameraFrames;
            m_pCameraFrames = nullptr;
        }

        if ( m_pQcarcamBuffer )
        {
            delete [] m_pQcarcamBuffer;
            m_pQcarcamBuffer = nullptr;
        }

        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in ready state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief Pause the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Pause()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        status = QCarCamPause( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            RIDEHAL_INFO( "QCarCamPause success" );
            m_state = RIDEHAL_COMPONENT_STATE_PAUSE;
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamPause fail" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in running state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief Resume the Camera object
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::Resume()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_PAUSE == m_state )
    {
        status = QCarCamResume( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            RIDEHAL_INFO( "QCarCamResume success" );
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamResume fail" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in pause state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief release a camera frame
/// @param frameIndex index of the frame
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::ReleaseFrame( uint32_t frameIndex )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        status = QCarCamReleaseFrame( m_QcarCamHndl, m_nStreamId, frameIndex );
        if ( QCARCAM_RET_OK == status )
        {
            RIDEHAL_INFO( "QCarCamReleaseFrame success for index: %d", frameIndex );
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamReleaseFrame fail for index: %d, status: %d", frameIndex, status );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in running state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief get a ready frame from camera
/// @return pointer to camera frame
CameraFrame_t *Camera::GetFrame(const QCarCamFrameInfo_t *pframeinfo)
{
    int32_t frameIndex = -1;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamFrameInfo_t frameInformation;
    CameraFrame_t *pCameraFrame = nullptr;
    uint64_t timeout = 0;

    if (!m_bRequestMode)
    {
        frameInformation.id = pframeinfo->id;
        status = QCarCamGetFrame( m_QcarCamHndl, &frameInformation, timeout, 0 );

        if ( QCARCAM_RET_OK == status )
        {
            frameIndex = frameInformation.bufferIndex;
            pCameraFrame = &m_pCameraFrames[frameIndex];
            pCameraFrame->timestamp = frameInformation.sofTimestamp.timestamp;
            pCameraFrame->timestampQGPTP = frameInformation.sofTimestamp.timestampGPTP;
            pCameraFrame->flags = frameInformation.flags;
            RIDEHAL_INFO( "GetFrame index: %d ptr: %p buffer: %p, size: %d, timestamp: %llu, timestampGPTP: %llu, flags: %x",
                    frameIndex, pCameraFrame, pCameraFrame->sharedBuffer.data(), pCameraFrame->sharedBuffer.size,
                    pCameraFrame->timestamp, pCameraFrame->timestampQGPTP, pCameraFrame->flags);
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamGetFrame failed, m_QcarCamHndl: %lu", m_QcarCamHndl );
        }
    }
    else
    {
        frameIndex = pframeinfo->bufferIndex;
        pCameraFrame = &m_pCameraFrames[frameIndex];
        pCameraFrame->timestamp = frameInformation.sofTimestamp.timestamp;
        pCameraFrame->timestampQGPTP = frameInformation.sofTimestamp.timestampGPTP;
        pCameraFrame->flags = frameInformation.flags;
        RIDEHAL_INFO( "GetFrame index: %d ptr: %p buffer: %p, size: %d, timestamp: %llu, timestampGPTP: %llu, flags: %x",
                    frameIndex, pCameraFrame, pCameraFrame->sharedBuffer.data(), pCameraFrame->sharedBuffer.size,
                    pCameraFrame->timestamp, pCameraFrame->timestampQGPTP, pCameraFrame->flags);
    }

    return pCameraFrame;
}

/// @brief resuest a new camera frame
/// @param pFrame the frame to request from camera
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::RequestFrame( CameraFrame_t *pFrame )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamRequest_t request = {};

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        if ( nullptr != pFrame )
        {
            request.numStreamRequests = 1;
            request.requestId = m_nRequestId;
            m_nRequestId ++;
            QCarCamStreamRequest_t* pStreamRequest = &request.streamRequests[0];
            pStreamRequest->bufferlistId = m_nStreamId;
            pStreamRequest->bufferIdx = pFrame->frameIndex;

            status = QCarCamSubmitRequest( m_QcarCamHndl, &request );

            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "RequestFrame fail m_QcarCamHndl: %lu, bufferlistId: %d, bufferIdx: %d request id: %d",
                        m_QcarCamHndl, pStreamRequest->bufferlistId, pStreamRequest->bufferIdx, request.requestId );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "RequestFrame success m_QcarCamHndl: %lu, bufferlistId: %d, bufferIdx: %d, request id: %d",
                        m_QcarCamHndl, pStreamRequest->bufferlistId, pStreamRequest->bufferIdx, request.requestId );
            }
        }
        else
        {
            RIDEHAL_ERROR( "pFrame is nullptr" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in running state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief register callback
/// @param frameCallback frame callback function
/// @param eventCallback event callback function
/// @param pAppPriv app private data
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::RegisterCallback( RideHal_CamFrameCallback_t frameCallback,
                                         RideHal_CamEventCallback_t eventCallback, void *pAppPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_FrameCallback = frameCallback;
    m_EventCallback = eventCallback;
    m_pAppPriv = pAppPriv;

    return ret;
}

/// @brief allocate buffers for camera
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::AllocateBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( m_nBufCnt > 0 )
    {
        m_pCameraFrames = new CameraFrame_t[m_nBufCnt];
        m_pQcarcamBuffer = new QCarCamBuffer_t[m_nBufCnt];
        m_qcarcamBuffers.id = m_nInputId;
        m_qcarcamBuffers.nBuffers = m_nBufCnt;
        m_qcarcamBuffers.pBuffers = m_pQcarcamBuffer;
        m_qcarcamBuffers.colorFmt = GetQcarCamFormat( m_colorFormat );

        m_qcarcamBuffers.flags = QCARCAM_BUFFER_FLAG_OS_HNDL;

        for ( uint32_t i = 0; i < m_nBufCnt; i++ )
        {
            if ( RIDEHAL_ERROR_NONE !=
                 m_pCameraFrames[i].sharedBuffer.Allocate( m_nWidth, m_nHeight, m_colorFormat ) )
            {
                RIDEHAL_ERROR( "Buffer allocation failed" );
                ret = RIDEHAL_ERROR_FAIL;
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
                m_pCameraFrames[i].frameIndex = i;
                RIDEHAL_INFO(
                        "register buffer index %d ptr: %p memHndl: %x va: %p width: %d, "
                        "height: %d, steide: %d size: %d",
                        i, &m_pCameraFrames[i], m_pQcarcamBuffer[i].planes[0].memHndl,
                        m_pCameraFrames[i].sharedBuffer.data(), m_pQcarcamBuffer[i].planes[0].width,
                        m_pQcarcamBuffer[i].planes[0].height, m_pQcarcamBuffer[i].planes[0].stride,
                        m_pQcarcamBuffer[i].planes[0].size );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // setup buffers
            status = QCarCamSetBuffers( m_QcarCamHndl, (const QCarCamBufferList_t *) &m_qcarcamBuffers );
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status,
                               m_QcarCamHndl );
                if ( m_pCameraFrames )
                {
                    delete [] m_pCameraFrames;
                    m_pCameraFrames = nullptr;
                }
                if ( m_pQcarcamBuffer )
                {
                    delete [] m_pQcarcamBuffer;
                    m_pQcarcamBuffer = nullptr;
                }
                ret = RIDEHAL_ERROR_FAIL;
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
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

/// @brief free camera buffer
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::FreeBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_INFO( "Camera::FreeBuffer" );

    if ( m_pCameraFrames )
    {
        for ( uint32_t i = 0; i < m_nBufCnt; i++ )
        {
            ret = m_pCameraFrames[i].sharedBuffer.Free();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Free buffer failed index: %d, ret: %d", i, ret );
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "m_pCameraFrames is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

/// @brief resuest a new camera frame
/// @param pBuffer a list of buffers to be set to camera
/// @param numBuffers number of buffers to be set
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::SetBuffers( const RideHal_SharedBuffer_t *pBuffer, uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        if ((nullptr == pBuffer) || (0 >= numBuffers))
        {
            RIDEHAL_ERROR( "invalid parameter pBuffer: %p, numBuffers: %d", pBuffer, numBuffers );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_nBufCnt = numBuffers;
            m_pCameraFrames = new CameraFrame_t[m_nBufCnt];
            m_pQcarcamBuffer = new QCarCamBuffer_t[m_nBufCnt];
            m_qcarcamBuffers.id = m_nInputId;
            m_qcarcamBuffers.nBuffers = m_nBufCnt;
            m_qcarcamBuffers.pBuffers = m_pQcarcamBuffer;
            m_qcarcamBuffers.colorFmt = GetQcarCamFormat( m_colorFormat );

            for ( uint32_t i = 0; i < m_nBufCnt; i++ )
            {
                m_pCameraFrames[i].sharedBuffer = pBuffer[i];

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

                m_pCameraFrames[i].frameIndex = i;
                RIDEHAL_INFO( "register buffer index %d ptr: %p memHndl: %x va: %p width: %d, "
                               "height: %d, steide: %d size: %d",
                               i, &m_pCameraFrames[i], m_pQcarcamBuffer[i].planes[0].memHndl,
                               m_pCameraFrames[i].sharedBuffer.data(), m_pQcarcamBuffer[i].planes[0].width,
                               m_pQcarcamBuffer[i].planes[0].height, m_pQcarcamBuffer[i].planes[0].stride,
                               m_pQcarcamBuffer[i].planes[0].size );
            }

            // setup buffers
            if ( QCARCAM_RET_OK !=
                    ( status = QCarCamSetBuffers( m_QcarCamHndl,
                                                  (const QCarCamBufferList_t *) &m_qcarcamBuffers ) ) )
            {
                RIDEHAL_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status,
                        m_QcarCamHndl );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "QCarCamSetBuffers successful" );
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in ready state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

/// @brief query camera info and get input ids
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::QueryInputs()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t inputCount  = 0;
    m_sCameraInputsInfo.numInputs = 0;

    RIDEHAL_LOG_INFO("QueryInputs");
    status = QCarCamQueryInputs(NULL, 0, &inputCount);

    if ( QCARCAM_RET_OK != status )
    {
        RIDEHAL_LOG_ERROR( "Failed QCarCamQueryInputs number of inputs %d", status );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else if ( 0 == inputCount )
    {
        RIDEHAL_LOG_ERROR( "Didn't detect any camera connection" );
    }
    else
    {
        m_sCameraInputsInfo.pCameraInputs = new QCarCamInput_t[inputCount];

        if ( nullptr == m_sCameraInputsInfo.pCameraInputs )
        {
            RIDEHAL_LOG_ERROR( "Failed to allocate memory" );
        }
        else
        {
            status = QCarCamQueryInputs(m_sCameraInputsInfo.pCameraInputs, inputCount, &m_sCameraInputsInfo.numInputs);

            if ( ( QCARCAM_RET_OK != status ) || ( m_sCameraInputsInfo.numInputs != inputCount ) )
            {
                RIDEHAL_LOG_ERROR( "Query failed QCarCamQueryInputs %d %d %d", status, m_sCameraInputsInfo.numInputs, inputCount );
                delete ( m_sCameraInputsInfo.pCameraInputs );
                m_sCameraInputsInfo.pCameraInputs = nullptr;
                m_sCameraInputsInfo.numInputs = 0;
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                for ( int i = 0; i < inputCount; i++ )
                {
                    RIDEHAL_LOG_INFO( "Available camera input id: %d", m_sCameraInputsInfo.pCameraInputs[i].inputId );
                }
            }
        }
    }

    return ret;
}

/// @brief get camera inputs info
/// @return RIDEHAL_ERROR_NONE on success, others on failure
RideHalError_e Camera::GetInputsInfo( CameraInputs_t *pCamInputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == m_sCameraInputsInfo.pCameraInputs )
    {
        RIDEHAL_LOG_ERROR( "Error in camera inputs" );
        pCamInputs->numInputs = 0;
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        pCamInputs->pCameraInputs = m_sCameraInputsInfo.pCameraInputs;
        pCamInputs->numInputs = m_sCameraInputsInfo.numInputs;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
