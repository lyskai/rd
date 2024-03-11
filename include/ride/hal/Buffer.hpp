// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_BUFFER_HPP_
#define _RIDE_HAL_BUFFER_HPP_

#include "ride/hal/Logger.hpp"
#include "ride/hal/Types.hpp"

namespace ride
{
namespace hal
{
/// @brief Allocate the DMA memory
/// @param pData [out] the allocated DMA data address
/// @param pDmaHandle [out] the allocated DMA handle
/// @param size the wanted DMA memory size
/// @param flags the DMA buffer flags
/// @param usage the DMA buffer usage
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHal_DmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size,
                                    RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage );

/// @brief Free the DMA memory
/// @param pData the allocated DMA data address
/// @param pDmaHandle the allocated DMA handle
/// @param size the wanted DMA memory size
/// @return RIDE_HAL_ERROR_NONE on success, others on failure
RideHalError_e RideHal_DmaFree( void *pData, uint64_t pDmaHandle, size_t size );

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_BUFFER_HPP_
