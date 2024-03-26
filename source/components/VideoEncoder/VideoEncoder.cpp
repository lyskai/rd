//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>
#include <vidc_ioctl.h>

#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoEncoder.hpp"

namespace ridehal
{
namespace component
{

RideHalError_e VideoEncoder::Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                                   Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;

    ret = ComponentIF::Init( pName, level );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_Width = pConfig->width;
        m_Height = pConfig->height;
        m_BitRate = pConfig->bitRate;
        m_FrameRate = pConfig->frameRate;
        m_Format = pConfig->format;
        m_InputDynamicMode = pConfig->bInputDynamicMode;
        m_OutputDynamicMode = pConfig->bOutputDynamicMode;

        m_VidcEncoderData.rateControl = (vidc_rate_control_mode_type) pConfig->rateControlMode;
        if ( VIDC_RATE_CONTROL_UNUSED == m_VidcEncoderData.rateControl )
        {
            RIDEHAL_ERROR( "Only support CBR_CFR and VBR_CFR rate control modes!" );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
        m_VidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.colorFormatConfig.color_format = GetVidcFormat( m_Format );
        if ( VIDC_COLOR_FORMAT_UNUSED == m_VidcEncoderData.colorFormatConfig.color_format )
        {
            RIDEHAL_ERROR( "Only support NV12 color format!" );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_VidcEncoderData.codec = DEFAULT_CODEC;               // VIDC_CODEC_HEVC
        m_VidcEncoderData.profile.profile = DEFAULT_PROFILE;   // VIDC_PROFILE_HEVC_MAIN
        m_VidcEncoderData.level.level = DEFAULT_LEVEL;         // VIDC_LEVEL_HEVC_4
        m_VidcEncoderData.numInputBufferReq =
                pConfig->numInputBufferReq ? pConfig->numInputBufferReq : DEFAULT_INPUT_BUFFER_REQ;
        m_VidcEncoderData.numOutputBufferReq = pConfig->numOutputBufferReq
                                                       ? pConfig->numOutputBufferReq
                                                       : DEFAULT_OUTPUT_BUFFER_REQ;
        m_VidcEncoderData.sessionCodec.session = VIDC_SESSION_ENCODE;
        m_VidcEncoderData.sessionCodec.codec = m_VidcEncoderData.codec;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_IoctlCb.handler = VideoEncoder::DeviceCallback;
        m_IoctlCb.data = (void *) this;

        RIDEHAL_DEBUG( "Opening vidc device" );
        m_VidcEncoderData.ioHandle = device_open( (char *) "VideoCore/vidc_drv", &m_IoctlCb );
        if ( nullptr == m_VidcEncoderData.ioHandle )
        {
            RIDEHAL_ERROR( "Failed to open vidc device!" );
            Teardown();
            ret = RIDE_HAL_ERROR_NULL_PTR;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_VidcEncoderData.state = VIDEO_ENCODER_STATE_LOADED;
        RIDEHAL_DEBUG( "Setting VIDC_I_SESSION_CODEC" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_SESSION_CODEC,
                             sizeof( vidc_session_codec_type ),
                             (uint8_t *) &m_VidcEncoderData.sessionCodec );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_SESSION_CODEC failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT" );
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
        m_VidcEncoderData.frameRate.fps_numerator = m_FrameRate;
        m_VidcEncoderData.frameRate.fps_denominator = 1;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT" );
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.frameRate.fps_numerator = m_FrameRate;
        m_VidcEncoderData.frameRate.fps_denominator = 1;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) &m_VidcEncoderData.colorFormatConfig );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_COLOR_FORMAT.VIDC_BUFFER_INPUT failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT" );
        m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.frameSize.width = m_Width;
        m_VidcEncoderData.frameSize.height = m_Height;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) &m_VidcEncoderData.frameSize );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT" );
        m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_OUTPUT;
        m_VidcEncoderData.frameSize.width = m_Width;
        m_VidcEncoderData.frameSize.height = m_Height;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) &m_VidcEncoderData.frameSize );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_INTRA_PERIOD" );
        m_VidcEncoderData.iPeriod.p_frames = pConfig->gop ? pConfig->gop : DEFAULT_NUM_P_BET_2I;
        m_VidcEncoderData.iPeriod.b_frames = DEFAULT_NUM_B_BET_2I;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_INTRA_PERIOD,
                             sizeof( vidc_iperiod_type ), (uint8_t *) &m_VidcEncoderData.iPeriod );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_ENC_INTRA_PERIOD failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_IDR_PERIOD" );
        m_VidcEncoderData.idrPeriod.idr_period = DEFAULT_IDR_PERIOD;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_IDR_PERIOD,
                             sizeof( vidc_idr_period_type ),
                             (uint8_t *) &m_VidcEncoderData.idrPeriod );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_ENC_IDR_PERIOD failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_RATE_CONTROL" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_RATE_CONTROL,
                             sizeof( vidc_rate_control_mode_type ),
                             (uint8_t *) &m_VidcEncoderData.rateControl );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_ENC_RATE_CONTROL failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
        m_VidcEncoderData.bitrate.target_bitrate = m_BitRate;
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                             sizeof( vidc_target_bitrate_type ),
                             (uint8_t *) &m_VidcEncoderData.bitrate );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_TARGET_BITRATE failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_PROFILE" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PROFILE,
                             sizeof( vidc_profile_type ), (uint8_t *) &m_VidcEncoderData.profile );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_PROFILE failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_LEVEL" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_LEVEL, sizeof( vidc_level_type ),
                             (uint8_t *) &m_VidcEncoderData.level );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_LEVEL failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_VPE_SPATIAL_TRANSFORM" );
        vidc_spatial_transform_type vidcSpatialTransform = { VIDC_ROTATE_NONE, VIDC_FLIP_NONE };
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_VPE_SPATIAL_TRANSFORM,
                             sizeof( vidc_spatial_transform_type ),
                             (uint8_t *) &vidcSpatialTransform );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_VPE_SPATIAL_TRANSFORM failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        vidc_session_qp_type vidc_qp = { 20, 20, 20 };
        RIDEHAL_DEBUG( "VIDC_I_ENC_SESSION_QP" );
        rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_ENC_SESSION_QP,
                             sizeof( vidc_session_qp_type ), (uint8_t *) &vidc_qp );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_ENC_SESSION_QP failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = GetInputInformation();
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = GetInputBufferRequirement();
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputBufferRequirement failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = GetOutputBufferRequirement();
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetOutputBufferRequirement failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( true == m_InputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable input dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_INPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                 sizeof( buffer_alloc_mode ), (uint8_t *) &buffer_alloc_mode );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_ALLOC_MODE failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " input buffers",
                           m_VidcEncoderData.numInputBufferReq );
            rc = AllocateBuffer( m_VidcEncoderData.ioHandle, &m_VidcEncoderData.vidcInputBufferInfo,
                                 VIDC_BUFFER_INPUT, m_VidcEncoderData.numInputBufferReq,
                                 m_VidcEncoderData.vidcInputBufferSize );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "Failed to allocate input buffers!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = AllocateFrameData( &m_VidcEncoderData.vidcInputFrameInfo,
                                m_VidcEncoderData.numInputBufferReq );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "AllocateFrameData vidcInputFrameInfo failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( m_OutputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable output dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_OUTPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                 sizeof( buffer_alloc_mode ), (uint8_t *) &buffer_alloc_mode );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_ALLOC_MODE failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " output buffers",
                           m_VidcEncoderData.numOutputBufferReq );

            rc = AllocateBuffer( m_VidcEncoderData.ioHandle,
                                 &m_VidcEncoderData.vidcOutputBufferInfo, VIDC_BUFFER_OUTPUT,
                                 m_VidcEncoderData.numOutputBufferReq,
                                 m_VidcEncoderData.vidcOutputBufferSize );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "Failed to allocate output buffers!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = AllocateFrameData( &m_VidcEncoderData.vidcOutputFrameInfo,
                                m_VidcEncoderData.numOutputBufferReq );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "AllocateFrameData vidcOutputFrameInfo failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Loading vidc resources" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0, nullptr,
                      0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_IDLE ) )
        {
            RIDEHAL_ERROR( "VIDC_IOCTL_LOAD_RESOURCES WaitForState STATE_IDLE fail!" );
            Teardown();
            ret = RIDE_HAL_ERROR_TIMEOUT;
        }
        else
        {
            RIDEHAL_DEBUG( "Successfully completed vidc initialization!" );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        PrintEncoderConfig();
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e VideoEncoder::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Starting vidc" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_EXECUTING ) )
        {
            RIDEHAL_ERROR( "VIDC_IOCTL_START WaitForState STATE_EXECUTING failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_TIMEOUT;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret && false == m_OutputDynamicMode )
    {
        for ( uint32_t counter = 0; counter < m_VidcEncoderData.numOutputBufferReq; counter++ )
        {
            rc = FillBuffer( counter );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "FillBuffer index=%" PRIu32 " failed!", counter );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        for ( uint16_t i = 0; i < m_VidcEncoderData.numInputBufferReq; i++ )
        {
            m_AvailableInputQueue.push( i );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Started vidc" );
        m_state = RIDE_HAL_COMPONENT_STATE_RUNNING;
    }

    return ret;
}

RideHalError_e VideoEncoder::SubmitInputFrame( const VideoEncoder_InputFrame_t *pInputFrame )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;
    const RideHal_SharedBuffer_t *inputBuffer = nullptr;
    uint16_t bufIndex = MAX_UINT16;
    vidc_frame_data_type *pFrameData = nullptr;
    vidc_buffer_info_type *pBuffer = nullptr;
    vidc_buffer_info_type vidc_buffer_info;
    int convertedSize;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == pInputFrame || nullptr == &pInputFrame->inputBuffer ) )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == m_InputDoneCb || nullptr == m_OutputDoneCb || nullptr == m_EventCb ) )
    {
        RIDEHAL_ERROR( "Not submitting since callback is not registered!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( m_AvailableInputQueue.empty() )
        {
            RIDEHAL_ERROR( "No empty input buffer available!" );
            ret = RIDE_HAL_ERROR_NORES;
        }
        else
        {
            bufIndex = m_AvailableInputQueue.front();
            m_AvailableInputQueue.pop();
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        inputBuffer = &pInputFrame->inputBuffer;
        pFrameData = m_VidcEncoderData.vidcInputFrameInfo[bufIndex];

        if ( false == m_InputDynamicMode )
        {
            pBuffer = m_VidcEncoderData.vidcInputBufferInfo[bufIndex];
        }
        else
        {
            pBuffer = &vidc_buffer_info;
        }

        if ( RIDE_HAL_IMAGE_FORMAT_NV12 != inputBuffer->imgProps.format )
        {
            RIDEHAL_ERROR( "unsupported input frame format %d",
                           (int) inputBuffer->imgProps.format );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( inputBuffer->imgProps.stride[1] != m_VidcEncoderData.planeDefY.actual_stride ||
             inputBuffer->imgProps.stride[2] != m_VidcEncoderData.planeDefUV.actual_stride )
        {
            RIDEHAL_ERROR( "input buffer stride is not same with actual strid" );
            // ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        convertedSize = m_VidcEncoderData.vidcInputBufferSize;
        if ( (size_t) convertedSize > inputBuffer->size && true == m_InputDynamicMode )
        {
            RIDEHAL_ERROR( "input buffer size %d is smaller than vidcInputBufferSize %d",
                           (int) inputBuffer->size, (int) m_VidcEncoderData.vidcInputBufferSize );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }


    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( (size_t) convertedSize > inputBuffer->size && false == m_InputDynamicMode )
        {
            convertedSize = inputBuffer->size;
        }

        if ( false == m_InputDynamicMode )
        {
            memcpy( pBuffer->buf_addr, inputBuffer->buffer.pData, convertedSize );
        }
        else
        {
            pBuffer->buf_addr = (uint8_t *) inputBuffer->buffer.pData;
            pBuffer->buf_size = convertedSize;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "input-start: bufIndex %d timestampNs %" PRIu64
                       " frameInfo %p pFrameData %p addr 0x%x handle 0x%x size %d",
                       bufIndex, pInputFrame->timestampNs, m_VidcEncoderData.vidcInputFrameInfo,
                       pFrameData, pBuffer->buf_addr, inputBuffer->buffer.dmaHandle,
                       pBuffer->buf_size );

        memset( pFrameData, 0, sizeof( vidc_frame_data_type ) );
        pFrameData->frm_clnt_data = bufIndex;
        pFrameData->buf_type = VIDC_BUFFER_INPUT;
        pFrameData->frame_addr = pBuffer->buf_addr;
        pFrameData->alloc_len = pBuffer->buf_size;
        pFrameData->frame_handle = (pmem_handle_t) inputBuffer->buffer.dmaHandle;

        pFrameData->data_len = convertedSize;
        pFrameData->timestamp = pInputFrame->timestampNs / 1000;   // ns convert to us
        pFrameData->mark_data = (unsigned long) pInputFrame->appMarkData;
        if ( nullptr != pInputFrame->onTheFlyCmd )
        {
            ret = Configure( pInputFrame->onTheFlyCmd );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        int rc = device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_EMPTY_INPUT_BUFFER,
                               (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER failed!" );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            if ( true == m_InputDynamicMode )
            {
                std::unique_lock<std::mutex> l( m_Mutex );
                m_InputFrameMap[bufIndex] = *pInputFrame;
            }
            RIDEHAL_DEBUG( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER frameTs: %" PRIu64,
                           pInputFrame->timestampNs );
        }
    }
    return ret;
}

RideHalError_e VideoEncoder::SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutputFrame )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting outputBuffer since encoder is shutting down!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == pOutputFrame || nullptr == &pOutputFrame->outputBuffer ) )
    {
        RIDEHAL_ERROR( "Not submitting empty outputBuffer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "SubmitOutputFrame begin" );
        uint16_t bufIndex = pOutputFrame->outputBuffer.buffer.id;
        if ( false == m_OutputDynamicMode )
        {
            rc = FillBuffer( bufIndex );
            if ( 0 != rc )
            {
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            std::unique_lock<std::mutex> l( m_Mutex );
            m_OutputFrameMap[bufIndex] = *pOutputFrame;

            rc = FillBuffer( &pOutputFrame->outputBuffer );
            if ( 0 != rc )
            {
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }
    RIDEHAL_DEBUG( "SubmitOutputFrame done" );

    return ret;
}

RideHalError_e VideoEncoder::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( VIDEO_ENCODER_STATE_EXECUTING == m_VidcEncoderData.state )
        {
            RIDEHAL_DEBUG( "Stopping vidc!" );
            int32_t rc = device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0,
                                       nullptr, 0 );
            if ( 0 != rc )
            {
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Stopped vidc" );
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e VideoEncoder::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Deiniting vidc" );
        bool rc = Teardown();
        if ( false == rc )
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Deinited vidc" );
        m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;
    }

    return ret;
}

RideHalError_e VideoEncoder::Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state && RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_WARN( "Not Configure since encoder is not ready or running!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Configuring vidc" );
        switch ( pCmd->propID )
        {
            case VIDEO_ENCODER_PROP_BITRATE:
                RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
                m_BitRate = pCmd->pValue;
                m_VidcEncoderData.bitrate.target_bitrate = m_BitRate;
                rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                                     sizeof( vidc_target_bitrate_type ),
                                     (uint8_t *) &m_VidcEncoderData.bitrate );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_TARGET_BITRATE failed!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            case VIDEO_ENCODER_PROP_FRAME_RATE:
                m_FrameRate = pCmd->pValue;
                RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT %d",
                               (int) m_FrameRate );
                m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
                m_VidcEncoderData.frameRate.fps_numerator = m_FrameRate;
                m_VidcEncoderData.frameRate.fps_denominator = 1;
                rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                                     sizeof( vidc_frame_rate_type ),
                                     (uint8_t *) &m_VidcEncoderData.frameRate );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT failed!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            default:
                RIDEHAL_ERROR( "propID %d not supported", (int) pCmd->propID );
                ret = RIDE_HAL_ERROR_UNSUPPORTED;
                break;
        }
    }
    return ret;
}

