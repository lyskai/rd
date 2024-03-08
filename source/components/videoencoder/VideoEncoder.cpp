//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include <MMCriticalSection.h>
#include <MMTimer.h>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <malloc.h>
#include <vidc_ioctl.h>

#include "ride/hal/Image.hpp"
#include "ride/hal/VideoEncoder.hpp"
using namespace ride::hal;

namespace ride
{
namespace hal
{
namespace component
{

#define CAM_ALIGN( sz, align ) ( ( ( sz ) + align - 1 ) & ( ~( align - 1 ) ) )


static constexpr uint16_t MAX_DEV_CMD_BUFFER_SIZE = 256;
static constexpr int WAIT_TIMEOUT_1_MSEC = 1;

VideoEncoder::VideoEncoder() {}
VideoEncoder::~VideoEncoder() {}

RideHalError_e VideoEncoder::Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                                   Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    ret = ComponentIF::Init( pName, pLogger );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {


        int32_t rc = 0;
        m_Width = pConfig->width;
        m_Height = pConfig->height;
        m_FrameRate = pConfig->frameRate;
        m_Format = convertFormat( pConfig->format );
        m_DynamicMode = pConfig->bDynamicMode;
        m_InputBufferDoneCb = pConfig->inputBufferDoneCb;
        m_OutputBufferDoneCb = pConfig->outputBufferDoneCb;

        if ( pConfig->rateControlMode != RIDE_HAL_CBR_CFR &&
             pConfig->rateControlMode != RIDE_HAL_VBR_CFR )
        {
            RIDEHAL_ERROR(
                    "Only RIDE_HAL_CBR_CFR and RIDE_HAL_VBR_CFR are allowed rate control modes!" );
            return RIDE_HAL_ERROR_UNSUPPORTED;
        }

        m_VidcEncoderData.bitrate.target_bitrate = pConfig->bitRate * 1000000;

        if ( pConfig->rateControlMode == RIDE_HAL_VBR_CFR )
        {
            m_VidcEncoderData.rateControl = VIDC_RATE_CONTROL_VBR_CFR;
        }
        else
        {
            m_VidcEncoderData.rateControl = VIDC_RATE_CONTROL_CBR_CFR;
        }

        m_VidcEncoderData.codec = VIDC_CODEC_HEVC;
        m_VidcEncoderData.profile.profile = VIDC_PROFILE_HEVC_MAIN10;   // VIDC_PROFILE_HEVC_MAIN
        m_VidcEncoderData.level.level = VIDC_LEVEL_HEVC_4;
        m_VidcEncoderData.iPeriod.p_frames = pConfig->gop;
        m_VidcEncoderData.iPeriod.b_frames = 0;
        m_VidcEncoderData.idrPeriod.idr_period = 1;
        m_VidcEncoderData.numInputBufferReq = pConfig->numInputBufferReq
                                                      ? pConfig->numInputBufferReq
                                                      : DEFAULT_VIDC_INPUT_BUFFER_REQ;
        m_VidcEncoderData.numOutputBufferReq = pConfig->numOutputBufferReq
                                                       ? pConfig->numOutputBufferReq
                                                       : DEFAULT_VIDC_OUTPUT_BUFFER_REQ;
        m_VidcEncoderData.sessionCodec.session = VIDC_SESSION_ENCODE;
        m_VidcEncoderData.sessionCodec.codec = m_VidcEncoderData.codec;

        printEncoderConfig();

        if ( MM_CriticalSection_Create( &m_VidcEncoderData.lock ) != 0 )
        {
            RIDEHAL_ERROR( "MM_CriticalSection_Create lock failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        if ( MM_CriticalSection_Create( &m_VidcEncoderData.fbdLock ) != 0 )
        {
            RIDEHAL_ERROR( "MM_CriticalSection_Create fbdLock failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        if ( MM_SignalQ_Create( &m_VidcEncoderData.fbdNotificationQ ) != 0 )
        {
            RIDEHAL_ERROR( "MM_SignalQ_Create fbdNotificationQ failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        if ( MM_Signal_Create( m_VidcEncoderData.fbdNotificationQ, nullptr, nullptr,
                               &m_VidcEncoderData.fbdNotification ) != 0 )
        {
            RIDEHAL_ERROR( "MM_Signal_Create fbdNotification failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        if ( MM_SignalQ_Create( &m_VidcEncoderData.specialEventNotificationQ ) != 0 )
        {
            RIDEHAL_ERROR( "MM_SignalQ_Create specialEventNotificationQ failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        if ( MM_Signal_Create( m_VidcEncoderData.specialEventNotificationQ, nullptr, nullptr,
                               &m_VidcEncoderData.specialEventNotification ) != 0 )
        {
            RIDEHAL_ERROR( "MM_Signal_Create specialEventNotification failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        // m_FbdThread = std::thread( [this] { this->fillBufferDoneHandler(); } );

        m_IoctlCb.handler = ride::hal::component::VideoEncoder::DeviceCallback;
        m_IoctlCb.data = (void *) this;

        RIDEHAL_DEBUG( "Opening vidc device" );
        m_VidcEncoderData.ioHandle = device_open( (char *) "VideoCore/vidc_drv", &m_IoctlCb );
        if ( m_VidcEncoderData.ioHandle == nullptr )
        {
            RIDEHAL_ERROR( "Failed to open vidc device!" );
            teardown();
            return RIDE_HAL_ERROR_NULL_PTR;
        }

        m_VidcEncoderData.state = VIDEO_ENCODER_STATE_LOADED;
        RIDEHAL_DEBUG( "Setting VIDC_I_SESSION_CODEC" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_SESSION_CODEC,
                             sizeof( vidc_session_codec_type ),
                             (uint8_t *) &m_VidcEncoderData.sessionCodec );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_SESSION_CODEC failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT" );
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
        m_VidcEncoderData.frameRate.fps_numerator = m_FrameRate;
        m_VidcEncoderData.frameRate.fps_denominator = 1;
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT" );
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.frameRate.fps_numerator = m_FrameRate;
        m_VidcEncoderData.frameRate.fps_denominator = 1;
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        m_VidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.colorFormatConfig.color_format = m_Format;
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) &m_VidcEncoderData.colorFormatConfig );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_COLOR_FORMAT.VIDC_BUFFER_INPUT failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT" );
        m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.frameSize.width = m_Width;
        m_VidcEncoderData.frameSize.height = m_Height;
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) &m_VidcEncoderData.frameSize );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT" );
        m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_OUTPUT;
        m_VidcEncoderData.frameSize.width = m_Width;
        m_VidcEncoderData.frameSize.height = m_Height;
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) &m_VidcEncoderData.frameSize );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_INTRA_PERIOD" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_INTRA_PERIOD,
                             sizeof( vidc_iperiod_type ), (uint8_t *) &m_VidcEncoderData.iPeriod );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_ENC_INTRA_PERIOD failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_IDR_PERIOD" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_IDR_PERIOD,
                             sizeof( vidc_idr_period_type ),
                             (uint8_t *) &m_VidcEncoderData.idrPeriod );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_ENC_IDR_PERIOD failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_RATE_CONTROL" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_RATE_CONTROL,
                             sizeof( vidc_rate_control_mode_type ),
                             (uint8_t *) &m_VidcEncoderData.rateControl );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_ENC_RATE_CONTROL failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }
        RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                             sizeof( vidc_target_bitrate_type ),
                             (uint8_t *) &m_VidcEncoderData.bitrate );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_TARGET_BITRATE failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_PROFILE" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PROFILE,
                             sizeof( vidc_profile_type ), (uint8_t *) &m_VidcEncoderData.profile );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_PROFILE failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_LEVEL" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_LEVEL, sizeof( vidc_level_type ),
                             (uint8_t *) &m_VidcEncoderData.level );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_LEVEL failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Setting VIDC_I_VPE_SPATIAL_TRANSFORM" );
        vidc_spatial_transform_type vidcSpatialTransform = { VIDC_ROTATE_NONE, VIDC_FLIP_NONE };
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_VPE_SPATIAL_TRANSFORM,
                             sizeof( vidc_spatial_transform_type ),
                             (uint8_t *) &vidcSpatialTransform );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_VPE_SPATIAL_TRANSFORM failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        vidc_session_qp_type vidc_qp = { 20, 20, 20 };
        RIDEHAL_DEBUG( "VIDC_I_ENC_SESSION_QP" );
        rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_SESSION_QP,
                             sizeof( vidc_session_qp_type ), (uint8_t *) &vidc_qp );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "setDrvProperty VIDC_I_ENC_SESSION_QP failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        rc = getInputInformation();
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputInformation failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        rc = getInputBufferRequirement();
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputBufferRequirement failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        rc = getOutputBufferRequirement();
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getOutputBufferRequirement failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        if ( m_DynamicMode )
        {
            RIDEHAL_DEBUG( "Enable input dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_INPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                 sizeof( buffer_alloc_mode ), (uint8_t *) &buffer_alloc_mode );
            if ( rc != 0 )
            {
                RIDEHAL_ERROR( "setDrvProperty VIDC_I_BUFFER_ALLOC_MODE failed!" );
                teardown();
                return RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " input buffers",
                           m_VidcEncoderData.numInputBufferReq );
            rc = allocateBuffer( m_VidcEncoderData.ioHandle, &m_VidcEncoderData.vidcInputBufferInfo,
                                 VIDC_BUFFER_INPUT, m_VidcEncoderData.numInputBufferReq,
                                 m_VidcEncoderData.vidcInputBufferSize );
            if ( rc != 0 )
            {
                RIDEHAL_ERROR( "Failed to allocate input buffers!" );
                teardown();
                return RIDE_HAL_ERROR_FAIL;
            }
        }

        rc = allocateFrameData( &m_VidcEncoderData.vidcInputFrameInfo,
                                m_VidcEncoderData.numInputBufferReq );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "allocateFrameData vidcInputFrameInfo failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Allocating %" PRIu32 " output buffers",
                       m_VidcEncoderData.numOutputBufferReq );

        rc = allocateBuffer( m_VidcEncoderData.ioHandle, &m_VidcEncoderData.vidcOutputBufferInfo,
                             VIDC_BUFFER_OUTPUT, m_VidcEncoderData.numOutputBufferReq,
                             m_VidcEncoderData.vidcOutputBufferSize );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "Failed to allocate output buffers!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        rc = allocateFrameData( &m_VidcEncoderData.vidcOutputFrameInfo,
                                m_VidcEncoderData.numOutputBufferReq );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "allocateFrameData vidcOutputFrameInfo failed!" );
            teardown();
            return RIDE_HAL_ERROR_FAIL;
        }

        RIDEHAL_DEBUG( "Loading vidc resources" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0, nullptr,
                      0 );
        RIDEHAL_DEBUG( "Successfully completed vidc initialization!" );
    }
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_State = RIDE_HAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e VideoEncoder::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_State )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        int32_t rc = 0;
        RIDEHAL_DEBUG( "Starting vidc" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
        if ( waitForState( VIDEO_ENCODER_STATE_EXECUTING ) != 0 )
        {
            RIDEHAL_ERROR( "waitForState STATE_EXECUTING failed!" );
            teardown();
            return RIDE_HAL_ERROR_TIMEOUT;
        }

        MM_CriticalSection_Enter( m_VidcEncoderData.fbdLock );
        for ( uint32_t counter = 0; counter < m_VidcEncoderData.numOutputBufferReq; counter++ )
        {
            rc = fillBuffer( counter );
            if ( rc != 0 )
            {
                RIDEHAL_ERROR( "fillBuffer index=%" PRIu32 " failed!", counter );
                MM_CriticalSection_Leave( m_VidcEncoderData.fbdLock );
                teardown();
                return RIDE_HAL_ERROR_FAIL;
            }
        }
        MM_CriticalSection_Leave( m_VidcEncoderData.fbdLock );

        for ( uint16_t i = 0; i < m_VidcEncoderData.numInputBufferReq; i++ )
        {
            m_AvailableInputBufferQueue.push( i );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_State = RIDE_HAL_COMPONENT_STATE_RUNNING;
    }

    return RIDE_HAL_ERROR_NONE;
}

