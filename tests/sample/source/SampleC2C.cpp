// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SampleC2C.hpp"

#define C2C_MAX_PIO_MSG_SIZE 256

namespace ridehal
{
namespace sample
{

typedef enum
{
    C2C_MSG_DMA_INIT = 1,
    C2C_MSG_DMA_SETUP = 2,
    C2C_MSG_DMA_READY = 3,
    C2C_MSG_DMA_STOP = 4,
} C2C_MsgType_t;

typedef struct
{
    C2C_MsgType_t type;
    uint32_t bufIdx;
    uint64_t frameId;
    uint64_t timestamp;
    union
    {
        /* data structure for connecting DMA channels between two devices */
        struct
        {
            uint64_t bufId;   /* Buffer ID to open a DMA connection and logical frame buffer */
            uint32_t bufSize; /* Receiver buffer size */
        } dma;
    };
} C2C_PioMsg_t;


SampleC2C::SampleC2C() {}
SampleC2C::~SampleC2C() {}

RideHalError_e SampleC2C::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    auto typeStr = Get( config, "type", "pub" );
    if ( "pub" == typeStr )
    {
        m_bIsPub = true;
    }
    else if ( "sub" == typeStr )
    {
        m_bIsPub = false;
    }
    else
    {
        RIDEHAL_ERROR( "invalid type <%s>\n", typeStr.c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( true == m_bIsPub )
    {
        m_buffersName = Get( config, "buffers_name", "" );
        if ( "" == m_buffersName )
        {
            RIDEHAL_ERROR( "no buffers_name\n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_queueDepth = Get( config, "queue_depth", 2u );
    m_width = Get( config, "width", 0u );
    if ( 0 == m_width )
    {
        RIDEHAL_ERROR( "invalid width" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    m_height = Get( config, "height", 0u );
    if ( 0 == m_height )
    {
        RIDEHAL_ERROR( "invalid height" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    m_format = Get( config, "format", RIDEHAL_IMAGE_FORMAT_NV12 );
    if ( RIDEHAL_IMAGE_FORMAT_MAX == m_format )
    {
        RIDEHAL_ERROR( "invalid format\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    m_poolSize = Get( config, "pool_size", 4u );
    m_channleId = Get( config, "channel", 0u );

    return ret;
}

RideHalError_e SampleC2C::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    TRACE_BEGIN( SYSTRACE_TASK_INIT );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_buffers.resize( m_poolSize );
        m_dmaInfos.resize( m_poolSize );
        if ( m_bIsPub )
        {
            ret = m_sub.Init( name, m_topicName, m_queueDepth );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = SampleIF::GetBuffers( m_buffersName, m_buffers.data(), m_poolSize );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Can't find input buffers: %s", m_buffersName.c_str() );
                }
            }
        }
        else
        {
            ret = m_pub.Init( name, m_topicName );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, m_width, m_height,
                                        m_format );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    ret = m_imagePool.GetBuffers( m_buffers.data(), m_poolSize );
                    /* release it, as just need the shared buffer image properties */
                    (void) m_imagePool.Deinit();
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_c2cFd = C2C_Open_Channel( m_channleId, C2C_MAX_PIO_MSG_SIZE );
        if ( m_c2cFd < 0 )
        {
            RIDEHAL_ERROR( "C2C open %u failed: %d", m_channleId, m_c2cFd );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == m_bIsPub ) )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &buffer = m_buffers[i];
            auto &dmaInfo = m_dmaInfos[i];
            m_dmaIndexMap[buffer.buffer.dmaHandle] = i;
            dmaInfo.dmaBufId = i;
            dmaInfo.dmaFd = C2C_Register_DMABuffer(
                    &dmaInfo.dmaBufId, (pmem_handle_t) buffer.buffer.dmaHandle, buffer.buffer.pData,
                    buffer.buffer.size, C2C_LOCAL_BUFFER );
            if ( dmaInfo.dmaFd < 0 )
            {
                RIDEHAL_ERROR( "C2C register DMA %u failed: %d", i, dmaInfo.dmaFd );
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( false == m_bIsPub ) )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &buffer = m_buffers[i];
            auto &dmaInfo = m_dmaInfos[i];
            m_dmaIndexMap[buffer.buffer.dmaHandle] = i;
            dmaInfo.dmaBufId = i;
            dmaInfo.dmaFd =
                    C2C_Request_DMABuffer( &dmaInfo.dmaBufId, &dmaInfo.pData, buffer.buffer.size );
            if ( dmaInfo.dmaFd < 0 )
            {
                RIDEHAL_ERROR( "C2C request DMA %u failed: %d", i, dmaInfo.dmaFd );
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
            else
            { /* update the virtual address */
                buffer.buffer.pData = dmaInfo.pData;
                buffer.buffer.dmaHandle = 0; /* not known */
            }
        }
    }

    TRACE_END( SYSTRACE_TASK_INIT );

    return ret;
}

RideHalError_e SampleC2C::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
    int rc;

    TRACE_BEGIN( SYSTRACE_TASK_START );

    if ( true == m_bIsPub )
    {
        /* notifier the receiver that we have initialised */
        msg.type = C2C_MSG_DMA_INIT;
        rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( C2C_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Send INIT failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "Send INIT DONE" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            for ( uint32_t i; i < m_poolSize; i++ )
            {
                auto &buffer = m_buffers[i];
                auto &dmaInfo = m_dmaInfos[i];
                rc = C2C_MsgRecv( m_c2cFd, (void *) &msg, sizeof( msg ) );
                if ( C2C_SUCCESS != rc )
                {
                    RIDEHAL_ERROR( "Recv SETUP for buffer %u failed: %d", i, rc );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else if ( C2C_MSG_DMA_SETUP != msg.type )
                {
                    RIDEHAL_ERROR( "Recv SETUP for buffer %u with invalid type: %d", i, msg.type );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else if ( msg.bufIdx != i )
                {
                    RIDEHAL_ERROR( "Recv SETUP for buffer %u with invalid idx: %u", i, msg.bufIdx );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else if ( buffer.buffer.size != msg.dma.bufSize )
                {
                    RIDEHAL_ERROR( "Recv SETUP for buffer %u with invalid size: %" PRIu64
                                   " != %" PRIu64,
                                   i, buffer.buffer.size, msg.dma.bufSize );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else
                {
                    RIDEHAL_DEBUG( "Recv SETUP for buffer %u", i );
                    rc = C2C_Connect_DMABuffer( dmaInfo.dmaFd, msg.dma.bufId, msg.dma.bufSize );
                    if ( C2C_SUCCESS != rc )
                    {
                        RIDEHAL_ERROR( "Connect DMA %u failed: %d", i, rc );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                    else
                    {
                        RIDEHAL_DEBUG( "Connect DMA %u Done", i );
                    }
                }

                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    break;
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleC2C::ThreadPubMain, this );
        }
    }
    else
    {
        /* wating for the initialization massasge from the sender */
        RIDEHAL_DEBUG( "Wait INIT" );
        rc = C2C_MsgRecv( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( ( C2C_SUCCESS != rc ) || ( C2C_MSG_DMA_INIT != msg.type ) )
        {
            RIDEHAL_ERROR( "Recv INIT failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "Recv INIT DONE" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            for ( uint32_t i; i < m_poolSize; i++ )
            {
                auto &buffer = m_buffers[i];
                auto &dmaInfo = m_dmaInfos[i];
                msg.type = C2C_MSG_DMA_SETUP;
                msg.bufIdx = i;
                msg.dma.bufId = dmaInfo.dmaBufId;
                msg.dma.bufSize = buffer.buffer.size;
                rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
                if ( C2C_SUCCESS != rc )
                {
                    RIDEHAL_ERROR( "Send SETUP for buffer %u failed: %d", i, rc );
                    ret = RIDEHAL_ERROR_FAIL;
                    break;
                }
                else
                {
                    RIDEHAL_DEBUG( "Send SETUP for buffer %u", i );
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleC2C::ThreadSubMain, this );
        }
    }
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

RideHalError_e SampleC2C::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
    int rc;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    if ( true == m_bIsPub )
    {
        msg.type = C2C_MSG_DMA_STOP;
        rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( C2C_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Send STOP for failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Disconnect_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                RIDEHAL_ERROR( "Disconnect DMA %u failed: %d", i, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleC2C::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rc;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    if ( true == m_bIsPub )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Free_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                RIDEHAL_ERROR( "Free DMA %u failed: %d", i, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }
    else
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Release_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                RIDEHAL_ERROR( "Free DMA %u failed: %d", i, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}


void SampleC2C::ThreadPubMain()
{
    RideHalError_e ret;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) C2C_MSG_DMA_READY };
    int rc;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            auto &buffer = frames.SharedBuffer( 0 );
            auto it = m_dmaIndexMap.find( buffer.buffer.dmaHandle );
            if ( it != m_dmaIndexMap.end() )
            {
                auto &dmaInfo = m_dmaInfos[it->second];
                rc = C2C_ScheduleDMA( dmaInfo.dmaFd, 0, /* local offset */
                                      0,                /* remote offset */
                                      buffer.buffer.size );
                if ( C2C_SUCCESS != rc )
                {
                    RIDEHAL_ERROR( "ScheduleDMA for buffer %u failed: %d", it->second, rc );
                }
                else
                {
                    msg.bufIdx = it->second;
                    msg.dma.bufId = dmaInfo.dmaBufId;
                    msg.dma.bufSize = buffer.buffer.size;
                    msg.frameId = frames.FrameId( 0 );
                    msg.timestamp = frames.Timestamp( 0 );
                    rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
                    if ( C2C_SUCCESS != rc )
                    {
                        RIDEHAL_ERROR( "Send READY for buffer %u failed: %d", it->second, rc );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                RIDEHAL_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

void SampleC2C::ThreadSubMain()
{
    RideHalError_e ret;
    int rc;
    while ( false == m_stop )
    {
        C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
        rc = C2C_MsgRecv( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( ( C2C_SUCCESS == rc ) && ( C2C_MSG_DMA_STOP == msg.type ) )
        {
            RIDEHAL_DEBUG( "Recv STOP" );
            break;
        }
        if ( ( C2C_SUCCESS != rc ) || ( C2C_MSG_DMA_READY != msg.type ) ||
             ( msg.bufIdx >= m_buffers.size() ) )
        {
            RIDEHAL_ERROR( "Recv READY failed: %d", rc );
        }
        else
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", msg.frameId,
                           msg.timestamp );
            PROFILER_BEGIN();
            TRACE_BEGIN( msg.frameId );
            SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
            pSharedBuffer->sharedBuffer = m_buffers[msg.bufIdx];
            pSharedBuffer->pubHandle = msg.bufIdx;
            std::shared_ptr<SharedBuffer_t> buffer(
                    pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) { delete pSharedBuffer; } );
            DataFrames_t frames;
            DataFrame_t frame;
            frame.frameId = msg.frameId;
            frame.buffer = buffer;
            frame.timestamp = msg.timestamp;
            frames.Add( frame );
            ret = m_pub.Publish( frames );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                PROFILER_END();
                TRACE_END( msg.frameId );
            }
            else
            {
                RIDEHAL_ERROR( "Failed to publish for %" PRIu64 " : %d", msg.frameId, ret );
            }
        }
    }
}

REGISTER_SAMPLE( C2C, SampleC2C );

}   // namespace sample
}   // namespace ridehal
