// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Buffer.hpp"
#include "ride/hal/BufferManager.hpp"
#include <string.h>
#include <unistd.h>

namespace ride
{
namespace hal
{

void RideHal_SharedBuffer::Init()
{
    memset( this, 0, sizeof( *this ) );
    this->buffer.pData = nullptr;
    this->buffer.dmaHandle = 0;
    this->buffer.size = 0;
    this->buffer.id = 0;
    this->buffer.pid = static_cast<uint64_t>( getpid() );
    this->buffer.usage = RIDE_HAL_BUFFER_USAGE_DEFAULT;
    this->buffer.flags = 0;
    this->size = 0;
    this->offset = 0;
    this->type = RIDE_HAL_BUFFER_TYPE_RAW;
}

RideHal_SharedBuffer::RideHal_SharedBuffer()
{
    Init();
}

RideHal_SharedBuffer::RideHal_SharedBuffer( const RideHal_SharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case RIDE_HAL_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case RIDE_HAL_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default:
            break;
    }
}

RideHal_SharedBuffer &RideHal_SharedBuffer::operator=( const RideHal_SharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case RIDE_HAL_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case RIDE_HAL_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default:
            break;
    }
    return *this;
}

RideHal_SharedBuffer::~RideHal_SharedBuffer() {}

RideHalError_e RideHal_SharedBuffer::Allocate( size_t size, RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
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
    else if ( nullptr != this->buffer.pData )
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
        this->buffer.pData = pData;
        this->buffer.dmaHandle = dmaHandle;
        this->buffer.size = size;
        this->buffer.usage = usage;
        this->buffer.flags = flags;
        this->size = size;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Register( this );
    }

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        if ( nullptr != pData )
        {
            (void) RideHal_DmaFree( pData, dmaHandle, size );
        }
        Init();
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::Free()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();

    if ( nullptr == pBufferManager )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( nullptr == this->buffer.pData )
    {
        ret = RIDE_HAL_ERROR_INVALID_BUF;
    }
    else
    {
        /* OK */
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Deregister( this->buffer.id );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = RideHal_DmaFree( this->buffer.pData, this->buffer.dmaHandle, this->buffer.size );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        Init();
    }

    return ret;
}

void RideHal_SharedBuffer::Log( int level, const char *pFormat, ... )
{
    va_list args;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    if ( nullptr != pBufferManager )
    {
        va_start( args, pFormat );
        pBufferManager->Log( (Logger_Level_e) level, pFormat, args );
        va_end( args );
    }
}


}   // namespace hal
}   // namespace ride
