// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_TYPES_HPP_
#define _RIDE_HAL_TYPES_HPP_

#include <cinttypes>
#include <cstddef>

namespace ridehal
{
namespace common
{

#define RIDE_HAL_NUM_IMAGE_PLANES ( 4 )
#define RIDE_HAL_NUM_DIMS ( 8 )

#ifndef RIDE_HAL_MAX_INPUTS
#define RIDE_HAL_MAX_INPUTS 32
#endif

/* Allocate uncached memory (default).*/
#define RIDE_HAL_BUFFER_FLAGS_CACHE_NONE (RideHal_BufferFlags_t) 0x00000000U

/* Allocate write-back, write-allocate memory, to be used if IP block is
 * coherent with CPU cache. */
#define RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA (RideHal_BufferFlags_t) 0x0000001U

#define RIDE_HAL_BUFFER_FLAGS_CACHE_MASK (RideHal_BufferFlags_t) 0x0000000FU

/// @brief RideHal Errors
///
/// Errors for Component API call
typedef enum
{
    RIDE_HAL_ERROR_NONE = 0,          ///<  No error.
    RIDE_HAL_ERROR_BAD_ARGUMENTS,     ///<  Bad arguments.
    RIDE_HAL_ERROR_BAD_OUTPUT,        ///<  Bad output.
    RIDE_HAL_ERROR_NULL_PTR,          ///<  NULL pointer.
    RIDE_HAL_ERROR_TYPE,              ///<  Error type.
    RIDE_HAL_ERROR_UNKNOWN,           ///<  Unknown error.
    RIDE_HAL_ERROR_FAIL,              ///<  Failure.
    RIDE_HAL_ERROR_NORES,             ///<  No resource of insufficient resource.
    RIDE_HAL_ERROR_INVALID_BUF,       ///<  Invalid buffer.
    RIDE_HAL_ERROR_UNSUPPORTED,       ///<  Unsupported.
    RIDE_HAL_ERROR_STATE,             ///<  Wrong state
    RIDE_HAL_ERROR_BUSY,              ///<  Device or resource busy
    RIDE_HAL_ERROR_EXISTS,            ///<  The object already exists
    RIDE_HAL_ERROR_ACCES,             ///<  Permission denied
    RIDE_HAL_ERROR_TIMEOUT,           ///<  Timeout
    RIDE_HAL_ERROR_NODATA,            ///<  No Data
    RIDE_HAL_ERROR_EXC_MAX,           ///<  Exceen maximum limitation
    RIDE_HAL_ERROR_MAX = 0x7FFFFFFF   ///<  Do not use.
} RideHalError_e;

/// @brief RideHal Buffer Flags
typedef uint32_t RideHal_BufferFlags_t;

/// @brief RideHal Buffer Type
typedef enum
{
    RIDE_HAL_BUFFER_TYPE_RAW = 0,   ///< The buffer for raw data
    RIDE_HAL_BUFFER_TYPE_IMAGE,     ///< The buffer for image
    RIDE_HAL_BUFFER_TYPE_TENSOR     ///< The buffer for tensor
} RideHal_BufferType_e;

/// @brief RideHal Buffer Usage
typedef enum
{
    RIDE_HAL_BUFFER_USAGE_DEFAULT = 0,   ///< Default
    RIDE_HAL_BUFFER_USAGE_CAMERA,        ///< Buffer used by camera
    RIDE_HAL_BUFFER_USAGE_GPU,           ///< Buffer used by GPU
    RIDE_HAL_BUFFER_USAGE_VPU,           ///< Buffer used by VPU
    RIDE_HAL_BUFFER_USAGE_EVA,           ///< Buffer used by EVA
    RIDE_HAL_BUFFER_USAGE_HTP,           ///< Buffer used by HTP
    RIDE_HAL_BUFFER_USAGE_MAX
} RideHal_BufferUsage_e;

/// @brief RideHal Computing Processor Type
typedef enum
{
    RIDE_HAL_PROCESSOR_HTP0,   ///< do computing on the processor HTP0
    RIDE_HAL_PROCESSOR_HTP1,   ///< do computing on the processor HTP1
    RIDE_HAL_PROCESSOR_CPU,    ///< do computing on the processor CPU
    RIDE_HAL_PROCESSOR_GPU,    ///< do computing on the processor GPU
    RIDE_HAL_PROCESSOR_MAX
} RideHal_ProcessorType_e;

/// @brief The attributes of an allocated DMA memory.
typedef struct
{
    void *pData;                 /* The buffer virtual address */
    uint64_t dmaHandle;          /* The buffer DMA handle */
    size_t size;                 /* The buffer size */
    uint64_t id;                 /* The unique ID assigned by the buffer manager */
    uint64_t pid;                /* The process id that allocated this buffer */
    RideHal_BufferUsage_e usage; /* The buffer usage */
    RideHal_BufferFlags_t flags; /* The buffer flags */
} RideHal_Buffer_t;

/// @brief The image format.
typedef enum
{
    /* Below formats for an image without compression */
    RIDE_HAL_IMAGE_FORMAT_RGB888 = 0,
    RIDE_HAL_IMAGE_FORMAT_BGR888,
    RIDE_HAL_IMAGE_FORMAT_UYVY,
    RIDE_HAL_IMAGE_FORMAT_NV12,
    RIDE_HAL_IMAGE_FORMAT_P010,
    RIDE_HAL_IMAGE_FORMAT_MAX,
    /* Below formats for an image with compression, such as by the Video Encoder */
    RIDE_HAL_IMAGE_FORMAT_COMPRESSED_MIN = 100,
    RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H264 = 100,
    RIDE_HAL_IMAGE_FORMAT_COMPRESSED_H265,
    RIDE_HAL_IMAGE_FORMAT_COMPRESSED_MAX,
} RideHal_ImageFormat_e;

/// @brief The image properties.
typedef struct
{
    RideHal_ImageFormat_e format; /* The image format */
    uint32_t batchSize;           /* The image batch size */
    uint32_t width;               /* The image width in pixels */
    uint32_t height;              /* The image height in pixels */
    /* The image stride along width in bytes for each plane */
    uint32_t stride[RIDE_HAL_NUM_IMAGE_PLANES];
    /* The image actual height in scanlines for each plane */
    uint32_t actualHeight[RIDE_HAL_NUM_IMAGE_PLANES];
    uint32_t numPlanes;      /* The number of the image planes */
    uint32_t extraPadding;   /* The extra paddings in bytes at the end of the last image plane */
    uint32_t compressedSize; /* the size in bytes of the compressed image */
} RideHal_ImageProps_t;

/// @brief The RideHal tensor data type.
typedef enum
{
    /// 8-bit integer type
    RIDEHAL_TENSOR_TYPE_INT_8,
    /// 16-bit integer type
    RIDEHAL_TENSOR_TYPE_INT_16,
    /// 32-bit integer type
    RIDEHAL_TENSOR_TYPE_INT_32,
    /// 64-bit integer type
    RIDEHAL_TENSOR_TYPE_INT_64,

    // Unsigned Int: 0x01XX
    RIDEHAL_TENSOR_TYPE_UINT_8,
    RIDEHAL_TENSOR_TYPE_UINT_16,
    RIDEHAL_TENSOR_TYPE_UINT_32,
    RIDEHAL_TENSOR_TYPE_UINT_64,

    // Float: 0x02XX
    RIDEHAL_TENSOR_TYPE_FLOAT_16,
    RIDEHAL_TENSOR_TYPE_FLOAT_32,
    RIDEHAL_TENSOR_TYPE_FLOAT_64,

    // Signed Fixed Point: 0x03XX
    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8,
    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16,
    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32,

    // Unsigned Fixed Point: 0x04XX
    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8,
    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16,
    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32,

    RIDE_HAL_TENSOR_TYPE_MAX,
} RideHal_TensorType_e;

/// @brief The tensor properties.
typedef struct
{
    RideHal_TensorType_e type;        /* The tensor type */
    uint32_t dims[RIDE_HAL_NUM_DIMS]; /* The tensor dimensions */
    uint32_t numDims;                 /* The number of dimensions */
} RideHal_TensorProps_t;

/// @brief RideHal Component state
typedef enum
{
    RIDE_HAL_COMPONENT_STATE_INITIAL = 0,      ///< the initial state
    RIDE_HAL_COMPONENT_STATE_INITIALIZING,     ///< the state druing initializing
    RIDE_HAL_COMPONENT_STATE_READY,            ///< the ready state
    RIDE_HAL_COMPONENT_STATE_STATING,          ///< the starting state
    RIDE_HAL_COMPONENT_STATE_RUNNING,          ///< the running state
    RIDE_HAL_COMPONENT_STATE_STOPING,          ///< the stoping state
    RIDE_HAL_COMPONENT_STATE_ERROR,            ///< the error state
    RIDE_HAL_COMPONENT_STATE_PAUSING,          ///< the pausing state
    RIDE_HAL_COMPONENT_STATE_PAUSE,            ///< the paused state
    RIDE_HAL_COMPONENT_STATE_RESUMING,         ///< the resuming state
    RIDE_HAL_COMPONENT_STATE_DEINITIALIZING,   ///< the state druing deinitializing
    RIDE_HAL_COMPONENT_STATE_MAX
} RideHal_ComponentState_t;

}   // namespace common
}   // namespace ridehal

#endif   // _RIDE_HAL_TYPES_HPP_