RideHalError_e VideoEncoder::RegisterCallback( VideoEncoder_InFrameCallback_t inputDoneCb,
                                               VideoEncoder_OutFrameCallback_t outputDoneCb,
                                               VideoEncoder_EventCallback_t eventCb,
                                               void *pAppPriv )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    m_InputDoneCb = inputDoneCb;
    m_OutputDoneCb = outputDoneCb;
    m_EventCb = eventCb;
    m_pAppPriv = pAppPriv;
    return ret;
}

bool VideoEncoder::Teardown()
{
    if ( VIDEO_ENCODER_STATE_EXECUTING ==
         m_VidcEncoderData.state )   // call from VideoEncoder::Init or Start
    {
        RIDEHAL_DEBUG( "Stopping vidc!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_IDLE ) )
        {
            RIDEHAL_ERROR( "WaitForState.state_idle.fail!" );
        }
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
                      0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_LOADED ) )
        {
            RIDEHAL_ERROR( "WaitForState.STATE_LOADED.fail!" );
        }
    }
    else if ( VIDEO_ENCODER_STATE_IDLE ==
              m_VidcEncoderData.state )   // call after VideoEncoder::Stop
    {
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
                      0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_LOADED ) )
        {
            RIDEHAL_ERROR( "WaitForState.STATE_LOADED.fail!" );
        }
    }

    if ( 0 != FreeInputBuffer() )
    {
        RIDEHAL_ERROR( "Failed to free input buffers!" );
    }
    if ( 0 != FreeOutputBuffer() )
    {
        RIDEHAL_ERROR( "Failed to free output buffers!" );
    }

    m_InputList.clear();
    m_OutputList.clear();
    m_InputFrameMap.clear();
    m_OutputFrameMap.clear();

    if ( m_VidcEncoderData.ioHandle )
    {
        device_close( m_VidcEncoderData.ioHandle );
    }

    return true;
}

