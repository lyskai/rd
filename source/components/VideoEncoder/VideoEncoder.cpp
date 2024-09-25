// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>

#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoEncoder.hpp"

namespace ridehal
{
namespace component
{

#define VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I 30
#define VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I 0
#define VIDEO_ENCODER_DEFAULT_IDR_PERIOD 1
#define VIDEO_ENCODER_DEFAULT_BIT_RATE 64000
#define VIDEO_ENCODER_DEFAULT_FRAME_RATE 30

#define VIDEO_ENCODER_MAX_BUFFER_REQ 64
#define VIDEO_ENCODER_MIN_BUFFER_REQ 4
#define VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE 256
#define VIDEO_ENCODER_WAIT_TIMEOUT_1_SEC 1000

#define ARRAY_SIZE( a ) ( sizeof( ( a ) ) / sizeof( ( a )[0] ) )

typedef struct
{
    uint32_t maxFrameSize;
    uint32_t maxSizePerSec;
    uint32_t maxBitRate;
    uint32_t level;
    uint32_t profile;
} ProfileLevel_t;

typedef struct
{
    const ProfileLevel_t *pTable;
    uint32_t num;
} ProfileLevelTableRef_t;

static const ProfileLevel_t s_profileLevelH264BaseLineTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 64000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_BASELINE },
        { 99, 1485, 128000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_BASELINE },
        { 396, 3000, 192000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_BASELINE },
        { 396, 6000, 384000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_BASELINE },
        { 396, 11880, 768000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_BASELINE },
        { 396, 11880, 2000000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_BASELINE },
        { 792, 19800, 4000000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_BASELINE },
        { 1620, 20250, 4000000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_BASELINE },
        { 1620, 40500, 10000000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_BASELINE },
        { 3600, 108000, 14000000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_BASELINE },
        { 5120, 216000, 20000000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_BASELINE },
        { 8192, 245760, 20000000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_BASELINE },
        { 8192, 245760, 50000000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_BASELINE },
        { 8704, 522240, 50000000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_BASELINE },
        { 22080, 589824, 135000000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_BASELINE },
        { 36864, 983040, 240000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_BASELINE } };
static const ProfileLevel_t s_profileLevelH264HighTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 80000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_HIGH },
        { 99, 1485, 200000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_HIGH },
        { 396, 3000, 300000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_HIGH },
        { 396, 6000, 600000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_HIGH },
        { 396, 11880, 1200000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_HIGH },
        { 396, 11880, 3125000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_HIGH },
        { 792, 19800, 6250000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_HIGH },
        { 1620, 20250, 6250000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_HIGH },
        { 1620, 40500, 15625000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_HIGH },
        { 3600, 108000, 21875000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_HIGH },
        { 5120, 216000, 31250000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_HIGH },
        { 8192, 245760, 31250000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_HIGH },
        { 8192, 245760, 62500000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_HIGH },
        { 8704, 522240, 62500000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_HIGH },
        { 22080, 589824, 168750000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_HIGH },
        { 36864, 983040, 300000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_HIGH } };
static const ProfileLevel_t s_profileLevelH264MainTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 64000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_MAIN },
        { 99, 1485, 128000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_MAIN },
        { 396, 3000, 192000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_MAIN },
        { 396, 6000, 384000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_MAIN },
        { 396, 11880, 768000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_MAIN },
        { 396, 11880, 2000000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_MAIN },
        { 792, 19800, 4000000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_MAIN },
        { 1620, 20250, 4000000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_MAIN },
        { 1620, 40500, 10000000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_MAIN },
        { 3600, 108000, 14000000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_MAIN },
        { 5120, 216000, 20000000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_MAIN },
        { 8192, 245760, 20000000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_MAIN },
        { 8192, 245760, 50000000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_MAIN },
        { 8704, 522240, 50000000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_MAIN },
        { 22080, 589824, 135000000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_MAIN },
        { 36864, 983040, 240000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_MAIN } };
static const ProfileLevel_t s_profileLevelHevcMainTable[] = {
        /*max sample per frame, max sample per sec, max bitrate, level, profile*/
        { 36864, 552960, 128000, VIDC_LEVEL_HEVC_1, VIDC_PROFILE_HEVC_MAIN },
        { 122880, 3686440, 1500000, VIDC_LEVEL_HEVC_2, VIDC_PROFILE_HEVC_MAIN },
        { 245760, 7372800, 3000000, VIDC_LEVEL_HEVC_21, VIDC_PROFILE_HEVC_MAIN },
        { 552960, 16588800, 6000000, VIDC_LEVEL_HEVC_3, VIDC_PROFILE_HEVC_MAIN },
        { 983040, 33177600, 10000000, VIDC_LEVEL_HEVC_31, VIDC_PROFILE_HEVC_MAIN },
        { 2228224, 66846720, 12000000, VIDC_LEVEL_HEVC_4, VIDC_PROFILE_HEVC_MAIN },
        { 2228224, 133693440, 20000000, VIDC_LEVEL_HEVC_41, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 267386880, 25000000, VIDC_LEVEL_HEVC_5, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 534773760, 40000000, VIDC_LEVEL_HEVC_51, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 1069547520, 60000000, VIDC_LEVEL_HEVC_52, VIDC_PROFILE_HEVC_MAIN },
        { 35651584, 1069547520, 60000000, VIDC_LEVEL_HEVC_6, VIDC_PROFILE_HEVC_MAIN } };
