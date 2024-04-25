// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_VIDEO_ENCODER_HPP_
#define _RIDEHAL_VIDEO_ENCODER_HPP_

#include "ridehal/component/ComponentIF.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>
#include <vidc_ioctl.h>
#include <vidc_types.h>


#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>

#endif

using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/** @brief This data type list the different rate control mode */
typedef enum
{
    VIDEO_ENCODER_RCM_CBR_CFR = VIDC_RATE_CONTROL_CBR_CFR,
    VIDEO_ENCODER_RCM_CBR_VFR = VIDC_RATE_CONTROL_CBR_VFR,
    VIDEO_ENCODER_RCM_VBR_CFR = VIDC_RATE_CONTROL_VBR_CFR,
    VIDEO_ENCODER_RCM_VBR_VFR = VIDC_RATE_CONTROL_VBR_VFR,
    VIDEO_ENCODER_RCM_UNUSED = VIDC_RATE_CONTROL_UNUSED
} VideoEncoder_RateControlMode_e;

/** @brief This data type list the different profile */
typedef enum
{
    VIDEO_ENCODER_PROFILE_H264_BASELINE = 0,
    VIDEO_ENCODER_PROFILE_H264_HIGH,
    VIDEO_ENCODER_PROFILE_H264_MAIN,
    VIDEO_ENCODER_PROFILE_HEVC_MAIN,
    VIDEO_ENCODER_PROFILE_HEVC_MAIN10,
    VIDEO_ENCODER_PROFILE_MAX
} VideoEncoder_Profile_e;

/** @brief This data type list the different frame types */
typedef enum
{
    VIDEO_ENCODER_FRAME_I = VIDC_FRAME_I,
    VIDEO_ENCODER_FRAME_P = VIDC_FRAME_P,
    VIDEO_ENCODER_FRAME_B = VIDC_FRAME_B,
    VIDEO_ENCODER_FRAME_IDR = VIDC_FRAME_IDR,
    VIDEO_ENCODER_FRAME_NOTCODED = VIDC_FRAME_NOTCODED,
    VIDEO_ENCODER_FRAME_YUV = VIDC_FRAME_YUV,
    VIDEO_ENCODER_FRAME_UNUSED = VIDC_FRAME_UNUSED
} VideoEncoder_FrameType_e;

/** @brief This data type list the different VideoEncoder Callback Event Type */
typedef enum
{
    VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE = 0,
    VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE,
    VIDEO_ENCODER_EVENT_ERROR = 0xf0000000
} VideoEncoder_EventType_e;

/** @brief This data type list the different VideoEncoder Property that can set dynamically */
typedef enum
{
    VIDEO_ENCODER_PROP_BITRATE = 0,
    VIDEO_ENCODER_PROP_FRAME_RATE,
} VideoEncoder_Prop_e;

/** @brief The VideoEncoder Init Config */
typedef struct
{
    uint32_t width;     /**< in pixels */
    uint32_t height;    /**< in pixels */
    uint32_t bitRate;   /**< bps */
    uint32_t gop;       /**< number of p frames in a period */
    uint32_t frameRate; /**< fps */
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;
    RideHal_SharedBuffer_t *pInputBufferList = nullptr; /**< set input buffer in non-dynamic mode */
    RideHal_SharedBuffer_t *pOutputBufferList =
            nullptr; /**< set output buffer in non-dynamic mode */
    VideoEncoder_RateControlMode_e rateControlMode;
    VideoEncoder_Profile_e profile;
    RideHal_ImageFormat_e inFormat;  /**< uncompressed type */
    RideHal_ImageFormat_e outFormat; /**< compressed type */
} VideoEncoder_Config_t;

/** @brief The VideoEncoder on-the-fly command */
typedef struct
{
    VideoEncoder_Prop_e propID;
    uint32_t value;
} VideoEncoder_OnTheFlyCmd_t;

