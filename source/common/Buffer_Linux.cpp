// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"

#include <fcntl.h>
#include <linux/dma-heap.h>
#include <plat_dmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace ridehal
{
namespace common
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
        RIDEHAL_LOG_ERROR( "DmaAllocate with pData or pDmaHandle is nullptr" );
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
            RIDEHAL_LOG_ERROR( "DmaAllocate failed to do dmabuf heap init: %d", devFd );
            ret = RIDE_HAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = dmabufheap_alloc( devFd, size, 0, &fd );
        if ( rc < 0 )
        {
            RIDEHAL_LOG_ERROR( "DmaAllocate failed to do dmabuf heap alloc: %d", rc );
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
            RIDEHAL_LOG_ERROR( "DmaAllocate failed to mmap" );
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
        RIDEHAL_LOG_ERROR( "DmaFree with pData is nullptr" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = munmap( pData, size );
        if ( 0 != rc )
        {
            RIDEHAL_LOG_ERROR( "DmaFree failed to do munmap for buffer %p: %d", pData, rc );
            ret = RIDE_HAL_ERROR_ACCES;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        rc = close( static_cast<int>( pDmaHandle ) );
        if ( 0 != rc )
        {
            RIDEHAL_LOG_ERROR( "DmaFree failed to close buffer %" PRIu64 ": %d", pDmaHandle, rc );
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
