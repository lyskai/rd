// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_VIDEO_ENCODER_HPP_
#define _RIDE_HAL_VIDEO_ENCODER_HPP_

#include "ridehal/component/ComponentIF.hpp"
#include <MMSignal.h>
#include <atomic>
#include <ioctlClient.h>
#include <map>
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <thread>
#include <unordered_map>
#include <vidc_types.h>


using namespace ridehal::common;

namespace ridehal
{
namespace component
{

typedef void ( *VideoEncoder_FrameCallback_t )( const RideHal_SharedBuffer_t *pSharedBuffer );

/// @brief The Rate Control Mode
typedef enum
{
    RIDE_HAL_CBR_CFR = 0,
    RIDE_HAL_VBR_CFR,
} VideoEncoder_RateControlMode_e;

typedef struct
{
    uint32_t width;   // TODO: add units
    uint32_t height;
    uint32_t bitRate;
    uint32_t gop;
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    float frameRate;
    VideoEncoder_RateControlMode_e rateControlMode;
    RideHal_ImageFormat_e format;
    VideoEncoder_FrameCallback_t inputBufferDoneCb;
    VideoEncoder_FrameCallback_t outputBufferDoneCb;
    bool bDynamicMode;
} VideoEncoder_Config_t;

/// @brief RideHal VideoEncoder state
typedef enum
{
    VIDEO_ENCODER_STATE_DEINIT = 0,
    VIDEO_ENCODER_STATE_LOADED = 1,
    VIDEO_ENCODER_STATE_IDLE = 2,
    VIDEO_ENCODER_STATE_EXECUTING = 3,
    VIDEO_ENCODER_STATE_PAUSE = 4,
    VIDEO_ENCODER_STATE_UNUSED = 0xf0000000
} VideoEncoder_State_t;

typedef struct   // TODO
{
    uint64_t timestampNs;   // frame data's timestamp. It will be copied to corresponding output
                            // compressed frame's VideoEncoder_InputInfo_t.
    void *appMarkData;      // frame data's mark data, this data will be copied to corresponding
                            // output compressed frame's VideoEncoder_OutputInfo_t. API won't touch
                            // this data, only copy it.
    struct onTheFlyCmd;     // use to send on-the-fly command to encoder, like intra refresh, bps
                            // reset and so on. Need discuss: pack such cmd into SubmitInputBuffer,
                            // or create a new API.
} VideoEncoder_InputInfo_t;

typedef struct   // TODO
{
    uint64_t timestampNs;
    void *appMarkData;
    int compressedFrameType;   // indicate it's I, P, B, IDR frame
    int seqheaderLength;       // indicate whether compressed frame data contain sequence header (if
                           // length is 0, means no sequence header) and sequence header data length
    int errorFlag;   // indicate whether some error occurred during encoding this frame, like
                     // overflow
    struct dataAttributes;   // store some encoder data attributes, like compressed frame's average
                             // qp.
} VideoEncoder_OutputInfo_t;

typedef struct
{
    ioctl_session_t *ioHandle;
    VideoEncoder_State_t state;
    MM_HANDLE lock;
    MM_HANDLE fbdLock;
    MM_HANDLE fbdNotification;
    MM_HANDLE fbdNotificationQ;
    // std::deque<vidc_frame_data_type *> fbdQ;
    MM_HANDLE specialEventNotification;
    MM_HANDLE specialEventNotificationQ;
    // ENCODER_STATUS currentStatus;
    vidc_buffer_info_type **vidcInputBufferInfo;
    vidc_buffer_info_type **vidcOutputBufferInfo;
    vidc_frame_data_type **vidcInputFrameInfo;
    vidc_frame_data_type **vidcOutputFrameInfo;
    vidc_session_codec_type sessionCodec;
    vidc_frame_size_type frameSize;
    vidc_frame_rate_type frameRate;
    vidc_color_format_config_type colorFormatConfig;
    vidc_plane_def_type planeDefY;
    vidc_plane_def_type planeDefUV;
    vidc_buffer_reqmnts_type inputBufferReq;
    vidc_buffer_reqmnts_type outputBufferReq;
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    uint32_t vidcInputBufferSize;
    uint32_t vidcOutputBufferSize;
    uint32_t numInputFrames;
    uint32_t numFbdFrames;
    vidc_rate_control_mode_type rateControl;
    vidc_target_bitrate_type bitrate;
    vidc_codec_type codec;
    vidc_iperiod_type iPeriod;
    vidc_idr_period_type idrPeriod;
    vidc_profile_type profile;
    vidc_level_type level;
} VidcEncoderData_t;

static constexpr uint8_t DEFAULT_VIDC_INPUT_BUFFER_REQ = 8;
static constexpr uint8_t DEFAULT_VIDC_OUTPUT_BUFFER_REQ = 8;

/**
 * @brief Top level control for interfacing with vidc based driver
 */
class VideoEncoder final : public ComponentIF
{
public:
    VideoEncoder();
    ~VideoEncoder();

