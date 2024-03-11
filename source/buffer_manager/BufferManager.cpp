// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/BufferManager.hpp"

namespace ride
{
namespace hal
{

BufferManager BufferManager::s_defaultBufferManager;

BufferManager::BufferManager()
{
    (void) LoggerIF::Init( "BUFMGR" );
}

BufferManager::~BufferManager() {}

BufferManager *BufferManager::GetDefaultBufferManager()
{
    return &s_defaultBufferManager;
}

RideHalError_e BufferManager::Register( RideHal_SharedBuffer_t *pSharedBuffer )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    std::lock_guard<std::mutex> l( m_Lock );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_IDAllocator++;
        pSharedBuffer->buffer.id = m_IDAllocator;
        m_bufferMap[pSharedBuffer->buffer.id] = *pSharedBuffer;
        RIDEHAL_DEBUG( "buffer manager: register %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 "\n",
                       pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
                       pSharedBuffer->buffer.size, pSharedBuffer->buffer.id );
    }

    return ret;
}

RideHalError_e BufferManager::Deregister( uint64_t id )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    std::lock_guard<std::mutex> l( m_Lock );

    auto it = m_bufferMap.find( id );
    if ( it != m_bufferMap.end() )
    {
        RideHal_SharedBuffer_t &sharedBuffer = it->second;
        RIDEHAL_DEBUG( "buffer manager: deregister %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 "\n",
                       sharedBuffer.buffer.pData, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.id );
        m_bufferMap.erase( it );
    }
    else
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e BufferManager::GetSharedBuffer( uint64_t id, RideHal_SharedBuffer_t *pSharedBuffer )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        std::lock_guard<std::mutex> l( m_Lock );
        auto it = m_bufferMap.find( id );
        if ( it != m_bufferMap.end() )
        {
            *pSharedBuffer = it->second;
        }
        else
        {
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    return ret;
}

}   // namespace hal
}   // namespace ride
