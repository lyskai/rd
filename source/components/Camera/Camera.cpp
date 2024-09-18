// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/Camera.hpp"
#include <cstring>
#include <thread>

#define MAX_QUERY_TIMES ( 20 )

namespace ridehal
{
namespace component
{

static int g_nCamInitRefCount = 0;
static std::mutex g_camInitMutex;

static CameraInputs_t s_cameraInputsInfo = { nullptr, nullptr, 0 };

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

static void FreeCameraInputsInfo( void )
{
    if ( nullptr != s_cameraInputsInfo.pCameraInputs )
    {
        delete ( s_cameraInputsInfo.pCameraInputs );
        s_cameraInputsInfo.pCameraInputs = nullptr;
    }
    if ( nullptr != s_cameraInputsInfo.pCamInputModes )
    {
        for ( uint32_t i = 0; i < s_cameraInputsInfo.numInputs; i++ )
        {
            if ( nullptr != s_cameraInputsInfo.pCamInputModes[i].pModes )
            {
                delete ( s_cameraInputsInfo.pCamInputModes[i].pModes );
                s_cameraInputsInfo.pCamInputModes[i].pModes = nullptr;
            }
        }
        delete ( s_cameraInputsInfo.pCamInputModes );
        s_cameraInputsInfo.pCamInputModes = nullptr;
    }

    s_cameraInputsInfo.numInputs = 0;
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
        case RIDEHAL_IMAGE_FORMAT_NV12_UBWC:
        {
            qcarcamFormat = QCARCAM_FMT_UBWC_NV12;
            break;
        }
        case RIDEHAL_IMAGE_FORMAT_P010:
        {
            qcarcamFormat = QCARCAM_FMT_P010;
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
        RIDEHAL_LOG_ERROR( "invalid pPrivateData" );
    }
    else
    {
        Camera *self = (Camera *) pPrivateData;
        status = self->QcarcamEventCb( hndl, eventId, pPayload );
    }

    return status;
}

QCarCamRet_e Camera::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                     const QCarCamEventPayload_t *pPayload )
{
    CameraFrame_t *pCameraFrame = nullptr;

    RIDEHAL_INFO( "QcarcamEventCb eventId: %u", eventId );
    switch ( eventId )
    {
        case QCARCAM_EVENT_FRAME_READY:
        {
            pCameraFrame = GetFrame( &pPayload->frameInfo );
            if ( nullptr != pCameraFrame )
            {
                m_FrameCallback( pCameraFrame, m_pAppPriv );
            }
            else
            {
                RIDEHAL_ERROR( "GetFrame failed, returning nullptr" );
            }

            break;
        }
        case QCARCAM_EVENT_INPUT_SIGNAL:
        {
            RIDEHAL_ERROR( "QCARCAM received new input signal" );
            break;
        }
        case QCARCAM_EVENT_ERROR:
        {
            RIDEHAL_ERROR( "QCARCAM_EVENT_ERROR" );
            m_EventCallback( eventId, (void *) pPayload, m_pAppPriv );
            break;
        }
        default:
        {
            RIDEHAL_ERROR( "event_cb Received unsupported event %d", eventId );
            break;
        }
    }

    return QCARCAM_RET_OK;
}

Camera::Camera()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamInit_t qcarcamInit = { 0 };
    qcarcamInit.apiVersion = QCARCAM_VERSION;
    QCarCamRet_e status = QCARCAM_RET_OK;
    m_QcarCamHndl = 0;

    std::lock_guard<std::mutex> guard( g_camInitMutex );
    if ( 0 == g_nCamInitRefCount )
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
            g_nCamInitRefCount++;

            ret = QueryInputs();

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_LOG_ERROR( "QueryInputs failed: %d", ret );
            }
        }
    }
    else
    {
        g_nCamInitRefCount++;
    }
}