int32_t VideoEncoder::FillBuffer( uint16_t bufIndex )
{
    vidc_frame_data_type *pFrameData = m_VidcEncoderData.vidcOutputFrameInfo[bufIndex];
    vidc_buffer_info_type *pBuffer = m_VidcEncoderData.vidcOutputBufferInfo[bufIndex];
    RIDEHAL_DEBUG(
            "FillBuffer vidcOutputFrameInfo=%p, bufIndex=%" PRId32
            ", pFrameData=%p, pBuffer->buf_addr=0x%x pBuffer->buf_handle=0x%x pBuffer->buf_size %d",
            m_VidcEncoderData.vidcOutputFrameInfo, bufIndex, pFrameData, pBuffer->buf_addr,
            pBuffer->buf_handle, pBuffer->buf_size );
    memset( pFrameData, 0, sizeof( vidc_frame_data_type ) );
    pFrameData->buf_type = VIDC_BUFFER_OUTPUT;
    pFrameData->frame_addr = pBuffer->buf_addr;
    pFrameData->alloc_len = pBuffer->buf_size;
    pFrameData->frame_handle = pBuffer->buf_handle;
    pFrameData->frm_clnt_data = bufIndex;
    return device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                         (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ), nullptr, 0 );
}

int32_t VideoEncoder::FillBuffer( const RideHal_SharedBuffer_t *pOutputBuffer )
{
    int32_t bufIndex = pOutputBuffer->buffer.id;
    vidc_frame_data_type *pFrameData = m_VidcEncoderData.vidcOutputFrameInfo[bufIndex];

    memset( pFrameData, 0, sizeof( vidc_frame_data_type ) );
    pFrameData->buf_type = VIDC_BUFFER_OUTPUT;
    pFrameData->frame_addr = (uint8_t *) pOutputBuffer->data();
    pFrameData->alloc_len = pOutputBuffer->size;
    pFrameData->frame_handle = (pmem_handle_t) pOutputBuffer->buffer.dmaHandle;
    pFrameData->frm_clnt_data = bufIndex;
    RIDEHAL_DEBUG( "FillBuffer vidcOutputFrameInfo=%p, bufIndex=%" PRId32
                   ", pFrameData=%p, pFrameData->frame_addr =0x%x pFrameData->frame_handle=0x%x "
                   "pOutputBuffer->size %d",
                   m_VidcEncoderData.vidcOutputFrameInfo, bufIndex, pFrameData,
                   pFrameData->frame_addr, pFrameData->frame_handle, pOutputBuffer->size );
    return device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                         (uint8_t *) pFrameData, sizeof( vidc_frame_data_type ), nullptr, 0 );
}