RideHalError_e VideoEncoder::SubmitInputBuffer( const RideHal_SharedBuffer_t *pSharedBuffer,
                                                uint64_t timestampNs )
{
    auto inputBuffer = pSharedBuffer;
    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_State )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
        return RIDE_HAL_ERROR_STATE;
    }

    if ( nullptr != inputBuffer )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        return RIDE_HAL_ERROR_NULL_PTR;
    }

    uint16_t bufId = MAX_UINT16;
    if ( m_AvailableInputBufferQueue.empty() )
    {
        RIDEHAL_ERROR( "No empty input buffer available!" );
        return RIDE_HAL_ERROR_NORES;
    }
    else
    {
        bufId = m_AvailableInputBufferQueue.front();
        m_AvailableInputBufferQueue.pop();
    }

    vidc_frame_data_type *pFrameData = m_VidcEncoderData.vidcInputFrameInfo[bufId];
    vidc_buffer_info_type vidc_buffer_info;
    vidc_buffer_info_type *pBuffer = nullptr;

    if ( false == m_DynamicMode )
    {
        pBuffer = m_VidcEncoderData.vidcInputBufferInfo[bufId];
    }
    else
    {
        pBuffer = &vidc_buffer_info;
    }

    int convertedSize;

    if ( ( ( inputBuffer->imgProps.format == RIDE_HAL_IMAGE_FORMAT_NV12 ) &&
           ( VIDC_COLOR_FORMAT_NV12 == m_Format ) ) )
    {
        convertedSize = m_VidcEncoderData.vidcInputBufferSize;
        if ( (size_t) convertedSize > inputBuffer->size )
        {
            convertedSize = inputBuffer->size;
        }

        if ( false == m_DynamicMode )
        {
            memcpy( pBuffer->buf_addr, inputBuffer->buffer.pData, convertedSize );
        }
        else
        {
            pBuffer->buf_addr = (uint8_t *) inputBuffer->buffer.pData;
            pBuffer->buf_size = convertedSize;
        }
    }
    else
    {
        RIDEHAL_ERROR( "unsupported input frame format %d", (int) inputBuffer->imgProps.format );
        return RIDE_HAL_ERROR_UNSUPPORTED;
    }


    m_VidcEncoderData.numInputFrames++;

    RIDEHAL_DEBUG( "input-start: bufId %d timestampNs %" PRIu64
                   " frameInfo %p pFrameData %p addr %p",
                   bufId, timestampNs, m_VidcEncoderData.vidcInputFrameInfo, pFrameData,
                   pBuffer->buf_addr );

    memset( pFrameData, 0, sizeof( vidc_frame_data_type ) );
    pFrameData->frm_clnt_data = bufId;
    pFrameData->buf_type = VIDC_BUFFER_INPUT;
    pFrameData->frame_addr = pBuffer->buf_addr;
    pFrameData->alloc_len = pBuffer->buf_size;
    pFrameData->frame_handle = (pmem_handle_t) inputBuffer->buffer.dmaHandle;

    pFrameData->data_len = convertedSize;
    pFrameData->timestamp = timestampNs / 1000000;   // ns convert to ms

    int rc = device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_EMPTY_INPUT_BUFFER,
                           (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ), nullptr, 0 );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "submitInputBuffer VIDC_IOCTL_EMPTY_INPUT_BUFFER failed!" );
        return RIDE_HAL_ERROR_FAIL;
    }
    else
    {
        if ( true == m_DynamicMode )
        {
            std::unique_lock<std::mutex> l( m_Mutex );
            m_InputBufferMap[bufId] = *pSharedBuffer;
        }
        RIDEHAL_DEBUG( "submitInputBuffer VIDC_IOCTL_EMPTY_INPUT_BUFFER frameTs: %" PRIu64,
                       timestampNs );
        return RIDE_HAL_ERROR_NONE;
    }
}

