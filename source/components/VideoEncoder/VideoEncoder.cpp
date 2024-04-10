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
    int32_t i, rc = 0;

    ret = ComponentIF::Init( pName, level );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = ValidateConfig( pConfig );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "Validate config failed!" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
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
            ret = RIDE_HAL_ERROR_FAIL;
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
        m_vidcEncoderData.iPeriod.p_frames =
                pConfig->gop ? pConfig->gop : VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I;
        m_vidcEncoderData.iPeriod.b_frames = VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I;
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
        m_vidcEncoderData.idrPeriod.idr_period = VIDEO_ENCODER_DEFAULT_IDR_PERIOD;
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
            m_inputList = (RideHal_SharedBuffer_t *) malloc( m_numInputBufferReq *
                                                             sizeof( RideHal_SharedBuffer_t ) );
            if ( nullptr == m_inputList )
            {
                RIDEHAL_ERROR( "m_inputList malloc failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
            else
            {
                rc = AllocateBuffer( m_vidcEncoderData.ioHandle, pConfig->inputBufferList,
                                     VIDC_BUFFER_INPUT, m_numInputBufferReq,
                                     m_vidcEncoderData.vidcInputBufferSize );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "Failed to allocate input buffers!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    for ( i = 0; i < m_numInputBufferReq; i++ )
                    {
                        VideoEncoder_InputFrame_t inputFrame;
                        inputFrame.sharedBuffer = m_inputList[i];
                        inputFrame.useFlag = false;
                        m_inputMap[m_inputList[i].buffer.dmaHandle] = inputFrame;
                    }
                }
            }
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
            m_outputList = (RideHal_SharedBuffer_t *) malloc( m_numOutputBufferReq *
                                                              sizeof( RideHal_SharedBuffer_t ) );
            if ( nullptr == m_outputList )
            {
                RIDEHAL_ERROR( "m_outputList malloc failed!" );
                Teardown();
                ret = RIDE_HAL_ERROR_FAIL;
            }
            else
            {
                rc = AllocateBuffer( m_vidcEncoderData.ioHandle, pConfig->outputBufferList,
                                     VIDC_BUFFER_OUTPUT, m_numOutputBufferReq,
                                     m_vidcEncoderData.vidcOutputBufferSize );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "Failed to allocate output buffers!" );
                    Teardown();
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    for ( i = 0; i < m_numOutputBufferReq; i++ )
                    {
                        VideoEncoder_OutputFrame_t outputFrame;
                        outputFrame.sharedBuffer = m_outputList[i];
                        outputFrame.useFlag = false;
                        m_outputMap[m_outputList[i].buffer.dmaHandle] = outputFrame;
                    }
                }
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
    int32_t i, rc = 0;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "can not Start since Init Not ready!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret &&
         ( nullptr == m_inputDoneCb || nullptr == m_outputDoneCb || nullptr == m_eventCb ) )
    {
        RIDEHAL_ERROR( "Not start since callback is not registered!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Starting vidc" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_EXECUTING ) )
        {
            RIDEHAL_ERROR( "VIDC_IOCTL_START WaitForState STATE_EXECUTING failed!" );
            ret = RIDE_HAL_ERROR_TIMEOUT;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        for ( i = 0; i < m_numOutputBufferReq; i++ )
        {
            if ( false == m_bOutputDynamicMode )
            {
                VideoEncoder_OutputFrame_t outputFrame;
                outputFrame.sharedBuffer = m_outputList[i];
                ret = SubmitOutputFrame( &outputFrame );
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Started vidc" );
        m_state = RIDE_HAL_COMPONENT_STATE_RUNNING;
    }

    return ret;
}

RideHalError_e VideoEncoder::SubmitInputFrame( const VideoEncoder_InputFrame_t *pInput )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t i, rc = 0;
    const RideHal_SharedBuffer_t *inputBuffer = nullptr;
    uint64_t frameId = MAX_UINT64;
    vidc_frame_data_type frameData;
    vidc_buffer_info_type bufferInfo;
    int convertedSize;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret && ( nullptr == pInput || nullptr == &pInput->sharedBuffer ) )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        inputBuffer = &pInput->sharedBuffer;
        bufferInfo.buf_addr = (uint8_t *) inputBuffer->data();
        convertedSize = m_vidcEncoderData.vidcInputBufferSize;
        rc = ValidateBuffer( inputBuffer, VIDC_BUFFER_INPUT );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "Validate Buffer Failed!" );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        frameId = inputBuffer->buffer.dmaHandle;
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

        RIDEHAL_DEBUG( "input-start: frameId 0x%x timestampNs %" PRIu64
                       " buf_addr 0x%x handle 0x%x buf_size %" PRIu32,
                       frameId, pInput->timestampNs, bufferInfo.buf_addr,
                       inputBuffer->buffer.dmaHandle, bufferInfo.buf_size );

        memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.frm_clnt_data = frameId;
        frameData.buf_type = VIDC_BUFFER_INPUT;
        frameData.frame_addr = bufferInfo.buf_addr;
        frameData.alloc_len = bufferInfo.buf_size;
        frameData.frame_handle = (pmem_handle_t) inputBuffer->buffer.dmaHandle;

        frameData.data_len = convertedSize;
        frameData.timestamp = pInput->timestampNs / 1000;   // ns convert to us
        frameData.mark_data = (unsigned long) pInput->appMarkData;
        if ( nullptr != pInput->onTheFlyCmd )
        {
            for ( i = 0; i < pInput->numCmd; i++ )
            {
                ret = Configure( &pInput->onTheFlyCmd[i] );
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_inLock );
        if ( true == m_bInputDynamicMode )
        {
            if ( m_inputMap.end() != m_inputMap.find( frameId ) )
            {
                RIDEHAL_DEBUG( "find frameId 0x%x", frameId );
                if ( false == m_inputMap[frameId].useFlag )
                {
                    m_inputMap[frameId].useFlag = true;
                    m_inputMap[frameId].timestampNs = pInput->timestampNs;
                    m_inputMap[frameId].appMarkData = pInput->appMarkData;
                }
                else
                {
                    RIDEHAL_ERROR( "input buffer not available now!" );
                    ret = RIDE_HAL_ERROR_NORES;
                }
            }
            else if ( m_inputMap.size() < m_numInputBufferReq )
            {
                m_inputMap[frameId] = *pInput;
                m_inputMap[frameId].useFlag = true;
            }
            else
            {
                RIDEHAL_ERROR( "No empty input buffer available!" );
                ret = RIDE_HAL_ERROR_NORES;
            }
        }
        else
        {
            if ( m_inputMap.end() != m_inputMap.find( frameId ) &&
                 false == m_inputMap[frameId].useFlag )
            {
                RIDEHAL_DEBUG( "find frameId 0x%x", frameId );
                m_inputMap[frameId].useFlag = true;
                m_inputMap[frameId].timestampNs = pInput->timestampNs;
                m_inputMap[frameId].appMarkData = pInput->appMarkData;
            }
            else
            {
                RIDEHAL_ERROR( "No empty input buffer available!" );
                ret = RIDE_HAL_ERROR_NORES;
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
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER frameTs: %" PRIu64,
                           pInput->timestampNs );
        }
    }
    return ret;
}

