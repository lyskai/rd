// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_VIDEO_ENCODER_HPP_
#define _RIDE_HAL_VIDEO_ENCODER_HPP_

#include "ridehal/component/ComponentIF.hpp"
#include <vidc_ioctl.h>
#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <vidc_client.h>
#endif

#include <boost/lockfree/queue.hpp>
#include <sys/uio.h>
#include <vidc_types.h>


using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/// @brief This data type list the different rate control mode
typedef enum
{
    VIDEO_ENCODER_RCM_CBR_CFR = VIDC_RATE_CONTROL_CBR_CFR,
    VIDEO_ENCODER_RCM_CBR_VFR = VIDC_RATE_CONTROL_CBR_VFR,
    VIDEO_ENCODER_RCM_VBR_CFR = VIDC_RATE_CONTROL_VBR_CFR,
    VIDEO_ENCODER_RCM_VBR_VFR = VIDC_RATE_CONTROL_VBR_VFR,
    VIDEO_ENCODER_RCM_UNUSED = VIDC_RATE_CONTROL_UNUSED
} VideoEncoder_RateControlMode_e;

/// @brief This data type list the different VideoEncoder state
typedef enum
{
    VIDEO_ENCODER_STATE_DEINIT = 0,
    VIDEO_ENCODER_STATE_LOADED,
    VIDEO_ENCODER_STATE_IDLE,
    VIDEO_ENCODER_STATE_EXECUTING,
    VIDEO_ENCODER_STATE_PAUSE,
    VIDEO_ENCODER_STATE_UNUSED = 0xf0000000
} VideoEncoder_State_e;

/// @brief This data type list the different frame types
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

/// @brief This data type list the different VideoEncoder Callback Event Type
typedef enum
{
    VIDEO_ENCODER_EVENT_FLUSH_INPUT_DONE = 0,
    VIDEO_ENCODER_EVENT_FLUSH_OUTPUT_DONE,
    VIDEO_ENCODER_EVENT_ERROR = 0xf0000000
} VideoEncoder_EventType_e;

/// @brief This data type list the different VideoEncoder Property that can set dynamically
typedef enum
{
    VIDEO_ENCODER_PROP_BITRATE = 0,
    VIDEO_ENCODER_PROP_FRAME_RATE,
} VideoEncoder_Prop_e;

/// @brief The VideoEncoder Init Config
typedef struct
{
    uint32_t width;       // pixel
    uint32_t height;      // pixel
    uint32_t bitRate;     // bps
    uint32_t gop;         // number of p frames in period
    uint32_t frameRate;   // fps
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;
    RideHal_SharedBuffer_t *inputBufferList = nullptr;    // set input buffer in non-dynamic mode
    RideHal_SharedBuffer_t *outputBufferList = nullptr;   // set output buffer in non-dynamic mode
    VideoEncoder_RateControlMode_e rateControlMode;
    RideHal_ImageFormat_e inFormat;    // uncompressed type
    RideHal_ImageFormat_e outFormat;   // compressed type
} VideoEncoder_Config_t;

/// @brief The VideoEncoder on-the-fly command
typedef struct
{
    VideoEncoder_Prop_e propID;
    uint32_t pValue;
} VideoEncoder_OnTheFlyCmd_t;

/// @brief The VideoEncoder Input Frame
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer;
    uint64_t timestampNs;   // frame data's timestamp.
    void *appMarkData;      // frame data's mark data, this data will be copied to corresponding
                            // output compressed frame's VideoEncoder_OutputFrame_t. API won't touch
                            // this data, only copy it.
    VideoEncoder_OnTheFlyCmd_t *onTheFlyCmd =
            nullptr;   // use to send on-the-fly commands to encoder, like
                       // intra refresh, bps reset and so on.
    uint32_t numCmd;   // number of on-the-fly commands
} VideoEncoder_InputFrame_t;

/// @brief The VideoEncoder Output Frame
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer;
    uint64_t timestampNs;
    void *appMarkData;
    uint32_t frameFlag;   // indicate whether some error occurred during encoding this frame, like
                          // overflow
    VideoEncoder_FrameType_e frameType;   // indicate it's I, P, B, or IDR frame
} VideoEncoder_OutputFrame_t;

typedef struct
{
    ioctl_session_t *ioHandle;
    VideoEncoder_State_e state;
    vidc_session_codec_type sessionCodec;
    vidc_frame_size_type frameSize;
    vidc_frame_rate_type frameRate;
    vidc_color_format_config_type colorFormatConfig;
    vidc_plane_def_type planeDefY;
    vidc_plane_def_type planeDefUV;
    vidc_buffer_reqmnts_type inputBufferReq;
    vidc_buffer_reqmnts_type outputBufferReq;
    uint32_t vidcInputBufferSize;
    uint32_t vidcOutputBufferSize;
    vidc_rate_control_mode_type rateControl;
    vidc_target_bitrate_type bitrate;
    vidc_codec_type codec;
    vidc_iperiod_type iPeriod;
    vidc_idr_period_type idrPeriod;
    vidc_profile_type profile;
    vidc_level_type level;
} VidcEncoderData_t;

