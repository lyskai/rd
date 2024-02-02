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
namespace memory
{

/// @brief Buffer
///
/// Buffer to allocate DMA memory for RAW
class Buffer
{
public:
    Buffer();
    ~Buffer();

    /// @brief Construct a buffer from another buffer
    /// @param rhs the buffer
    /// the rhs buffer will be release when the new buffer created
    /// @return void
    Buffer( Buffer &&rhs );
    Buffer &operator=( Buffer &&rhs );

    /// @brief Allocate the DMA memory
    /// @param size the wanted DMA memory size
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( size_t size,
                             RideHal_BufferFlags_t flags = RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT );

    /// @brief Free the DMA memory
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Free();

    /// @brief Get the shared buffer information
    /// @param pSharedBuffer pointer to hold the shared buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer );

protected:
    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param ... variable arguments
    /// @return void
    void Log( Logger::Level_e level, const char *pFormat, ... );

private:
    void ResetSharedBuffer();

protected:
    RideHal_SharedBuffer_t m_sharedBuffer;
};

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

}   // namespace memory
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_BUFFER_HPP_
