// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>

#include <vidc_ioctl.h>
#include <vidc_types.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoDecoder.hpp"

namespace ridehal
{
namespace component
{

static constexpr uint16_t VIDEO_DECODER_DEFAULT_FRAME_RATE = 30;
static constexpr uint16_t VIDEO_MAX_DEV_CMD_BUFFER_SIZE = 256;
static constexpr int WAIT_TIMEOUT_1_MSEC = 1;
static constexpr int WAIT_TIME_FOR_STATE_CHANGE_IN_SEC = 1;
static constexpr int WAIT_TIME_COUNTER_FOR_STATE_CHANGE_IN_MSEC = 1000;

#define VIDEO_DECODER_MAX_BUFFER_REQ 64
#define VIDEO_DECODER_MIN_BUFFER_REQ 4

#define ARRAY_SIZE( a ) ( sizeof( ( a ) ) / sizeof( ( a )[0] ) )

/** @brief Store the data interacted with video core */
typedef struct
{
    ioctl_session_t *pIoHandle; /**< The IOSession */
    vidc_codec_type codec;      /**< The codec selection for decoder */
    vidc_level_type level;
    vidc_session_codec_type sessionCodec; /**< Session & Codec type setting for decoder */

    vidc_output_order_type order;
    vidc_enable_type enable;
    vidc_frame_size_type frameSize;
    vidc_frame_rate_type frameRate; /**< Frame rate */
    vidc_color_format_config_type
            colorFormatConfig; /**< Configures the uncompressed Buffer format */
    vidc_plane_def_type planeDef;

    vidc_buffer_reqmnts_type inputBufferReq;  /**< Get/Set input buffer requirements from vidc */
    vidc_buffer_reqmnts_type outputBufferReq; /**< Get/Set output buffer requirements from vidc */
    uint32_t vidcInputBufferSize;             /**< The size of inputBuffer */
    uint32_t vidcOutputBufferSize;            /**< The size of outputBuffer */
} VidcDecoderContext_t;

static const char *VidcErrToStr( vidc_status_type err )
{
    const char *ret;
    switch ( err )
    {
        case VIDC_ERR_NONE:
            ret = "No error.";
            break;
        case VIDC_ERR_INDEX_NOMORE:
            ret = "No More indices can be enumerated";
            break;
        case VIDC_ERR_FAIL:
            ret = "General failure";
            break;
        case VIDC_ERR_ALLOC_FAIL:
            ret = "Failed while allocating memory";
            break;
        case VIDC_ERR_ILLEGAL_OP:
            ret = "Illegal operation was requested";
            break;
        case VIDC_ERR_BAD_PARAM:
            ret = "Bad paramter(s) or memory pointer(s) provided";
            break;
        case VIDC_ERR_BAD_HANDLE:
            ret = "Bad driver or client handle provided";
            break;
        case VIDC_ERR_NOT_SUPPORTED:
            ret = "API is currently not supported";
            break;
        case VIDC_ERR_BAD_STATE:
            ret = "Requested API is not supported in current device or client state";
            break;
        case VIDC_ERR_MAX_CLIENT:
            ret = "Allowed maximum number of clients are already opened";
            break;
        case VIDC_ERR_IFRAME_EXPECTED:
            ret = " Decoding: VIDC was expecting I-frame which was not provided";
            break;
        case VIDC_ERR_HW_FATAL:
            ret = "Fatal irrecoverable hardware error detected";
            break;
        case VIDC_ERR_BITSTREAM_ERR:
            ret = "Decoding: Error in input frame detected and frame processing "
                  "skipped";
            break;
        case VIDC_ERR_SEQHDR_PARSE_FAIL:
            ret = "Sequence header parsing failed in decoder";
            break;
        case VIDC_ERR_INSUFFICIENT_BUFFER:
            ret = "Error to indicate that the buffer supplied was insufficient in "
                  "size";
            break;
        case VIDC_ERR_BAD_POWER_STATE:
            ret = "Error indicating that the device is in bad power state and can not "
                  "service the request API.";
            break;
        case VIDC_ERR_NO_VALID_SESSION:
            ret = "The error is returned in case a session related API call be made "
                  "wihtout"
                  "initializing a session. (For e.g. if ABORT is called on a hanlde "
                  "for"
                  "which vidc_initialize() has not been called";
            break;
        case VIDC_ERR_TIMEOUT:
            ret = "API was not completed and returned with a timeout";
            break;
        case VIDC_ERR_CMDQFULL:
            ret = "API request was not accepted as command queue is full ";
            break;
        case VIDC_ERR_START_CODE_NOT_FOUND:
            ret = "Valid start code was not found within provided compressed video "
                  "frame";
            break;
        case VIDC_ERR_UNSUPPORTED_STREAM:
            ret = "Error indicates stream is unsupported by video core";
            break;
        case VIDC_ERR_SESSION_PICTURE_DROPPED:
            ret = "Error indicates frame was dropped as per VIDC_I_DEC_PICTYPE "
                  "property";
            break;
        case VIDC_ERR_CLIENTFATAL:
            ret = "To indicate irrecoverable fatal client error was detected."
                  "Client should initiate session clean up.";
            break;
        case VIDC_ERR_NONCOMPLIANT_STREAM:
            ret = "Error indicates stream is non-compliant";
            break;
        case VIDC_ERR_SPURIOUS_INTERRUPT:
            ret = "Error indicates spurious interrupt handled and no further"
                  "processing required";
            break;
        case VIDC_ERR_UNUSED:
        default:
            ret = "Unknown error code";
            break;
    }

    return ret;
}

RideHalError_e VideoDecoder::InitDriver()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_SESSION_CODEC" );
        ret = SetDrvProperty( VIDC_I_SESSION_CODEC, sizeof( vidc_session_codec_type ),
                              (uint8_t *) ( &ctx->sessionCodec ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT" );
        ctx->frameRate.buf_type = VIDC_BUFFER_OUTPUT;
        ctx->frameRate.fps_numerator = m_frameRate;
        ctx->frameRate.fps_denominator = 1;
        ret = SetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                              (uint8_t *) ( &ctx->frameRate ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT" );
        ctx->frameRate.buf_type = VIDC_BUFFER_INPUT;
        ctx->frameRate.fps_numerator = m_frameRate;
        ctx->frameRate.fps_denominator = 1;
        ret = SetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                              (uint8_t *) ( &ctx->frameRate ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        vidc_color_format_config_type vidcColorFmt;
        vidcColorFmt.buf_type = VIDC_BUFFER_OUTPUT;
        vidcColorFmt.color_format = VIDC_COLOR_FORMAT_NV12;
        ret = SetDrvProperty( VIDC_I_COLOR_FORMAT, sizeof( vidc_color_format_config_type ),
                              (uint8_t *) &vidcColorFmt );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_DEC_ORDER_DECODE" );

        ctx->order.output_order = VIDC_DEC_ORDER_DECODE;
        ret = SetDrvProperty( VIDC_I_DEC_OUTPUT_ORDER, sizeof( vidc_output_order_type ),
                              (uint8_t *) &ctx->order );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_DEC_CONT_ON_RECONFIG" );

        ctx->enable.enable = true;
        ret = SetDrvProperty( VIDC_I_DEC_CONT_ON_RECONFIG, sizeof( vidc_enable_type ),
                              (uint8_t *) &ctx->enable );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT" );
        ctx->frameSize.buf_type = VIDC_BUFFER_INPUT;
        ctx->frameSize.width = m_width;
        ctx->frameSize.height = m_height;
        ret = SetDrvProperty( VIDC_I_FRAME_SIZE, sizeof( vidc_frame_size_type ),
                              (uint8_t *) ( &ctx->frameSize ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT" );
        ctx->frameSize.buf_type = VIDC_BUFFER_OUTPUT;
        ctx->frameSize.width = m_width;
        ctx->frameSize.height = m_height;
        ret = SetDrvProperty( VIDC_I_FRAME_SIZE, sizeof( vidc_frame_size_type ),
                              (uint8_t *) ( &ctx->frameSize ) );
    }

    return ret;
}

RideHalError_e VideoDecoder::Init( const char *pName, const VideoDecoder_Config_t *pConfig,
                                   Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i, rc = 0;
    VidcDecoderContext_t *ctx = nullptr;

    ret = ComponentIF::Init( pName, level );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;

        RIDEHAL_INFO( "video-decoder init begin" );

        ret = ValidateConfig( pName, pConfig );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_vidcDecoderContext = (VidcDecoderContext_t *) malloc( sizeof( VidcDecoderContext_t ) );
        if ( nullptr == m_vidcDecoderContext )
        {
            RIDEHAL_ERROR( "driver context memory alloc failed" );
            ret = RIDEHAL_ERROR_NOMEM;
        }
        else
        {
            ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = InitFromConfig( pName, pConfig );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_ioctlCb.handler = VideoDecoder::DeviceCallback;
        m_ioctlCb.data = (void *) this;

        RIDEHAL_DEBUG( "Opening vidc device" );
        ctx->pIoHandle =
                device_open( (char *) "VideoCore/vidc_drv", (ioctl_callback_t *) &m_ioctlCb );
        if ( nullptr == ctx->pIoHandle )
        {
            RIDEHAL_ERROR( "Failed to open vidc device!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            RIDEHAL_INFO( "Open vidc device succeed" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = InitDriver();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetOutputInformation();
    }

    if ( ( false == m_bInputDynamicMode ) && ( nullptr != pConfig->pInputBufferList ) )
    {
        m_bInputNonDynamicAppAllocBuffer = true;
    }
    if ( ( false == m_bOutputDynamicMode ) && ( nullptr != pConfig->pOutputBufferList ) )
    {
        m_bOutputNonDynamicAppAllocBuffer = true;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetInputBufferRequirement();
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && m_bInputNonDynamicAppAllocBuffer )
    {
        for ( i = 0; i < m_numInputBuffer; i++ )
        {
            ret = ValidateBuffer( &pConfig->pInputBufferList[i], VIDEO_CODEC_BUF_TYPE_INPUT );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "validate VIDC_BUFFER_INPUT failed" );
                break;
            }
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && m_bOutputNonDynamicAppAllocBuffer )
    {
        for ( i = 0; i < m_numOutputBuffer; i++ )
        {
            ret = ValidateBuffer( &pConfig->pOutputBufferList[i], VIDEO_CODEC_BUF_TYPE_OUTPUT );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "validate VIDC_BUFFER_OUTPUT failed" );
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( true == m_bInputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable input dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            (void) memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_INPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            ret = SetDrvProperty( VIDC_I_BUFFER_ALLOC_MODE, sizeof( buffer_alloc_mode ),
                                  (uint8_t *) ( &buffer_alloc_mode ) );
        }
        else
        {
            ret = InitBufferForNonDynamicMode( pConfig->pInputBufferList,
                                               VIDEO_CODEC_BUF_TYPE_INPUT );
            if ( ( RIDEHAL_ERROR_NONE == ret ) && ( false == m_bInputNonDynamicAppAllocBuffer ) )
            {
                ret = AllocateBuffer( VIDEO_CODEC_BUF_TYPE_INPUT );
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = SetBuffer( VIDEO_CODEC_BUF_TYPE_INPUT );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( true == m_bOutputDynamicMode )
        {
            RIDEHAL_DEBUG( "Enable output dynamic mode" );
            vidc_buffer_alloc_mode_type buffer_alloc_mode;
            (void) memset( &buffer_alloc_mode, 0, sizeof( vidc_buffer_alloc_mode_type ) );
            buffer_alloc_mode.buf_type = VIDC_BUFFER_OUTPUT;
            buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
            ret = SetDrvProperty( VIDC_I_BUFFER_ALLOC_MODE, sizeof( buffer_alloc_mode ),
                                  (uint8_t *) ( &buffer_alloc_mode ) );
        }
        else
        {
            ret = InitBufferForNonDynamicMode( pConfig->pOutputBufferList,
                                               VIDEO_CODEC_BUF_TYPE_OUTPUT );
            if ( ( RIDEHAL_ERROR_NONE == ret ) && ( false == m_bOutputNonDynamicAppAllocBuffer ) )
            {
                ret = AllocateBuffer( VIDEO_CODEC_BUF_TYPE_OUTPUT );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Loading vidc resources" );
        rc = device_ioctl( ctx->pIoHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0, nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Loading vidc resources failed! rc=0x%x, %s", rc,
                           VidcErrToStr( vidc_status_type( rc ) ) );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "Successfully completed vidc initialization!" );
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_READY );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "VIDC_IOCTL_LOAD_RESOURCES WaitForState.state_ready fail!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        PrintDecoderConfig();
        RIDEHAL_INFO( "video-decoder init done" );
    }
    else
    {
        RIDEHAL_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

RideHalError_e VideoDecoder::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i, rc = 0;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "can not Start since Init Not ready!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( nullptr == m_inputDoneCb ) || ( nullptr == m_outputDoneCb ) ||
           ( nullptr == m_eventCb ) ) )
    {
        RIDEHAL_ERROR( "Not start since callback is not registered!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STARTING;

        RIDEHAL_DEBUG( "START_INPUT begin" );
        vidc_start_mode_type start_mode = VIDC_START_INPUT;
        rc = device_ioctl( ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle,
                           VIDC_IOCTL_START, (uint8 *) &start_mode, sizeof( vidc_start_mode_type ),
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Starting vidc failed! rc=0x%x %s", rc,
                           VidcErrToStr( vidc_status_type( rc ) ) );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_RUNNING );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "VIDC_IOCTL_START START_INPUT failed!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            RIDEHAL_DEBUG( "START_INPUT done" );
        }
    }

    RIDEHAL_INFO( "dec start done" );

    return ret;
}

RideHalError_e VideoDecoder::HandleOutputReconfig()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;

    m_OutputReconfigInprogress = true;

    RIDEHAL_INFO( "handle output-reconfig begin" );

    ret = GetOutputBufferRequirement();

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( false == m_bOutputDynamicMode ) )
    {
        ret = SetBuffer( VIDEO_CODEC_BUF_TYPE_OUTPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "START_OUTPUT begin" );
        vidc_start_mode_type start_mode = VIDC_START_OUTPUT;
        rc = device_ioctl( ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle,
                           VIDC_IOCTL_START, (uint8 *) &start_mode, sizeof( vidc_start_mode_type ),
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "vidc START_OUTPUT failed! rc=0x%x %s", rc,
                           VidcErrToStr( vidc_status_type( rc ) ) );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "START_OUTPUT sent" );
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::FinishOutputReconfig()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i;

    if ( m_OutputReconfigInprogress )
    {
        m_OutputReconfigInprogress = false;

        if ( false == m_bOutputDynamicMode )
        {
            for ( i = 0; i < m_numOutputBuffer; i++ )
            {
                VideoDecoder_OutputFrame_t outputFrame;
                outputFrame.sharedBuffer = m_pOutputList[i];
                ret = SubmitOutputFrame( &outputFrame );
            }
        }

        RIDEHAL_INFO( "handle output-reconfig done" );
    }
    return ret;
}

RideHalError_e VideoDecoder::SubmitInputFrame( const VideoDecoder_InputFrame_t *pInput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i, rc = 0;
    const RideHal_SharedBuffer_t *inputBuffer = nullptr;
    uint64_t handle = MAX_UINT64;
    vidc_frame_data_type frameData;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since decoder is not running" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( nullptr == pInput ) || ( nullptr == pInput->sharedBuffer.data() ) ) )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        inputBuffer = &pInput->sharedBuffer;
        ret = ValidateBuffer( inputBuffer, VIDEO_CODEC_BUF_TYPE_INPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        handle = inputBuffer->buffer.dmaHandle;

        RIDEHAL_DEBUG( "dec-input-begin: handle 0x%x tsNs %" PRIu64 " markData %" PRIu64
                       " buf_addr 0x%x handle 0x%x buf_size %" PRIu32,
                       handle, pInput->timestampNs, pInput->appMarkData, inputBuffer->data(),
                       inputBuffer->buffer.dmaHandle, inputBuffer->size );

        (void) memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.frm_clnt_data = handle;
        frameData.buf_type = VIDC_BUFFER_INPUT;
        frameData.frame_addr = (uint8_t *) inputBuffer->data();
        frameData.alloc_len = inputBuffer->buffer.size;
#if defined( __QNXNTO__ )
        frameData.frame_handle = (pmem_handle_t) inputBuffer->buffer.dmaHandle;
#else
        frameData.frame_handle = (int) reinterpret_cast<uint64_t>( inputBuffer->buffer.dmaHandle );
#endif
        frameData.data_len = inputBuffer->size;
        frameData.timestamp = pInput->timestampNs / 1000;   // ns convert to us
        frameData.mark_data = (unsigned long) pInput->appMarkData;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_inLock );
        if ( true == m_bInputDynamicMode )
        {
            if ( m_inputMap.end() != m_inputMap.find( handle ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                if ( false == m_inputMap[handle].useFlag )
                {
                    m_inputMap[handle].useFlag = true;
                    m_inputMap[handle].timestampNs = pInput->timestampNs;
                    m_inputMap[handle].appMarkData = pInput->appMarkData;
                }
                else
                {
                    RIDEHAL_ERROR( "input buffer not available now!" );
                    ret = RIDEHAL_ERROR_NOMEM;
                }
            }
            else if ( m_inputMap.size() < m_numInputBuffer )
            {
                m_inputMap[handle].useFlag = true;
                m_inputMap[handle].timestampNs = pInput->timestampNs;
                m_inputMap[handle].appMarkData = pInput->appMarkData;
                m_inputMap[handle].sharedBuffer = pInput->sharedBuffer;
            }
            else
            {
                RIDEHAL_ERROR( "No empty input buffer available!" );
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }
        else
        {
            if ( ( m_inputMap.end() != m_inputMap.find( handle ) ) &&
                 ( false == m_inputMap[handle].useFlag ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                m_inputMap[handle].useFlag = true;
                m_inputMap[handle].timestampNs = pInput->timestampNs;
                m_inputMap[handle].appMarkData = pInput->appMarkData;
            }
            else
            {
                RIDEHAL_ERROR( "No empty input buffer available!" );
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        rc = device_ioctl( ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle,
                           VIDC_IOCTL_EMPTY_INPUT_BUFFER, (uint8_t *) ( &frameData ),
                           sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER failed! rc=0x%x", rc );
            m_inputMap[handle].useFlag = false;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER frameTs: %" PRIu64,
                           pInput->timestampNs );
        }
    }
    return ret;
}

/*
 * For output buffer none dynamic mode:
 *   this function must be called in OutFrameCallback when output buffer return back from driver.
 */
RideHalError_e VideoDecoder::SubmitOutputFrame( const VideoDecoder_OutputFrame_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;
    uint64_t handle = MAX_UINT64;
    vidc_frame_data_type frameData;
    const RideHal_SharedBuffer_t *outputBuffer = nullptr;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Not submitting outputBuffer since decoder not running" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( false == m_OutputStarted )
    {
        RIDEHAL_ERROR( "output not started" );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else if ( m_OutputReconfigInprogress )
    {
        /* when handle output-reconfig, app submitOutputFrame should be forbiden */
        RIDEHAL_ERROR( "Not submitting outputBuffer since doing output reconfig" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( nullptr == pOutput ) )
    {
        RIDEHAL_ERROR( "Not submitting empty outputBuffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "SubmitOutputFrame begin" );
        outputBuffer = &pOutput->sharedBuffer;
        handle = outputBuffer->buffer.dmaHandle;
        ret = ValidateBuffer( outputBuffer, VIDEO_CODEC_BUF_TYPE_OUTPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_outLock );
        if ( true == m_bOutputDynamicMode )
        {
            if ( m_outputMap.end() != m_outputMap.find( handle ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                if ( false == m_outputMap[handle].useFlag )
                {
                    m_outputMap[handle].useFlag = true;
                }
                else
                {
                    RIDEHAL_ERROR( "output buffer not available now!" );
                    ret = RIDEHAL_ERROR_NOMEM;
                }
            }
            else if ( m_outputMap.size() < m_numOutputBuffer )
            {
                m_outputMap[handle].sharedBuffer = *outputBuffer;
                m_outputMap[handle].useFlag = true;
            }
            else
            {
                RIDEHAL_ERROR( "No empty output buffer available!" );
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }
        else
        {
            if ( ( m_outputMap.end() != m_outputMap.find( handle ) ) &&
                 ( false == m_outputMap[handle].useFlag ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                m_outputMap[handle].useFlag = true;
            }
            else
            {
                RIDEHAL_ERROR( "No empty output buffer available!" );
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        (void) memset( &frameData, 0, sizeof( vidc_frame_data_type ) );
        frameData.buf_type = VIDC_BUFFER_OUTPUT;
        frameData.frame_addr = (uint8_t *) outputBuffer->data();
        frameData.alloc_len = outputBuffer->buffer.size;
#if defined( __QNXNTO__ )
        frameData.frame_handle = (pmem_handle_t) outputBuffer->buffer.dmaHandle;
#else
        frameData.frame_handle = (int) reinterpret_cast<uint64_t>( outputBuffer->buffer.dmaHandle );
#endif
        frameData.frm_clnt_data = handle;

        RIDEHAL_INFO( "FillBuffer, handle=0x%x , frameData.frame_addr=0x%x "
                      "frameData.frame_handle=0x%x "
                      "frameData.alloc_len %" PRIu32 " outputBuffer->size %" PRIu32,
                      handle, frameData.frame_addr, frameData.frame_handle, frameData.alloc_len,
                      outputBuffer->size );

        rc = device_ioctl( ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle,
                           VIDC_IOCTL_FILL_OUTPUT_BUFFER, (uint8_t *) ( &frameData ),
                           sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SubmitOutputFrame FillBuffer 0x%x failed rc 0x%x", handle, rc );
            m_outputMap[handle].useFlag = false;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "dec-output-begin, handle 0x%x", handle );
        }
    }
    RIDEHAL_DEBUG( "SubmitOutputFrame done" );

    return ret;
}

RideHalError_e VideoDecoder::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STOPING;
        RIDEHAL_DEBUG( "Stopping vidc!" );
        rc = device_ioctl( ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle,
                           VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Stop vidc failed! rc=0x%x", rc );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_READY );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "WaitForState.state_ready.fail!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_ERROR != m_state ) )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( RIDEHAL_COMPONENT_STATE_READY == m_state ) )
    {
        RIDEHAL_DEBUG( "Deiniting vidc" );
        RIDEHAL_DEBUG( "Releasing vidc resources!" );
        m_state = RIDEHAL_COMPONENT_STATE_DEINITIALIZING;
        rc = device_ioctl( ctx->pIoHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Releasing vidc resources failed! rc=0x%x", rc );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_INITIAL );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "WaitForState.state_initial.fail!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = FreeInputBuffer();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = FreeOutputBuffer();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ctx->pIoHandle != nullptr )
        {
            (void) device_close( ctx->pIoHandle );
            ctx->pIoHandle = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_vidcDecoderContext )
        {
            free( m_vidcDecoderContext );
            m_vidcDecoderContext = nullptr;
        }

        RIDEHAL_DEBUG( "Deinited component" );
        ret = ComponentIF::Deinit();
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Something wrong happened in Deinit" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
    }
    else
    {
        RIDEHAL_INFO( "Deinited done" );
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
    }

    return ret;
}

// only supported for non-dynamic mode
RideHalError_e VideoDecoder::GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetInputBuffers" );

    if ( m_bInputDynamicMode )
    {
        RIDEHAL_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == pInputList )
        {
            RIDEHAL_ERROR( "pInputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num != m_numInputBuffer )
        {
            RIDEHAL_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                           m_numInputBuffer );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_pInputList )
        {
            RIDEHAL_ERROR( "input buffer is not ready!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( int i = 0; i < m_numInputBuffer; i++ )
        {
            pInputList[i] = m_pInputList[i];
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetOutputBuffers" );

    if ( m_bOutputDynamicMode )
    {
        RIDEHAL_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == pOutputList )
        {
            RIDEHAL_ERROR( "pOutputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num != m_numOutputBuffer )
        {
            RIDEHAL_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                           m_numOutputBuffer );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_pOutputList )
        {
            RIDEHAL_ERROR( "output buffer is not ready!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( int i = 0; i < m_numOutputBuffer; i++ )
        {
            pOutputList[i] = m_pOutputList[i];
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::RegisterCallback( VideoDecoder_InFrameCallback_t inputDoneCb,
                                               VideoDecoder_OutFrameCallback_t outputDoneCb,
                                               VideoDecoder_EventCallback_t eventCb,
                                               void *pAppPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == inputDoneCb ) || ( nullptr == outputDoneCb ) || ( nullptr == eventCb ) )
    {
        RIDEHAL_ERROR( "callback is NULL pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_inputDoneCb = inputDoneCb;
        m_outputDoneCb = outputDoneCb;
        m_eventCb = eventCb;
        m_pAppPriv = pAppPriv;
    }
    return ret;
}

void VideoDecoder::PrintDecoderConfig()
{
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    RIDEHAL_DEBUG( "DecoderConfig: FrameWidth = %" PRIu32, m_width );
    RIDEHAL_DEBUG( "DecoderConfig: FrameHeight = %" PRIu32, m_height );
    RIDEHAL_DEBUG( "DecoderConfig: fps = %" PRIu32, m_frameRate );
    if ( VIDC_CODEC_HEVC == ctx->codec )
    {
        RIDEHAL_DEBUG( "DecoderConfig: Codec = H265" );
        RIDEHAL_DEBUG( "DecoderConfig: Level = 0x%x", ctx->level.level );
    }
    else if ( VIDC_CODEC_H264 == ctx->codec )
    {
        RIDEHAL_DEBUG( "DecoderConfig: Codec = H264" );
        RIDEHAL_DEBUG( "DecoderConfig: Level = 0x%x", ctx->level.level );
    }
    else
    {
        RIDEHAL_DEBUG( "DecoderConfig: Codec type = 0x%x", ctx->codec );
    }
    RIDEHAL_DEBUG( "DecoderConfig: InBufferCount = %" PRIu32 " [0 means minimum]",
                   m_numInputBuffer );
    RIDEHAL_DEBUG( "DecoderConfig: OutBufferCount = %" PRIu32 " [0 means minimum]",
                   m_numOutputBuffer );
    RIDEHAL_DEBUG( "DecoderConfig: inBufSize = %" PRIu32, ctx->vidcInputBufferSize );
    RIDEHAL_DEBUG( "DecoderConfig: outBufSize = %" PRIu32, ctx->vidcOutputBufferSize );
}

int VideoDecoder::DeviceCbHandler( uint8_t *msg, uint32_t length )
{
    (void) length;
    vidc_drv_msg_info_type *pEvent = (vidc_drv_msg_info_type *) msg;
    vidc_frame_data_type *pFrameData = nullptr;

    switch ( pEvent->event_type )
    {
        case VIDC_EVT_RESP_FLUSH_INPUT_DONE:
            m_eventCb( VIDEO_DECODER_EVENT_FLUSH_INPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            RIDEHAL_ERROR( "not support input-reconfig" );
            m_eventCb( VIDEO_DECODER_EVENT_INPUT_RECONFIG, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            RIDEHAL_DEBUG( "dec-input-done: handle 0x%x", pFrameData->frm_clnt_data );
            {
                VideoDecoder_InputInfo_t *pInputInfo = nullptr;
                {
                    std::unique_lock<std::mutex> lck( m_inLock );
                    if ( m_inputMap.end() != m_inputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_inputMap[pFrameData->frm_clnt_data].useFlag = false;
                        pInputInfo = &m_inputMap[pFrameData->frm_clnt_data];
                    }
                }
                if ( nullptr != pInputInfo )
                {
                    VideoDecoder_InputFrame_t inputFrame;
                    inputFrame.sharedBuffer = pInputInfo->sharedBuffer;
                    inputFrame.timestampNs = pInputInfo->timestampNs;
                    inputFrame.appMarkData = pInputInfo->appMarkData;
                    m_inputDoneCb( &inputFrame, m_pAppPriv );
                }
                else
                {
                    RIDEHAL_ERROR( "input handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            RIDEHAL_DEBUG( "dec-output-done: handle 0x%x ts %" PRIu64
                           " size %u alloc_len %u frameType %" PRIu64 " "
                           "pFrameData %p frame_addr %p flag 0x%x ",
                           pFrameData->frm_clnt_data, pFrameData->timestamp, pFrameData->data_len,
                           pFrameData->alloc_len, pFrameData->frame_type, pFrameData,
                           pFrameData->frame_addr, pFrameData->flags );

            if ( nullptr != pFrameData->frame_addr )
            {
                VideoDecoder_OutputInfo_t *pOutputInfo = nullptr;
                {
                    std::unique_lock<std::mutex> lck( m_outLock );
                    if ( m_outputMap.end() != m_outputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_outputMap[pFrameData->frm_clnt_data].useFlag = false;
                        pOutputInfo = &m_outputMap[pFrameData->frm_clnt_data];
                    }
                }
                if ( nullptr != pOutputInfo )
                {
                    RIDEHAL_DEBUG( "Decoded frame is ready - calling callback!" );
                    if ( (uint8_t *) pOutputInfo->sharedBuffer.data() == pFrameData->frame_addr )
                    {
                        VideoDecoder_OutputFrame_t outputFrame;
                        outputFrame.sharedBuffer = pOutputInfo->sharedBuffer;
                        outputFrame.appMarkData = (uint64_t) pFrameData->mark_data;
                        outputFrame.timestampNs =
                                pFrameData->timestamp * 1000;   // convert us to ns
                        outputFrame.frameFlag = pFrameData->flags;
                        m_outputDoneCb( &outputFrame, m_pAppPriv );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "output handle = 0x%x, frame_addr %p is not match with "
                                       "buffer data %p",
                                       pFrameData->frm_clnt_data, pFrameData->frame_addr,
                                       (uint8_t *) pOutputInfo->sharedBuffer.data() );
                        m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
                    }
                }
                else
                {
                    RIDEHAL_ERROR( "output handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }

            if ( ( pFrameData->flags & VIDC_FRAME_FLAG_EOS ) > 0 )
            {
                RIDEHAL_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            RIDEHAL_DEBUG( "FLUSH_OUTPUT_DONE event" );
            m_eventCb( VIDEO_DECODER_EVENT_FLUSH_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT2_DONE:
            RIDEHAL_DEBUG( "FLUSH_OUTPUT2_DONE event" );
            m_eventCb( VIDEO_DECODER_EVENT_FLUSH_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            RIDEHAL_INFO( "output-reconfig received" );
            if ( RIDEHAL_ERROR_NONE != HandleOutputReconfig() )
            {
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
                RIDEHAL_ERROR( "handle output-reconfig failed" );
                m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
            }
            else
            {
                m_eventCb( VIDEO_DECODER_EVENT_OUTPUT_RECONFIG, pEvent, m_pAppPriv );
            }
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            // smooth streaming case - notifies about resolution change (not handled)
            RIDEHAL_ERROR( "not support info output-reconfig" );
            m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_HWFATAL:
            RIDEHAL_ERROR( "hw fatal" );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            RIDEHAL_ERROR( "client fatal" );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START:
            if ( RIDEHAL_COMPONENT_STATE_STARTING == m_state )
            {
                RIDEHAL_INFO( "Started vidc event" );
            }
            else
            {
                RIDEHAL_ERROR( "Started vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_START_INPUT_DONE:
            RIDEHAL_INFO( "START_INPUT_DONE event received" );
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            break;
        case VIDC_EVT_RESP_START_OUTPUT_DONE:
            RIDEHAL_INFO( "START_OUTPUT_DONE event received" );
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            m_OutputStarted = true;
            (void) FinishOutputReconfig();
            break;
        case VIDC_EVT_RESP_STOP:
            if ( RIDEHAL_COMPONENT_STATE_STOPING == m_state )
            {
                RIDEHAL_INFO( "Stopped vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_READY;
            }
            else
            {
                RIDEHAL_ERROR( "Stopped vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_PAUSE:
            if ( RIDEHAL_COMPONENT_STATE_PAUSING == m_state )
            {
                RIDEHAL_INFO( "Paused vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_PAUSE;
            }
            else
            {
                RIDEHAL_ERROR( "Paused vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_RESUME:
            if ( RIDEHAL_COMPONENT_STATE_RESUMING == m_state )
            {
                RIDEHAL_INFO( "Resumed vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            }
            else
            {
                RIDEHAL_ERROR( "Resumed vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_LOAD_RESOURCES:
            if ( RIDEHAL_COMPONENT_STATE_INITIALIZING == m_state )
            {
                RIDEHAL_INFO( "Loaded vidc resources event" );
                m_state = RIDEHAL_COMPONENT_STATE_READY;
            }
            else
            {
                RIDEHAL_ERROR( "Loaded vidc resources from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_RELEASE_RESOURCES:
            if ( RIDEHAL_COMPONENT_STATE_DEINITIALIZING == m_state )
            {
                RIDEHAL_INFO( "Released vidc resources event" );
                m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            }
            else
            {
                RIDEHAL_ERROR( "Released vidc resources from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RELEASE_BUFFER_REFERENCE:
            RIDEHAL_ERROR( "Release buffer reference event" );
            m_eventCb( VIDEO_DECODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        default:
            RIDEHAL_ERROR( "Unknown event_type: %d", pEvent->event_type );
            break;
    }
    return 0;
}

int VideoDecoder::DeviceCallback( uint8_t *msg, uint32_t length, void *cdata )
{
    VideoDecoder *self = (VideoDecoder *) cdata;
    return self->DeviceCbHandler( msg, length );
}

RideHalError_e VideoDecoder::ValidateConfig( const char *name, const VideoDecoder_Config_t *cfg )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == name || nullptr == cfg )
    {
        RIDEHAL_ERROR( "cfg is null pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        size_t len = strlen( name );
        if ( ( len < 2 ) || ( len > 16 ) )
        {
            RIDEHAL_ERROR( "name length %" PRIu32 " not in range [2, 16] ", len );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( cfg->width < 128 ) || ( cfg->height < 128 ) || ( cfg->width > 8192 ) ||
             ( cfg->height > 8192 ) )
        {
            RIDEHAL_ERROR( "width %" PRIu32 " height %" PRIu32 " not in [128, 8192] ", cfg->width,
                           cfg->height );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 != cfg->inFormat ) &&
         ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 != cfg->inFormat ) )
    {
        RIDEHAL_ERROR( "input format: %d not supported!", cfg->inFormat );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( cfg->numInputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
           ( cfg->numInputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numInputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       cfg->numInputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ,
                       VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_numInputBuffer = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( cfg->numOutputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
           ( cfg->numOutputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numOutputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       cfg->numOutputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ,
                       VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_numOutputBuffer = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == cfg->bInputDynamicMode ) &&
         ( nullptr != cfg->pInputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide inputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == cfg->bOutputDynamicMode ) &&
         ( nullptr != cfg->pOutputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide outputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e VideoDecoder::InitFromConfig( const char *name, const VideoDecoder_Config_t *cfg )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_Name = name;
        m_width = cfg->width;
        m_height = cfg->height;
        m_frameRate = ( cfg->frameRate > 0 ) ? cfg->frameRate : VIDEO_DECODER_DEFAULT_FRAME_RATE;

        m_inFormat = cfg->inFormat;
        m_bInputDynamicMode = cfg->bInputDynamicMode;
        m_bOutputDynamicMode = cfg->bOutputDynamicMode;
        m_numInputBuffer = cfg->numInputBuffer;
        m_numOutputBuffer = cfg->numOutputBuffer;

        /* now only support h264 and h265 */
        if ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 == m_inFormat )
        {
            ctx->codec = VIDC_CODEC_HEVC;
        }
        else
        {
            ctx->codec = VIDC_CODEC_H264;
        }

        ctx->sessionCodec.session = VIDC_SESSION_DECODE;
        ctx->sessionCodec.codec = ctx->codec;
    }

    return ret;
}

RideHalError_e VideoDecoder::ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                             VideoCodec_BufType bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "pBuffer is nullptr, invalid" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( pBuffer->imgProps.width != m_width ) || ( pBuffer->imgProps.height != m_height ) )
        {
            RIDEHAL_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32
                           " is not match m_width %" PRIu32 " m_height %" PRIu32,
                           pBuffer->imgProps.width, pBuffer->imgProps.height, m_width, m_height );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( bufferType == VIDEO_CODEC_BUF_TYPE_INPUT )
        {
            if ( pBuffer->imgProps.format != m_inFormat )
            {
                RIDEHAL_ERROR( "pBuffer format %d  is not match m_inFormat %d",
                               pBuffer->imgProps.format, m_inFormat );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
            else if ( 0 == pBuffer->size )
            {
                RIDEHAL_INFO( "pBuffer size %zu is invalid", pBuffer->size );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
        else
        {
            if ( pBuffer->imgProps.format != m_outFormat )
            {
                RIDEHAL_ERROR( "pBuffer format %d  is not match m_outFormat %d",
                               pBuffer->imgProps.format, m_outFormat );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
            else if ( (size_t) ctx->vidcOutputBufferSize > pBuffer->size )
            {
                RIDEHAL_ERROR( "pBuffer size %zu is smaller than vidcOutputBufferSize %" PRIu32,
                               pBuffer->size, ctx->vidcOutputBufferSize );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::GetDrvProperty( uint32_t id, uint32_t nPktSize, uint8_t *pPkt )
{
    int32_t rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint8_t dev_cmd_buffer[VIDEO_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;
    vidc_property_id_type propId = (vidc_property_id_type) id;
    ioctl_session_t *ioHandle = ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle;

    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE || nMsgSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE )
    {
        RIDEHAL_ERROR( "GetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, pPkt, nPktSize );
        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( ioHandle, VIDC_IOCTL_GET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           pPkt, nPktSize );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "GetDrvProperty propId=0x%x failed! rc=0x%x", propId, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::SetDrvProperty( uint32_t id, uint32_t nPktSize, uint8_t *pPkt )
{
    int32_t rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint8_t dev_cmd_buffer[VIDEO_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;
    vidc_property_id_type propId = (vidc_property_id_type) id;
    ioctl_session_t *ioHandle = ( (VidcDecoderContext_t *) m_vidcDecoderContext )->pIoHandle;

    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE || nMsgSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE )
    {
        RIDEHAL_ERROR( "SetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, pPkt, nPktSize );

        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( ioHandle, VIDC_IOCTL_SET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty propId=0x%x failed! rc=0x%x", propId, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    return ret;
}

RideHalError_e VideoDecoder::WaitForState( RideHal_ComponentState_t expectedState )
{
    int32_t counter = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    while ( m_state != expectedState )
    {
        (void) MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > WAIT_TIMEOUT_1_MSEC )
        {
            RIDEHAL_ERROR( "WaitForState timeout!" );
            ret = RIDEHAL_ERROR_TIMEOUT;
            break;
        }
    }
    return ret;
}

RideHalError_e VideoDecoder::InitBufferForNonDynamicMode( RideHal_SharedBuffer_t *pBufList,
                                                          VideoCodec_BufType bufferType )
{
    int32_t i;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    if ( VIDEO_CODEC_BUF_TYPE_INPUT == bufferType )
    {
        RIDEHAL_DEBUG( "Allocating %" PRIu32 " input buffers", m_numInputBuffer );
        m_pInputList = (RideHal_SharedBuffer_t *) malloc( m_numInputBuffer *
                                                          sizeof( RideHal_SharedBuffer_t ) );
        if ( nullptr == m_pInputList )
        {
            RIDEHAL_ERROR( "m_inputList malloc failed!" );
            ret = RIDEHAL_ERROR_NOMEM;
        }
        else
        {
            if ( m_bInputNonDynamicAppAllocBuffer )
            {
                for ( i = 0; i < m_numInputBuffer; i++ )
                {
                    m_pInputList[i] = pBufList[i];
                }
            }
        }
    }
    else if ( VIDEO_CODEC_BUF_TYPE_OUTPUT == bufferType )
    {
        RIDEHAL_DEBUG( "Allocating %" PRIu32 " output buffers", m_numOutputBuffer );
        m_pOutputList = (RideHal_SharedBuffer_t *) malloc( m_numOutputBuffer *
                                                           sizeof( RideHal_SharedBuffer_t ) );
        if ( nullptr == m_pOutputList )
        {
            RIDEHAL_ERROR( "m_outputList malloc failed!" );
            ret = RIDEHAL_ERROR_NOMEM;
        }
        else
        {
            if ( m_bOutputNonDynamicAppAllocBuffer )
            {
                for ( i = 0; i < m_numOutputBuffer; i++ )
                {
                    m_pOutputList[i] = pBufList[i];
                }
            }
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::AllocateBuffer( VideoCodec_BufType bufferType )
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;
    int32_t bufCnt, bufSize;

    if ( VIDEO_CODEC_BUF_TYPE_INPUT == bufferType )
    {
        bufCnt = m_numInputBuffer;
        bufSize = ctx->vidcInputBufferSize;
    }
    else if ( VIDEO_CODEC_BUF_TYPE_OUTPUT == bufferType )
    {
        bufCnt = m_numOutputBuffer;
        bufSize = ctx->vidcOutputBufferSize;
    }
    else
    {
        RIDEHAL_ERROR( "Wrong bufferType: 0x%x", bufferType );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "bufSize=%" PRId32 " bufCnt=%" PRId32, bufSize, bufCnt );

        for ( i = 0; i < bufCnt; i++ )
        {
            RideHal_SharedBuffer_t sharedBuffer;
            if ( VIDEO_CODEC_BUF_TYPE_INPUT == bufferType )
            {
                RideHal_ImageProps_t imgProps;
                imgProps.batchSize = 1;
                imgProps.width = m_width;
                imgProps.height = m_height;
                imgProps.compressedSize = bufSize;
                imgProps.format = m_inFormat;
                ret = sharedBuffer.Allocate( &imgProps );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate inputBuffer failed %d for index=%" PRId32, ret, i );
                }
                else
                {
                    m_pInputList[i] = sharedBuffer;
                    RIDEHAL_DEBUG( "m_pInputList[%" PRId32 "] 0x%x", i, &m_pInputList[i] );
                }
            }
            else if ( VIDEO_CODEC_BUF_TYPE_OUTPUT == bufferType )
            {
                ret = sharedBuffer.Allocate( m_width, m_height, m_outFormat );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Allocate outputBuffer failed %d for index=%" PRId32, ret, i );
                }
                else
                {
                    m_pOutputList[i] = sharedBuffer;
                    RIDEHAL_DEBUG( "m_pOutputList[%" PRId32 "] 0x%x", i, &m_pOutputList[i] );
                }
            }
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::SetBuffer( VideoCodec_BufType bufferType )
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    vidc_buffer_type vidcBufType;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;
    ioctl_session_t *ioHandle = ctx->pIoHandle;
    RideHal_SharedBuffer_t sharedBuffer;
    RideHal_SharedBuffer_t *bufList;
    int32_t bufCnt, bufSize;

    if ( VIDEO_CODEC_BUF_TYPE_INPUT == bufferType )
    {
        bufCnt = m_numInputBuffer;
        bufSize = ctx->vidcInputBufferSize;
        vidcBufType = VIDC_BUFFER_INPUT;
        bufList = m_pInputList;
    }
    else if ( VIDEO_CODEC_BUF_TYPE_OUTPUT == bufferType )
    {
        bufCnt = m_numOutputBuffer;
        bufSize = ctx->vidcOutputBufferSize;
        vidcBufType = VIDC_BUFFER_OUTPUT;
        bufList = m_pOutputList;
    }
    else
    {
        RIDEHAL_ERROR( "Wrong bufferType: 0x%x", bufferType );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( i = 0; i < bufCnt; i++ )
        {
            vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };
            (void) memset( &buf_info, 0, sizeof( vidc_buffer_info_type ) );

            sharedBuffer = bufList[i];

            buf_info.buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
            buf_info.buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
            buf_info.buf_handle = (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
            buf_info.buf_type = vidcBufType;
            buf_info.contiguous = true;
            buf_info.buf_size = bufSize;
            // register it before lookup in future for local allocation
            RIDEHAL_DEBUG( "set-buffer to driver [%" PRId32
                           "]: buf_addr = 0x%x, buf_handle = 0x%x, "
                           "buf_size = %d",
                           i, buf_info.buf_addr, buf_info.buf_handle, bufSize );
            // for non dynamic mode, need to set buffer to driver
            rc = device_ioctl( ioHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t *) ( &buf_info ),
                               sizeof( vidc_buffer_info_type ), nullptr, 0 );
            if ( VIDC_ERR_NONE != rc )
            {
                RIDEHAL_ERROR( " set-buffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32
                               " rc=0x%x",
                               i, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }

            if ( VIDEO_CODEC_BUF_TYPE_INPUT == bufferType )
            {
                VideoDecoder_InputInfo_t inputInfo;
                inputInfo.sharedBuffer = sharedBuffer;
                inputInfo.useFlag = false;
                m_inputMap[sharedBuffer.buffer.dmaHandle] = inputInfo;
            }
            else
            {
                VideoDecoder_OutputInfo_t outputInfo;
                outputInfo.sharedBuffer = sharedBuffer;
                outputInfo.useFlag = false;
                m_outputMap[sharedBuffer.buffer.dmaHandle] = outputInfo;
            }
        }
    }

    return ret;
}


RideHalError_e VideoDecoder::GetOutputInformation()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    ctx->colorFormatConfig.buf_type = VIDC_BUFFER_OUTPUT;
    ret = GetDrvProperty( VIDC_I_COLOR_FORMAT, sizeof( vidc_color_format_config_type ),
                          (uint8_t *) &ctx->colorFormatConfig );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ctx->planeDef.buf_type = VIDC_BUFFER_OUTPUT;
        ctx->planeDef.plane_index = 1;
        ret = GetDrvProperty( VIDC_I_PLANE_DEF, sizeof( vidc_plane_def_type ),
                              (uint8_t *) &ctx->planeDef );
    }
    else
    {
        RIDEHAL_ERROR( "get output color format failed" );
    }

    return ret;
}

RideHalError_e VideoDecoder::GetInputBufferRequirement()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    ctx->inputBufferReq.buf_type = VIDC_BUFFER_INPUT;
    ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                          (uint8_t *) ( &ctx->inputBufferReq ) );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "input-bufs: InputDynamic:%d, AppAlloc:%d; app req:%" PRIu32
                      ", driver need:%" PRIu32,
                      m_bInputDynamicMode, m_bInputNonDynamicAppAllocBuffer, m_numInputBuffer,
                      ctx->inputBufferReq.actual_count );

        if ( m_bInputDynamicMode || m_bInputNonDynamicAppAllocBuffer )
        {
            if ( ctx->inputBufferReq.actual_count > m_numInputBuffer )
            {
                RIDEHAL_ERROR( "input-bufs: app config count:%" PRIu32
                               ", but driver need count:%" PRIu32,
                               m_numInputBuffer, ctx->inputBufferReq.actual_count );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ctx->inputBufferReq.actual_count < m_numInputBuffer )
        {
            ctx->inputBufferReq.actual_count = m_numInputBuffer;
        }

        m_numInputBuffer = ctx->inputBufferReq.actual_count;
        ctx->vidcInputBufferSize = ctx->inputBufferReq.size;
        RIDEHAL_DEBUG( "input-bufs: count:%" PRIu32 ", size=%" PRIu32, m_numInputBuffer,
                       ctx->vidcInputBufferSize );

        ret = SetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &ctx->inputBufferReq ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &ctx->inputBufferReq ) );
        RIDEHAL_INFO( "input-bufs: config done, req:%" PRIu32 ", actual_count:%" PRIu32,
                      m_numInputBuffer, ctx->inputBufferReq.actual_count );
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( m_numInputBuffer != ctx->inputBufferReq.actual_count ) )
    {
        RIDEHAL_ERROR( "input-bufs: req:%" PRIu32 " and actual_count:%" PRIu32 " must be same!",
                       m_numInputBuffer, ctx->inputBufferReq.actual_count );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e VideoDecoder::GetOutputBufferRequirement()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    ctx->outputBufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                          (uint8_t *) ( &ctx->outputBufferReq ) );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "output-bufs: OutputDynamic:%d, AppAlloc:%d; app req:%" PRIu32
                      ", driver need:%" PRIu32,
                      m_bOutputDynamicMode, m_bOutputNonDynamicAppAllocBuffer, m_numOutputBuffer,
                      ctx->outputBufferReq.actual_count );

        if ( ctx->outputBufferReq.actual_count > m_numOutputBuffer )
        {
            RIDEHAL_ERROR( "output-bufs: app config count:%" PRIu32
                           ", but driver need count:%" PRIu32,
                           m_numOutputBuffer, ctx->outputBufferReq.actual_count );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ctx->outputBufferReq.actual_count < m_numOutputBuffer )
        {
            ctx->outputBufferReq.actual_count = m_numOutputBuffer;
        }

        m_numOutputBuffer = ctx->outputBufferReq.actual_count;
        ctx->vidcOutputBufferSize = ctx->outputBufferReq.size;
        RIDEHAL_DEBUG( "output-bufs: count:%" PRIu32 ", size:%" PRIu32, m_numOutputBuffer,
                       ctx->vidcOutputBufferSize );

        ret = SetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &ctx->outputBufferReq ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &ctx->outputBufferReq ) );
        RIDEHAL_INFO( "output-bufs: config done, req:%" PRIu32 ", actual_count:%" PRIu32,
                      m_numOutputBuffer, ctx->outputBufferReq.actual_count );
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( m_numOutputBuffer != ctx->outputBufferReq.actual_count ) )
    {
        RIDEHAL_ERROR( "output-bufs: req:%" PRIu32 " and actual_count:%" PRIu32 " must be same!",
                       m_numOutputBuffer, ctx->outputBufferReq.actual_count );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e VideoDecoder::FreeOutputBuffer()
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    vidc_buffer_info_type outbuf = { VIDC_BUFFER_UNUSED, 0 };
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    RIDEHAL_DEBUG( "FreeOutputBuffer:" );
    if ( nullptr != m_pOutputList )   // it means non dynamic mode
    {
        for ( i = 0; i < m_numOutputBuffer; i++ )
        {
            outbuf.buf_addr = (uint8_t *) m_pOutputList[i].data();
#if defined( __QNXNTO__ )
            outbuf.buf_handle = (pmem_handle_t) m_pOutputList[i].buffer.dmaHandle;
#else
            outbuf.buf_handle =
                    (int) reinterpret_cast<uint64_t>( m_pOutputList[i].buffer.dmaHandle );
#endif
            outbuf.buf_type = VIDC_BUFFER_OUTPUT;
            outbuf.contiguous = true;
            outbuf.buf_size = m_pOutputList[i].size;
            rc = device_ioctl( ctx->pIoHandle, VIDC_IOCTL_FREE_BUFFER, (uint8_t *) ( &outbuf ),
                               sizeof( vidc_buffer_info_type ), nullptr, 0 );
            if ( VIDC_ERR_NONE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "FreeOutputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32
                               " failed! rc=0x%x, %s",
                               i, rc, VidcErrToStr( vidc_status_type( rc ) ) );
            }
            if ( false == m_bOutputNonDynamicAppAllocBuffer )
            {
                // allocated in component, so free in component
                ret = m_pOutputList[i].Free();
            }
        }
        RIDEHAL_DEBUG( "Free m_pOutputList" );
        free( m_pOutputList );
        m_pOutputList = nullptr;
    }

    return ret;
}

RideHalError_e VideoDecoder::FreeInputBuffer()
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };
    VidcDecoderContext_t *ctx = (VidcDecoderContext_t *) m_vidcDecoderContext;

    RIDEHAL_DEBUG( "FreeInputBuffer:" );
    if ( nullptr != m_pInputList )   // it means non dynamic mode
    {
        for ( i = 0; i < m_numInputBuffer; i++ )
        {
            inbuf.buf_addr = (uint8_t *) m_pInputList[i].data();
#if defined( __QNXNTO__ )
            inbuf.buf_handle = (pmem_handle_t) m_pInputList[i].buffer.dmaHandle;
#else
            inbuf.buf_handle = (int) reinterpret_cast<uint64_t>( m_pInputList[i].buffer.dmaHandle );
#endif
            inbuf.buf_type = VIDC_BUFFER_INPUT;
            inbuf.contiguous = true;
            inbuf.buf_size = m_pInputList[i].size;
            rc = device_ioctl( ctx->pIoHandle, VIDC_IOCTL_FREE_BUFFER, (uint8_t *) ( &inbuf ),
                               sizeof( vidc_buffer_info_type ), nullptr, 0 );
            if ( VIDC_ERR_NONE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "FreeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32
                               " failed! rc=0x%x, %s",
                               i, rc, VidcErrToStr( vidc_status_type( rc ) ) );
            }
            if ( false == m_bInputNonDynamicAppAllocBuffer )
            {
                // allocated in component, so free in component
                ret = m_pInputList[i].Free();
            }
        }
        RIDEHAL_DEBUG( "Free m_pInputList" );
        free( m_pInputList );
        m_pInputList = nullptr;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
