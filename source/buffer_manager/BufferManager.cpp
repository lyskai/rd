// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/BufferManager.hpp"

namespace ride
{
namespace hal
{
namespace memory
{

BufferManager BufferManager::s_defaultBufferManager;

BufferManager::BufferManager() {}

BufferManager::~BufferManager() {}

BufferManager *BufferManager::GetDefaultBufferManager()
{
    return &s_defaultBufferManager;
}

RideHalError_e BufferManager::Register( Buffer *pBuffer, uint64_t *pID )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    RideHal_SharedBuffer_t *pSharedBuffer = nullptr;

    if ( ( nullptr == pBuffer ) || ( nullptr == pID ) )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        pSharedBuffer = new RideHal_SharedBuffer_t;
        if ( nullptr == pSharedBuffer )
        {
            ret = RIDE_HAL_ERROR_NORES;
        }
        else
        {
            ret = pBuffer->GetSharedBuffer( pSharedBuffer );
        }
    }

    std::lock_guard<std::mutex> l( m_Lock );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_IDAllocator++;
        pSharedBuffer->buffer.id = m_IDAllocator;
        *pID = m_IDAllocator;
        m_bufferMap[pSharedBuffer->buffer.id] = pSharedBuffer;
        Log( Logger::Level_e::DEBUG,
             "buffer manager: register %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 "\n",
             pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
             pSharedBuffer->buffer.size, pSharedBuffer->buffer.id );
    }

    return ret;
}

RideHalError_e BufferManager::Deregister( uint64_t id )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    RideHal_SharedBuffer_t *pSharedBuffer = nullptr;

    std::lock_guard<std::mutex> l( m_Lock );

    auto it = m_bufferMap.find( id );
    if ( it != m_bufferMap.end() )
    {
        pSharedBuffer = it->second;
        m_bufferMap.erase( it );
        Log( Logger::Level_e::DEBUG,
             "buffer manager: deregister %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 "\n",
             pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
             pSharedBuffer->buffer.size, pSharedBuffer->buffer.id );
        delete pSharedBuffer;
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
            *pSharedBuffer = *( it->second );
        }
        else
        {
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    return ret;
}

void BufferManager::Log( Logger::Level_e level, const char *pFormat, va_list args )
{
    // TODO:
    vprintf( pFormat, args );
}

void BufferManager::Log( Logger::Level_e level, const char *pFormat, ... )
{
    va_list args;

    va_start( args, pFormat );
    Log( level, pFormat, args );
    va_end( args );
}

}   // namespace memory
}   // namespace hal
}   // namespace ride
