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

RideHalError_e Buffer::Allocate( size_t size, RideHal_BufferFlags_t flags,
                                 RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    void *pData = nullptr;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    pmem_handle_t pmemHandle = nullptr;
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
        pData = pmem_malloc_ext_v2( size, pmemID, pmemFlags, PMEM_ALIGNMENT_4K, 0x0, &pmemHandle,
                                    NULL );
        if ( nullptr == pData )
        {
            ret = RIDE_HAL_ERROR_NORES;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_sharedBuffer.buffer.pData = pData;
        m_sharedBuffer.buffer.dmaHandle = static_cast<uint64_t>( (uintptr_t) pmemHandle );
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
    }
    else
    {
        if ( nullptr != pData )
        {
            pmem_free( pData );
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
        pmem_free( m_sharedBuffer.buffer.pData );
        ResetSharedBuffer();
    }

    return ret;
}
}   // namespace memory
}   // namespace hal
}   // namespace ride