RideHalError_e VideoEncoder::SubmitOutputBuffer( const RideHal_SharedBuffer_t *pSharedBuffer )
{
    int32_t rc = fillBuffer( pSharedBuffer->buffer.id );
    if ( rc != 0 )
    {
        return RIDE_HAL_ERROR_FAIL;
    }
    return RIDE_HAL_ERROR_NONE;
}

RideHalError_e VideoEncoder::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_State )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( m_VidcEncoderData.state == VIDEO_ENCODER_STATE_EXECUTING )
        {
            RIDEHAL_DEBUG( "Stopping vidc!" );
            int32_t rc = device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0,
                                       nullptr, 0 );
            if ( rc != 0 )
            {
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_State = RIDE_HAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e VideoEncoder::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_State )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {

        bool rc = teardown();
        if ( rc != 0 )
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_State = RIDE_HAL_COMPONENT_STATE_INITIAL;
    }

    return ret;
}

bool VideoEncoder::teardown()
{
    if ( m_VidcEncoderData.state ==
         VIDEO_ENCODER_STATE_EXECUTING )   // call from VideoEncoder::Init or Start
    {
        RIDEHAL_DEBUG( "Stopping vidc!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
        if ( waitForState( VIDEO_ENCODER_STATE_IDLE ) != 0 )
        {
            RIDEHAL_ERROR( "waitForState.state_idle.fail!" );
        }
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
                      0 );
        if ( waitForState( VIDEO_ENCODER_STATE_LOADED ) != 0 )
        {
            RIDEHAL_ERROR( "waitForState.STATE_LOADED.fail!" );
        }
    }
    else if ( m_VidcEncoderData.state ==
              VIDEO_ENCODER_STATE_IDLE )   // call after VideoEncoder::Stop
    {
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
                      0 );
    }

    // if ( m_FbdThread.joinable() )
    //{
    // RIDEHAL_DEBUG( "Joining FbdThread" );
    //    m_FbdThread.join();
    //}
    // RIDEHAL_DEBUG( "Cleaning up buffers and releasing locks/signals" );

    if ( freeInputBuffer() != 0 )
    {
        RIDEHAL_ERROR( "Failed to free input buffers!" );
    }
    if ( freeOutputBuffer() != 0 )
    {
        RIDEHAL_ERROR( "Failed to free output buffers!" );
    }
    // m_Buffers.clear();
    if ( m_VidcEncoderData.ioHandle )
    {
        device_close( m_VidcEncoderData.ioHandle );
    }
    MM_CriticalSection_Release( m_VidcEncoderData.lock );
    MM_CriticalSection_Release( m_VidcEncoderData.fbdLock );
    MM_Signal_Release( m_VidcEncoderData.fbdNotification );
    MM_SignalQ_Release( m_VidcEncoderData.fbdNotificationQ );
    MM_Signal_Release( m_VidcEncoderData.specialEventNotification );
    MM_SignalQ_Release( m_VidcEncoderData.specialEventNotificationQ );
    // m_VidcEncoderData.fbdQ.clear();
    return true;
}

