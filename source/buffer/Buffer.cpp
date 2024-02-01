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
