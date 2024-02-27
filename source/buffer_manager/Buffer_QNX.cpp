// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Buffer.hpp"
#include "ride/hal/BufferManager.hpp"
#include <pmem.h>

namespace ride
{
namespace hal
{
namespace memory
{

static uint32_t s_usageToPMemID[RIDE_HAL_BUFFER_USAGE_MAX] = {
        PMEM_DMA_ID,                  /* RIDE_HAL_BUFFER_USAGE_DEFAULT */
        PMEM_CAMERA_ID,               /* RIDE_HAL_BUFFER_USAGE_CAMERA */
        PMEM_GRAPHICS_FRAMEBUFFER_ID, /* RIDE_HAL_BUFFER_USAGE_GPU */
        PMEM_VIDEO_ID,                /* RIDE_HAL_BUFFER_USAGE_VPU */
        PMEM_EVA_ID,                  /* RIDE_HAL_BUFFER_USAGE_EVA */
        PMEM_DSP_ID                   /* RIDE_HAL_BUFFER_USAGE_HTP */
};

RideHalError_e RideHal_DmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size,
                                    RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    pmem_handle_t pmemHandle = nullptr;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( 0 != ( flags & RIDE_HAL_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( usage < RIDE_HAL_BUFFER_USAGE_MAX )
        {
            pmemID = s_usageToPMemID[usage];
        }
        else
        {
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        *pData = pmem_malloc_ext_v2( size, pmemID, pmemFlags, PMEM_ALIGNMENT_4K, 0x0, &pmemHandle,
                                     NULL );
        if ( nullptr == *pData )
        {
            ret = RIDE_HAL_ERROR_NORES;
        }
        else
        {
            *pDmaHandle = static_cast<uint64_t>( (uintptr_t) pmemHandle );
        }
    }

    return ret;
}

RideHalError_e RideHal_DmaFree( void *pData, uint64_t pDmaHandle, size_t size )
{
    int rc = 0;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();

    if ( nullptr == pData )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        rc = pmem_free( pData );
        if ( 0 != rc )
        {
            ret = RIDE_HAL_ERROR_ACCES;
        }
    }

    return ret;
}
}   // namespace memory
}   // namespace hal
}   // namespace ride
