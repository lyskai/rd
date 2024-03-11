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
    RIDE_HAL_TENSOR_TYPE_INT8,
    RIDE_HAL_TENSOR_TYPE_INT16,
    RIDE_HAL_TENSOR_TYPE_INT32,
    RIDE_HAL_TENSOR_TYPE_UINT8,
    RIDE_HAL_TENSOR_TYPE_UINT16,
    RIDE_HAL_TENSOR_TYPE_UINT32,
    RIDE_HAL_TENSOR_TYPE_FLOAT16,
    RIDE_HAL_TENSOR_TYPE_FLOAT32,
    RIDE_HAL_TENSOR_TYPE_MAX
} RideHal_TensorType_e;

/// @brief The tensor properties.
typedef struct
{
    RideHal_TensorType_e type;        /* The tensor type */
    uint32_t dims[RIDE_HAL_NUM_DIMS]; /* The tensor dimensions */
    uint32_t numDims;                 /* The number of dimensions */
} RideHal_TensorProps_t;


/// @brief RideHal Shared Buffer between Components for zero copy purpose
typedef struct RideHal_SharedBuffer
{
    RideHal_Buffer_t buffer;   /* The shared buffer */
    size_t size;               /* The size of the valid buffer in the shared buffer */
    size_t offset;             /* The offset of the valid buffer in the shared buffer */
    RideHal_BufferType_e type; /* The buffer type */
    union
    {
        RideHal_ImageProps_t imgProps;     /* The image properties if type is IMAGE */
        RideHal_TensorProps_t tensorProps; /* The tensor properties if type is TENSOR */
    };

public:
    RideHal_SharedBuffer();
    ~RideHal_SharedBuffer();

    /// @brief Construct an shared buffer from another shared buffer
    /// @param rhs the shared buffer
    /// @return void
    RideHal_SharedBuffer( const RideHal_SharedBuffer &rhs );
    RideHal_SharedBuffer &operator=( const RideHal_SharedBuffer &rhs );

    /// @brief Allocate the DMA memory
    /// @param size the wanted DMA memory size
    /// @param usage the DMA buffer usage
    /// @param flags the DMA buffer flags
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( size_t size,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA );

    /// @brief Allocate the DMA memory for the image with the best strides/paddings
    /// that can be shared among CPU/GPU/VPU/HTP, etc
    /// @param width the image width
    /// @param height the image height
    /// @param format the image format
    /// @param usage the DMA buffer usage
    /// @param flags the DMA buffer flags
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( uint32_t width, uint32_t height, RideHal_ImageFormat_e format,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA );


    /// @brief Allocate the DMA memory for batched image with best strides/paddings
    /// that can be shared among CPU/GPU/VPU/HTP, etc
    /// @param batchSize the image batch size
    /// @param width the image width
    /// @param height the image height
    /// @param format the image format
    /// @param usage the DMA buffer usage
    /// @param flags the DMA buffer flags
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                             RideHal_ImageFormat_e format,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA );

    /// @brief Allocate the DMA memory for image with specified image properties
    /// @param pImgProps the specified image properties
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( const RideHal_ImageProps_t *pImgProps,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA );

    /// @brief Allocate the DMA memory for tensor with specified tensor properties
    /// @param pTensorProps the specified tensor properties
    /// @param usage the DMA buffer usage
    /// @param flags the DMA buffer flags
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( const RideHal_TensorProps_t *pTensorProps,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA );

    /// @brief Free the DMA memory
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Free();

    /// @brief Get the shared buffer information
    /// @param sharedBuffer pointer to hold the shared buffer information for
    /// the image batches specified by batchOffset and batchSize
    /// @param batchOffset the image batch offset
    /// @param batchSize the image batch size
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( RideHal_SharedBuffer *pSharedBuffer, uint32_t batchOffset,
                                    uint32_t batchSize = 1 );


    /// @brief get the valid buffer virtual address
    /// @return the valid buffer virtual address
    void *data() const { return (void *) ( (uintptr_t) buffer.pData + offset ); }

private:
    /// @brief Initialize the shared buffer variables
    void Init();

    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param ... variable arguments
    /// @return void
    void Log( int level, const char *pFormat, ... );

} RideHal_SharedBuffer_t;

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_TYPES_HPP_