/** @brief The VideoEncoder Input Frame */
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer;
    uint64_t timestampNs; /**< frame data's timestamp. */
    void *pAppMarkData;   /**< frame data's mark data, this data will be copied to corresponding
                          output   compressed frame's VideoEncoder_OutputFrame_t. API won't touch this
                          data,   only copy it. */
    VideoEncoder_OnTheFlyCmd_t *pOnTheFlyCmd =
            nullptr; /**< use to send on-the-fly commands to encoder, like intra refresh, bps reset
                     and so on. */
    uint32_t numCmd = 0; /**< number of on-the-fly commands. */
} VideoEncoder_InputFrame_t;

/** @brief The VideoEncoder Output Frame */
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer;
    uint64_t timestampNs;
    void *pAppMarkData;
    uint32_t frameFlag; /**< indicate whether some error occurred during encoding this frame. */
    VideoEncoder_FrameType_e frameType; /**< indicate it's I, P, B, or IDR frame. */
} VideoEncoder_OutputFrame_t;

/** @brief Store the data interacted with video core */
typedef struct
{
    ioctl_session_t *pIoHandle;           /**< The IOSession */
    vidc_session_codec_type sessionCodec; /**< Session & Codec type setting for encoder */
    vidc_frame_size_type frameSize;       /**< The frame resolution */
    vidc_frame_rate_type frameRate;       /**< Frame rate for Encoder */
    vidc_color_format_config_type
            colorFormatConfig;                /**< Configures the uncompressed Buffer format */
    vidc_plane_def_type planeDefY;            /**< Specifies layout of raw data for planeY */
    vidc_plane_def_type planeDefUV;           /**< Specifies layout of raw data for planeUV */
    vidc_buffer_reqmnts_type inputBufferReq;  /**< Get/Set input buffer requirements from vidc */
    vidc_buffer_reqmnts_type outputBufferReq; /**< Get/Set output buffer requirements from vidc */
    uint32_t vidcInputBufferSize;             /**< The size of inputBuffer */
    uint32_t vidcOutputBufferSize;            /**< The size of outputBuffer */
    vidc_rate_control_mode_type rateControl;  /**< The encoder rate control type */
    vidc_target_bitrate_type bitrate;         /**< The encoder target bitrate */
    vidc_codec_type codec;                    /**< The codec selection for Encoding */
    vidc_iperiod_type iPeriod;      /**< The data type to set I frame period pattern for encoder */
    vidc_idr_period_type idrPeriod; /**< The IDR frame periodicity within Intra coded frames */
    vidc_profile_type profile;      /**< The codec profile payload */
    vidc_level_type level;          /**< The codec level payload */
} VidcEncoderData_t;

/** @brief callback for input frame done */
typedef void ( *VideoEncoder_InFrameCallback_t )( const VideoEncoder_InputFrame_t *pInputFrame,
                                                  void *pPrivData );
/** @brief callback for output frame done */
typedef void ( *VideoEncoder_OutFrameCallback_t )( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                                   void *pPrivData );
/** @brief callback for event */
typedef void ( *VideoEncoder_EventCallback_t )( const VideoEncoder_EventType_e eventId,
                                                const void *pEvent, void *pPrivData );


/** @brief Top level control for interfacing with vidc based driver */
class VideoEncoder final : public ComponentIF
{
public:
    /** @brief Default constructor */
    VideoEncoder() = default;
    /** @brief Default destructor */
    ~VideoEncoder() = default;