int32_t VideoEncoder::fillBuffer( int32_t bufId )
{
    vidc_frame_data_type *pFrameData = m_VidcEncoderData.vidcOutputFrameInfo[bufId];
    vidc_buffer_info_type *pBuffer = m_VidcEncoderData.vidcOutputBufferInfo[bufId];
    RIDEHAL_DEBUG( "fillBuffer vidcOutputFrameInfo=%p, bufId=%" PRId32
                   ", pFrameData=%p, pBuffer->buf_addr=%p",
                   m_VidcEncoderData.vidcOutputFrameInfo, bufId, pFrameData, pBuffer->buf_addr );
    memset( pFrameData, 0, sizeof( vidc_frame_data_type ) );
    pFrameData->buf_type = VIDC_BUFFER_OUTPUT;
    pFrameData->frame_addr = pBuffer->buf_addr;
    pFrameData->alloc_len = pBuffer->buf_size;
    pFrameData->frm_clnt_data = bufId;
    return device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                         (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ), nullptr, 0 );
}

void VideoEncoder::printEncoderConfig()
{
    RIDEHAL_DEBUG( "EncoderConfig: FrameWidth = %" PRIu32, m_Width );
    RIDEHAL_DEBUG( "EncoderConfig: FrameHeight = %" PRIu32, m_Height );
    RIDEHAL_DEBUG( "EncoderConfig: InFPS = %" PRIu32, m_FrameRate );
    RIDEHAL_DEBUG( "EncoderConfig: RateControl = 0x % x ", m_VidcEncoderData.rateControl );
    RIDEHAL_DEBUG( "EncoderConfig: BitRate = %" PRIu32, m_VidcEncoderData.bitrate.target_bitrate );
    if ( m_VidcEncoderData.codec == VIDC_CODEC_HEVC )
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = H265" );
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_VidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_VidcEncoderData.level.level );
    }
    else
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = UNKNOWN 0x%x", m_VidcEncoderData.codec );
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_VidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_VidcEncoderData.level.level );
    }
    RIDEHAL_DEBUG( "EncoderConfig: NumPframes = %" PRIu32, m_VidcEncoderData.iPeriod.p_frames );
    RIDEHAL_DEBUG( "EncoderConfig: NumBframes = %" PRIu32, m_VidcEncoderData.iPeriod.b_frames );
    RIDEHAL_DEBUG( "EncoderConfig: InBufferCount = %" PRIu32 " [0 means minimum]",
                   m_VidcEncoderData.numInputBufferReq );
    RIDEHAL_DEBUG( "EncoderConfig: OutBufferCount = %" PRIu32 " [0 means minimum]",
                   m_VidcEncoderData.numOutputBufferReq );
    RIDEHAL_DEBUG( "EncoderConfig: IdrPeriod = %" PRIu32, m_VidcEncoderData.idrPeriod.idr_period );
}

