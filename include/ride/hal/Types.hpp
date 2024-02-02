// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_TYPES_HPP_
#define _RIDE_HAL_TYPES_HPP_

#include <cinttypes>
#include <cstddef>

namespace ride
{
namespace hal
{

#define RIDE_HAL_NUM_IMAGE_PLANES ( 4 )
#define RIDE_HAL_NUM_DIMS ( 8 )

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

typedef struct
{
    void *pData;                 /* the buffer virtual address */
    uint64_t dmaHandle;          /* the buffer DMA handle */
    size_t size;                 /* the buffer size */
    uint64_t id;                 /* the unique ID assigned by the buffer maneger */
    uint64_t pid;                /* the process id */
    RideHal_BufferUsage_e usage; /* the buffer usage */
    RideHal_BufferFlags_t flags; /* the buffer flags */
} RideHal_Buffer_t;

typedef enum
{
    RIDE_HAL_IMAGE_FORMAT_RGB888,
    RIDE_HAL_IMAGE_FORMAT_BGR888,
    RIDE_HAL_IMAGE_FORMAT_UYVY,
    RIDE_HAL_IMAGE_FORMAT_NV12,
    RIDE_HAL_IMAGE_FORMAT_P010,
    RIDE_HAL_IMAGE_FORMAT_MAX,
} RideHal_ImageFormat_e;

typedef struct
{
    RideHal_ImageFormat_e format;
    uint32_t batchSize;
    uint32_t width;
    uint32_t height;
    uint32_t stride[RIDE_HAL_NUM_IMAGE_PLANES];
    uint32_t actualHeight[RIDE_HAL_NUM_IMAGE_PLANES];
    uint32_t numPlanes;
    uint32_t extraPadding;
} RideHal_ImageProps_t;

typedef enum
{
    RIDE_HAL_TEBSOR_TYPE_INT8,
    RIDE_HAL_TEBSOR_TYPE_INT16,
    RIDE_HAL_TEBSOR_TYPE_INT32,
    RIDE_HAL_TEBSOR_TYPE_UINT8,
    RIDE_HAL_TEBSOR_TYPE_UINT16,
    RIDE_HAL_TEBSOR_TYPE_UINT32,
    RIDE_HAL_TEBSOR_TYPE_FLOAT16,
    RIDE_HAL_TEBSOR_TYPE_FLOAT32,
    RIDE_HAL_TEBSOR_TYPE_MAX
} RideHal_TensorType_e;

typedef struct
{
    RideHal_TensorType_e type;
    uint32_t dims[RIDE_HAL_NUM_DIMS];
    uint32_t numDims;
} RideHal_TensorProps_t;


/// @brief RideHal Shared Buffer between Components for zero copy purpose
typedef struct
{
    RideHal_Buffer_t buffer;   /* The shared buffer */
    size_t size;               /* The size of the valid buffer in the shared buffer */
    size_t offset;             /* The offset of the valid buffer in the shared buffer */
    RideHal_BufferType_e type; /* The buffer type */
    union
    {
        RideHal_ImageProps_t imgProps;     /* the image properties if type is IMAGE */
        RideHal_TensorProps_t tensorProps; /* the tensor properties if type is TENSOR */
    };

public:
    /// @brief get the valid buffer virtual address
    /// @return the valid buffer virtual address
    void *data() { return (void *) ( (uintptr_t) buffer.pData + offset ); }
} RideHal_SharedBuffer_t;

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_TYPES_HPP_
