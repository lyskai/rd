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

RideHalError_e RideHal_DmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size,
                                    RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    heap_type heapType = ID_DMA_BUF_HEAP_UNCACHED;
    void *pAddr = nullptr;
    int fd = -1;
    int devFd = -1;
    int rc = 0;
    (void) usage;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
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
        pAddr = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
        if ( nullptr != pAddr )
        {
            *pData = pAddr;
            *pDmaHandle = static_cast<uint64_t>( fd );
        }
        else
        {
            ret = RIDE_HAL_ERROR_FAIL;
            close( fd );
        }
    }

    /* clean up */
    if ( devFd >= 0 )
    {
        dmabufheap_release( devFd );
    }

    return ret;
}

RideHalError_e RideHal_DmaFree( void *pData, uint64_t pDmaHandle, size_t size )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    int rc = 0;

    if ( nullptr == pData )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = munmap( pData, size );
        if ( 0 != rc )
        {
            ret = RIDE_HAL_ERROR_ACCES;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = close( static_cast<int>( pDmaHandle ) );
        if ( 0 != rc )
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    return ret;
}
}   // namespace hal
}   // namespace ride
