// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_BUFFER_MANAGER_HPP_
#define _RIDE_HAL_BUFFER_MANAGER_HPP_

#include <map>
#include <mutex>

#include "ride/hal/Buffer.hpp"
#include "ride/hal/Logger.hpp"
#include "ride/hal/Types.hpp"

namespace ride
{
namespace hal
{
namespace memory
{

/// @brief Buffer Manager
///
/// Manage the allocated DMA buffer
class BufferManager
{
public:
    BufferManager();
    ~BufferManager();

    /// @brief Register the allocated buffer to the buffer manager
    /// @param pBuffer the allocated buffer
    /// @param pID [out] the unique ID assigned by the buffer manager
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Register( Buffer *pBuffer, uint64_t *pID );

    /// @brief unregister the buffer from the buffer manager
    /// @param id the unique ID of the buffer assigned by the buffer manager
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deregister( uint64_t id );

    /// @brief Get the shared buffer information
    /// @param id the unique ID of the buffer assigned by the buffer manager
    /// @param pSharedBuffer pointer to hold the shared buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( uint64_t id, RideHal_SharedBuffer_t *pSharedBuffer );

    /// @brief Assign a logger to the buffer manager
    /// @param pLogger the logger used by the buffer manager to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e SetLogger( Logger *pLogger );

    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param ... variable arguments
    /// @return void
    void Log( Logger::Level_e level, const char *pFormat, ... );

    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param args variable arguments
    /// @return void
    void Log( Logger::Level_e level, const char *pFormat, va_list args );

public:
    /// @brief Get the default buffer manager
    /// @return the buffer manager pointer, nullptr on failure
    static BufferManager *GetDefaultBufferManager();

private:
    std::mutex m_Lock;
    std::map<uint64_t, RideHal_SharedBuffer_t *> m_bufferMap;
    uint64_t m_IDAllocator = 0;
    Logger *m_pLogger = nullptr;

private:
    static BufferManager s_defaultBufferManager;
};

}   // namespace memory
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_BUFFER_MANAGER_HPP_