Camera::~Camera()
{
    QCarCamRet_e status = QCARCAM_RET_OK;

    std::lock_guard<std::mutex> guard( g_camInitMutex );
    if ( 0 < g_nCamInitRefCount )
    {
        g_nCamInitRefCount--;

        if ( 0 == g_nCamInitRefCount )
        {
            status = QCarCamUninitialize();
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_LOG_ERROR( "QCarCamUninitialize failed: %d", status );
            }

            FreeCameraInputsInfo();
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

RideHalError_e Camera::Init( const char *pName, const Camera_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t param = 0;

    if ( nullptr == pConfig )
    {
        RIDEHAL_ERROR( "pConfig is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( 0 < pConfig->numStream ) && ( MAX_CAMERA_STREAM >= pConfig->numStream ) )
    {
        for ( int i = 0; i < pConfig->numStream; i++ )
        {
            m_streamConfig[i] = pConfig->streamConfig[i];
        }
    }
    else
    {
        RIDEHAL_ERROR( "Invalid numStream: %d", pConfig->numStream );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bIsAllocator = pConfig->bAllocator;
        m_nInputId = pConfig->inputId;
        m_bRequestMode = pConfig->bRequestMode;
        m_nNumStream = pConfig->numStream;
        ret = ComponentIF::Init( pName, level );

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "ComponentIF::Init failed" );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else if ( RIDEHAL_COMPONENT_STATE_INITIAL == m_state )
        {
            m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;
        }
        else
        {
            RIDEHAL_ERROR( "Camera not in initial state: %d", m_state );
            ret = RIDEHAL_ERROR_BAD_STATE;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        CameraInputs_t camInputsInfo;
        ret = GetInputsInfo( &camInputsInfo );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "GetInputsInfo failed: %d", ret );
        }
        else
        {
            QCarCamInputModes_t *pCamInputModes = nullptr;
            for ( uint32_t i = 0; i < camInputsInfo.numInputs; i++ )
            {
                if ( camInputsInfo.pCameraInputs[i].inputId == m_nInputId )
                {
                    pCamInputModes = &camInputsInfo.pCamInputModes[i];
                    break;
                }
            }

            if ( nullptr == pCamInputModes )
            {
                RIDEHAL_ERROR( "camera input id %u not found", m_nInputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( 0 == pCamInputModes->numModes )
            {
                RIDEHAL_ERROR( "no mode 0 for camera input id %u", m_nInputId );
                ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            }
            else
            {
                // do nothing
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QCarCamInputStream_t inputParams = { 0 };
        inputParams.inputId = pConfig->inputId;
        inputParams.srcId = pConfig->srcId;
        inputParams.inputMode = pConfig->inputMode;

        QCarCamOpen_t openParams = { (QCarCamOpmode_e) 0, 0 };
        openParams.opMode = (QCarCamOpmode_e) pConfig->opMode;
        openParams.numInputs = 1;
        openParams.inputs[0] = inputParams;

        if ( m_bRequestMode )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_REQUEST_MODE;
        }

        status = QCarCamOpen( &openParams, &m_QcarCamHndl );
        if ( ( QCARCAM_RET_OK != status ) || ( !m_QcarCamHndl ) )
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
        QCarCamIspUsecaseConfig_t ispConfig = { 0 };

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
        QCarCamFrameDropConfig_t frameDropConfig = { 0 };
        frameDropConfig.frameDropPeriod = GetNumBitsOfInteger( pConfig->camFrameDropPat );
        frameDropConfig.frameDropPattern = pConfig->camFrameDropPat;

        if ( 0 == pConfig->camFrameDropPat )
        {
            RIDEHAL_INFO( "Ignore frame drop config" );
        }
        else
        {
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
    else if ( !m_QcarCamHndl )
    {
        RIDEHAL_ERROR( "Error happens in Init" );
        (void) QCarCamClose( m_QcarCamHndl );
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
        m_QcarCamHndl = 0;
    }
    else
    {
        /* Nothing to do */
    }

    return ret;
}

RideHalError_e Camera::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STARTING;
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

            if ( m_bRequestMode )
            {
                m_nRequestId = 0;

                for ( uint32_t i = 0; i < m_nNumStream; i++ )
                {
                    for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
                    {
                        uint32_t streamId = m_streamConfig[i].streamId;
                        ret = RequestFrame( &m_pCameraFrames[streamId][j] );
                        if ( RIDEHAL_ERROR_NONE != ret )
                        {
                            RIDEHAL_ERROR( "RequestFrame failed, index: %u %u", i, j );
                            break;
                        }
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
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "QCarCamRelease failed %d", status );
                ret = RIDEHAL_ERROR_FAIL;
            }

            status = QCarCamClose( m_QcarCamHndl );
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "QCarCamClose failed %d", status );
                ret = RIDEHAL_ERROR_FAIL;
            }

            if ( m_bIsAllocator )
            {
                if ( RIDEHAL_ERROR_NONE != FreeBuffer() )
                {
                    RIDEHAL_ERROR( "Error in free buffer" );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
        }

        for ( uint32_t i = 0; i < m_nNumStream; i++ )
        {
            if ( nullptr != m_pCameraFrames[i] )
            {
                delete[] m_pCameraFrames[i];
                m_pCameraFrames[i] = nullptr;
            }

            if ( nullptr != m_pQcarcamBuffer[i] )
            {
                delete[] m_pQcarcamBuffer[i];
                m_pQcarcamBuffer[i] = nullptr;
            }
        }

        if ( RIDEHAL_ERROR_NONE != ComponentIF::Deinit() )
        {
            RIDEHAL_ERROR( "Fail to deinit ComponentIF" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in ready state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

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

RideHalError_e Camera::ReleaseFrame( const CameraFrame_t *pFrame )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Camera not in running state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pFrame )
    {
        RIDEHAL_ERROR( "pFrame is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        status = QCarCamReleaseFrame( m_QcarCamHndl, pFrame->streamId, pFrame->frameIndex );
        if ( QCARCAM_RET_OK == status )
        {
            RIDEHAL_INFO( "QCarCamReleaseFrame success for index: %u", pFrame->frameIndex );
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamReleaseFrame fail for id: %d index: %u, status: %d",
                           pFrame->streamId, pFrame->frameIndex, status );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

CameraFrame_t *Camera::GetFrame( const QCarCamFrameInfo_t *pFrameinfo )
{
    uint32_t frameIndex = 0;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamFrameInfo_t frameInformation = { 0 };
    CameraFrame_t *pCameraFrame = nullptr;
    uint64_t timeout = 0;
    frameInformation.id = pFrameinfo->id;

    if ( !m_bRequestMode )
    {
        status = QCarCamGetFrame( m_QcarCamHndl, &frameInformation, timeout, 0 );

        if ( QCARCAM_RET_OK == status )
        {
            frameIndex = frameInformation.bufferIndex;
            pCameraFrame = &m_pCameraFrames[frameInformation.id][frameIndex];
            pCameraFrame->timestamp = frameInformation.sofTimestamp.timestamp;
            pCameraFrame->timestampQGPTP = frameInformation.sofTimestamp.timestampGPTP;
            pCameraFrame->flags = frameInformation.flags;
            RIDEHAL_INFO( "GetFrame index: %u ptr: %p buffer: %p, size: %u, timestamp: %llu, "
                          "timestampGPTP: %llu, flags: %x",
                          frameIndex, pCameraFrame, pCameraFrame->sharedBuffer.data(),
                          pCameraFrame->sharedBuffer.size, pCameraFrame->timestamp,
                          pCameraFrame->timestampQGPTP, pCameraFrame->flags );
        }
        else
        {
            RIDEHAL_ERROR( "QCarCamGetFrame failed, m_QcarCamHndl: %lu", m_QcarCamHndl );
        }
    }
    else
    {
        frameIndex = pFrameinfo->bufferIndex;
        pCameraFrame = &m_pCameraFrames[pFrameinfo->id][frameIndex];
        pCameraFrame->timestamp = pFrameinfo->sofTimestamp.timestamp;
        pCameraFrame->timestampQGPTP = pFrameinfo->sofTimestamp.timestampGPTP;
        pCameraFrame->flags = pFrameinfo->flags;
        RIDEHAL_INFO( "GetFrame index: %u ptr: %p buffer: %p, size: %d, timestamp: %llu, "
                      "timestampGPTP: %llu, flags: %x",
                      frameIndex, pCameraFrame, pCameraFrame->sharedBuffer.data(),
                      pCameraFrame->sharedBuffer.size, pCameraFrame->timestamp,
                      pCameraFrame->timestampQGPTP, pCameraFrame->flags );
    }

    return pCameraFrame;
}

RideHalError_e Camera::RequestFrame( const CameraFrame_t *pFrame )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamRequest_t request = {};

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        if ( nullptr != pFrame )
        {
            uint32_t bufferListId = pFrame->streamId;
            request.numStreamRequests = 1;

            request.requestId = __atomic_add_fetch( &m_nRequestId, 1, __ATOMIC_RELAXED );

            QCarCamStreamRequest_t *pStreamRequest = &request.streamRequests[0];
            pStreamRequest->bufferlistId = bufferListId;
            pStreamRequest->bufferIdx = pFrame->frameIndex;

            status = QCarCamSubmitRequest( m_QcarCamHndl, &request );

            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "RequestFrame fail m_QcarCamHndl: %lu, bufferlistId: %u, bufferIdx: "
                               "%u request id: %u",
                               m_QcarCamHndl, pStreamRequest->bufferlistId,
                               pStreamRequest->bufferIdx, request.requestId );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "RequestFrame success m_QcarCamHndl: %lu, bufferlistId: %u, "
                              "bufferIdx: %u, request id: %u",
                              m_QcarCamHndl, pStreamRequest->bufferlistId,
                              pStreamRequest->bufferIdx, request.requestId );
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

RideHalError_e Camera::RegisterCallback( RideHal_CamFrameCallback_t frameCallback,
                                         RideHal_CamEventCallback_t eventCallback, void *pAppPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_FrameCallback = frameCallback;
    m_EventCallback = eventCallback;
    m_pAppPriv = pAppPriv;

    return ret;
}

RideHalError_e Camera::AllocateBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t streamId = 0;

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        if ( 0 < m_streamConfig[i].bufCnt )
        {
            streamId = m_streamConfig[i].streamId;

            m_pCameraFrames[streamId] = new CameraFrame_t[m_streamConfig[i].bufCnt];
            m_pQcarcamBuffer[streamId] = new QCarCamBuffer_t[m_streamConfig[i].bufCnt];
            m_qcarcamBuffers[streamId].id = m_streamConfig[i].streamId;
            m_qcarcamBuffers[streamId].nBuffers = m_streamConfig[i].bufCnt;
            m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffer[streamId];
            m_qcarcamBuffers[streamId].colorFmt = GetQcarCamFormat( m_streamConfig[i].format );
            m_qcarcamBuffers[streamId].flags = QCARCAM_BUFFER_FLAG_OS_HNDL;
        }
        else
        {
            RIDEHAL_ERROR( "Invalid buffer count: %d", m_streamConfig[i].bufCnt );
            ret = RIDEHAL_ERROR_FAIL;
            break;
        }

        for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
        {
            if ( RIDEHAL_ERROR_NONE != m_pCameraFrames[streamId][j].sharedBuffer.Allocate(
                                               m_streamConfig[i].width, m_streamConfig[i].height,
                                               m_streamConfig[i].format ) )
            {
                RIDEHAL_ERROR( "Buffer allocation failed" );
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
            else
            {
                m_pQcarcamBuffer[streamId][j].numPlanes = 1;
                m_pQcarcamBuffer[streamId][j].planes[0].memHndl =
                        m_pCameraFrames[streamId][j].sharedBuffer.buffer.dmaHandle;
                m_pQcarcamBuffer[streamId][j].planes[0].width =
                        m_pCameraFrames[streamId][j].sharedBuffer.imgProps.width;
                m_pQcarcamBuffer[streamId][j].planes[0].height =
                        m_pCameraFrames[streamId][j].sharedBuffer.imgProps.height;
                m_pQcarcamBuffer[streamId][j].planes[0].stride =
                        m_pCameraFrames[streamId][j].sharedBuffer.imgProps.stride[0];
                m_pQcarcamBuffer[streamId][j].planes[0].size =
                        m_pCameraFrames[streamId][j].sharedBuffer.size;
                m_pCameraFrames[streamId][j].frameIndex = j;
                m_pCameraFrames[streamId][j].streamId = m_streamConfig[i].streamId;
                RIDEHAL_INFO( "register buffer index %u ptr: %p memHndl: %llu va: %p width: %u, "
                              "height: %u, stride: %u size: %u",
                              i, &m_pCameraFrames[streamId][j],
                              m_pQcarcamBuffer[streamId][j].planes[0].memHndl,
                              m_pCameraFrames[streamId][j].sharedBuffer.data(),
                              m_pQcarcamBuffer[streamId][j].planes[0].width,
                              m_pQcarcamBuffer[streamId][j].planes[0].height,
                              m_pQcarcamBuffer[streamId][j].planes[0].stride,
                              m_pQcarcamBuffer[streamId][j].planes[0].size );
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            // setup buffers
            status = QCarCamSetBuffers( m_QcarCamHndl,
                                        (const QCarCamBufferList_t *) &m_qcarcamBuffers[streamId] );
            if ( QCARCAM_RET_OK != status )
            {
                RIDEHAL_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status,
                               m_QcarCamHndl );
                if ( nullptr != m_pCameraFrames[streamId] )
                {
                    delete[] m_pCameraFrames[streamId];
                    m_pCameraFrames[streamId] = nullptr;
                }
                if ( nullptr != m_pQcarcamBuffer[streamId] )
                {
                    delete[] m_pQcarcamBuffer[streamId];
                    m_pQcarcamBuffer[streamId] = nullptr;
                }
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "QCarCamSetBuffers successful" );
            }
        }
    }

    return ret;
}

RideHalError_e Camera::FreeBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_INFO( "Camera::FreeBuffer" );

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        if ( nullptr != m_pCameraFrames[i] )
        {
            for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
            {
                ret = m_pCameraFrames[i][j].sharedBuffer.Free();
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Free buffer failed index: %u %u, ret: %d", i, j, ret );
                }
            }
        }
        else
        {
            RIDEHAL_ERROR( "m_pCameraFrames is nullptr" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    return ret;
}

RideHalError_e Camera::SetBuffers( const RideHal_SharedBuffer_t *pBuffer, uint32_t numBuffers,
                                   uint32_t streamId )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        if ( ( nullptr == pBuffer ) || ( 0 >= numBuffers ) )
        {
            RIDEHAL_ERROR( "invalid parameter pBuffer: %p, numBuffers: %u", pBuffer, numBuffers );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            uint32_t index = 0;
            bool bFound = false;

            for ( index = 0; index < m_nNumStream; index++ )
            {
                if ( nullptr == m_pCameraFrames[index] )
                {
                    bFound = true;
                    break;
                }
            }

            if ( bFound == false )
            {
                RIDEHAL_ERROR( "Did not find free index unexpected" );
            }
            else
            {
                m_pCameraFrames[index] = new CameraFrame_t[numBuffers];
                m_pQcarcamBuffer[index] = new QCarCamBuffer_t[numBuffers];
                m_qcarcamBuffers[index].id = streamId;
                m_qcarcamBuffers[index].nBuffers = numBuffers;
                m_qcarcamBuffers[index].pBuffers = m_pQcarcamBuffer[index];
                m_qcarcamBuffers[index].colorFmt = GetQcarCamFormat( pBuffer[0].imgProps.format );

                for ( uint32_t i = 0; i < numBuffers; i++ )
                {
                    m_pCameraFrames[index][i].sharedBuffer = pBuffer[i];

                    m_pQcarcamBuffer[index][i].numPlanes = 1;
                    m_pQcarcamBuffer[index][i].planes[0].memHndl =
                            m_pCameraFrames[index][i].sharedBuffer.buffer.dmaHandle;
                    m_pQcarcamBuffer[index][i].planes[0].width =
                            m_pCameraFrames[index][i].sharedBuffer.imgProps.width;
                    m_pQcarcamBuffer[index][i].planes[0].height =
                            m_pCameraFrames[index][i].sharedBuffer.imgProps.height;
                    m_pQcarcamBuffer[index][i].planes[0].stride =
                            m_pCameraFrames[index][i].sharedBuffer.imgProps.stride[0];
                    m_pQcarcamBuffer[index][i].planes[0].size =
                            m_pCameraFrames[index][i].sharedBuffer.size;

                    m_pCameraFrames[index][i].streamId = streamId;
                    m_pCameraFrames[index][i].frameIndex = i;
                    RIDEHAL_INFO(
                            "register buffer index %u ptr: %p memHndl: %llu va: %p width: %u, "
                            "height: %u, stride: %u size: %u",
                            i, &m_pCameraFrames[index][i],
                            m_pQcarcamBuffer[index][i].planes[0].memHndl,
                            m_pCameraFrames[index][i].sharedBuffer.data(),
                            m_pQcarcamBuffer[index][i].planes[0].width,
                            m_pQcarcamBuffer[index][i].planes[0].height,
                            m_pQcarcamBuffer[index][i].planes[0].stride,
                            m_pQcarcamBuffer[index][i].planes[0].size );
                }

                // setup buffers
                if ( QCARCAM_RET_OK !=
                     ( status = QCarCamSetBuffers(
                               m_QcarCamHndl,
                               (const QCarCamBufferList_t *) &m_qcarcamBuffers[index] ) ) )
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
    }
    else
    {
        RIDEHAL_ERROR( "Camera not in ready state: %d", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e Camera::QueryInputs()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t inputCount = 0;
    uint32_t queryCount = 0;
    s_cameraInputsInfo.numInputs = 0;

    do
    {
        status = QCarCamQueryInputs( NULL, 0, &inputCount );
        if ( ( QCARCAM_RET_OK != status ) || ( 0 == inputCount ) )
        {
            queryCount++;
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
        else
        {
            break;
        }
    } while ( ( 0 == inputCount ) && ( queryCount < MAX_QUERY_TIMES ) );

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
        s_cameraInputsInfo.pCameraInputs = new QCarCamInput_t[inputCount];
        s_cameraInputsInfo.pCamInputModes = new QCarCamInputModes_t[inputCount];
        if ( ( nullptr == s_cameraInputsInfo.pCameraInputs ) ||
             ( nullptr == s_cameraInputsInfo.pCamInputModes ) )
        {
            RIDEHAL_LOG_ERROR( "Failed to allocate memory" );
            ret = RIDEHAL_ERROR_NOMEM;
        }
        else
        {
            (void) memset( s_cameraInputsInfo.pCamInputModes, 0,
                           sizeof( QCarCamInputModes_t ) * inputCount );

            status = QCarCamQueryInputs( s_cameraInputsInfo.pCameraInputs, inputCount,
                                         &s_cameraInputsInfo.numInputs );

            if ( ( QCARCAM_RET_OK != status ) || ( s_cameraInputsInfo.numInputs != inputCount ) )
            {
                RIDEHAL_LOG_ERROR( "Query failed QCarCamQueryInputs %u %u: ret = %d",
                                   s_cameraInputsInfo.numInputs, inputCount, status );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                for ( uint32_t i = 0; i < inputCount; i++ )
                {
                    RIDEHAL_LOG_INFO( "Available camera input id: %u, numModes = %u",
                                      s_cameraInputsInfo.pCameraInputs[i].inputId,
                                      s_cameraInputsInfo.pCameraInputs[i].numModes );

                    s_cameraInputsInfo.pCamInputModes[i].pModes =
                            new QCarCamMode_t[s_cameraInputsInfo.pCameraInputs[i].numModes];
                    s_cameraInputsInfo.pCamInputModes[i].numModes =
                            s_cameraInputsInfo.pCameraInputs[i].numModes;
                    if ( nullptr != s_cameraInputsInfo.pCamInputModes[i].pModes )
                    {
                        status =
                                QCarCamQueryInputModes( s_cameraInputsInfo.pCameraInputs[i].inputId,
                                                        &s_cameraInputsInfo.pCamInputModes[i] );
                        if ( QCARCAM_RET_OK != status )
                        {
                            RIDEHAL_LOG_ERROR( "Query Input Modes failed for input %u: ret = %d",
                                               s_cameraInputsInfo.pCameraInputs[i].inputId,
                                               status );
                            ret = RIDEHAL_ERROR_FAIL;
                            break;
                        }
                        else
                        {
                            RIDEHAL_LOG_INFO(
                                    "Found camera with input %du: mode 0 src 0: resolution %ux%u",
                                    s_cameraInputsInfo.pCameraInputs[i].inputId,
                                    s_cameraInputsInfo.pCamInputModes[i].pModes[0].sources[0].width,
                                    s_cameraInputsInfo.pCamInputModes[i]
                                            .pModes[0]
                                            .sources[0]
                                            .height );
                        }
                    }
                    else
                    {
                        RIDEHAL_LOG_ERROR( "Failed to allocate memory for input %u modes",
                                           s_cameraInputsInfo.pCameraInputs[i].inputId );
                        ret = RIDEHAL_ERROR_NOMEM;
                    }
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            FreeCameraInputsInfo();
        }
    }

    return ret;
}

RideHalError_e Camera::GetInputsInfo( CameraInputs_t *pCamInputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pCamInputs )
    {
        RIDEHAL_LOG_ERROR( "pCamInputs is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == s_cameraInputsInfo.pCameraInputs )
    {
        RIDEHAL_LOG_ERROR( "Error in camera inputs" );
        pCamInputs->numInputs = 0;
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        *pCamInputs = s_cameraInputsInfo;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
