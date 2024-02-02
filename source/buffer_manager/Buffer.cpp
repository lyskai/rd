// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Buffer.hpp"
#include "ride/hal/BufferManager.hpp"
#include <unistd.h>

namespace ride
{
namespace hal
{
namespace memory
{

void Buffer::ResetSharedBuffer()
{
    m_sharedBuffer.buffer.pData = nullptr;
    m_sharedBuffer.buffer.dmaHandle = 0;
    m_sharedBuffer.buffer.size = 0;
    m_sharedBuffer.buffer.id = 0;
    m_sharedBuffer.buffer.pid = static_cast<uint64_t>( getpid() );
    m_sharedBuffer.buffer.usage = RIDE_HAL_BUFFER_USAGE_DEFAULT;
    m_sharedBuffer.buffer.flags = 0;
    m_sharedBuffer.size = 0;
    m_sharedBuffer.offset = 0;
}

Buffer::Buffer()
{
    ResetSharedBuffer();
    m_sharedBuffer.type = RIDE_HAL_BUFFER_TYPE_RAW;
}

Buffer::Buffer( Buffer &&rhs )
{
    RideHalError_e ret = rhs.GetSharedBuffer( &m_sharedBuffer );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rhs.ResetSharedBuffer();
    }
}

Buffer &Buffer::operator=( Buffer &&rhs )
{
    RideHalError_e ret = rhs.GetSharedBuffer( &m_sharedBuffer );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rhs.ResetSharedBuffer();
    }

    return *this;
}

Buffer::~Buffer() {}

RideHalError_e Buffer::Allocate( size_t size, RideHal_BufferFlags_t flags,
                                 RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    void *pData = nullptr;
    uint64_t dmaHandle = 0;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    uint64_t id;

    if ( nullptr == pBufferManager )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( nullptr != m_sharedBuffer.buffer.pData )
    {
        ret = RIDE_HAL_ERROR_EXISTS;
    }
    else
    {
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = RideHal_DmaAllocate( &pData, &dmaHandle, size, flags, usage );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_sharedBuffer.buffer.pData = pData;
        m_sharedBuffer.buffer.dmaHandle = dmaHandle;
        m_sharedBuffer.buffer.size = size;
        m_sharedBuffer.buffer.usage = usage;
        m_sharedBuffer.buffer.flags = flags;
        m_sharedBuffer.size = size;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Register( this, &id );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_sharedBuffer.buffer.id = id;
    }
    else
    {
        if ( nullptr != pData )
        {
            (void) RideHal_DmaFree( pData, dmaHandle, size );
        }
        ResetSharedBuffer();
    }

    return ret;
}

RideHalError_e Buffer::Free()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();

    if ( nullptr == pBufferManager )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( nullptr == m_sharedBuffer.buffer.pData )
    {
        ret = RIDE_HAL_ERROR_INVALID_BUF;
    }
    else
    {
        /* OK */
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Deregister( m_sharedBuffer.buffer.id );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = RideHal_DmaFree( m_sharedBuffer.buffer.pData, m_sharedBuffer.buffer.dmaHandle,
                               m_sharedBuffer.buffer.size );
        ResetSharedBuffer();
    }

    return ret;
}

RideHalError_e Buffer::GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( nullptr == m_sharedBuffer.buffer.pData )
    {
        ret = RIDE_HAL_ERROR_INVALID_BUF;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        *pSharedBuffer = m_sharedBuffer;
    }

    return ret;
}

void Buffer::Log( Logger::Level_e level, const char *pFormat, ... )
{
    va_list args;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    if ( nullptr != pBufferManager )
    {
        va_start( args, pFormat );
        pBufferManager->Log( level, pFormat, args );
        va_end( args );
    }
}
}   // namespace memory
}   // namespace hal
}   // namespace ride
