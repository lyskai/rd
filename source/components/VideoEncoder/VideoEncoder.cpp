//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>

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
        m_width = pConfig->width;
        m_height = pConfig->height;
        m_bitRate = pConfig->bitRate;
        m_frameRate = pConfig->frameRate;
        m_inFormat = pConfig->inFormat;
        m_outFormat = pConfig->outFormat;
        m_bInputDynamicMode = pConfig->bInputDynamicMode;
        m_bOutputDynamicMode = pConfig->bOutputDynamicMode;

        m_vidcEncoderData.rateControl = (vidc_rate_control_mode_type) pConfig->rateControlMode;
        if ( VIDC_RATE_CONTROL_UNUSED == m_vidcEncoderData.rateControl )
        {
            RIDEHAL_ERROR( "rate control mode: %d not supported!", (int) pConfig->rateControlMode );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }

        m_vidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.colorFormatConfig.color_format = GetVidcFormat( m_inFormat );
        if ( VIDC_COLOR_FORMAT_UNUSED == m_vidcEncoderData.colorFormatConfig.color_format )
        {
            RIDEHAL_ERROR( "input format: %d not supported!", (int) m_inFormat );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat )
        {
            m_vidcEncoderData.codec = VIDC_CODEC_HEVC;
            m_vidcEncoderData.profile.profile = VIDC_PROFILE_HEVC_MAIN;
            m_vidcEncoderData.level.level = VIDC_LEVEL_HEVC_4;
        }
        else if ( RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H264 == m_outFormat )
        {
            m_vidcEncoderData.codec = VIDC_CODEC_H264;
            m_vidcEncoderData.profile.profile = VIDC_PROFILE_H264_MAIN;
            m_vidcEncoderData.level.level = VIDC_LEVEL_H264_1p1;
        }
        else
        {
            RIDEHAL_ERROR( "output format: %d not supported!", (int) m_outFormat );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }

        m_vidcEncoderData.sessionCodec.session = VIDC_SESSION_ENCODE;
        m_vidcEncoderData.sessionCodec.codec = m_vidcEncoderData.codec;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( pConfig->numInputBufferReq > MAX_BUFFER_REQ ||
             pConfig->numInputBufferReq < MIN_BUFFER_REQ )
        {
            RIDEHAL_ERROR( "numInputBufferReq: %d too small or too large! MIN_BUFFER_REQ %d, "
                           "MAX_BUFFER_REQ %d ",
                           (int) pConfig->numInputBufferReq, MIN_BUFFER_REQ, MAX_BUFFER_REQ );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
        else
        {
            m_numInputBufferReq = pConfig->numInputBufferReq;
        }

        if ( pConfig->numOutputBufferReq > MAX_BUFFER_REQ ||
             pConfig->numOutputBufferReq < MIN_BUFFER_REQ )
        {
            RIDEHAL_ERROR( "numOutputBufferReq: %d too small or too large! MIN_BUFFER_REQ %d, "
                           "MAX_BUFFER_REQ %d ",
                           (int) pConfig->numOutputBufferReq, MIN_BUFFER_REQ, MAX_BUFFER_REQ );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
        else
        {
            m_numOutputBufferReq = pConfig->numOutputBufferReq;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_ioctlCb.handler = VideoEncoder::DeviceCallback;
        m_ioctlCb.data = (void *) this;

        RIDEHAL_DEBUG( "Opening vidc device" );
        m_vidcEncoderData.ioHandle = device_open( (char *) "VideoCore/vidc_drv", &m_ioctlCb );
        if ( nullptr == m_vidcEncoderData.ioHandle )
        {
            RIDEHAL_ERROR( "Failed to open vidc device!" );
            Teardown();
            ret = RIDE_HAL_ERROR_NULL_PTR;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_vidcEncoderData.state = VIDEO_ENCODER_STATE_LOADED;
        RIDEHAL_DEBUG( "Setting VIDC_I_SESSION_CODEC" );
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_SESSION_CODEC,
                             sizeof( vidc_session_codec_type ),
                             (uint8_t *) ( &m_vidcEncoderData.sessionCodec ) );
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
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
        m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
        m_vidcEncoderData.frameRate.fps_denominator = 1;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
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
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
        m_vidcEncoderData.frameRate.fps_denominator = 1;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) ( &m_vidcEncoderData.colorFormatConfig ) );
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
        m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.frameSize.width = m_width;
        m_vidcEncoderData.frameSize.height = m_height;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) ( &m_vidcEncoderData.frameSize ) );
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
        m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_OUTPUT;
        m_vidcEncoderData.frameSize.width = m_width;
        m_vidcEncoderData.frameSize.height = m_height;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                             sizeof( vidc_frame_size_type ),
                             (uint8_t *) ( &m_vidcEncoderData.frameSize ) );
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
        m_vidcEncoderData.iPeriod.p_frames = pConfig->gop ? pConfig->gop : DEFAULT_NUM_P_BET_2I;
        m_vidcEncoderData.iPeriod.b_frames = DEFAULT_NUM_B_BET_2I;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_ENC_INTRA_PERIOD,
                             sizeof( vidc_iperiod_type ),
                             (uint8_t *) ( &m_vidcEncoderData.iPeriod ) );
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
        m_vidcEncoderData.idrPeriod.idr_period = DEFAULT_IDR_PERIOD;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_ENC_IDR_PERIOD,
                             sizeof( vidc_idr_period_type ),
                             (uint8_t *) ( &m_vidcEncoderData.idrPeriod ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_ENC_RATE_CONTROL,
                             sizeof( vidc_rate_control_mode_type ),
                             (uint8_t *) ( &m_vidcEncoderData.rateControl ) );
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
        m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                             sizeof( vidc_target_bitrate_type ),
                             (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_PROFILE,
                             sizeof( vidc_profile_type ),
                             (uint8_t *) ( &m_vidcEncoderData.profile ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_LEVEL, sizeof( vidc_level_type ),
                             (uint8_t *) ( &m_vidcEncoderData.level ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_VPE_SPATIAL_TRANSFORM,
                             sizeof( vidc_spatial_transform_type ),
                             (uint8_t *) ( &vidcSpatialTransform ) );
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
        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_ENC_SESSION_QP,
                             sizeof( vidc_session_qp_type ), (uint8_t *) ( &vidc_qp ) );
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
        m_inputList = (VideoEncoder_InputFrame_t *) malloc( m_numInputBufferReq *
                                                            sizeof( VideoEncoder_InputFrame_t ) );
        if ( nullptr == m_inputList )
        {
            RIDEHAL_ERROR( "m_inputList malloc failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( true == m_bInputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable input dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_INPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                 sizeof( buffer_alloc_mode ), (uint8_t *) ( &buffer_alloc_mode ) );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_ALLOC_MODE failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " input buffers", m_numInputBufferReq );
            rc = AllocateBuffer( m_vidcEncoderData.ioHandle, &m_vidcEncoderData.vidcInputBufferInfo,
                                 VIDC_BUFFER_INPUT, m_numInputBufferReq,
                                 m_vidcEncoderData.vidcInputBufferSize );
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
        m_outputList = (VideoEncoder_OutputFrame_t *) malloc(
                m_numOutputBufferReq * sizeof( VideoEncoder_OutputFrame_t ) );
        if ( nullptr == m_outputList )
        {
            RIDEHAL_ERROR( "m_outputList malloc failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( true == m_bOutputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable output dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_OUTPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                 sizeof( buffer_alloc_mode ), (uint8_t *) ( &buffer_alloc_mode ) );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_ALLOC_MODE failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " output buffers", m_numOutputBufferReq );

            rc = AllocateBuffer( m_vidcEncoderData.ioHandle,
                                 &m_vidcEncoderData.vidcOutputBufferInfo, VIDC_BUFFER_OUTPUT,
                                 m_numOutputBufferReq, m_vidcEncoderData.vidcOutputBufferSize );
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
        RIDEHAL_DEBUG( "Loading vidc resources" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0, nullptr,
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
        RIDEHAL_ERROR( "Init Not ready!" );
        Teardown();
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == m_inputDoneCb || nullptr == m_outputDoneCb || nullptr == m_eventCb ) )
    {
        RIDEHAL_ERROR( "Not start since callback is not registered!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Starting vidc" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_EXECUTING ) )
        {
            RIDEHAL_ERROR( "VIDC_IOCTL_START WaitForState STATE_EXECUTING failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_TIMEOUT;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret && false == m_bOutputDynamicMode )
    {
        for ( uint32_t counter = 0; counter < m_numOutputBufferReq; counter++ )
        {
            ret = SubmitOutputFrame( &m_outputList[counter] );
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        for ( uint16_t i = 0; i < m_numInputBufferReq; i++ )
        {
            m_availableInputQueue.push( i );
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
    uint16_t frameIndex = MAX_UINT16;
    vidc_frame_data_type frameData;
    vidc_buffer_info_type bufferInfo;
    int convertedSize;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
        Teardown();
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == pInputFrame || nullptr == &pInputFrame->sharedBuffer ) )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        inputBuffer = &pInputFrame->sharedBuffer;
        bufferInfo.buf_addr = (uint8_t *) inputBuffer->buffer.pData;
        if ( m_availableInputQueue.empty() )
        {
            RIDEHAL_ERROR( "No empty input buffer available!" );
            Teardown();
            ret = RIDE_HAL_ERROR_NORES;
        }
        else
        {
            frameIndex = m_availableInputQueue.front();
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( false == m_bInputDynamicMode )
        {
            if ( frameIndex != pInputFrame->frameIndex )
            {
                RIDEHAL_ERROR( "available frameIndex %d not match with pInputFrame->frameIndex %d!",
                               (int) frameIndex, (int) pInputFrame->frameIndex );
                Teardown();
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                m_availableInputQueue.pop();
            }
        }
        else
        {
            m_inputList[frameIndex] = *pInputFrame;
            m_inputList[frameIndex].frameIndex = frameIndex;
            m_availableInputQueue.pop();
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( VIDC_COLOR_FORMAT_UNUSED == GetVidcFormat( inputBuffer->imgProps.format ) )
        {
            RIDEHAL_ERROR( "unsupported input frame format %d",
                           (int) inputBuffer->imgProps.format );
            Teardown();
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( inputBuffer->imgProps.stride[0] != m_vidcEncoderData.planeDefY.actual_stride ||
             inputBuffer->imgProps.stride[1] != m_vidcEncoderData.planeDefUV.actual_stride )
        {
            RIDEHAL_ERROR( "input buffer stride [%" PRIu32 "][%" PRIu32
                           "] is not same with actual strid [%" PRIu32 "][%" PRIu32 "] ",
                           inputBuffer->imgProps.stride[0], inputBuffer->imgProps.stride[1],
                           m_vidcEncoderData.planeDefY.actual_stride,
                           m_vidcEncoderData.planeDefUV.actual_stride );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        convertedSize = m_vidcEncoderData.vidcInputBufferSize;
        if ( (size_t) convertedSize > inputBuffer->size && true == m_bInputDynamicMode )
        {
            RIDEHAL_ERROR( "inputBuffer size %d is smaller than vidcInputBufferSize %d",
                           (int) inputBuffer->size, (int) m_vidcEncoderData.vidcInputBufferSize );
            Teardown();
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }


    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( (size_t) convertedSize > inputBuffer->size &&
             false == m_bInputDynamicMode )   // this should not happen
        {
            convertedSize = inputBuffer->size;
        }

        if ( true == m_bInputDynamicMode )
        {
            bufferInfo.buf_size = convertedSize;
        }
        else
        {
            bufferInfo.buf_size = inputBuffer->size;
        }

        RIDEHAL_DEBUG( "input-start: frameIndex %d timestampNs %" PRIu64
                       " addr 0x%x handle 0x%x size %d",
                       frameIndex, pInputFrame->timestampNs, bufferInfo.buf_addr,
                       inputBuffer->buffer.dmaHandle, bufferInfo.buf_size );

        memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.frm_clnt_data = frameIndex;
        frameData.buf_type = VIDC_BUFFER_INPUT;
        frameData.frame_addr = bufferInfo.buf_addr;
        frameData.alloc_len = bufferInfo.buf_size;
        frameData.frame_handle = (pmem_handle_t) inputBuffer->buffer.dmaHandle;

        frameData.data_len = convertedSize;
        frameData.timestamp = pInputFrame->timestampNs / 1000;   // ns convert to us
        frameData.mark_data = (unsigned long) pInputFrame->appMarkData;
        if ( nullptr != pInputFrame->onTheFlyCmd )
        {
            int i = 0;
            for ( i = 0; i < pInputFrame->numCmd; i++ )
            {
                ret = Configure( &pInputFrame->onTheFlyCmd[i] );
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        int rc = device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_EMPTY_INPUT_BUFFER,
                               (uint8_t *) ( &frameData ), sizeof( vidc_frame_data_type ), nullptr,
                               0 );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER failed!" );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
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
    uint16_t frameIndex = MAX_UINT16;
    vidc_frame_data_type frameData;
    const RideHal_SharedBuffer_t *outputBuffer = nullptr;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state && RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_WARN( "Not submitting outputBuffer since encoder is not ready or encoder is "
                      "shutting down!" );
        Teardown();
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == pOutputFrame || nullptr == &pOutputFrame->sharedBuffer ) )
    {
        RIDEHAL_ERROR( "Not submitting empty outputBuffer!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "SubmitOutputFrame begin" );
        outputBuffer = &pOutputFrame->sharedBuffer;
        frameIndex = pOutputFrame->frameIndex;
        if ( frameIndex < 0 || frameIndex >= m_numOutputBufferReq )
        {
            RIDEHAL_ERROR( "output frameIndex = %" PRIu64 " is invalid", frameIndex );
            Teardown();
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( true == m_bOutputDynamicMode )
        {
            m_outputList[frameIndex] = *pOutputFrame;
        }

        memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.buf_type = VIDC_BUFFER_OUTPUT;
        frameData.frame_addr = (uint8_t *) outputBuffer->buffer.pData;
        frameData.alloc_len = outputBuffer->size;
        frameData.frame_handle = (pmem_handle_t) outputBuffer->buffer.dmaHandle;
        frameData.frm_clnt_data = frameIndex;
        RIDEHAL_DEBUG(
                "FillBuffer, frameIndex=%" PRId32
                ", frameData.frame_addr=0x%x frameData.frame_handle=0x%x outputBuffer->size %d",
                frameIndex, frameData.frame_addr, frameData.frame_handle, outputBuffer->size );

        rc = device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                           (uint8_t *) ( &frameData ), sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SubmitOutputFrame FillBuffer %d failed", (int) frameIndex );
            Teardown();
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "SubmitOutputFrame VIDC_IOCTL_FILL_OUTPUT_BUFFER done" );
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
        if ( VIDEO_ENCODER_STATE_EXECUTING == m_vidcEncoderData.state )
        {
            RIDEHAL_DEBUG( "Stopping vidc!" );
            int32_t rc = device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0,
                                       nullptr, 0 );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "Stop vidc failed" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_IDLE ) )
        {
            RIDEHAL_ERROR( "WaitForState.state_idle.fail!" );
        }
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
            RIDEHAL_ERROR( "release source failed" );
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

RideHalError_e VideoEncoder::GetInputBufferList( VideoEncoder_InputFrame_t **pInputList )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    RIDEHAL_DEBUG( "GetInputBufferList" );
    if ( nullptr != m_inputList )
    {
        *pInputList = m_inputList;
    }
    else
    {
        RIDEHAL_ERROR( "input buffer is not ready!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    return ret;
}

RideHalError_e VideoEncoder::GetOutputBufferList( VideoEncoder_OutputFrame_t **pOutputList )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    RIDEHAL_DEBUG( "GetOutputBufferList" );
    if ( nullptr != m_outputList )
    {
        *pOutputList = m_outputList;
        RIDEHAL_DEBUG( "pOutputList %p", pOutputList );
    }
    else
    {
        RIDEHAL_ERROR( "output buffer is not ready!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
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
        Teardown();
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret && nullptr == pCmd )
    {
        RIDEHAL_ERROR( "pCmd is NULL pointer!" );
        Teardown();
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Configuring vidc" );
        switch ( pCmd->propID )
        {
            case VIDEO_ENCODER_PROP_BITRATE:
                RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
                m_bitRate = pCmd->pValue;
                m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
                rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                                     sizeof( vidc_target_bitrate_type ),
                                     (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_TARGET_BITRATE failed!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            case VIDEO_ENCODER_PROP_FRAME_RATE:
                m_frameRate = pCmd->pValue;
                RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT %d",
                               (int) m_frameRate );
                m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
                m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
                m_vidcEncoderData.frameRate.fps_denominator = 1;
                rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                                     sizeof( vidc_frame_rate_type ),
                                     (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT failed!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            default:
                RIDEHAL_ERROR( "propID %d not supported", (int) pCmd->propID );
                Teardown();
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

    if ( nullptr == inputDoneCb || nullptr == outputDoneCb || nullptr == eventCb )
    {
        RIDEHAL_ERROR( "callback is NULL pointer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_inputDoneCb = inputDoneCb;
        m_outputDoneCb = outputDoneCb;
        m_eventCb = eventCb;
        m_pAppPriv = pAppPriv;
    }
    return ret;
}

bool VideoEncoder::Teardown()
{
    if ( VIDEO_ENCODER_STATE_EXECUTING ==
         m_vidcEncoderData.state )   // call from VideoEncoder::Init or Start
    {
        RIDEHAL_DEBUG( "Stopping vidc!" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_IDLE ) )
        {
            RIDEHAL_ERROR( "WaitForState.state_idle.fail!" );
        }
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
                      0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_LOADED ) )
        {
            RIDEHAL_ERROR( "WaitForState.STATE_LOADED.fail!" );
        }
    }
    else if ( VIDEO_ENCODER_STATE_IDLE ==
              m_vidcEncoderData.state )   // call after VideoEncoder::Stop
    {
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr,
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

    if ( nullptr != m_inputList ) free( m_inputList );
    if ( nullptr != m_outputList ) free( m_outputList );

    if ( m_vidcEncoderData.ioHandle )
    {
        device_close( m_vidcEncoderData.ioHandle );
    }

    return true;
}

void VideoEncoder::PrintEncoderConfig()
{
    RIDEHAL_DEBUG( "EncoderConfig: FrameWidth = %" PRIu32, m_width );
    RIDEHAL_DEBUG( "EncoderConfig: FrameHeight = %" PRIu32, m_height );
    RIDEHAL_DEBUG( "EncoderConfig: InFPS = %" PRIu32, m_frameRate );
    RIDEHAL_DEBUG( "EncoderConfig: RateControl = 0x % x ", m_vidcEncoderData.rateControl );
    RIDEHAL_DEBUG( "EncoderConfig: BitRate = %" PRIu32, m_vidcEncoderData.bitrate.target_bitrate );
    if ( VIDC_CODEC_HEVC == m_vidcEncoderData.codec )
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = H265" );
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    else
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = UNKNOWN 0x%x", m_vidcEncoderData.codec );
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    RIDEHAL_DEBUG( "EncoderConfig: NumPframes = %" PRIu32, m_vidcEncoderData.iPeriod.p_frames );
    RIDEHAL_DEBUG( "EncoderConfig: NumBframes = %" PRIu32, m_vidcEncoderData.iPeriod.b_frames );
    RIDEHAL_DEBUG( "EncoderConfig: InBufferCount = %" PRIu32 " [0 means minimum]",
                   m_numInputBufferReq );
    RIDEHAL_DEBUG( "EncoderConfig: OutBufferCount = %" PRIu32 " [0 means minimum]",
                   m_numOutputBufferReq );
    RIDEHAL_DEBUG( "EncoderConfig: IdrPeriod = %" PRIu32, m_vidcEncoderData.idrPeriod.idr_period );
}

vidc_color_format_type VideoEncoder::GetVidcFormat( RideHal_ImageFormat_e format )
{
    // only support NV12 and p010 now
    vidc_color_format_type ret = VIDC_COLOR_FORMAT_UNUSED;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_NV12:
            ret = VIDC_COLOR_FORMAT_NV12;
            break;
        case RIDE_HAL_IMAGE_FORMAT_P010:
            ret = VIDC_COLOR_FORMAT_NV12_P010;
            break;
        default:
            RIDEHAL_ERROR( "format %d not supported", (int) format );
            ret = VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
    return ret;
}

int VideoEncoder::DeviceCallback( uint8_t *msg, uint32_t length )
{
    (void) length;
    vidc_drv_msg_info_type *pEvent = (vidc_drv_msg_info_type *) msg;
    vidc_frame_data_type *pFrameData = nullptr;

    switch ( pEvent->event_type )
    {
        case VIDC_EVT_RESP_FLUSH_INPUT_DONE:
            m_eventCb( VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;

            if ( pFrameData->frm_clnt_data < 0 || pFrameData->frm_clnt_data >= m_numInputBufferReq )
            {
                RIDEHAL_ERROR( "input frameIndex = %" PRIu64 " is invalid",
                               pFrameData->frm_clnt_data );
                m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            }
            else
            {
                auto inputFrame = &m_inputList[pFrameData->frm_clnt_data];
                m_inputDoneCb( inputFrame, m_pAppPriv );
                m_availableInputQueue.push( pFrameData->frm_clnt_data );
            }

            RIDEHAL_DEBUG( "input-done: frameIndex  %" PRIu64, pFrameData->frm_clnt_data );
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;

            RIDEHAL_DEBUG( "encode-done: frameIndex %u timestamp %lu size %u frameType %lu "
                           "pFrameData %p frame_addr %p flag 0x %x ",
                           pFrameData->frm_clnt_data, pFrameData->timestamp, pFrameData->data_len,
                           pFrameData->frame_type, pFrameData, pFrameData->frame_addr,
                           pFrameData->flags );

            if ( nullptr != pFrameData->frame_addr )
            {
                if ( pFrameData->frm_clnt_data < 0 ||
                     pFrameData->frm_clnt_data >= m_numOutputBufferReq )
                {
                    RIDEHAL_ERROR( "output frameIndex = %" PRIu64 " is invalid",
                                   pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
                }
                else
                {
                    auto outputFrame = &m_outputList[pFrameData->frm_clnt_data];
                    RIDEHAL_DEBUG( "Encoded frame is ready - calling callback!" );
                    if ( outputFrame->sharedBuffer.buffer.pData == pFrameData->frame_addr )
                    {
                        outputFrame->appMarkData = (void *) pFrameData->mark_data;
                        outputFrame->frameType = (VideoEncoder_FrameType_e) pFrameData->frame_type;
                        outputFrame->timestampNs =
                                pFrameData->timestamp * 1000;   // convert us to ns
                        outputFrame->frameFlag = pFrameData->flags;
                        m_outputDoneCb( outputFrame, m_pAppPriv );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "output bufIndex = %" PRIu64
                                       " frame_addr %p is not match with buffer pData %p",
                                       pFrameData->frm_clnt_data, pFrameData->frame_addr,
                                       outputFrame->sharedBuffer.buffer.pData );
                        m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
                    }
                }
            }

            if ( pFrameData->flags & VIDC_FRAME_FLAG_EOS )
            {
                RIDEHAL_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            m_eventCb( VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_HWFATAL:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_EXECUTING;
            break;
        case VIDC_EVT_RESP_STOP:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_IDLE;
            break;
        case VIDC_EVT_RESP_PAUSE:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_PAUSE;
            break;
        case VIDC_EVT_RESP_RESUME:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_EXECUTING;
            break;
        case VIDC_EVT_RESP_LOAD_RESOURCES:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_IDLE;
            break;
        case VIDC_EVT_RESP_RELEASE_RESOURCES:
            m_vidcEncoderData.state = VIDEO_ENCODER_STATE_LOADED;
            break;
        case VIDC_EVT_RELEASE_BUFFER_REFERENCE:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, &pEvent->payload, m_pAppPriv );
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
    int32_t rc = 0;
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
        rc = -1;
    }

    return rc;
}

int32_t VideoEncoder::SetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                                      uint32_t nPktSize, uint8_t *pPkt )
{
    int32_t rc = 0;
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
        rc = -1;
    }
    return rc;
}

int32_t VideoEncoder::WaitForState( VideoEncoder_State_e expectedState )
{
    int32_t counter = 0;
    int32_t rc = 0;

    while ( m_vidcEncoderData.state != expectedState )
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

int32_t VideoEncoder::AllocateBuffer( ioctl_session_t *ioHandle, vidc_buffer_info_type **pBufInfo,
                                      vidc_buffer_type bufferType, int32_t bufCntMin,
                                      int32_t bufSize )
{
    int32_t rc = 0;
    int32_t i = 0;
    int32_t nMsgSize = sizeof( vidc_buffer_info_type );
    *pBufInfo = (vidc_buffer_info_type *) malloc( sizeof( vidc_buffer_info_type ) * bufCntMin );
    if ( nullptr == *pBufInfo )
    {
        RIDEHAL_ERROR( "AllocateBuffer failed!" );
        rc = -1;
    }
    else
    {
        vidc_buffer_info_type *pBuf = *pBufInfo;
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };
        RIDEHAL_DEBUG( "*pBufInfo=%p bufCntMin=%" PRId32, *pBufInfo, bufCntMin );
        for ( i = 0; i < bufCntMin; i++ )
        {
            memset( &pBuf[i], 0, sizeof( vidc_buffer_info_type ) );

            RideHal_SharedBuffer_t sharedBuffer;
            if ( VIDC_BUFFER_INPUT == bufferType )
            {
                auto ret = sharedBuffer.Allocate( m_width, m_height, m_inFormat );
                if ( RIDE_HAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate inputBuffer failed %d for index=%" PRId32, ret, i );
                    rc = -1;
                }
                else
                {
                    m_inputList[i].sharedBuffer = sharedBuffer;
                    m_inputList[i].frameIndex = i;
                    RIDEHAL_DEBUG( "m_inputList[%" PRId32 "] %p", i, m_inputList[i] );
                }
            }
            else if ( VIDC_BUFFER_OUTPUT == bufferType )
            {
                RideHal_ImageProps_t imgProps;
                imgProps.batchSize = 1;
                imgProps.width = m_width;
                imgProps.height = m_height;
                imgProps.compressedSize = bufSize;
                imgProps.format = m_outFormat;
                auto ret = sharedBuffer.Allocate( &imgProps );
                if ( RIDE_HAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate outputBuffer failed %d for index=%" PRId32, ret, i );
                    rc = -1;
                }
                else
                {
                    m_outputList[i].sharedBuffer = sharedBuffer;
                    m_outputList[i].frameIndex = i;
                    RIDEHAL_DEBUG( "m_outputList[%" PRId32 "] %p", i, m_outputList[i] );
                }
            }

            if ( 0 == rc )
            {
                pBuf[i].buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
                pBuf[i].buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
                pBuf[i].buf_handle =
                        (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
                if ( nullptr == pBuf[i].buf_addr )
                {
                    RIDEHAL_ERROR( "AllocateBuffer (allocPmem) failed for index=%" PRId32, i );
                }
                else
                {
                    pBuf[i].buf_type = bufferType;
                    pBuf[i].contiguous = true;
                    pBuf[i].buf_size = bufSize;
                    memcpy( &buf_info, &pBuf[i], sizeof( vidc_buffer_info_type ) );
                    // register it before lookup in future for local allocation
                    RIDEHAL_DEBUG( "AllocateBuffer [%d]: buf_addr = 0x%x, buf_handle = 0x%x, "
                                   "buf_size = %d",
                                   i, pBuf[i].buf_addr, pBuf[i].buf_handle, bufSize );
                    rc = device_ioctl( ioHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t *) ( &buf_info ),
                                       nMsgSize, nullptr, 0 );
                    if ( 0 != rc )
                    {
                        RIDEHAL_ERROR(
                                " AllocateBuffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32
                                " rc=0x%x",
                                i, rc );
                        break;
                    }
                }
            }
        }
    }
    return rc;
}

int32_t VideoEncoder::GetInputInformation()
{
    int32_t rc = 0;

    m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
    rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_SIZE,
                         sizeof( vidc_frame_size_type ),
                         (uint8_t *) ( &m_vidcEncoderData.frameSize ) );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_FRAME_SIZE failed!" );
    }
    if ( 0 == rc )
    {
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                             sizeof( vidc_frame_rate_type ),
                             (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_FRAME_RATE failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_vidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_COLOR_FORMAT,
                             sizeof( vidc_color_format_config_type ),
                             (uint8_t *) ( &m_vidcEncoderData.colorFormatConfig ) );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_COLOR_FORMAT failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_vidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefY.plane_index = 1;
        rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) ( &m_vidcEncoderData.planeDefY ) );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "GetInputInformation GetDrvProperty VIDC_I_PLANE_DEF_Y failed!" );
        }
    }
    if ( 0 == rc )
    {
        m_vidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefUV.plane_index = 2;
        rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_PLANE_DEF,
                             sizeof( vidc_plane_def_type ),
                             (uint8_t *) ( &m_vidcEncoderData.planeDefUV ) );
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

    m_vidcEncoderData.inputBufferReq.buf_type = VIDC_BUFFER_INPUT;
    rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    else
    {
        if ( m_vidcEncoderData.inputBufferReq.actual_count < m_numInputBufferReq )
        {
            m_vidcEncoderData.inputBufferReq.actual_count = m_numInputBufferReq;
        }

        m_numInputBufferReq = m_vidcEncoderData.inputBufferReq.actual_count;
        m_vidcEncoderData.vidcInputBufferSize = m_vidcEncoderData.inputBufferReq.size;
        RIDEHAL_DEBUG( "input-bufs: count=%" PRIu32 ", size=%" PRIu32, m_numInputBufferReq,
                       m_vidcEncoderData.vidcInputBufferSize );

        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                             sizeof( vidc_buffer_reqmnts_type ),
                             (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        }
        else
        {
            rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                                 sizeof( vidc_buffer_reqmnts_type ),
                                 (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "GetDrvProperty.VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
            }
        }
    }

    return rc;
}

int32_t VideoEncoder::GetOutputBufferRequirement()
{
    int32_t rc = 0;

    m_vidcEncoderData.outputBufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                         sizeof( vidc_buffer_reqmnts_type ),
                         (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );
    if ( 0 != rc )
    {
        RIDEHAL_ERROR( "GetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
    }
    else
    {
        if ( m_vidcEncoderData.outputBufferReq.actual_count < m_numOutputBufferReq )
        {
            m_vidcEncoderData.outputBufferReq.actual_count = m_numOutputBufferReq;
        }

        m_numOutputBufferReq = m_vidcEncoderData.outputBufferReq.actual_count;
        m_vidcEncoderData.vidcOutputBufferSize = m_vidcEncoderData.outputBufferReq.size;
        RIDEHAL_DEBUG( "output-bufs: count=%" PRIu32 ", size=%" PRIu32, m_numOutputBufferReq,
                       m_vidcEncoderData.vidcOutputBufferSize );

        rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                             sizeof( vidc_buffer_reqmnts_type ),
                             (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
        }
        else
        {
            rc = GetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_BUFFER_REQUIREMENTS,
                                 sizeof( vidc_buffer_reqmnts_type ),
                                 (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );
            if ( 0 != rc )
            {
                RIDEHAL_ERROR( "GetDrvProperty VIDC_I_BUFFER_REQUIREMENTS failed %d", rc );
            }
        }
    }

    return rc;
}

int32_t VideoEncoder::FreeOutputBuffer()
{
    uint32_t i = 0;
    int32_t rc = 0;
    vidc_buffer_info_type outbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( nullptr != m_vidcEncoderData.vidcOutputBufferInfo )
    {
        for ( i = 0; i < m_numOutputBufferReq; i++ )
        {
            memcpy( &outbuf, &m_vidcEncoderData.vidcOutputBufferInfo[i],
                    sizeof( vidc_buffer_info_type ) );

            outbuf.buf_addr = m_vidcEncoderData.vidcOutputBufferInfo[i].buf_addr;
            if ( VIDC_ERR_NONE != device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) ( &outbuf ),
                                                sizeof( vidc_buffer_info_type ), nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( "FreeOutputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }

        RIDEHAL_DEBUG( "FreeOutputBufferInfo" );
        vidc_buffer_info_type *pBufInfo = m_vidcEncoderData.vidcOutputBufferInfo;
        free( pBufInfo );
        m_vidcEncoderData.vidcOutputBufferInfo = nullptr;
    }

    return rc;
}

int32_t VideoEncoder::FreeInputBuffer()
{
    int32_t rc = 0;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( nullptr != m_vidcEncoderData.vidcInputBufferInfo )
    {
        for ( uint32_t i = 0; i < m_numInputBufferReq; i++ )
        {
            memcpy( &inbuf, &m_vidcEncoderData.vidcInputBufferInfo[i],
                    sizeof( vidc_buffer_info_type ) );
            inbuf.buf_addr = m_vidcEncoderData.vidcInputBufferInfo[i].buf_addr;
            if ( VIDC_ERR_NONE != device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) ( &inbuf ),
                                                sizeof( vidc_buffer_info_type ), nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( "FreeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }

        RIDEHAL_DEBUG( "FreeInputBufferInfo" );
        vidc_buffer_info_type *pBufInfo = m_vidcEncoderData.vidcInputBufferInfo;
        free( pBufInfo );
        m_vidcEncoderData.vidcInputBufferInfo = nullptr;
    }

    return rc;
}

}   // namespace component
}   // namespace ridehal