static const ProfileLevel_t s_profileLevelHevcMain10Table[] = {
        /*max sample per frame, max sample per sec, max bitrate, level, profile*/
        { 36864, 552960, 128000, VIDC_LEVEL_HEVC_1, VIDC_PROFILE_HEVC_MAIN10 },
        { 122880, 3686440, 1500000, VIDC_LEVEL_HEVC_2, VIDC_PROFILE_HEVC_MAIN10 },
        { 245760, 7372800, 3000000, VIDC_LEVEL_HEVC_21, VIDC_PROFILE_HEVC_MAIN10 },
        { 552960, 16588800, 6000000, VIDC_LEVEL_HEVC_3, VIDC_PROFILE_HEVC_MAIN10 },
        { 983040, 33177600, 10000000, VIDC_LEVEL_HEVC_31, VIDC_PROFILE_HEVC_MAIN10 },
        { 2228224, 66846720, 12000000, VIDC_LEVEL_HEVC_4, VIDC_PROFILE_HEVC_MAIN10 },
        { 2228224, 133693440, 20000000, VIDC_LEVEL_HEVC_41, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 267386880, 25000000, VIDC_LEVEL_HEVC_5, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 534773760, 40000000, VIDC_LEVEL_HEVC_51, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 1069547520, 60000000, VIDC_LEVEL_HEVC_52, VIDC_PROFILE_HEVC_MAIN10 },
        { 35651584, 1069547520, 60000000, VIDC_LEVEL_HEVC_6, VIDC_PROFILE_HEVC_MAIN10 } };

static const ProfileLevelTableRef_t s_profileLevelTables[] = {
        { s_profileLevelH264BaseLineTable, ARRAY_SIZE( s_profileLevelH264BaseLineTable ) },
        { s_profileLevelH264HighTable, ARRAY_SIZE( s_profileLevelH264HighTable ) },
        { s_profileLevelH264MainTable, ARRAY_SIZE( s_profileLevelH264MainTable ) },
        { s_profileLevelHevcMainTable, ARRAY_SIZE( s_profileLevelHevcMainTable ) },
        { s_profileLevelHevcMain10Table, ARRAY_SIZE( s_profileLevelHevcMain10Table ) } };


