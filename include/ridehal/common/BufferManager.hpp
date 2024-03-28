// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_BUFFER_MANAGER_HPP_
#define _RIDE_HAL_BUFFER_MANAGER_HPP_

#include <map>
#include <mutex>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

namespace ridehal
{
namespace common
{

/// @brief Buffer Manager
///
/// Manage the allocated DMA buffer
class BufferManager
{
public:
    BufferManager();
    ~BufferManager();

    /// @brief Initialize the buffer manager
    /// @param pName the buffer manager unique instance name
    /// @param level the logger message level
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Register the allocated shared buffer to the buffer manager
    /// @param pSharedBuffer the allocated shared buffer
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Register( RideHal_SharedBuffer_t *pSharedBuffer );

    /// @brief unregister the buffer from the buffer manager
    /// @param id the unique ID of the buffer assigned by the buffer manager
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deregister( uint64_t id );

    /// @brief Get the shared buffer information
    /// @param id the unique ID of the buffer assigned by the buffer manager
    /// @param pSharedBuffer pointer to hold the shared buffer information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetSharedBuffer( uint64_t id, RideHal_SharedBuffer_t *pSharedBuffer );

public:
    /// @brief Get the default buffer manager
    /// @return the buffer manager pointer, nullptr on failure
    static BufferManager *GetDefaultBufferManager();

private:
    std::mutex m_lock;
    std::map<uint64_t, RideHal_SharedBuffer_t> m_bufferMap;
    uint64_t m_IDAllocator = 0;

private:
    RIDEHAL_DECLARE_LOGGER();
    static std::mutex s_Lock;
    static BufferManager *s_pDefaultBufferManager;
};

}   // namespace common
}   // namespace ridehal

#endif   // _RIDE_HAL_BUFFER_MANAGER_HPP_