void VideoEncoder::PrintEncoderConfig()
{
    RIDEHAL_DEBUG( "EncoderConfig: FrameWidth = %" PRIu32, m_Width );
    RIDEHAL_DEBUG( "EncoderConfig: FrameHeight = %" PRIu32, m_Height );
    RIDEHAL_DEBUG( "EncoderConfig: InFPS = %" PRIu32, m_FrameRate );
    RIDEHAL_DEBUG( "EncoderConfig: RateControl = 0x % x ", m_VidcEncoderData.rateControl );
    RIDEHAL_DEBUG( "EncoderConfig: BitRate = %" PRIu32, m_VidcEncoderData.bitrate.target_bitrate );
    if ( VIDC_CODEC_HEVC == m_VidcEncoderData.codec )
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

vidc_color_format_type VideoEncoder::GetVidcFormat( RideHal_ImageFormat_e format )
{
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_NV12:
            return VIDC_COLOR_FORMAT_NV12;
            break;
        default:
            RIDEHAL_ERROR( "format %d not supported", (int) format );
            return VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
}

int VideoEncoder::DeviceCallback( uint8_t *msg, uint32_t length )
{
    (void) length;
    vidc_drv_msg_info_type *pEvent = (vidc_drv_msg_info_type *) msg;
    vidc_frame_data_type *pFrameData = nullptr;

    switch ( pEvent->event_type )
    {
        case VIDC_EVT_RESP_FLUSH_INPUT_DONE:
            m_EventCb( VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            if ( true == m_InputDynamicMode )
            {
                std::unique_lock<std::mutex> lck( m_Mutex );
                if ( m_InputFrameMap.end() != m_InputFrameMap.find( pFrameData->frm_clnt_data ) )
                {
                    auto inputFrame = &m_InputFrameMap[pFrameData->frm_clnt_data];
                    m_InputDoneCb( inputFrame, m_pAppPriv );
                    m_InputFrameMap.erase( pFrameData->frm_clnt_data );
                }
                else
                {
                    RIDEHAL_ERROR( "input bufIndex = %" PRIu64 " is invalid",
                                   pFrameData->frm_clnt_data );
                }
            }
            else
            {
                auto inputFrame = &m_InputList[pFrameData->frm_clnt_data];
                m_InputDoneCb( inputFrame, m_pAppPriv );
            }
            m_AvailableInputQueue.push( pFrameData->frm_clnt_data );
            RIDEHAL_DEBUG( "input-done: bufIndex %u ", pFrameData->frm_clnt_data );
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            memcpy( m_VidcEncoderData.vidcOutputFrameInfo[pFrameData->frm_clnt_data], pFrameData,
                    sizeof( vidc_frame_data_type ) );

            RIDEHAL_DEBUG( "encode-done: bufIndex %u timestamp %lu size %u frameType %lu "
                           "pFrameData %p frame_addr %p flag 0x %x ",
                           pFrameData->frm_clnt_data, pFrameData->timestamp, pFrameData->data_len,
                           pFrameData->frame_type, pFrameData, pFrameData->frame_addr,
                           pFrameData->flags );
            if ( nullptr != pFrameData->frame_addr )
            {
                if ( false == m_OutputDynamicMode )
                {
                    auto outputFrame = &m_OutputList[pFrameData->frm_clnt_data];
                    RIDEHAL_DEBUG( "Encoded frame is ready - calling callback!" );
                    outputFrame->outputBuffer.buffer.pData = pFrameData->frame_addr;
                    outputFrame->outputBuffer.buffer.size = pFrameData->data_len;
                    outputFrame->outputBuffer.buffer.dmaHandle =
                            (uint64_t) pFrameData->frame_handle;
                    outputFrame->outputBuffer.buffer.id = (uint64_t) pFrameData->frm_clnt_data;
                    outputFrame->appMarkData = (void *) pFrameData->mark_data;
                    outputFrame->frameType = (VideoEncoder_FrameType_e) pFrameData->frame_type;
                    outputFrame->timestampNs = pFrameData->timestamp * 1000;   // convert us to ns
                    outputFrame->frameFlag = pFrameData->flags;
                    m_OutputDoneCb( outputFrame, m_pAppPriv );
                }
                else
                {
                    std::unique_lock<std::mutex> lck( m_Mutex );
                    if ( m_OutputFrameMap.end() !=
                         m_OutputFrameMap.find( pFrameData->frm_clnt_data ) )
                    {
                        auto outputFrame = &m_OutputFrameMap[pFrameData->frm_clnt_data];
                        if ( outputFrame->outputBuffer.buffer.pData == pFrameData->frame_addr )
                        {
                            RIDEHAL_DEBUG( "Encoded frame is ready - calling callback!" );
                            outputFrame->appMarkData = (void *) pFrameData->mark_data;
                            outputFrame->frameType =
                                    (VideoEncoder_FrameType_e) pFrameData->frame_type;
                            outputFrame->timestampNs =
                                    pFrameData->timestamp * 1000;   // convert us to ns
                            outputFrame->frameFlag = pFrameData->flags;
                            m_OutputDoneCb( outputFrame, m_pAppPriv );
                            m_OutputFrameMap.erase( pFrameData->frm_clnt_data );
                        }
                        else
                        {
                            RIDEHAL_ERROR( "output bufIndex = %" PRIu64 " is wrong",
                                           pFrameData->frm_clnt_data );
                        }
                    }
                    else
                    {
                        RIDEHAL_ERROR( "output bufIndex = %" PRIu64 " is invalid",
                                       pFrameData->frm_clnt_data );
                    }
                }
            }

            if ( pFrameData->flags & VIDC_FRAME_FLAG_EOS )
            {
                RIDEHAL_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            m_EventCb( VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_HWFATAL:
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
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
            m_EventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
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

int32_t VideoEncoder::GetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
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
    if ( VIDC_ERR_NONE != status )
    {
        RIDEHAL_DEBUG( "GetDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
        return -1;
    }

    return 0;
}

int32_t VideoEncoder::SetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
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
    if ( VIDC_ERR_NONE != status )
    {
        RIDEHAL_DEBUG( "SetDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
        return -1;
    }
    return 0;
}

int32_t VideoEncoder::WaitForState( VideoEncoder_State_e expectedState )
{
    int32_t counter = 0;
    int32_t rc = 0;

    while ( m_VidcEncoderData.state != expectedState )
    {
        MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > WAIT_TIMEOUT_1_SEC )
        {
            RIDEHAL_ERROR( "WaitForState timeout!" );
            rc = -1;
            break;
        }
    }
    return rc;
}

void VideoEncoder::FreeFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt )
{
    RIDEHAL_DEBUG( "FreeFrameData - frameCnt: %" PRId32, frameCnt );
    if ( nullptr != *pFrameData )
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

int32_t VideoEncoder::AllocateFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt )
{
    RIDEHAL_DEBUG( "AllocateFrameData - frameCnt: %" PRId32, frameCnt );
    FreeFrameData( pFrameData, frameCnt );
    *pFrameData = (vidc_frame_data_type **) malloc( sizeof( vidc_frame_data_type * ) * frameCnt );
    if ( nullptr == *pFrameData )
    {
        RIDEHAL_ERROR( "AllocateFrameData failed!" );
        return -1;
    }
    else
    {
        int32_t i = 0;
        vidc_frame_data_type **pFrame = *pFrameData;
        for ( i = 0; i < frameCnt; i++ )
        {
            pFrame[i] = (vidc_frame_data_type *) malloc( sizeof( vidc_frame_data_type ) );
            if ( 0 == pFrame[i] )
            {
                RIDEHAL_ERROR( "AllocateFrameData failed for index=%" PRId32, i );
                return -1;
            }
            memset( pFrame[i], 0, sizeof( vidc_frame_data_type ) );
        }
    }
    return 0;
}

int32_t VideoEncoder::AllocateBuffer( ioctl_session_t *ioHandle, vidc_buffer_info_type ***pBufInfo,
                                      vidc_buffer_type bufferType, int32_t bufCntMin,
                                      int32_t bufSize )
{
    int32_t rc = 0;
    int32_t i = 0;
    int32_t nMsgSize = sizeof( vidc_buffer_info_type );
    *pBufInfo = (vidc_buffer_info_type **) malloc( sizeof( vidc_buffer_info_type * ) * bufCntMin );
    if ( nullptr == *pBufInfo )
    {
        RIDEHAL_ERROR( "AllocateBuffer failed!" );
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
            if ( 0 == pBuf[i] )
            {
                RIDEHAL_ERROR( "AllocateBuffer (malloc) failed for index=%" PRId32, i );
                return -1;
            }
            memset( pBuf[i], 0, sizeof( vidc_buffer_info_type ) );

            RideHal_SharedBuffer_t sharedBuffer;
            auto ret = sharedBuffer.Allocate( bufSize );

            if ( VIDC_BUFFER_INPUT == bufferType )
            {
                VideoEncoder_InputFrame_t inputFrame;
                inputFrame.inputBuffer = sharedBuffer;
                m_InputList.push_back( inputFrame );
            }
            else if ( VIDC_BUFFER_OUTPUT == bufferType )
            {
                VideoEncoder_OutputFrame_t outputFrame;
                outputFrame.outputBuffer = sharedBuffer;
                m_OutputList.push_back( outputFrame );
            }

            pBuf[i]->buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
            pBuf[i]->buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
            pBuf[i]->buf_handle = (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
            if ( nullptr == pBuf[i]->buf_addr )
            {
                RIDEHAL_ERROR( "AllocateBuffer (allocPmem) failed for index=%" PRId32, i );
            }
            else
            {
                pBuf[i]->buf_type = bufferType;
                pBuf[i]->contiguous = true;
                pBuf[i]->buf_size = bufSize;
                memcpy( &buf_info, pBuf[i], sizeof( vidc_buffer_info_type ) );
                // register it before lookup in future for local allocation
                RIDEHAL_DEBUG(
                        "AllocateBuffer [%d]: buf_addr = 0x%x, buf_handle = 0x%x, buf_size = %d", i,
                        pBuf[i]->buf_addr, pBuf[i]->buf_handle, bufSize );
                rc = device_ioctl( ioHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t *) ( &buf_info ),
                                   nMsgSize, nullptr, 0 );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( " AllocateBuffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32
                                   " rc=0x%x",
                                   i, rc );
                    break;
                }
            }
        }
    }
    return rc;
}


int32_t VideoEncoder::GetInputInformation()
{
    int32_t rc = 0;

    m_VidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
    rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                         sizeof( vidc_frame_size_type ), (uint8_t *) &m_VidcEncoderData.frameSize );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_FRAME_SIZE failed!" );
    }
    if ( 0 == rc )
    {
        m_VidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) &m_VidcEncoderData.frameRate );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_FRAME_RATE failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_VidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) &m_VidcEncoderData.colorFormatConfig );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_COLOR_FORMAT failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_VidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.planeDefY.plane_index = 1;
        rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) &m_VidcEncoderData.planeDefY );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_PLANE_DEF_Y failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_VidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_VidcEncoderData.planeDefUV.plane_index = 2;
        rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) &m_VidcEncoderData.planeDefUV );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_PLANE_DEF_UV failed!" );
        }
    }
    return rc;
}