RideHalError_e VideoEncoder::Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                                   Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i, rc = 0;

    ret = ComponentIF::Init( pName, level );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;
        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "pConfig is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_width = pConfig->width;
        m_height = pConfig->height;
        m_bitRate = ( pConfig->bitRate > 0 ) ? pConfig->bitRate : VIDEO_ENCODER_DEFAULT_BIT_RATE;
        m_frameRate =
                ( pConfig->frameRate > 0 ) ? pConfig->frameRate : VIDEO_ENCODER_DEFAULT_FRAME_RATE;
        m_vidcEncoderData.rateControl = (vidc_rate_control_mode_type) pConfig->rateControlMode;
        m_inFormat = pConfig->inFormat;
        m_vidcEncoderData.colorFormatConfig.color_format = GetVidcFormat( m_inFormat );
        m_outFormat = pConfig->outFormat;
        m_bInputDynamicMode = pConfig->bInputDynamicMode;
        m_bOutputDynamicMode = pConfig->bOutputDynamicMode;
        m_numInputBufferReq = pConfig->numInputBufferReq;
        m_numOutputBufferReq = pConfig->numOutputBufferReq;
        if ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat )
        {
            m_vidcEncoderData.codec = VIDC_CODEC_HEVC;
        }
        else if ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 == m_outFormat )
        {
            m_vidcEncoderData.codec = VIDC_CODEC_H264;
        }
        else
        {
            /* now only support h264 and h265; will check in ValidateConfig() */
        }
        m_vidcEncoderData.sessionCodec.session = VIDC_SESSION_ENCODE;
        m_vidcEncoderData.sessionCodec.codec = m_vidcEncoderData.codec;
        ret = ValidateConfig( pConfig );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SetVidcProfileLevel( pConfig->profile );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_ioctlCb.handler = VideoEncoder::DeviceCallback;
        m_ioctlCb.data = (void *) this;

        RIDEHAL_DEBUG( "Opening vidc device" );
        m_vidcEncoderData.pIoHandle = device_open( (char *) "VideoCore/vidc_drv", &m_ioctlCb );
        if ( nullptr == m_vidcEncoderData.pIoHandle )
        {
            RIDEHAL_ERROR( "Failed to open vidc device!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_SESSION_CODEC" );
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_SESSION_CODEC,
                              sizeof( vidc_session_codec_type ),
                              (uint8_t *) ( &m_vidcEncoderData.sessionCodec ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT" );
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
        m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
        m_vidcEncoderData.frameRate.fps_denominator = 1;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_RATE,
                              sizeof( vidc_frame_rate_type ),
                              (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_INPUT" );
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
        m_vidcEncoderData.frameRate.fps_denominator = 1;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_RATE,
                              sizeof( vidc_frame_rate_type ),
                              (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        m_vidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_COLOR_FORMAT,
                              sizeof( vidc_color_format_config_type ),
                              (uint8_t *) ( &m_vidcEncoderData.colorFormatConfig ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_INPUT" );
        m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.frameSize.width = m_width;
        m_vidcEncoderData.frameSize.height = m_height;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_SIZE,
                              sizeof( vidc_frame_size_type ),
                              (uint8_t *) ( &m_vidcEncoderData.frameSize ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_SIZE.VIDC_BUFFER_OUTPUT" );
        m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_OUTPUT;
        m_vidcEncoderData.frameSize.width = m_width;
        m_vidcEncoderData.frameSize.height = m_height;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_SIZE,
                              sizeof( vidc_frame_size_type ),
                              (uint8_t *) ( &m_vidcEncoderData.frameSize ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_INTRA_PERIOD" );
        m_vidcEncoderData.iPeriod.p_frames =
                ( pConfig->gop > 0 ) ? pConfig->gop : VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I;
        m_vidcEncoderData.iPeriod.b_frames = VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_ENC_INTRA_PERIOD,
                              sizeof( vidc_iperiod_type ),
                              (uint8_t *) ( &m_vidcEncoderData.iPeriod ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_IDR_PERIOD" );
        m_vidcEncoderData.idrPeriod.idr_period = VIDEO_ENCODER_DEFAULT_IDR_PERIOD;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_ENC_IDR_PERIOD,
                              sizeof( vidc_idr_period_type ),
                              (uint8_t *) ( &m_vidcEncoderData.idrPeriod ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_ENC_RATE_CONTROL" );
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_ENC_RATE_CONTROL,
                              sizeof( vidc_rate_control_mode_type ),
                              (uint8_t *) ( &m_vidcEncoderData.rateControl ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
        m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_TARGET_BITRATE,
                              sizeof( vidc_target_bitrate_type ),
                              (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_PROFILE" );
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_PROFILE,
                              sizeof( vidc_profile_type ),
                              (uint8_t *) ( &m_vidcEncoderData.profile ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_LEVEL" );
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_LEVEL, sizeof( vidc_level_type ),
                              (uint8_t *) ( &m_vidcEncoderData.level ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_VPE_SPATIAL_TRANSFORM" );
        vidc_spatial_transform_type vidcSpatialTransform = { VIDC_ROTATE_NONE, VIDC_FLIP_NONE };
        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_VPE_SPATIAL_TRANSFORM,
                              sizeof( vidc_spatial_transform_type ),
                              (uint8_t *) ( &vidcSpatialTransform ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetInputInformation();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetInputBufferRequirement();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetOutputBufferRequirement();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( false == m_bInputDynamicMode ) && ( nullptr != pConfig->pInputBufferList ) )
        {
            for ( i = 0; i < m_numInputBufferReq; i++ )
            {
                ret = ValidateBuffer( &pConfig->pInputBufferList[i], VIDC_BUFFER_INPUT );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "validate VIDC_BUFFER_INPUT failed" );
                    break;
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( false == m_bOutputDynamicMode ) && ( nullptr != pConfig->pOutputBufferList ) )
        {
            for ( i = 0; i < m_numOutputBufferReq; i++ )
            {
                ret = ValidateBuffer( &pConfig->pOutputBufferList[i], VIDC_BUFFER_OUTPUT );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "validate VIDC_BUFFER_OUTPUT failed" );
                    break;
                }
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
            ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                  sizeof( buffer_alloc_mode ), (uint8_t *) ( &buffer_alloc_mode ) );
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " input buffers", m_numInputBufferReq );
            m_pInputList = (RideHal_SharedBuffer_t *) malloc( m_numInputBufferReq *
                                                              sizeof( RideHal_SharedBuffer_t ) );
            if ( nullptr == m_pInputList )
            {
                RIDEHAL_ERROR( "m_inputList malloc failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                if ( nullptr != pConfig->pInputBufferList )
                {
                    m_bInputConfigBuffer = true;
                }
                ret = PrepareBuffer( m_vidcEncoderData.pIoHandle, pConfig->pInputBufferList,
                                     VIDC_BUFFER_INPUT, m_numInputBufferReq,
                                     m_vidcEncoderData.vidcInputBufferSize );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to allocate input buffers!" );
                }
                else
                {
                    for ( i = 0; i < m_numInputBufferReq; i++ )
                    {
                        VideoEncoder_InputInfo_t inputInfo;
                        inputInfo.sharedBuffer = m_pInputList[i];
                        inputInfo.bUseFlag = false;
                        m_inputMap[m_pInputList[i].buffer.dmaHandle] = inputInfo;
                    }
                }
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
            ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_ALLOC_MODE,
                                  sizeof( buffer_alloc_mode ), (uint8_t *) ( &buffer_alloc_mode ) );
        }
        else
        {
            RIDEHAL_DEBUG( "Allocating %" PRIu32 " output buffers", m_numOutputBufferReq );
            m_pOutputList = (RideHal_SharedBuffer_t *) malloc( m_numOutputBufferReq *
                                                               sizeof( RideHal_SharedBuffer_t ) );
            if ( nullptr == m_pOutputList )
            {
                RIDEHAL_ERROR( "m_outputList malloc failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                if ( nullptr != pConfig->pOutputBufferList )
                {
                    m_bOutputConfigBuffer = true;
                }
                ret = PrepareBuffer( m_vidcEncoderData.pIoHandle, pConfig->pOutputBufferList,
                                     VIDC_BUFFER_OUTPUT, m_numOutputBufferReq,
                                     m_vidcEncoderData.vidcOutputBufferSize );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to allocate output buffers!" );
                }
                else
                {
                    for ( i = 0; i < m_numOutputBufferReq; i++ )
                    {
                        VideoEncoder_OutputInfo_t outputInfo;
                        outputInfo.sharedBuffer = m_pOutputList[i];
                        outputInfo.bUseFlag = false;
                        m_outputMap[m_pOutputList[i].buffer.dmaHandle] = outputInfo;
                    }
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Loading vidc resources" );
        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0,
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Loading vidc resources failed! rc=0x%x", rc );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
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
        PrintEncoderConfig();
        RIDEHAL_DEBUG( "Successfully completed vidc initialization!" );
    }
    else
    {
        RIDEHAL_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

RideHalError_e VideoEncoder::Start()
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
        RIDEHAL_DEBUG( "Starting vidc" );
        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "Starting vidc failed! rc=0x%x", rc );
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_RUNNING );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "VIDC_IOCTL_START WaitForState.state_running failed!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( i = 0; i < m_numOutputBufferReq; i++ )
        {
            if ( false == m_bOutputDynamicMode )
            {
                VideoEncoder_OutputFrame_t outputFrame;
                outputFrame.sharedBuffer = m_pOutputList[i];
                ret = SubmitOutputFrame( &outputFrame );
            }
        }
    }

    RIDEHAL_INFO( "enc start done" );

    return ret;
}

RideHalError_e VideoEncoder::SubmitInputFrame( const VideoEncoder_InputFrame_t *pInput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t i, rc = 0;
    const RideHal_SharedBuffer_t *inputBuffer = nullptr;
    uint64_t handle = MAX_UINT64;
    vidc_frame_data_type frameData;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
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
        ret = ValidateBuffer( inputBuffer, VIDC_BUFFER_INPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        handle = inputBuffer->buffer.dmaHandle;

        RIDEHAL_DEBUG( "enc-input-start: bufHandle 0x%x markData %" PRIu64 " tsNs %" PRIu64
                       " addr 0x%x alloc_len %" PRIu32 " data_len %" PRIu32,
                       handle, pInput->appMarkData, pInput->timestampNs,
                       (uint8_t *) inputBuffer->data(), inputBuffer->buffer.size,
                       inputBuffer->size );

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
        if ( nullptr != pInput->pOnTheFlyCmd )
        {
            for ( i = 0; i < pInput->numCmd; i++ )
            {
                ret = Configure( &pInput->pOnTheFlyCmd[i] );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "config propID %d failed", pInput->pOnTheFlyCmd[i].propID );
                    break;
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_inLock );
        if ( true == m_bInputDynamicMode )
        {
            if ( m_inputMap.end() != m_inputMap.find( handle ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                if ( false == m_inputMap[handle].bUseFlag )
                {
                    m_inputMap[handle].bUseFlag = true;
                    m_inputMap[handle].timestampNs = pInput->timestampNs;
                    m_inputMap[handle].appMarkData = pInput->appMarkData;
                }
                else
                {
                    RIDEHAL_ERROR( "input buffer not available now!" );
                    ret = RIDEHAL_ERROR_NOMEM;
                }
            }
            else if ( m_inputMap.size() < m_numInputBufferReq )
            {
                m_inputMap[handle].bUseFlag = true;
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
                 ( false == m_inputMap[handle].bUseFlag ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                m_inputMap[handle].bUseFlag = true;
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
        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_EMPTY_INPUT_BUFFER,
                           (uint8_t *) ( &frameData ), sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER failed! rc=0x%x", rc );
            m_inputMap[handle].bUseFlag = false;
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

RideHalError_e VideoEncoder::SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;
    uint64_t handle = MAX_UINT64;
    vidc_frame_data_type frameData;
    const RideHal_SharedBuffer_t *outputBuffer = nullptr;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting outputBuffer since encoder is "
                      "shutting down!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
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
        ret = ValidateBuffer( outputBuffer, VIDC_BUFFER_OUTPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_outLock );
        if ( true == m_bOutputDynamicMode )
        {
            if ( m_outputMap.end() != m_outputMap.find( handle ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                if ( false == m_outputMap[handle].bUseFlag )
                {
                    m_outputMap[handle].bUseFlag = true;
                }
                else
                {
                    RIDEHAL_ERROR( "output buffer not available now!" );
                    ret = RIDEHAL_ERROR_NOMEM;
                }
            }
            else if ( m_outputMap.size() < m_numOutputBufferReq )
            {
                m_outputMap[handle].sharedBuffer = *outputBuffer;
                m_outputMap[handle].bUseFlag = true;
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
                 ( false == m_outputMap[handle].bUseFlag ) )
            {
                RIDEHAL_DEBUG( "find handle 0x%x", handle );
                m_outputMap[handle].bUseFlag = true;
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

        RIDEHAL_DEBUG(
                "FillBuffer, handle=0x%x , frameData.frame_addr=0x%x frameData.frame_handle=0x%x "
                "frameData.alloc_len %" PRIu32 " outputBuffer->size %" PRIu32,
                handle, frameData.frame_addr, frameData.frame_handle, frameData.alloc_len,
                outputBuffer->size );

        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER,
                           (uint8_t *) ( &frameData ), sizeof( vidc_frame_data_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SubmitOutputFrame FillBuffer 0x%x failed rc 0x%x", handle, rc );
            m_outputMap[handle].bUseFlag = false;
            ret = RIDEHAL_ERROR_FAIL;
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
        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
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

RideHalError_e VideoEncoder::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t rc = 0;

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
        rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0,
                           nullptr, 0 );
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
        if ( m_vidcEncoderData.pIoHandle != nullptr )
        {
            (void) device_close( m_vidcEncoderData.pIoHandle );
            m_vidcEncoderData.pIoHandle = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
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
        RIDEHAL_DEBUG( "Deinited done" );
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
    }

    return ret;
}

RideHalError_e VideoEncoder::GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetInputBuffers" );

    if ( nullptr != m_pInputList )   // it means non-dynamic mode
    {
        if ( nullptr == pInputList )
        {
            RIDEHAL_ERROR( "pInputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num == m_numOutputBufferReq )
        {
            for ( int i = 0; i < m_numInputBufferReq; i++ )
            {
                pInputList[i] = m_pInputList[i];
            }
        }
        else
        {
            RIDEHAL_ERROR( "the provided array size %" PRIu32
                           " too small to hold all buffer infos!",
                           num );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }
    else
    {
        RIDEHAL_ERROR( "input buffer is not ready!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e VideoEncoder::GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetOutputBuffers" );

    if ( nullptr != m_pOutputList )   // it means non-dynamic mode
    {
        if ( nullptr == pOutputList )
        {
            RIDEHAL_ERROR( "pOutputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num == m_numOutputBufferReq )
        {
            for ( int i = 0; i < m_numOutputBufferReq; i++ )
            {
                pOutputList[i] = m_pOutputList[i];
            }
        }
        else
        {
            RIDEHAL_ERROR( "the provided array size %" PRIu32
                           " too small to hold all buffer infos!",
                           num );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }
    else
    {
        RIDEHAL_ERROR( "output buffer is not ready!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e VideoEncoder::Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_READY != m_state ) )
    {
        RIDEHAL_WARN( "Not Configure since encoder is not ready or running!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( nullptr == pCmd ) )
    {
        RIDEHAL_ERROR( "pCmd is NULL pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Configuring vidc" );
        switch ( pCmd->propID )
        {
            case VIDEO_ENCODER_PROP_BITRATE:
                RIDEHAL_DEBUG( "Setting VIDC_I_TARGET_BITRATE %" PRIu32, m_bitRate );
                m_bitRate = pCmd->value;
                m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
                ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_TARGET_BITRATE,
                                      sizeof( vidc_target_bitrate_type ),
                                      (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
                break;
            case VIDEO_ENCODER_PROP_FRAME_RATE:
                m_frameRate = pCmd->value;
                RIDEHAL_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT %" PRIu32,
                               m_frameRate );
                m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
                m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
                m_vidcEncoderData.frameRate.fps_denominator = 1;
                ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_RATE,
                                      sizeof( vidc_frame_rate_type ),
                                      (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
                break;
            default:
                RIDEHAL_ERROR( "propID %d not supported", pCmd->propID );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    else if ( VIDC_CODEC_H264 == m_vidcEncoderData.codec )
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec = H264" );
        RIDEHAL_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        RIDEHAL_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    else
    {
        RIDEHAL_DEBUG( "EncoderConfig: Codec type = 0x%x", m_vidcEncoderData.codec );
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
        case RIDEHAL_IMAGE_FORMAT_NV12:
            ret = VIDC_COLOR_FORMAT_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12_UBWC:
            ret = VIDC_COLOR_FORMAT_NV12_UBWC;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            ret = VIDC_COLOR_FORMAT_NV12_P010;
            break;
        default:
            RIDEHAL_ERROR( "format %d not supported", format );
            ret = VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
    return ret;
}

RideHalError_e VideoEncoder::SetVidcProfileLevel( VideoEncoder_Profile_e profile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t i, num = 0;
    const ProfileLevel_t *pTable = nullptr;
    uint32_t mbPerFrame = 0, mbPerSec = 0;
    uint64_t samplePerFrame = 0, samplePerSec = 0;
    bool bFindFlag = false;
    m_vidcEncoderData.level.level = 0;
    m_vidcEncoderData.profile.profile = 0;

    if ( profile < VIDEO_ENCODER_PROFILE_MAX )
    {
        pTable = s_profileLevelTables[profile].pTable;
        num = s_profileLevelTables[profile].num;

        if ( ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 == m_outFormat ) &&
             ( profile <= VIDEO_ENCODER_PROFILE_H264_MAIN ) )
        {
            mbPerFrame = ( ( m_height + 15 ) >> 4 ) * ( ( m_width + 15 ) >> 4 );
            mbPerSec = mbPerFrame * m_frameRate;
            RIDEHAL_DEBUG( "mbPerFrame %" PRIu32 " mbPerSec %" PRIu32, mbPerFrame, mbPerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( mbPerFrame <= pTable[i].maxFrameSize )
                {
                    if ( mbPerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_bitRate <= pTable[i].maxBitRate )
                        {
                            RIDEHAL_DEBUG( "set level = %" PRIu32 ", profile = %" PRIu32,
                                           pTable[i].level, pTable[i].profile );
                            m_vidcEncoderData.level.level = pTable[i].level;
                            m_vidcEncoderData.profile.profile = pTable[i].profile;
                        }
                    }
                }
            }
        }
        else if ( ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat ) &&
                  ( profile >= VIDEO_ENCODER_PROFILE_HEVC_MAIN ) )
        {
            samplePerFrame = (uint64_t) m_height * m_width;
            samplePerSec = samplePerFrame * m_frameRate;
            RIDEHAL_DEBUG( "samplePerFrame %" PRIu64 " samplePerSec %" PRIu64, samplePerFrame,
                           samplePerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( samplePerFrame <= pTable[i].maxFrameSize )
                {
                    if ( samplePerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_bitRate <= pTable[i].maxBitRate )
                        {
                            RIDEHAL_DEBUG( "set level = %" PRIu32 ", profile = %" PRIu32,
                                           pTable[i].level, pTable[i].profile );
                            m_vidcEncoderData.level.level = pTable[i].level;
                            m_vidcEncoderData.profile.profile = pTable[i].profile;
                        }
                    }
                }
            }
        }
        else
        {
            RIDEHAL_ERROR( "m_outFormat %d not match with profile %d", m_outFormat, profile );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( ( 0 == m_vidcEncoderData.level.level ) || ( 0 == m_vidcEncoderData.profile.profile ) )
    {
        RIDEHAL_ERROR( "profile %d not supported", profile );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
                VideoEncoder_InputInfo_t *pInputInfo = nullptr;
                {
                    std::unique_lock<std::mutex> lck( m_inLock );
                    if ( m_inputMap.end() != m_inputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_inputMap[pFrameData->frm_clnt_data].bUseFlag = false;
                        pInputInfo = &m_inputMap[pFrameData->frm_clnt_data];
                    }
                }
                if ( nullptr != pInputInfo )
                {
                    VideoEncoder_InputFrame_t inputFrame;
                    inputFrame.sharedBuffer = pInputInfo->sharedBuffer;
                    inputFrame.timestampNs = pInputInfo->timestampNs;
                    inputFrame.appMarkData = pInputInfo->appMarkData;
                    m_inputDoneCb( &inputFrame, m_pAppPriv );
                }
                else
                {
                    RIDEHAL_ERROR( "input handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }
            RIDEHAL_DEBUG( "enc-input-done: bufHandle 0x%x", pFrameData->frm_clnt_data );
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            pFrameData = &pEvent->payload.frame_data;
            RIDEHAL_DEBUG( "enc-encode-done: bufHandle 0x%x ts %" PRIu64
                           "us size %u frameType %" PRIu64 " "
                           "pFrameData %p frame_addr %p flag 0x%x ",
                           pFrameData->frm_clnt_data, pFrameData->timestamp, pFrameData->data_len,
                           pFrameData->frame_type, pFrameData, pFrameData->frame_addr,
                           pFrameData->flags );

            if ( nullptr != pFrameData->frame_addr )
            {
                VideoEncoder_OutputInfo_t *pOutputInfo = nullptr;
                {
                    std::unique_lock<std::mutex> lck( m_outLock );
                    if ( m_outputMap.end() != m_outputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_outputMap[pFrameData->frm_clnt_data].bUseFlag = false;
                        pOutputInfo = &m_outputMap[pFrameData->frm_clnt_data];
                    }
                }
                if ( nullptr != pOutputInfo )
                {
                    if ( (uint8_t *) pOutputInfo->sharedBuffer.data() == pFrameData->frame_addr )
                    {
                        VideoEncoder_OutputFrame_t outputFrame;
                        outputFrame.sharedBuffer = pOutputInfo->sharedBuffer;
                        outputFrame.appMarkData = (uint64_t) pFrameData->mark_data;
                        outputFrame.sharedBuffer.size = pFrameData->data_len;
                        outputFrame.frameType = (VideoEncoder_FrameType_e) pFrameData->frame_type;
                        outputFrame.timestampNs =
                                pFrameData->timestamp * 1000;   // convert us to ns
                        outputFrame.frameFlag = pFrameData->flags;
                        RIDEHAL_DEBUG(
                                "Encoded frame is ready - calling callback, markData: %" PRIu64,
                                outputFrame.appMarkData );
                        m_outputDoneCb( &outputFrame, m_pAppPriv );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "output handle = 0x%x, frame_addr %p is not match with "
                                       "buffer data %p",
                                       pFrameData->frm_clnt_data, pFrameData->frame_addr,
                                       (uint8_t *) pOutputInfo->sharedBuffer.data() );
                        m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                    }
                }
                else
                {
                    RIDEHAL_ERROR( "output handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
                }
            }

            if ( ( pFrameData->flags & VIDC_FRAME_FLAG_EOS ) > 0 )
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
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START:
            if ( RIDEHAL_COMPONENT_STATE_STARTING == m_state )
            {
                RIDEHAL_DEBUG( "Started vidc" );
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            }
            else
            {
                RIDEHAL_ERROR( "Started vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RESP_STOP:
            if ( RIDEHAL_COMPONENT_STATE_STOPING == m_state )
            {
                RIDEHAL_DEBUG( "Stopped vidc" );
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
                RIDEHAL_DEBUG( "Paused vidc" );
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
                RIDEHAL_DEBUG( "Resumed vidc" );
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
                RIDEHAL_DEBUG( "Loaded vidc resources" );
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
                RIDEHAL_DEBUG( "Released vidc resources" );
                m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            }
            else
            {
                RIDEHAL_ERROR( "Released vidc resources from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDC_EVT_RELEASE_BUFFER_REFERENCE:
            m_eventCb( VIDEO_ENCODER_EVENT_ERROR, pEvent, m_pAppPriv );
            break;
        default:
            RIDEHAL_ERROR( "Unknown event_type: %d", pEvent->event_type );
            break;
    }
    return 0;
}

int VideoEncoder::DeviceCallback( uint8_t *msg, uint32_t length, void *cdata )
{
    VideoEncoder *self = (VideoEncoder *) cdata;
    return self->DeviceCallback( msg, length );
}

RideHalError_e VideoEncoder::ValidateConfig( const VideoEncoder_Config_t *pConfig )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( m_width < 128 ) || ( m_height < 128 ) || ( m_width > 8192 ) || ( m_height > 8192 ) )
    {
        RIDEHAL_ERROR( "m_width %" PRIu32 " m_height %" PRIu32 " not in range!", m_width,
                       m_height );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( VIDC_RATE_CONTROL_UNUSED == m_vidcEncoderData.rateControl ) )
    {
        RIDEHAL_ERROR( "rate control mode: 0x%x not supported!", pConfig->rateControlMode );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( VIDC_COLOR_FORMAT_UNUSED == m_vidcEncoderData.colorFormatConfig.color_format ) )
    {
        RIDEHAL_ERROR( "input format: %d not supported!", m_inFormat );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 != m_outFormat ) &&
         ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 != m_outFormat ) )
    {
        RIDEHAL_ERROR( "output format: %d not supported!", m_outFormat );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( pConfig->numInputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
           ( pConfig->numInputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numInputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       pConfig->numInputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                       VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_numInputBufferReq = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( pConfig->numOutputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
           ( pConfig->numOutputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numOutputBufferReq: %" PRIu32
                       " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       pConfig->numOutputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                       VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_numOutputBufferReq = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == m_bInputDynamicMode ) &&
         ( nullptr != pConfig->pInputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide inputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == m_bOutputDynamicMode ) &&
         ( nullptr != pConfig->pOutputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide outputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e VideoEncoder::ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                             vidc_buffer_type bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( ( pBuffer->imgProps.width != m_width ) || ( pBuffer->imgProps.height != m_height ) )
    {
        RIDEHAL_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32 " is not match m_width %" PRIu32
                       " m_height %" PRIu32,
                       pBuffer->imgProps.width, pBuffer->imgProps.height, m_width, m_height );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }

    if ( bufferType == VIDC_BUFFER_INPUT )
    {
        if ( pBuffer->imgProps.format != m_inFormat )
        {
            RIDEHAL_ERROR( "pBuffer format %d  is not match m_inFormat %d",
                           pBuffer->imgProps.format, m_inFormat );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
        if ( ( pBuffer->imgProps.stride[0] != m_vidcEncoderData.planeDefY.actual_stride ) ||
             ( pBuffer->imgProps.stride[1] != m_vidcEncoderData.planeDefUV.actual_stride ) )
        {
            RIDEHAL_ERROR( "pBuffer stride [%" PRIu32 "][%" PRIu32
                           "] is not same with actual strid [%" PRIu32 "][%" PRIu32 "] ",
                           pBuffer->imgProps.stride[0], pBuffer->imgProps.stride[1],
                           m_vidcEncoderData.planeDefY.actual_stride,
                           m_vidcEncoderData.planeDefUV.actual_stride );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
        if ( (size_t) m_vidcEncoderData.vidcInputBufferSize > pBuffer->size )
        {
            RIDEHAL_ERROR( "pBuffer size %zu is smaller than vidcInputBufferSize %" PRIu32,
                           pBuffer->size, m_vidcEncoderData.vidcInputBufferSize );
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
    }

    return ret;
}

RideHalError_e VideoEncoder::GetDrvProperty( ioctl_session_t *pIoHandle,
                                             vidc_property_id_type propId, uint32_t nPktSize,
                                             uint8_t *pPkt )
{
    int32_t rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint8_t dev_cmd_buffer[VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;
    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE ||
         nMsgSize > VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE )
    {
        RIDEHAL_ERROR( "SetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, pPkt, nPktSize );
        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( pIoHandle, VIDC_IOCTL_GET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           pPkt, nPktSize );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "GetDrvProperty propId=0x%x failed! rc=0x%x", propId, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e VideoEncoder::SetDrvProperty( ioctl_session_t *pIoHandle,
                                             vidc_property_id_type propId, uint32_t nPktSize,
                                             uint8_t *pPkt )
{
    int32_t rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint8_t dev_cmd_buffer[VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;

    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE ||
         nMsgSize > VIDEO_ENCODER_MAX_DEV_CMD_BUFFER_SIZE )
    {
        RIDEHAL_ERROR( "SetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, pPkt, nPktSize );

        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( pIoHandle, VIDC_IOCTL_SET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            RIDEHAL_ERROR( "SetDrvProperty propId=0x%x failed! rc=0x%x", propId, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    return ret;
}

RideHalError_e VideoEncoder::WaitForState( RideHal_ComponentState_t expectedState )
{
    int32_t counter = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    while ( m_state != expectedState )
    {
        (void) MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > VIDEO_ENCODER_WAIT_TIMEOUT_1_SEC )
        {
            RIDEHAL_ERROR( "WaitForState timeout!" );
            ret = RIDEHAL_ERROR_TIMEOUT;
            break;
        }
    }
    return ret;
}

RideHalError_e VideoEncoder::PrepareBuffer( ioctl_session_t *pIoHandle,
                                            RideHal_SharedBuffer_t *pBufferList,
                                            vidc_buffer_type bufferType, int32_t bufCntMin,
                                            int32_t bufSize )
{
    int32_t i, rc = 0;
    int32_t nMsgSize = sizeof( vidc_buffer_info_type );
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "bufSize=%" PRId32 " bufCntMin=%" PRId32, bufSize, bufCntMin );
    for ( i = 0; i < bufCntMin; i++ )
    {
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };
        (void) memset( &buf_info, 0, sizeof( vidc_buffer_info_type ) );

        RideHal_SharedBuffer_t sharedBuffer;
        if ( VIDC_BUFFER_INPUT == bufferType )
        {
            if ( nullptr == pBufferList )
            {
                ret = sharedBuffer.Allocate( m_width, m_height, m_inFormat );
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
            else
            {
                sharedBuffer = pBufferList[i];
                m_pInputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_pInputList[%" PRId32 "] 0x%x", i, &m_pInputList[i] );
            }
        }
        else if ( VIDC_BUFFER_OUTPUT == bufferType )
        {
            if ( nullptr == pBufferList )
            {
                RideHal_ImageProps_t imgProps;
                imgProps.batchSize = 1;
                imgProps.width = m_width;
                imgProps.height = m_height;
                imgProps.numPlanes = 1;
                imgProps.planeBufSize[0] = bufSize;
                imgProps.format = m_outFormat;
                ret = sharedBuffer.Allocate( &imgProps );
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
            else
            {
                sharedBuffer = pBufferList[i];
                m_pOutputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_pOutputList[%" PRId32 "] 0x%x", i, &m_pOutputList[i] );
            }
        }
        else
        {
            RIDEHAL_ERROR( "Wrong bufferType: 0x%x", bufferType );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            buf_info.buf_addr = (uint8_t *) sharedBuffer.data();
#if defined( __QNXNTO__ )
            buf_info.buf_handle = (pmem_handle_t) sharedBuffer.buffer.dmaHandle;
#else
            buf_info.buf_handle = (int) reinterpret_cast<uint64_t>( sharedBuffer.buffer.dmaHandle );
#endif
            buf_info.buf_type = bufferType;
            buf_info.contiguous = true;
            buf_info.buf_size = bufSize;
            // register it before lookup in future for local allocation
            RIDEHAL_DEBUG( "PrepareBuffer [%" PRId32 "]: buf_addr = 0x%x, buf_handle = 0x%x, "
                           "buf_size = %d",
                           i, buf_info.buf_addr, buf_info.buf_handle, bufSize );
            rc = device_ioctl( pIoHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t *) ( &buf_info ),
                               nMsgSize, nullptr, 0 );
            if ( VIDC_ERR_NONE != rc )
            {
                RIDEHAL_ERROR( " PrepareBuffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32
                               " rc=0x%x",
                               i, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            break;
        }
    }

    return ret;
}

RideHalError_e VideoEncoder::GetInputInformation()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_vidcEncoderData.frameSize.buf_type = VIDC_BUFFER_INPUT;
    ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_SIZE,
                          sizeof( vidc_frame_size_type ),
                          (uint8_t *) ( &m_vidcEncoderData.frameSize ) );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_FRAME_RATE,
                              sizeof( vidc_frame_rate_type ),
                              (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
    }
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_vidcEncoderData.colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_COLOR_FORMAT,
                              sizeof( vidc_color_format_config_type ),
                              (uint8_t *) ( &m_vidcEncoderData.colorFormatConfig ) );
    }
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_vidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefY.plane_index = 1;
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_PLANE_DEF,
                              sizeof( vidc_plane_def_type ),
                              (uint8_t *) ( &m_vidcEncoderData.planeDefY ) );
    }
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_vidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefUV.plane_index = 2;
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_PLANE_DEF,
                              sizeof( vidc_plane_def_type ),
                              (uint8_t *) ( &m_vidcEncoderData.planeDefUV ) );
    }

    return ret;
}

RideHalError_e VideoEncoder::GetInputBufferRequirement()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_vidcEncoderData.inputBufferReq.buf_type = VIDC_BUFFER_INPUT;
    ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                          sizeof( vidc_buffer_reqmnts_type ),
                          (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_vidcEncoderData.inputBufferReq.actual_count < m_numInputBufferReq )
        {
            m_vidcEncoderData.inputBufferReq.actual_count = m_numInputBufferReq;
        }

        m_numInputBufferReq = m_vidcEncoderData.inputBufferReq.actual_count;
        m_vidcEncoderData.vidcInputBufferSize = m_vidcEncoderData.inputBufferReq.size;
        RIDEHAL_DEBUG( "input-bufs: count=%" PRIu32 ", size=%" PRIu32, m_numInputBufferReq,
                       m_vidcEncoderData.vidcInputBufferSize );

        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                              sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                              sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &m_vidcEncoderData.inputBufferReq ) );
    }

    return ret;
}

RideHalError_e VideoEncoder::GetOutputBufferRequirement()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_vidcEncoderData.outputBufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                          sizeof( vidc_buffer_reqmnts_type ),
                          (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_vidcEncoderData.outputBufferReq.actual_count < m_numOutputBufferReq )
        {
            m_vidcEncoderData.outputBufferReq.actual_count = m_numOutputBufferReq;
        }

        m_numOutputBufferReq = m_vidcEncoderData.outputBufferReq.actual_count;
        m_vidcEncoderData.vidcOutputBufferSize = m_vidcEncoderData.outputBufferReq.size;
        RIDEHAL_DEBUG( "output-bufs: count=%" PRIu32 ", size=%" PRIu32, m_numOutputBufferReq,
                       m_vidcEncoderData.vidcOutputBufferSize );

        ret = SetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                              sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetDrvProperty( m_vidcEncoderData.pIoHandle, VIDC_I_BUFFER_REQUIREMENTS,
                              sizeof( vidc_buffer_reqmnts_type ),
                              (uint8_t *) ( &m_vidcEncoderData.outputBufferReq ) );
    }

    return ret;
}

RideHalError_e VideoEncoder::FreeOutputBuffer()
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    vidc_buffer_info_type outbuf = { VIDC_BUFFER_UNUSED, 0 };

    RIDEHAL_DEBUG( "FreeOutputBuffer:" );
    if ( nullptr != m_pOutputList )   // it means non dynamic mode
    {
        for ( i = 0; i < m_numOutputBufferReq; i++ )
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
            rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_FREE_BUFFER,
                               (uint8_t *) ( &outbuf ), sizeof( vidc_buffer_info_type ), nullptr,
                               0 );
            if ( VIDC_ERR_NONE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "FreeOutputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32
                               " failed! rc=0x%x",
                               i, rc );
            }
            if ( false == m_bOutputConfigBuffer )
            {
                ret = m_pOutputList[i].Free();
            }
        }
        RIDEHAL_DEBUG( "Free m_pOutputList" );
        free( m_pOutputList );
        m_pOutputList = nullptr;
    }

    return ret;
}

RideHalError_e VideoEncoder::FreeInputBuffer()
{
    int32_t i, rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    vidc_buffer_info_type inbuf = { VIDC_BUFFER_UNUSED, 0 };
    RIDEHAL_DEBUG( "FreeInputBuffer:" );
    if ( nullptr != m_pInputList )   // it means non dynamic mode
    {
        for ( i = 0; i < m_numInputBufferReq; i++ )
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
            rc = device_ioctl( m_vidcEncoderData.pIoHandle, VIDC_IOCTL_FREE_BUFFER,
                               (uint8_t *) ( &inbuf ), sizeof( vidc_buffer_info_type ), nullptr,
                               0 );
            if ( VIDC_ERR_NONE != rc )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "FreeInputBuffer VIDC_IOCTL_FREE_BUFFER index=%" PRIu32
                               " failed! rc=0x%x",
                               i, rc );
            }
            if ( false == m_bInputConfigBuffer )
            {
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
