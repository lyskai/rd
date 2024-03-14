// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/common/BufferManager.hpp"
#include <stdio.h>

namespace ridehal
{
namespace common
{


std::mutex BufferManager::s_Lock;
static BufferManager s_dftBufMgr;
BufferManager *BufferManager::s_pDefaultBufferManager = nullptr;

BufferManager::BufferManager() {}

BufferManager::~BufferManager() {}

RideHalError_e BufferManager::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        printf( "WARINING: failed to init logger for BUFMGR %s: ret = %d\n", pName, ret );
    }
    ret = RIDE_HAL_ERROR_NONE; /* ignore logger init error */

    return ret;
}

BufferManager *BufferManager::GetDefaultBufferManager()
{
    std::lock_guard<std::mutex> l( s_Lock );
    if ( nullptr == s_pDefaultBufferManager )
    {
        s_pDefaultBufferManager = &s_dftBufMgr;
        (void) s_dftBufMgr.Init( "BUFMGR" );
    }

    return s_pDefaultBufferManager;
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
}   // namespace common
}   // namespace ridehal