int32_t VideoEncoder::GetInputBufferRequirement()
{
    int32_t rc = 0;

    m_VidcEncoderData.inputBufferReq.buf_type = VIDC_BUFFER_INPUT;
    rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
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

    rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "SetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.inputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    return rc;
}

int32_t VideoEncoder::GetOutputBufferRequirement()
{
    int32_t rc = 0;

    m_VidcEncoderData.outputBufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
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

    rc = SetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        return rc;
    }

    rc = GetDrvProperty( m_VidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) &m_VidcEncoderData.outputBufferReq );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    return rc;
}

int32_t VideoEncoder::FreeOutputBuffer()
{
    uint32_t i = 0;
    int32_t rc = 0;
    vidc_buffer_info_type outbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( nullptr == m_VidcEncoderData.vidcOutputBufferInfo )
    {
        return rc;
    }
    for ( i = 0; i < m_VidcEncoderData.numOutputBufferReq; i++ )
    {
        if ( nullptr != m_VidcEncoderData.vidcOutputBufferInfo[i] )
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
                        " FreeOutputBuffer  VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!", i );
            }
        }
    }
    FreeFrameData( &m_VidcEncoderData.vidcOutputFrameInfo, m_VidcEncoderData.numOutputBufferReq );
    return rc;
}

int32_t VideoEncoder::FreeInputBuffer()
{
    int32_t rc = 0;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( nullptr == m_VidcEncoderData.vidcInputBufferInfo )
    {
        return rc;
    }
    for ( uint32_t i = 0; i < m_VidcEncoderData.numInputBufferReq; i++ )
    {
        if ( nullptr != m_VidcEncoderData.vidcInputBufferInfo[i] )
        {
            memcpy( &inbuf, m_VidcEncoderData.vidcInputBufferInfo[i],
                    sizeof( vidc_buffer_info_type ) );
            inbuf.buf_addr = m_VidcEncoderData.vidcInputBufferInfo[i]->buf_addr;
            if ( VIDC_ERR_NONE != device_ioctl( m_VidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) &inbuf, sizeof( vidc_buffer_info_type ),
                                                nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( " FreeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }
    }
    FreeFrameData( &m_VidcEncoderData.vidcInputFrameInfo, m_VidcEncoderData.numInputBufferReq );
    return rc;
}

}   // namespace component
}   // namespace ridehal