vidc_color_format_type VideoEncoder::convertFormat( RideHal_ImageFormat_e format )
{
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
            return VIDC_COLOR_FORMAT_RGB888;
            break;
        case RIDE_HAL_IMAGE_FORMAT_BGR888:
            return VIDC_COLOR_FORMAT_BGR888;
            break;
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
            return VIDC_COLOR_FORMAT_UYVY;
            break;
        case RIDE_HAL_IMAGE_FORMAT_NV12:
            return VIDC_COLOR_FORMAT_NV12;
            break;
        case RIDE_HAL_IMAGE_FORMAT_P010:
            return VIDC_COLOR_FORMAT_NV12_P010;
            break;
        default:
            RIDEHAL_DEBUG( "format %d not supported", (int) format );
            return VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
}

int VideoEncoder::DeviceCallback( uint8_t *msg, uint32_t length )
{
    (void) length;
    vidc_drv_msg_info_type *pEvent = (vidc_drv_msg_info_type *) msg;

    vidc_frame_data_type *pFrameData = nullptr;
    bool consumed = false;

    switch ( pEvent->event_type )
    {
        case VIDC_EVT_RESP_FLUSH_INPUT_DONE:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::INPUT_PORT_FLUSH_DONE;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::ERROR;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            if ( m_DynamicMode )
            {
                std::unique_lock<std::mutex> lck( m_Mutex );
                if ( m_InputBufferMap.end() != m_InputBufferMap.find( pFrameData->frm_clnt_data ) )
                {
                    auto inputBuffer = &m_InputBufferMap[pFrameData->frm_clnt_data];
                    m_InputBufferDoneCb( inputBuffer );
                    m_InputBufferMap.erase( pFrameData->frm_clnt_data );
                }
                else
                {
                    RIDEHAL_ERROR( "input bufId = %" PRIu64 " is invalid",
                                   pFrameData->frm_clnt_data );
                }
            }
            m_AvailableInputBufferQueue.push( pFrameData->frm_clnt_data );
            RIDEHAL_DEBUG( "input-done: bufId %u inputCount %u", pFrameData->frm_clnt_data,
                           m_VidcEncoderData.numInputFrames );

            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            memcpy( m_VidcEncoderData.vidcOutputFrameInfo[pFrameData->frm_clnt_data], pFrameData,
                    sizeof( vidc_frame_data_type ) );
            // MM_CriticalSection_Enter( self->m_VidcEncoderData.fbdLock );
            // self->m_VidcEncoderData.fbdQ.push_back(
            //        self->m_VidcEncoderData.vidcOutputFrameInfo[pFrameData->frm_clnt_data] );
            // MM_Signal_Set( self->m_VidcEncoderData.fbdNotification );
            // MM_CriticalSection_Leave( self->m_VidcEncoderData.fbdLock );

            consumed = false;
            if ( pFrameData->data_len > 0 )
            {
                RIDEHAL_DEBUG( "encode-done: bufId %u timestamp %lu size %u frameType %lu "
                               "pFrameData %p frame_addr %p flag 0x %x ",
                               pFrameData->frm_clnt_data, pFrameData->timestamp,
                               pFrameData->data_len, pFrameData->frame_type, pFrameData,
                               pFrameData->frame_addr, pFrameData->flags );

                if ( pFrameData->frame_addr != nullptr )
                {
                    RIDEHAL_DEBUG( "Encoded frame is ready - calling callback!" );

                    consumed = true;
                    RideHal_SharedBuffer_t shareBuffer;
                    shareBuffer.buffer.pData = pFrameData->frame_addr;
                    shareBuffer.buffer.size = pFrameData->data_len;
                    shareBuffer.buffer.dmaHandle = (uint64_t) pFrameData->frame_handle;
                    shareBuffer.buffer.id = (uint64_t) pFrameData->frm_clnt_data;

                    m_OutputBufferDoneCb( &shareBuffer );
                    int32_t rc = fillBuffer( shareBuffer.buffer.id );
                    if ( rc != 0 )
                    {
                        RIDEHAL_ERROR( "fillBuffer index=%" PRIu64 " failed!",
                                       shareBuffer.buffer.id );
                    }
                }
            }

            if ( pFrameData->flags & VIDC_FRAME_FLAG_EOS )
            {
                RIDEHAL_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            if ( false == consumed )
            {
                int rc = device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                                       (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ),
                                       nullptr, 0 );
                if ( rc != 0 )
                {
                    RIDEHAL_ERROR(
                            "VIDC_EVT_RESP_OUTPUT_DONE VIDC_IOCTL_FILL_OUTPUT_BUFFER failed!" );
                }
            }
            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::OUTPUT_PORT_FLUSH_DONE;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::ERROR;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            // smooth streaming case - notifies about resolution change (not
            // handled)
            break;
        case VIDC_EVT_ERR_HWFATAL:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::ERROR;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::ERROR;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        case VIDC_EVT_RESP_START:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_EXECUTING;
            break;
        case VIDC_EVT_RESP_STOP:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_IDLE;
            break;
        case VIDC_EVT_RESP_PAUSE:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_PAUSE;
            break;
        case VIDC_EVT_RESP_RESUME:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_EXECUTING;
            break;
        case VIDC_EVT_RESP_LOAD_RESOURCES:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_IDLE;
            break;
        case VIDC_EVT_RESP_RELEASE_RESOURCES:
            m_VidcEncoderData.state = VIDEO_ENCODER_STATE_LOADED;
            break;
        case VIDC_EVT_RELEASE_BUFFER_REFERENCE:
            /*MM_CriticalSection_Enter( pData->lock );
            pData->currentStatus = ENCODER_STATUS::ERROR;
            MM_Signal_Set( pData->specialEventNotification );
            MM_CriticalSection_Leave( pData->lock );*/
            break;
        default:
            break;
    }
    return 0;
}