RideHalError_e VideoEncoder::SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutput )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int32_t rc = 0;
    uint64_t frameId = MAX_UINT64;
    vidc_frame_data_type frameData;
    const RideHal_SharedBuffer_t *outputBuffer = nullptr;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state && RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_WARN( "Not submitting outputBuffer since encoder is not ready or encoder is "
                      "shutting down!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret && nullptr == pOutput )
    {
        RIDEHAL_ERROR( "Not submitting empty outputBuffer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "SubmitOutputFrame begin" );
        outputBuffer = &pOutput->sharedBuffer;
        frameId = outputBuffer->buffer.dmaHandle;
        std::unique_lock<std::mutex> auto_lock( m_outLock );
        if ( true == m_bOutputDynamicMode )
        {
            if ( m_outputMap.end() != m_outputMap.find( frameId ) )
            {
                RIDEHAL_DEBUG( "find frameId 0x%x", frameId );
                if ( false == m_outputMap[frameId].useFlag )
                {
                    m_outputMap[frameId].useFlag = true;
                }
                else
                {
                    RIDEHAL_ERROR( "output buffer not available now!" );
                    ret = RIDE_HAL_ERROR_NORES;
                }
            }
            else if ( m_outputMap.size() < m_numOutputBufferReq )
            {
                VideoEncoder_OutputFrame_t outputFrame;
                outputFrame.sharedBuffer = *outputBuffer;
                m_outputMap[frameId] = outputFrame;
                m_outputMap[frameId].useFlag = true;
            }
            else
            {
                RIDEHAL_ERROR( "No empty output buffer available!" );
                ret = RIDE_HAL_ERROR_NORES;
            }
        }
        else
        {
            if ( m_outputMap.end() != m_outputMap.find( frameId ) &&
                 false == m_outputMap[frameId].useFlag )
            {
                RIDEHAL_DEBUG( "find frameId 0x%x", frameId );
                m_outputMap[frameId].useFlag = true;
            }
            else
            {
                RIDEHAL_ERROR( "No empty output buffer available!" );
                ret = RIDE_HAL_ERROR_NORES;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.buf_type = VIDC_BUFFER_OUTPUT;
        frameData.frame_addr = (uint8_t *) outputBuffer->data();
        frameData.alloc_len = outputBuffer->buffer.size;
        frameData.frame_handle = (pmem_handle_t) outputBuffer->buffer.dmaHandle;
        frameData.frm_clnt_data = frameId;

        RIDEHAL_DEBUG(
                "FillBuffer, frameId=0x%x , frameData.frame_addr=0x%x frameData.frame_handle=0x%x "
                "frameData.alloc_len %" PRIu32 " outputBuffer->size %" PRIu32,
                frameId, frameData.frame_addr, frameData.frame_handle, frameData.alloc_len,
                outputBuffer->size );

        rc = device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                           (uint8_t *) ( &frameData ), sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "SubmitOutputFrame FillBuffer 0x%x failed rc 0x%x", frameId, rc );
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
            Teardown();
            ret = RIDE_HAL_ERROR_TIMEOUT;
        }
        else
        {
            RIDEHAL_DEBUG( "Stopped vidc" );
            m_state = RIDE_HAL_COMPONENT_STATE_READY;
        }
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
        int32_t rc = Teardown();
        if ( 0 != rc )
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

RideHalError_e VideoEncoder::GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t size )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetInputBuffers" );

    if ( nullptr != m_inputList )   // it means dynamic mode
    {
        if ( nullptr == pInputList )
        {
            RIDEHAL_ERROR( "pInputList is null pointer!" );
            ret = RIDE_HAL_ERROR_NULL_PTR;
        }
        else
        {
            if ( size == m_numOutputBufferReq )
            {
                for ( int i = 0; i < m_numInputBufferReq; i++ )
                {
                    pInputList[i] = m_inputList[i];
                }
            }
            else
            {
                RIDEHAL_ERROR( "the provided array size %" PRIu32
                               " too small to hold all buffer infos!",
                               size );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "input buffer is not ready!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    return ret;
}

RideHalError_e VideoEncoder::GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t size )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetOutputBuffers" );

    if ( nullptr != m_outputList )   // it means dynamic mode
    {
        if ( nullptr == pOutputList )
        {
            RIDEHAL_ERROR( "pOutputList is null pointer!" );
            ret = RIDE_HAL_ERROR_NULL_PTR;
        }
        else
        {
            if ( size == m_numOutputBufferReq )
            {
                for ( int i = 0; i < m_numOutputBufferReq; i++ )
                {
                    pOutputList[i] = m_outputList[i];
                }
            }
            else
            {
                RIDEHAL_ERROR( "the provided array size %" PRIu32
                               " too small to hold all buffer infos!",
                               size );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "output buffer is not ready!" );
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
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret && nullptr == pCmd )
    {
        RIDEHAL_ERROR( "pCmd is NULL pointer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Configuring vidc" );
        switch ( pCmd->propID )
        {
            case VIDEO_ENCODER_PROP_BITRATE:
                RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE %" PRIu32, m_bitRate );
                m_bitRate = pCmd->pValue;
                m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
                rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_TARGET_BITRATE,
                                     sizeof( vidc_target_bitrate_type ),
                                     (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_TARGET_BITRATE failed!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            case VIDEO_ENCODER_PROP_FRAME_RATE:
                m_frameRate = pCmd->pValue;
                RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT %" PRIu32,
                               m_frameRate );
                m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
                m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
                m_vidcEncoderData.frameRate.fps_denominator = 1;
                rc = SetDrvProperty( m_vidcEncoderData.ioHandle, VIDC_I_FRAME_RATE,
                                     sizeof( vidc_frame_rate_type ),
                                     (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
                if ( 0 != rc )
                {
                    RIDEHAL_ERROR( "SetDrvProperty VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT failed!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                break;
            default:
                RIDEHAL_ERROR( "propID %d not supported", pCmd->propID );
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

int32_t VideoEncoder::Teardown()
{
    int32_t rc = 0;
    if ( VIDEO_ENCODER_STATE_EXECUTING ==
         m_vidcEncoderData.state )   // call from VideoEncoder::Init or Start
    {
        RIDEHAL_DEBUG( "Stopping vidc!" );
        device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
        if ( 0 != WaitForState( VIDEO_ENCODER_STATE_IDLE ) )
        {
            rc = -1;
            RIDEHAL_ERROR( "WaitForState.state_idle.fail!" );
        }
        else
        {
            RIDEHAL_DEBUG( "Stopped vidc" );
            m_state = RIDE_HAL_COMPONENT_STATE_READY;

            RIDEHAL_DEBUG( "Releasing vidc resources!" );
            device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0,
                          nullptr, 0 );
            if ( 0 != WaitForState( VIDEO_ENCODER_STATE_LOADED ) )
            {
                rc = -1;
                RIDEHAL_ERROR( "WaitForState.STATE_LOADED.fail!" );
            }
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
            rc = -1;
            RIDEHAL_ERROR( "WaitForState.STATE_LOADED.fail!" );
        }
    }

    if ( 0 == rc && VIDEO_ENCODER_STATE_LOADED == m_vidcEncoderData.state )
    {
        if ( 0 != FreeInputBuffer() )
        {
            RIDEHAL_ERROR( "Failed to free input buffers!" );
        }

        if ( 0 != FreeOutputBuffer() )
        {
            RIDEHAL_ERROR( "Failed to free output buffers!" );
        }

        if ( m_vidcEncoderData.ioHandle )
        {
            device_close( m_vidcEncoderData.ioHandle );
        }

        m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;
    }

    return rc;
}

void VideoEncoder::PrintEncoderConfig()
{
    RIDEHAL_DEBUG( "EncoderConfig: FrameWidth = %" PRIu32, m_width );
    RIDEHAL_DEBUG( "EncoderConfig: FrameHeight = %" PRIu32, m_height );
    RIDEHAL_DEBUG( "EncoderConfig: InFPS = %" PRIu32, m_frameRate );
    RIDEHAL_DEBUG( "EncoderConfig: RateControl = 0x%x ", m_vidcEncoderData.rateControl );
    RIDEHAL_DEBUG( "EncoderConfig: BitRate = %" PRIu32, m_vidcEncoderData.bitrate.target_bitrate );
    if ( VIDC_CODEC_HEVC == m_vidcEncoderData.codec )
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = H265" );
    }
    else if ( VIDC_CODEC_H264 == m_vidcEncoderData.codec )
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = H264" );
    }
    else
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = UNKNOWN 0x%x", m_vidcEncoderData.codec );
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
            RIDEHAL_ERROR( "format %d not supported", format );
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
            m_eventCb( VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            {
                std::unique_lock<std::mutex> lck( m_inLock );
                if ( m_inputMap.end() != m_inputMap.find( pFrameData->frm_clnt_data ) )
                {
                    m_inputMap[pFrameData->frm_clnt_data].useFlag = false;
                    auto inputFrame = &m_inputMap[pFrameData->frm_clnt_data];
                    m_inputDoneCb( inputFrame, m_pAppPriv );
                }
                else
                {
                    RIDEHAL_ERROR( "input frameId = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }
            RIDEHAL_DEBUG( "input-done: frameId  0x%x", pFrameData->frm_clnt_data );
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            RIDEHAL_DEBUG(
                    "encode-done: frameId 0x%x timestamp %" PRIu64 " size %u frameType %" PRIu64 " "
                    "pFrameData %p frame_addr %p flag 0x%x ",
                    pFrameData->frm_clnt_data, pFrameData->timestamp, pFrameData->data_len,
                    pFrameData->frame_type, pFrameData, pFrameData->frame_addr, pFrameData->flags );

            if ( nullptr != pFrameData->frame_addr )
            {
                VideoEncoder_OutputFrame_t *outputFrame = nullptr;
                {
                    std::unique_lock<std::mutex> lck( m_outLock );
                    if ( m_outputMap.end() != m_outputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_outputMap[pFrameData->frm_clnt_data].useFlag = false;
                        outputFrame = &m_outputMap[pFrameData->frm_clnt_data];
                    }
                }
                if ( nullptr != outputFrame )
                {
                    RIDEHAL_DEBUG( "Encoded frame is ready - calling callback!" );
                    if ( (uint8_t *) outputFrame->sharedBuffer.data() == pFrameData->frame_addr )
                    {
                        outputFrame->appMarkData = (void *) pFrameData->mark_data;
                        outputFrame->sharedBuffer.size = pFrameData->data_len;
                        outputFrame->frameType = (VideoEncoder_FrameType_e) pFrameData->frame_type;
                        outputFrame->timestampNs =
                                pFrameData->timestamp * 1000;   // convert us to ns
                        outputFrame->frameFlag = pFrameData->flags;
                        m_outputDoneCb( outputFrame, m_pAppPriv );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "output frameId = 0x%x, frame_addr %p is not match with "
                                       "buffer data %p",
                                       pFrameData->frm_clnt_data, pFrameData->frame_addr,
                                       (uint8_t *) outputFrame->sharedBuffer.data() );
                        m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                    }
                }
                else
                {
                    RIDEHAL_ERROR( "output frameId = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }

            if ( pFrameData->flags & VIDC_FRAME_FLAG_EOS )
            {
                RIDEHAL_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            m_eventCb( VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_HWFATAL:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
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
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
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

int32_t VideoEncoder::ValidateConfig( const VideoEncoder_Config_t *pConfig )
{
    int32_t i, rc = 0;
    m_width = pConfig->width;
    m_height = pConfig->height;
    if ( ( 0 == m_width ) || ( 0 == m_height ) )
    {
        RIDEHAL_ERROR( "m_width %" PRIu32 " m_height%" PRIu32 " should not be zero!", m_width,
                       m_height );
        rc = -1;
    }
    m_bitRate = pConfig->bitRate ? pConfig->bitRate : VIDEO_ENCODER_DEFAULT_BIT_RATE;
    m_frameRate = pConfig->frameRate ? pConfig->frameRate : VIDEO_ENCODER_DEFAULT_FRAME_RATE;

    m_vidcEncoderData.rateControl = (vidc_rate_control_mode_type) pConfig->rateControlMode;
    if ( VIDC_RATE_CONTROL_UNUSED == m_vidcEncoderData.rateControl )
    {
        RIDEHAL_ERROR( "rate control mode: 0x%x not supported!", pConfig->rateControlMode );
        rc = -1;
    }

    m_inFormat = pConfig->inFormat;
    m_vidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
    m_vidcEncoderData.colorFormatConfig.color_format = GetVidcFormat( m_inFormat );
    if ( VIDC_COLOR_FORMAT_UNUSED == m_vidcEncoderData.colorFormatConfig.color_format )
    {
        RIDEHAL_ERROR( "input format: %d not supported!", m_inFormat );
        rc = -1;
    }

    m_outFormat = pConfig->outFormat;
    if ( RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat )
    {
        m_vidcEncoderData.codec = VIDC_CODEC_HEVC;
    }
    else if ( RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H264 == m_outFormat )
    {
        m_vidcEncoderData.codec = VIDC_CODEC_H264;
    }
    else
    {
        RIDEHAL_ERROR( "output format: %d not supported!", m_outFormat );
        rc = -1;
    }

    m_bInputDynamicMode = pConfig->bInputDynamicMode;
    m_bOutputDynamicMode = pConfig->bOutputDynamicMode;

    m_vidcEncoderData.sessionCodec.session = VIDC_SESSION_ENCODE;
    m_vidcEncoderData.sessionCodec.codec = m_vidcEncoderData.codec;

    if ( pConfig->numInputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ||
         pConfig->numInputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ )
    {
        RIDEHAL_ERROR( "numInputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       pConfig->numInputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                       VIDEO_ENCODER_MAX_BUFFER_REQ );
        rc = -1;
    }
    else
    {
        m_numInputBufferReq = pConfig->numInputBufferReq;
    }

    if ( pConfig->numOutputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ||
         pConfig->numOutputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ )
    {
        RIDEHAL_ERROR( "numOutputBufferReq: %" PRIu32
                       " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       pConfig->numOutputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                       VIDEO_ENCODER_MAX_BUFFER_REQ );
        rc = -1;
    }
    else
    {
        m_numOutputBufferReq = pConfig->numOutputBufferReq;
    }

    if ( true == m_bInputDynamicMode && nullptr != pConfig->inputBufferList )
    {
        RIDEHAL_ERROR( "should not provide inputbuffer in config in dynamic mode!" );
        rc = -1;
    }

    if ( true == m_bOutputDynamicMode && nullptr != pConfig->outputBufferList )
    {
        RIDEHAL_ERROR( "should not provide outputbuffer in config in dynamic mode!" );
        rc = -1;
    }

    if ( 0 == rc && false == m_bInputDynamicMode && nullptr != pConfig->inputBufferList )
    {
        for ( i = 0; i < m_numInputBufferReq; i++ )
        {
            rc = ValidateBuffer( &pConfig->inputBufferList[i], VIDC_BUFFER_INPUT );
            if ( 0 != rc ) break;
        }
    }

    if ( 0 == rc && false == m_bOutputDynamicMode && nullptr != pConfig->outputBufferList )
    {
        for ( i = 0; i < m_numOutputBufferReq; i++ )
        {
            rc = ValidateBuffer( &pConfig->outputBufferList[i], VIDC_BUFFER_OUTPUT );
            if ( 0 != rc ) break;
        }
    }

    return rc;
}

int32_t VideoEncoder::ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                      vidc_buffer_type bufferType )
{
    int32_t rc = 0;
    if ( pBuffer->imgProps.width != m_width || pBuffer->imgProps.height != m_height )
    {
        RIDEHAL_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32 " is not match m_width %" PRIu32
                       " m_height %" PRIu32,
                       pBuffer->imgProps.width, pBuffer->imgProps.height, m_width, m_height );
        rc = -1;
    }

    if ( bufferType == VIDC_BUFFER_INPUT )
    {
        if ( pBuffer->imgProps.format != m_inFormat )
        {
            RIDEHAL_ERROR( "pBuffer format %d  is not match m_inFormat %d",
                           pBuffer->imgProps.format, m_inFormat );
            rc = -1;
        }
        if ( pBuffer->imgProps.stride[0] != m_vidcEncoderData.planeDefY.actual_stride ||
             pBuffer->imgProps.stride[1] != m_vidcEncoderData.planeDefUV.actual_stride )
        {
            RIDEHAL_ERROR( "pBuffer stride [%" PRIu32 "][%" PRIu32
                           "] is not same with actual strid [%" PRIu32 "][%" PRIu32 "] ",
                           pBuffer->imgProps.stride[0], pBuffer->imgProps.stride[1],
                           m_vidcEncoderData.planeDefY.actual_stride,
                           m_vidcEncoderData.planeDefUV.actual_stride );
            rc = -1;
        }
        if ( (size_t) m_vidcEncoderData.vidcInputBufferSize > pBuffer->size &&
             true == m_bInputDynamicMode )
        {
            RIDEHAL_ERROR( "pBuffer size %zu is smaller than vidcInputBufferSize %" PRIu32,
                           pBuffer->size, m_vidcEncoderData.vidcInputBufferSize );
            rc = -1;
        }
    }
    else
    {
        if ( pBuffer->imgProps.format != m_outFormat )
        {
            RIDEHAL_ERROR( "pBuffer format %d  is not match m_outFormat %d",
                           pBuffer->imgProps.format, m_outFormat );
            rc = -1;
        }
    }

    return rc;
}

int32_t VideoEncoder::GetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                                      uint32_t nPktSize, uint8_t *pPkt )
{
    int32_t rc = 0;
    uint8_t dev_cmd_buffer[VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
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
        RIDEHAL_ERROR( "GetDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
        rc = -1;
    }

    return rc;
}

int32_t VideoEncoder::SetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                                      uint32_t nPktSize, uint8_t *pPkt )
{
    int32_t rc = 0;
    uint8_t dev_cmd_buffer[VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
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
        RIDEHAL_ERROR( "SetDrvProperty propId=0x%x failed! Status code=0x%x", propId, status );
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
        if ( counter > VIDEO_ENCODER_WAIT_TIMEOUT_1_SEC )
        {
            RIDEHAL_ERROR( "WaitForState timeout!" );
            rc = -1;
            break;
        }
    }
    return rc;
}

int32_t VideoEncoder::AllocateBuffer( ioctl_session_t *ioHandle, RideHal_SharedBuffer_t *bufferList,
                                      vidc_buffer_type bufferType, int32_t bufCntMin,
                                      int32_t bufSize )
{
    int32_t rc = 0;
    int32_t i = 0;
    int32_t nMsgSize = sizeof( vidc_buffer_info_type );

    RIDEHAL_DEBUG( "bufSize=%" PRId32 " bufCntMin=%" PRId32, bufSize, bufCntMin );
    for ( i = 0; i < bufCntMin; i++ )
    {
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };
        memset( &buf_info, 0, sizeof( vidc_buffer_info_type ) );

        RideHal_SharedBuffer_t sharedBuffer;
        if ( VIDC_BUFFER_INPUT == bufferType )
        {
            if ( nullptr == bufferList )
            {
                auto ret = sharedBuffer.Allocate( m_width, m_height, m_inFormat );
                if ( RIDE_HAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate inputBuffer failed %d for index=%" PRId32, ret, i );
                    rc = -1;
                }
                else
                {
                    m_inputList[i] = sharedBuffer;
                    RIDEHAL_DEBUG( "m_inputList[%" PRId32 "] 0x%x", i, &m_inputList[i] );
                }
            }
            else
            {
                sharedBuffer = bufferList[i];
                m_inputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_inputList[%" PRId32 "] 0x%x", i, &m_inputList[i] );
            }
        }
        else if ( VIDC_BUFFER_OUTPUT == bufferType )
        {
            if ( nullptr == bufferList )
            {
                RideHal_ImageProps_t imgProps;
                imgProps.batchSize = 1;
                imgProps.width = m_width;
                imgProps.height = m_height;
                imgProps.compressedSize = bufSize;
                imgProps.format = m_outFormat;
                auto ret = sharedBuffer.Allocate( &imgProps );
                // auto ret = sharedBuffer.Allocate( bufSize );
                if ( RIDE_HAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate outputBuffer failed %d for index=%" PRId32, ret, i );
                    rc = -1;
                }
                else
                {
                    m_outputList[i] = sharedBuffer;
                    RIDEHAL_DEBUG( "m_outputList[%" PRId32 "] 0x%x", i, &m_outputList[i] );
                }
            }
            else
            {
                sharedBuffer = bufferList[i];
                m_outputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_outputList[%" PRId32 "] 0x%x", i, &m_outputList[i] );
            }
        }

        if ( 0 == rc )
        {
            buf_info.buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
            buf_info.buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
            buf_info.buf_handle = (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
            if ( nullptr == buf_info.buf_addr )
            {
                RIDEHAL_ERROR( "AllocateBuffer (allocPmem) failed for index=%" PRId32, i );
            }
            else
            {
                buf_info.buf_type = bufferType;
                buf_info.contiguous = true;
                buf_info.buf_size = bufSize;
                // register it before lookup in future for local allocation
                RIDEHAL_DEBUG( "AllocateBuffer [%" PRId32 "]: buf_addr = 0x%x, buf_handle = 0x%x, "
                               "buf_size = %d",
                               i, buf_info.buf_addr, buf_info.buf_handle, bufSize );
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

    if ( nullptr != m_outputList )   // it means non dynamic mode
    {
        for ( uint32_t i = 0; i < m_numOutputBufferReq; i++ )
        {
            outbuf.buf_addr = (uint8_t *) m_outputList[i].data();
            outbuf.buf_handle = (pmem_handle_t) m_outputList[i].buffer.dmaHandle;
            outbuf.buf_type = VIDC_BUFFER_OUTPUT;
            outbuf.contiguous = true;
            outbuf.buf_size = m_outputList[i].size;
            if ( VIDC_ERR_NONE != device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) ( &outbuf ),
                                                sizeof( vidc_buffer_info_type ), nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( "FreeOutputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }

        RIDEHAL_DEBUG( "Free m_outputList" );
        free( m_outputList );
        m_outputList = nullptr;
    }

    return rc;
}

int32_t VideoEncoder::FreeInputBuffer()
{
    int32_t rc = 0;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };

    if ( nullptr != m_inputList )   // it means non dynamic mode
    {
        for ( uint32_t i = 0; i < m_numInputBufferReq; i++ )
        {
            inbuf.buf_addr = (uint8_t *) m_inputList[i].data();
            inbuf.buf_handle = (pmem_handle_t) m_inputList[i].buffer.dmaHandle;
            inbuf.buf_type = VIDC_BUFFER_INPUT;
            inbuf.contiguous = true;
            inbuf.buf_size = m_inputList[i].size;
            if ( VIDC_ERR_NONE != device_ioctl( m_vidcEncoderData.ioHandle, VIDC_IOCTL_FREE_BUFFER,
                                                (uint8_t *) ( &inbuf ),
                                                sizeof( vidc_buffer_info_type ), nullptr, 0 ) )
            {
                rc = -1;
                RIDEHAL_ERROR( "FreeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 " failed!",
                               i );
            }
        }

        RIDEHAL_DEBUG( "Free m_inputList" );
        free( m_inputList );
        m_inputList = nullptr;
    }

    return rc;
}

}   // namespace component
}   // namespace ridehal
