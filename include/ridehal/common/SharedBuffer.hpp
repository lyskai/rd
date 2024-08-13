// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SHARED_BUFFER_HPP
#define RIDEHAL_SHARED_BUFFER_HPP

#include "ridehal/common/Types.hpp"

namespace ridehal
{
namespace common
{

/** @brief RideHal Shared Buffer between Components for zero copy purpose */
typedef struct RideHal_SharedBuffer
{
    RideHal_Buffer_t buffer;   /**< The shared buffer */
    size_t size;               /**< The size of the valid buffer in the shared buffer */
    size_t offset;             /**< The offset of the valid buffer in the shared buffer */
    RideHal_BufferType_e type; /**< The buffer type */
    union
    {
        /**< RAW has no properties. */
        RideHal_ImageProps_t imgProps;     /**< The image properties if type is IMAGE */
        RideHal_TensorProps_t tensorProps; /**< The tensor properties if type is TENSOR */
    };

public:
    RideHal_SharedBuffer();
    ~RideHal_SharedBuffer();

    /**
     * @brief Construct an shared buffer from another shared buffer
     * @param[in] rhs the shared buffer
     * @return void
     */
    RideHal_SharedBuffer( const RideHal_SharedBuffer &rhs );
    RideHal_SharedBuffer &operator=( const RideHal_SharedBuffer &rhs );

    /**
     * @brief Allocate the DMA memory
     * @param[in] size the wanted DMA memory size
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Allocate( size_t size,
                             RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_DEFAULT,
                             RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for the image with the best strides/paddings
     * that can be shared among CPU/GPU/VPU/HTP, etc
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Allocate( uint32_t width, uint32_t height, RideHal_ImageFormat_e format,
                             RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for batched image with best strides/paddings
     * that can be shared among CPU/GPU/VPU/HTP, etc
     * @param[in] batchSize the image batch size
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                             RideHal_ImageFormat_e format,
                             RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for image with specified image properties
     * @param[in] pImgProps the specified image properties
     * @param[in] flags the DMA buffer flags
     * @param[in] usage the DMA buffer usage
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Allocate( const RideHal_ImageProps_t *pImgProps,
                             RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_CAMERA,
                             RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for tensor with specified tensor properties
     * @param[in] pTensorProps the specified tensor properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Allocate( const RideHal_TensorProps_t *pTensorProps,
                             RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_HTP,
                             RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Free the DMA memory
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Free();

    /**
     * @brief Get the shared buffer information
     * @param[out] pSharedBuffer pointer to hold the shared buffer information for
     * the image batches specified by batchOffset and batchSize
     * @param[in] batchOffset the image batch offset
     * @param[in] batchSize the image batch size
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetSharedBuffer( RideHal_SharedBuffer *pSharedBuffer, uint32_t batchOffset,
                                    uint32_t batchSize = 1 );

    /**
     * @brief get the valid buffer virtual address
     * @return the valid buffer virtual address
     */
    void *data() const { return (void *) ( ( (uint8_t *) buffer.pData ) + offset ); }

    /**
     * @brief Convert the shared buffer type from image to tensor
     * @param[out] pSharedBuffer pointer to hold the shared buffer information for
     * the tensor that converted from the image
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note: The image must has 1 plane and has no paddings .
     */
    RideHalError_e ImageToTensor( RideHal_SharedBuffer *pSharedBuffer );

    /**
     * @brief Convert the shared buffer type from image to tensor luma and chroma
     * @param[out] pLuma pointer to hold the shared buffer information for
     * the tensor that represent the luma "luminance" plane Y.
     * @param[out] pChroma pointer to hold the shared buffer information for
     * the tensor that represent the chroma plane.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note: The image must be in format NV12 or P010 and has no paddings.
     */
    RideHalError_e ImageToTensor( RideHal_SharedBuffer *pLuma, RideHal_SharedBuffer *pChroma );

private:
    /**
     * @brief Initialize the shared buffer variables
     */
    void Init();
} RideHal_SharedBuffer_t;

/**
 * @brief Allocate the DMA memory
 * @param[out] pData the allocated DMA data address
 * @param[out] pDmaHandle the allocated DMA handle
 * @param[in] size the wanted DMA memory size
 * @param[in] flags the DMA buffer flags
 * @param[in] usage the DMA buffer usage
 * @return RIDEHAL_ERROR_NONE on success, others on failure
 */
RideHalError_e RideHal_DmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size,
                                    RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage );

/**
 * @brief Free the DMA memory
 * @param pData the allocated DMA data address
 * @param dmaHandle the allocated DMA handle
 * @param size the wanted DMA memory size
 * @return RIDEHAL_ERROR_NONE on success, others on failure
 */
RideHalError_e RideHal_DmaFree( void *pData, uint64_t dmaHandle, size_t size );
}   // namespace common
}   // namespace ridehal

#endif   // RIDEHAL_SHARED_BUFFER_HPP
