// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Buffer.hpp"
#include "ride/hal/BufferManager.hpp"

#include <fcntl.h>
#include <linux/dma-heap.h>
#include <plat_dmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace ride
{
namespace hal
{
namespace memory
{

RideHalError_e Buffer::Allocate( size_t size, RideHal_BufferFlags_t flags,
                                 RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    void *pData = nullptr;
    heap_type heapType = ID_DMA_BUF_HEAP_UNCACHED;
    int fd = -1;
    int devFd = -1;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    uint64_t id;
    int rc = 0;
    (void) usage;

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
        /* convert ride hal flags to the dma buf heap type */
        if ( 0 != ( flags & RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            heapType = ID_DMA_BUF_HEAP_CACHED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        devFd = dmabufheap_init( heapType );
        if ( devFd < 0 )
        {
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = dmabufheap_alloc( devFd, size, 0, &fd );
        if ( rc < 0 )
        {
            ret = RIDE_HAL_ERROR_NORES;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        pData = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
        if ( nullptr == pData )
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_sharedBuffer.buffer.pData = pData;
        m_sharedBuffer.buffer.dmaHandle = static_cast<uint64_t>( fd );
        m_sharedBuffer.buffer.size = size;
        m_sharedBuffer.buffer.id = id;
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
        dmabufheap_release( devFd );
    }
    else
    {
        if ( nullptr != pData )
        {
            (void) munmap( pData, size );
        }

        if ( fd >= 0 )
        {
            close( fd );
        }

        if ( devFd >= 0 )
        {
            dmabufheap_release( devFd );
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
        munmap( m_sharedBuffer.buffer.pData, m_sharedBuffer.buffer.size );
        close( static_cast<int>( m_sharedBuffer.buffer.dmaHandle ) );
        ResetSharedBuffer();
    }

    return ret;
}
}   // namespace memory
}   // namespace hal
}   // namespace ride