    /**
     * @brief Init the video encoder
     * @param pName the video encoder unique instance name
     * @param pConfig pointer to the video config information
     * @param level the log level used by the video encoder, default is error
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Start the video encoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Stop the video encoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief deinitialize the video encoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @brief submit a video frame to VIDC driver for encoding
     * @param pInput pointer to hold the video frame information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e SubmitInputFrame( const VideoEncoder_InputFrame_t *pInput );

    /**
     * @brief submit a video frame back to VIDC driver
     * @param pOutput pointer to hold the video frame information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutput );

    /**
     * @brief get video input buffers to submit input in non-dynamic mode
     * @param pInputList pointer to hold the video input buffer list
     * @param num size of pInputList
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t num );

    /**
     * @brief get video output buffers to submit output in non-dynamic mode
     * @param pOutputList pointer to hold the video output buffer list
     * @param num size of pOutputList
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t num );

    /**
     * @brief set config dynamically to VIDC driver
     * @param pCmd pointer to the video config information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd );

    /**
     * @brief register callback
     * @param inputDoneCb input frame callback function
     * @param outputDoneCb output frame callback function
     * @param eventCb event callback function
     * @param pAppPriv app private data
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterCallback( VideoEncoder_InFrameCallback_t inputDoneCb,
                                     VideoEncoder_OutFrameCallback_t outputDoneCb,
                                     VideoEncoder_EventCallback_t eventCb, void *pAppPriv );

private:
    static int DeviceCallback( uint8_t *msg, uint32_t length, void *cdata );
    int DeviceCallback( uint8_t *msg, uint32_t length );

    RideHalError_e GetDrvProperty( ioctl_session_t *pIoHandle, vidc_property_id_type propId,
                                   uint32_t nPktSize, uint8_t *pPkt );
    RideHalError_e SetDrvProperty( ioctl_session_t *pIoHandle, vidc_property_id_type propId,
                                   uint32_t nPktSize, uint8_t *pPkt );
    RideHalError_e WaitForState( RideHal_ComponentState_t expectedState );
    RideHalError_e PrepareBuffer( ioctl_session_t *pIoHandle, RideHal_SharedBuffer_t *pBufferList,
                                  vidc_buffer_type bufferType, int32_t bufCntMin, int32_t bufSize );
    RideHalError_e GetInputInformation( void );
    RideHalError_e GetInputBufferRequirement( void );
    RideHalError_e GetOutputBufferRequirement( void );
    RideHalError_e FreeOutputBuffer( void );
    RideHalError_e FreeInputBuffer( void );
    void PrintEncoderConfig( void );
    vidc_color_format_type GetVidcFormat( RideHal_ImageFormat_e );
    RideHalError_e SetVidcProfileLevel( VideoEncoder_Profile_e );
    RideHalError_e ValidateConfig( const VideoEncoder_Config_t *pConfig );
    RideHalError_e ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                   vidc_buffer_type bufferType );

    VideoEncoder_InFrameCallback_t m_inputDoneCb = nullptr;
    VideoEncoder_OutFrameCallback_t m_outputDoneCb = nullptr;
    VideoEncoder_EventCallback_t m_eventCb = nullptr;
    void *m_pAppPriv = nullptr;

    VidcEncoderData_t m_vidcEncoderData{};
    ioctl_callback_t m_ioctlCb = { 0 };

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_bitRate = 0;
    uint32_t m_frameRate = 0;
    uint32_t m_numInputBufferReq = 0;
    uint32_t m_numOutputBufferReq = 0;
    bool m_bInputDynamicMode = true;
    bool m_bOutputDynamicMode = true;
    bool m_bInputConfigBuffer = false;
    bool m_bOutputConfigBuffer = false;
    RideHal_ImageFormat_e m_inFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    RideHal_ImageFormat_e m_outFormat = RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265;

    RideHal_SharedBuffer_t *m_pInputList =
            nullptr; /**< store input buffer that allocated inside videoencoder */
    RideHal_SharedBuffer_t *m_pOutputList =
            nullptr; /**< store output buffer that allocated inside videoencoder */

    typedef struct
    {
        RideHal_SharedBuffer_t sharedBuffer;
        uint64_t timestampNs;
        void *pAppMarkData;
        bool useFlag = false; /**< indicate whether sharedBuffer is using by driver or available */
    } VideoEncoder_InputInfo_t;

    typedef struct
    {
        RideHal_SharedBuffer_t sharedBuffer;
        bool useFlag = false; /**< indicate whether sharedBuffer is using by driver or available */
    } VideoEncoder_OutputInfo_t;

    std::mutex m_inLock;
    std::unordered_map<uint64_t, VideoEncoder_InputInfo_t> m_inputMap; /**< store input info */
    std::mutex m_outLock;
    std::unordered_map<uint64_t, VideoEncoder_OutputInfo_t> m_outputMap; /**< store output info */
};

}   // namespace component
}   // namespace ridehal

#endif   // _RIDEHAL_VIDEO_ENCODER_HPP_