#define DEFAULT_NUM_P_BET_2I 30
#define DEFAULT_NUM_B_BET_2I 0
#define DEFAULT_IDR_PERIOD 1
#define DEFAULT_BIT_RATE 64000
#define DEFAULT_FRAME_RATE 30

#define MAX_BUFFER_REQ 64
#define MIN_BUFFER_REQ 2

#define MAX_DEV_CMD_BUFFER_SIZE 256
#define WAIT_TIMEOUT_1_SEC 1000

typedef void ( *VideoEncoder_InFrameCallback_t )( const VideoEncoder_InputFrame_t *pInputFrame,
                                                  void *pPrivData );
typedef void ( *VideoEncoder_OutFrameCallback_t )( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                                   void *pPrivData );
typedef void ( *VideoEncoder_EventCallback_t )( const VideoEncoder_EventType_e eventId,
                                                const void *pEvent, void *pPrivData );


/**
 * @brief Top level control for interfacing with vidc based driver
 */
class VideoEncoder final : public ComponentIF
{
public:
    /// @brief Default constructor
    VideoEncoder() = default;
    /// @brief Default destructor
    ~VideoEncoder() = default;

    /// @brief Init the video encoder
    /// @param pName the video encoder unique instance name
    /// @param pConfig pointer to the video config information
    /// @param level the log level used by the video encoder, default is error
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Start the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

    /// @brief submit a video frame to VIDC driver for encoding
    /// @param pInputFrame pointer to hold the video frame information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SubmitInputFrame( const VideoEncoder_InputFrame_t *pInputFrame );

    /// @brief submit a video frame back to VIDC driver
    /// @param pOutputFrame pointer to the video frame information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutputFrame );

    /// @brief get video input buffers to submit input in non-dynamic mode
    /// @param pInputList pointer to hold the video input buffer list
    /// @param size size of pInputList
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t size );

    /// @brief get video output buffers to submit output in non-dynamic mode
    /// @param pOutputList pointer to hold the video output buffer list
    /// @param size size of pOutputList
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t size );


    /// @brief set config dynamically to VIDC driver
    /// @param pCmd pointer to the video config information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd );

    /// @brief register callback
    /// @param inputDoneCb input frame callback function
    /// @param outputDoneCb output frame callback function
    /// @param eventCb event callback function
    /// @param pAppPriv app private data
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterCallback( VideoEncoder_InFrameCallback_t inputDoneCb,
                                     VideoEncoder_OutFrameCallback_t outputDoneCb,
                                     VideoEncoder_EventCallback_t eventCb, void *pAppPriv );

private:
    static int DeviceCallback( uint8_t *msg, uint32_t length, void *cdata );
    int DeviceCallback( uint8_t *msg, uint32_t length );

    int32_t GetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                            uint32_t nPktSize, uint8_t *pPkt );
    int32_t SetDrvProperty( ioctl_session_t *ioHandle, vidc_property_id_type propId,
                            uint32_t nPktSize, uint8_t *pPkt );
    int32_t WaitForState( VideoEncoder_State_e expectedState );
    int32_t AllocateBuffer( ioctl_session_t *ioHandle, RideHal_SharedBuffer_t *bufferList,
                            vidc_buffer_type bufferType, int32_t bufCntMin, int32_t bufSize );
    int32_t GetInputInformation( void );
    int32_t GetInputBufferRequirement( void );
    int32_t GetOutputBufferRequirement( void );
    int32_t FreeOutputBuffer( void );
    int32_t FreeInputBuffer( void );
    void PrintEncoderConfig( void );
    vidc_color_format_type GetVidcFormat( RideHal_ImageFormat_e );
    int32_t Teardown(); /* release all the resources */

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
    uint32_t m_numInputBufferReq = 8;
    uint32_t m_numOutputBufferReq = 8;
    bool m_bInputDynamicMode = true;
    bool m_bOutputDynamicMode = true;
    RideHal_ImageFormat_e m_inFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    RideHal_ImageFormat_e m_outFormat = RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265;

    VideoEncoder_InputFrame_t *m_inputList;   /* store input */
    VideoEncoder_OutputFrame_t *m_outputList; /* store output */
    boost::lockfree::queue<uint16_t, boost::lockfree::fixed_sized<false>>
            m_availableInputQueue; /* store available input index*/
    boost::lockfree::queue<uint16_t, boost::lockfree::fixed_sized<false>>
            m_availableOutputQueue; /* store available output index*/
};

}   // namespace component
}   // namespace ridehal

#endif   // _RIDE_HAL_VIDEO_ENCODER_HPP_