int VideoEncoder::DeviceCallback( uint8_t *msg, uint32_t length, void *cdata )
{
    VideoEncoder *self = (VideoEncoder *) cdata;
    return self->DeviceCallback( msg, length );
}

int32_t VideoEncoder::getDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                                      uint32_t nPktSize, uint8_t *pPkt )
{
    uint8_t dev_cmd_buffer[MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    int32_t nMsgSize = ( int32_t )( sizeof( vidc_property_hdr_type ) + nPktSize );
    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    memcpy( pProp->payload, pPkt, nPktSize );
    pProp->prop_hdr.size = nPktSize;
    pProp->prop_hdr.prop_id = propId;

    int32_t status = device_ioctl( ioHandle, VIDC_IOCTL_GET_PROPERTY, dev_cmd_buffer, nMsgSize,
                                   pPkt, nPktSize );
    if ( status != VIDC_ERR_NONE )
    {
        RIDEHAL_DEBUG( "getDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
        return -1;
    }

    return 0;
}

int32_t VideoEncoder::setDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                                      uint32_t nPktSize, uint8_t *pPkt )
{
    uint8_t dev_cmd_buffer[MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    int32_t nMsgSize = ( int32_t )( sizeof( vidc_property_hdr_type ) + nPktSize );
    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    memcpy( pProp->payload, pPkt, nPktSize );
    pProp->prop_hdr.size = nPktSize;
    pProp->prop_hdr.prop_id = propId;

    int32_t status =
            device_ioctl( ioHandle, VIDC_IOCTL_SET_PROPERTY, dev_cmd_buffer, nMsgSize, nullptr, 0 );
    if ( status != VIDC_ERR_NONE )
    {
        RIDEHAL_DEBUG( "setDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
        return -1;
    }
    return 0;
}

int32_t VideoEncoder::waitForState( VideoEncoder_State_t expectedState )
{
    int32_t counter = 0;
    int32_t rc = 0;

    while ( m_VidcEncoderData.state != expectedState )
    {
        MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > 1000 )   // 1 second
        {
            RIDEHAL_ERROR( "waitForState timeout!" );
            rc = -1;
            break;
        }
    }
    return rc;
}

void VideoEncoder::freeFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt )
{
    RIDEHAL_DEBUG( "freeFrameData - frameCnt: %" PRId32, frameCnt );
    if ( *pFrameData != nullptr )
    {
        int i = 0;
        vidc_frame_data_type **pFrame = *pFrameData;
        for ( i = 0; i < frameCnt; i++ )
        {
            free( pFrame[i] );
        }
        free( pFrame );
        *pFrameData = nullptr;
    }
}

int32_t VideoEncoder::allocateFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt )
{
    RIDEHAL_DEBUG( "allocateFrameData - frameCnt: %" PRId32, frameCnt );
    freeFrameData( pFrameData, frameCnt );
    *pFrameData = (vidc_frame_data_type **) malloc( sizeof( vidc_frame_data_type * ) * frameCnt );
    if ( *pFrameData == nullptr )
    {
        RIDEHAL_ERROR( "allocateFrameData failed!" );
        return -1;
    }
    else
    {
        int32_t i = 0;
        vidc_frame_data_type **pFrame = *pFrameData;
        for ( i = 0; i < frameCnt; i++ )
        {
            pFrame[i] = (vidc_frame_data_type *) malloc( sizeof( vidc_frame_data_type ) );
            if ( pFrame[i] == 0 )
            {
                RIDEHAL_ERROR( "allocateFrameData failed for index=%" PRId32, i );
                return -1;
            }
            memset( pFrame[i], 0, sizeof( vidc_frame_data_type ) );
        }
    }
    return 0;
}

