// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_IMAGE_HPP_
#define _RIDE_HAL_IMAGE_HPP_

#include "ride/hal/Buffer.hpp"

namespace ride
{
namespace hal
{
namespace memory
{

class Image : public Buffer
{
public:
    Image();
    ~Image();

    /// @brief Allocate the DMA memory for image with best strides/paddings
    /// that can be shared among CPU/GPU/VPU/HTP, etc
    /// @param width the image width
    /// @param height the image height
    /// @param format the image format
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( uint32_t width, uint32_t height, RideHal_ImageFormat_e format,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA );

    /// @brief Allocate the DMA memory for batched image with best strides/paddings
    /// that can be shared among CPU/GPU/VPU/HTP, etc
    /// @param batchSize the image batch size
    /// @param width the image width
    /// @param height the image height
    /// @param format the image format
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                             RideHal_ImageFormat_e format,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA );

    /// @brief Allocate the DMA memory for image with specified image properties
    /// @param pImgProps the specified image properties
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( const RideHal_ImageProps_t *pImgProps,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_CAMERA );

    /// @brief Get the shared buffer information
    /// @param pSharedBuffer pointer to hold the shared buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer );

    /// @brief Get the shared buffer information
    /// @param sharedBuffer pointer to hold the shared buffer information for
    /// the image batches specified by batchOffset and batchSize
    /// @param batchOffset the image batch offset
    /// @param batchSize the image batch size
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer, uint32_t batchOffset,
                                    uint32_t batchSize = 1 );
};

}   // namespace memory
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_IMAGE_HPP_