    /// @brief Init the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const VideoEncoder_Config_t *pConfig, Logger *pLogger );

    /// @brief Start the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the video encoder
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

    /// @brief submit a video frame and timestamp to VIDC driver for encoding
    /// @param pSharedBuffer pointer to hold the video buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SubmitInputBuffer( const RideHal_SharedBuffer_t *pSharedBuffer,
                                      uint64_t timestampNs );

    /// @brief submit a video frame back to VIDC driver
    /// @param pSharedBuffer pointer to the video buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SubmitOutputBuffer( const RideHal_SharedBuffer_t *pSharedBuffer );

private:
    static int DeviceCallback( uint8_t *msg, uint32_t length, void *cdata );
    int DeviceCallback( uint8_t *msg, uint32_t length );

    int32_t getDrvProperty( ioctl_session_t *io_handle, vidc_property_id_type propId,
                            uint32_t nPktSize, uint8_t *pPkt );
    int32_t setDrvProperty( ioctl_session_t *io_handle, vidc_property_id_type propId,
                            uint32_t nPktSize, uint8_t *pPkt );
    int32_t waitForState( VideoEncoder_State_t expectedState );
    void freeFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt );
    int32_t allocateFrameData( vidc_frame_data_type ***pFrameData, int32_t frameCnt );
    int32_t allocateBuffer( ioctl_session_t *ioHandle, vidc_buffer_info_type ***pBufInfo,
                            vidc_buffer_type bufferType, int32_t bufCntMin, int32_t bufSize );
    int32_t fillBuffer( int32_t bufferId );
    int32_t getInputInformation( void );
    int32_t getInputBufferRequirement( void );
    int32_t getOutputBufferRequirement( void );
    int32_t freeOutputBuffer( void );
    int32_t freeInputBuffer( void );
    int emptyBufferDoneHandler( void );
    // int fillBufferDoneHandler( void );
    void printEncoderConfig( void );
    vidc_color_format_type convertFormat( RideHal_ImageFormat_e );
    bool teardown(); /* change teardown() from public to private */

    VideoEncoder_FrameCallback_t m_InputBufferDoneCb = nullptr;
    VideoEncoder_FrameCallback_t m_OutputBufferDoneCb = nullptr;

    VidcEncoderData_t m_VidcEncoderData{};
    ioctl_callback_t m_IoctlCb = { 0 };

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_FrameRate = 0;

    // std::thread m_FbdThread;

    vidc_color_format_type m_Format;

    std::mutex m_Mutex;
    std::map<uint16_t, RideHal_SharedBuffer_t> m_InputBufferMap;
    std::queue<uint16_t> m_AvailableInputBufferQueue;
    bool m_DynamicMode = true;
};

}   // namespace component
}   // namespace ridehal

#endif   // _RIDE_HAL_VIDEO_ENCODER_HPP_