int32_t VideoEncoder::allocateBuffer( ioctl_session_t *ioHandle, vidc_buffer_info_type ***pBufInfo,
                                      vidc_buffer_type bufferType, int32_t bufCntMin,
                                      int32_t bufSize )
{
    int32_t rc = 0;
    int32_t i = 0;
    int32_t nMsgSize = sizeof( vidc_buffer_info_type );
    *pBufInfo = (vidc_buffer_info_type **) malloc( sizeof( vidc_buffer_info_type * ) * bufCntMin );
    if ( *pBufInfo == nullptr )
    {
        RIDEHAL_ERROR( "allocateBuffer failed!" );
        return -1;
    }
    else
    {
        vidc_buffer_info_type **pBuf = *pBufInfo;
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };
        RIDEHAL_DEBUG( "*pBufInfo=%p bufCntMin=%" PRId32, *pBufInfo, bufCntMin );
        for ( i = 0; i < bufCntMin; i++ )
        {
            pBuf[i] = (vidc_buffer_info_type *) malloc( sizeof( vidc_buffer_info_type ) );
            if ( pBuf[i] == 0 )
            {
                RIDEHAL_ERROR( "allocateBuffer (malloc) failed for index=%" PRId32, i );
                return -1;
            }
            memset( pBuf[i], 0, sizeof( vidc_buffer_info_type ) );
            ride::hal::memory::Buffer buffer;
            RideHal_SharedBuffer_t sharedBuffer;
            auto ret = buffer.Allocate( bufSize );
            ret = buffer.GetSharedBuffer( &sharedBuffer );
            // m_Buffers.push_back( buffer );
            pBuf[i]->buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
            pBuf[i]->buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
            pBuf[i]->buf_handle = (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
            if ( nullptr == pBuf[i]->buf_addr )
            {
                RIDEHAL_ERROR( "allocateBuffer (allocPmem) failed for index=%" PRId32, i );
            }
            else
            {
                pBuf[i]->buf_type = bufferType;
                pBuf[i]->contiguous = true;
                pBuf[i]->buf_size = bufSize;
                memcpy( &buf_info, pBuf[i], sizeof( vidc_buffer_info_type ) );
                // register it before lookup in future for local allocation
                rc = device_ioctl( ioHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t *) ( &buf_info ),
                                   nMsgSize, nullptr, 0 );
                if ( rc != 0 )
                {
                    RIDEHAL_ERROR( " allocateBuffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32
                                   " rc=0x%x",
                                   i, rc );
                    break;
                }
            }
        }
    }
    return rc;
}


