// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/common/BufferManager.hpp"
#include <sstream>
#include <stdio.h>

namespace ridehal
{
namespace common
{

std::mutex BufferManager::s_Lock;
static BufferManager s_dftBufMgr;
BufferManager *BufferManager::s_pDefaultBufferManager = nullptr;

static std::string GetBufferTextInfo( const RideHal_SharedBuffer_t *pSharedBuffer )
{
    std::string str = "";
    std::stringstream ss;

    if ( RIDE_HAL_BUFFER_TYPE_RAW == pSharedBuffer->type )
    {
        str = "Raw";
    }
    else if ( RIDE_HAL_BUFFER_TYPE_IMAGE == pSharedBuffer->type )
    {
        ss << "Image format=" << pSharedBuffer->imgProps.format
           << " batch=" << pSharedBuffer->imgProps.batchSize
           << " resolution=" << pSharedBuffer->imgProps.width << "x"
           << pSharedBuffer->imgProps.height;
        if ( pSharedBuffer->imgProps.format < RIDE_HAL_IMAGE_FORMAT_MAX )
        {
            ss << " stride=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.stride[i] << ", ";
            }
            ss << "] actual height=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.actualHeight[i] << ", ";
            }
            ss << "], extraPadding=" << pSharedBuffer->imgProps.extraPadding;
        }
        else
        {
            ss << " compressedSize=" << pSharedBuffer->imgProps.compressedSize;
        }
        str = ss.str();
    }
    else if ( RIDE_HAL_BUFFER_TYPE_TENSOR == pSharedBuffer->type )
    {
        ss << "Tensor type=" << pSharedBuffer->tensorProps.type << " dims=[";
        for ( uint32_t i = 0; i < pSharedBuffer->tensorProps.numDims; i++ )
        {
            ss << pSharedBuffer->tensorProps.dims[i] << ", ";
        }
        ss << "]";
        str = ss.str();
    }
    else
    {
    }

    return str;
}

BufferManager::BufferManager() {}

BufferManager::~BufferManager() {}

RideHalError_e BufferManager::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        fprintf( stderr, "WARINING: failed to init logger for BUFMGR %s: ret = %d\n", pName, ret );
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
        RIDEHAL_ERROR( "buffer is nullptr" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }

    std::lock_guard<std::mutex> l( m_lock );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_IDAllocator++;
        pSharedBuffer->buffer.id = m_IDAllocator;
        m_bufferMap[pSharedBuffer->buffer.id] = *pSharedBuffer;
        RIDEHAL_INFO( "register %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                      pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
                      pSharedBuffer->buffer.size, pSharedBuffer->buffer.id,
                      GetBufferTextInfo( pSharedBuffer ).c_str() );
    }

    return ret;
}

RideHalError_e BufferManager::Deregister( uint64_t id )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    std::lock_guard<std::mutex> l( m_lock );

    auto it = m_bufferMap.find( id );
    if ( it != m_bufferMap.end() )
    {
        RideHal_SharedBuffer_t &sharedBuffer = it->second;
        RIDEHAL_INFO( "deregister %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                      sharedBuffer.buffer.pData, sharedBuffer.buffer.dmaHandle,
                      sharedBuffer.buffer.size, sharedBuffer.buffer.id,
                      GetBufferTextInfo( &sharedBuffer ).c_str() );
        m_bufferMap.erase( it );
    }
    else
    {
        RIDEHAL_ERROR( "buffer %" PRIu64 " not existed", id );
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
        std::lock_guard<std::mutex> l( m_lock );
        auto it = m_bufferMap.find( id );
        if ( it != m_bufferMap.end() )
        {
            *pSharedBuffer = it->second;
        }
        else
        {
            RIDEHAL_ERROR( "buffer %" PRIu64 " not found", id );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