int32_t VideoEncoder::getInputInformation()
{
    int32_t rc = 0;

    m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
    rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                         sizeof( vidc_frame_size_type ), (uint8_t *) &m_VidcEncoderData.frameSize );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "getInputInformation getDrvProperty VIDC_I_FRAME_SIZE failed!" );
    }
    if ( rc == 0 )
    {
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputInformation getDrvProperty VIDC_I_FRAME_RATE failed!" );
        }
    }
    if ( rc == 0 )
    {
        m_VidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) &m_VidcEncoderData.colorFormatConfig );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputInformation getDrvProperty VIDC_I_COLOR_FORMAT failed!" );
        }
    }
    if ( rc == 0 )
    {
        m_VidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.planeDefY.plane_index = 1;
        rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) &m_VidcEncoderData.planeDefY );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputInformation getDrvProperty VIDC_I_PLANE_DEF_Y failed!" );
        }
    }
    if ( rc == 0 )
    {
        m_VidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.planeDefUV.plane_index = 2;
        rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) &m_VidcEncoderData.planeDefUV );
        if ( rc != 0 )
        {
            RIDEHAL_ERROR( "getInputInformation getDrvProperty VIDC_I_PLANE_DEF_UV failed!" );
        }
    }
    return rc;
}

int32_t VideoEncoder::getInputBufferRequirement()
{
    int32_t rc = 0;

    m_VidcEncoderData.inputBufferReq.buf_type = VIDC_BUFFER_INPUT;
    rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "getDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    if ( m_VidcEncoderData.inputBufferReq.actual_count < m_VidcEncoderData.numInputBufferReq )
    {
        m_VidcEncoderData.inputBufferReq.actual_count = m_VidcEncoderData.numInputBufferReq;
    }

    m_VidcEncoderData.numInputBufferReq = m_VidcEncoderData.inputBufferReq.actual_count;
    m_VidcEncoderData.vidcInputBufferSize = m_VidcEncoderData.inputBufferReq.size;
    RIDEHAL_DEBUG( "input-bufs: count=%" PRIu32 ", size=%" PRIu32,
                   m_VidcEncoderData.numInputBufferReq, m_VidcEncoderData.vidcInputBufferSize );

    rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "setDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "getDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    return rc;
}

int32_t VideoEncoder::getOutputBufferRequirement()
{
    int32_t rc = 0;

    m_VidcEncoderData.outputBufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "getDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    if ( m_VidcEncoderData.outputBufferReq.actual_count < m_VidcEncoderData.numOutputBufferReq )
    {
        m_VidcEncoderData.outputBufferReq.actual_count = m_VidcEncoderData.numOutputBufferReq;
    }

    m_VidcEncoderData.numOutputBufferReq = m_VidcEncoderData.outputBufferReq.actual_count;
    m_VidcEncoderData.vidcOutputBufferSize = m_VidcEncoderData.outputBufferReq.size;
    RIDEHAL_DEBUG( "output-bufs: count=%" PRIu32 ", size=%" PRIu32,
                   m_VidcEncoderData.numOutputBufferReq, m_VidcEncoderData.vidcOutputBufferSize );

    rc = setDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "setDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    rc = getDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( rc != 0 )
    {
        RIDEHAL_ERROR( "getDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    return rc;
}

int32_t VideoEncoder::freeOutputBuffer()
{
    uint32_t i = 0;
    int32_t rc = 0;
    vidc_buffer_info_type outbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( m_VidcEncoderData.vidcOutputBufferInfo == nullptr )
    {
        return rc;
    }
    for ( i = 0; i < m_VidcEncoderData.numOutputBufferReq; i++ )
    {
        if ( m_VidcEncoderData.vidcOutputBufferInfo[i] != nullptr )
        {
            memcpy( &outbuf, m_VidcEncoderData.vidcOutputBufferInfo[i],
                    sizeof( vidc_buffer_info_type ) );

            outbuf.buf_addr = m_VidcEncoderData.vidcOutputBufferInfo[i]->buf_addr;
            if ( VIDC_ERR_NONE != device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) &outbuf,
                                                sizeof( vidc_buffer_info_type ), nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR(
                        " freeOutputBuffer  VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!", i );
            }
        }
    }
    freeFrameData( &m_VidcEncoderData.vidcOutputFrameInfo, m_VidcEncoderData.numOutputBufferReq );
    return rc;
}

int32_t VideoEncoder::freeInputBuffer()
{
    int32_t rc = 0;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( m_VidcEncoderData.vidcInputBufferInfo == 0 )
    {
        return rc;
    }
    for ( uint32_t i = 0; i < m_VidcEncoderData.numInputBufferReq; i++ )
    {
        if ( m_VidcEncoderData.vidcInputBufferInfo[i] != nullptr )
        {
            memcpy( &inbuf, m_VidcEncoderData.vidcInputBufferInfo[i],
                    sizeof( vidc_buffer_info_type ) );
            inbuf.buf_addr = m_VidcEncoderData.vidcInputBufferInfo[i]->buf_addr;
            if ( VIDC_ERR_NONE != device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) &inbuf, sizeof( vidc_buffer_info_type ),
                                                nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( " freeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }
    }
    freeFrameData( &m_VidcEncoderData.vidcInputFrameInfo, m_VidcEncoderData.numInputBufferReq );
    return rc;
}

}   // namespace component
}   // namespace hal
}   // namespace ride
