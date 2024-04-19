//  Copyright 2020-2022 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#include "FadasSrv.hpp"

#include <rpcmem.h>

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

std::mutex FadasSrv::s_coreLock[RIDE_HAL_PROCESSOR_MAX];
std::mutex FadasSrv::s_FadasLock;
remote_handle64 FadasSrv::s_handle64[RIDE_HAL_PROCESSOR_MAX] = { 0, 0, 0, 0 };
bool FadasSrv::s_initialized[RIDE_HAL_PROCESSOR_MAX] = { false, false, false, false };
uint64_t FadasSrv::s_useRef[RIDE_HAL_PROCESSOR_MAX] = { 0, 0, 0, 0 };
std::map<void *, FadasSrv::MemInfo> FadasSrv::s_memMaps[RIDE_HAL_PROCESSOR_MAX];
int FadasSrv::s_client = 1;

extern "C"
{
    int get_extended_domains_id( int domain, int session );
    void remote_register_buf_v2( int ext_domain_id, void *buf, int size, int fd );
    void remote_register_buf_attr_v2( int ext_domain_id, void *buf, int size, int fd, int attr );
}

RideHalError_e FadasSrv::InitCPU()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( FADAS_ERROR_NONE != FadasRemap_Init( nullptr ) )
    {
        RIDEHAL_ERROR( "FadasRemap_Init failed!" );
        ret = RIDE_HAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e FadasSrv::InitDSP( RideHal_ProcessorType_e coreId )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    std::string envName = "RIDEHAL_FADAS_CLIENT_ID";
    char *envValue = getenv( envName.c_str() );
    if ( nullptr != envValue )
    {
        s_client = atoi( envValue );
    }
    if ( ( 0 > s_client ) || ( 12 < s_client ) )
    {
        s_client = 1;
        RIDEHAL_INFO( "Invalid client id = %d, reset to 1!", s_client );
    }

    std::string uriFadas = FadasIface_URI;
    std::string uriDomain = CDSP_DOMAIN;
    if ( RIDE_HAL_PROCESSOR_HTP1 == coreId )
    {
        uriDomain = CDSP1_DOMAIN;
    }
    std::string uriClient = FADAS_CLIENT_URI + std::to_string( s_client );
    std::string uriFull = uriFadas + uriDomain + uriClient;
    const char *uri = const_cast<char *>( uriFull.c_str() );
    RIDEHAL_INFO( "uri = %s", uri );

    remote_handle64 handle64 = 0;
    int domain = CDSP_DOMAIN_ID;
    if ( RIDE_HAL_PROCESSOR_HTP1 == coreId )
    {
        domain = CDSP1_DOMAIN_ID;
    }

    if ( 0 == s_handle64[coreId] )
    {
        // Use unsigned PD for DSP
        if ( remote_session_control )
        {
            struct remote_rpc_control_unsigned_module data;
            data.enable = 1;
            data.domain = domain;
            int nErr = remote_session_control( DSPRPC_CONTROL_UNSIGNED_MODULE,
                                               reinterpret_cast<void *>( &data ), sizeof( data ) );
        }
        else
        {
            RIDEHAL_ERROR( "Unsigned PD not supported on this device!" );
            ret = RIDE_HAL_ERROR_FAIL;
        }

        auto retVal = FadasIface_open( uri, &handle64 );
        if ( AEE_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Failed to open fadas: %d", retVal );
            ret = RIDE_HAL_ERROR_FAIL;
        }

        s_handle64[coreId] = handle64;
    }
    else
    {
        handle64 = s_handle64[coreId];
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        int32_t ans = 0xFFFFFFFF;
        FadasIface_FadasInit( handle64, &ans );
        if ( FADAS_ERROR_NONE != ans )
        {
            RIDEHAL_ERROR( "FAILED:  FadasIface_FadasInit - 0x%x", ans );
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            constexpr size_t FADAS_VERSION_LEN = 64;
            char version[FADAS_VERSION_LEN] = { 0 };
            auto retVal = FadasIface_FadasVersion( handle64, reinterpret_cast<uint8_t *>( version ),
                                                   FADAS_VERSION_LEN );
            if ( retVal != AEE_SUCCESS )
            {
                RIDEHAL_ERROR( "FAILED: FadasIface_FadasVersion - 0x%x", retVal );
                ret = RIDE_HAL_ERROR_FAIL;
            }
            else
            {
                RIDEHAL_INFO( "Initialized FastADAS(DSP, %s) successfully", version );
            }
        }
    }

    return ret;
}

RideHalError_e FadasSrv::Init( RideHal_ProcessorType_e coreId, const char *pName,
                               Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( ( RIDE_HAL_PROCESSOR_HTP0 == coreId ) || ( RIDE_HAL_PROCESSOR_HTP1 == coreId ) ||
         ( RIDE_HAL_PROCESSOR_CPU == coreId ) || ( RIDE_HAL_PROCESSOR_GPU == coreId ) )
    {
        m_processor = coreId;
    }
    else
    {
        RIDEHAL_ERROR( "Invalid processor type!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( false == s_initialized[coreId] ) && ( RIDE_HAL_ERROR_NONE == ret ) )
    {
        if ( ( RIDE_HAL_PROCESSOR_HTP0 == coreId ) || ( RIDE_HAL_PROCESSOR_HTP1 == coreId ) )
        {
            ret = InitDSP( coreId );
        }
        else
        {
            ret = InitCPU();
        }

        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            s_initialized[coreId] = true;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        s_useRef[coreId]++;
    }

    return ret;
}

RideHalError_e FadasSrv::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( s_initialized[m_processor] && ( s_useRef[m_processor] > 0 ) )
    {
        s_useRef[m_processor]--;
        if ( 0 == s_useRef[m_processor] )
        {
            auto &memMap = s_memMaps[m_processor];
            std::vector<void *> ptrs;
            for ( auto &it : memMap )
            {
                ptrs.push_back( it.first );
            }
            for ( auto ptr : ptrs )
            {
                DeregBuf( ptr );
            }
            if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
                 ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
            {
                FadasIface_FadasDeInit( s_handle64[m_processor] );
            }
            else
            {
                if ( FADAS_ERROR_NONE != FadasRemap_DeInit() )
                {
                    RIDEHAL_ERROR( "FadasRemap_DeInit failed!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
            }
            memMap.clear();
            s_initialized[m_processor] = false;
        }
    }

    return ret;
}

remote_handle64 FadasSrv::GetRemoteHandle64()
{
    return s_handle64[m_processor];
}

int32_t FadasSrv::RegBuf( const RideHal_SharedBuffer_t *pBuffer, FadasBufType_e bufferType )
{
    std::lock_guard<std::mutex> l( s_coreLock[m_processor] );
    int32_t fd = -1;

    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "null buffer!" );
    }
    else if ( RIDE_HAL_BUFFER_TYPE_IMAGE != pBuffer->type )
    {
        RIDEHAL_ERROR( "Shared buffer type is not image!" );
    }
    else
    {
        auto &memMap = s_memMaps[m_processor];
        auto handle64 = s_handle64[m_processor];
        void *ptr = pBuffer->buffer.pData;
        size_t size = pBuffer->buffer.size;
        size_t offset = pBuffer->offset;
        uint32_t batch = pBuffer->imgProps.batchSize;
        int dmaHandle = (int) pBuffer->buffer.dmaHandle;
        size_t sizeOne = ( size_t )( pBuffer->size / batch );

        auto it = memMap.find( pBuffer->data() );
        if ( it == memMap.end() )
        {
            if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
                 ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
            {
                int extDomainId = 0;
                int domain = CDSP_DOMAIN_ID;
                if ( RIDE_HAL_PROCESSOR_HTP1 == m_processor )
                {
                    domain = CDSP1_DOMAIN_ID;
                }
                int client = s_client;
                extDomainId = get_extended_domains_id( domain, client );

                fd = rpcmem_to_fd( ptr );
                if ( fd < 0 )
                {
#if defined( __QNXNTO__ )
                    remote_register_buf_v2( extDomainId, ptr, size, 0 );
#else
                    remote_register_buf_v2( extDomainId, ptr, size, dmaHandle );
#endif
                    fd = rpcmem_to_fd( (void *) pBuffer->buffer.pData );
                    if ( fd < 0 )
                    {
                        RIDEHAL_ERROR( "rpcmem_to_fd failed, fd = %d", fd );
                    }
                }

                auto nErr = fastrpc_mmap( extDomainId, fd, ptr, 0, size, FASTRPC_MAP_FD_DELAYED );
                if ( ( AEE_EALREADY != nErr ) && ( AEE_SUCCESS != nErr ) )
                {
                    RIDEHAL_ERROR( "Failed to fastrpc_mmap ptr %p(%d, %llu): ret = %d\n", ptr, fd,
                                   size, nErr );
                    fd = -1;
                }

                auto ret = FadasIface_mmap( handle64, fd, (uint32_t) size );
                if ( AEE_SUCCESS != ret )
                {
                    RIDEHAL_ERROR( "Failed to map ptr %p(%d, %llu): ret = %d\n", ptr, fd, size,
                                   ret );
                    fd = -1;
                }

                uint32_t status = 0;
                if ( FADAS_BUF_TYPE_IN == bufferType )
                {
                    ret = FadasIface_FadasRegBuf( handle64, FADAS_BUF_TYPE_IN_NSP, fd, sizeOne,
                                                  offset, batch );
                }
                else if ( FADAS_BUF_TYPE_OUT == bufferType )
                {
                    ret = FadasIface_FadasRegBuf( handle64, FADAS_BUF_TYPE_OUT_NSP, fd, sizeOne,
                                                  offset, batch );
                }
                else if ( FADAS_BUF_TYPE_INOUT == bufferType )
                {
                    ret = FadasIface_FadasRegBuf( handle64, FADAS_BUF_TYPE_INOUT_NSP, fd, sizeOne,
                                                  offset, batch );
                }
                else
                {
                    RIDEHAL_ERROR( "Wrong buffer type = %d", bufferType );
                    fd = -1;
                }

                if ( ( AEE_SUCCESS != ret ) || ( FADAS_ERROR_NONE != (FadasError_e) status ) )
                {
                    RIDEHAL_ERROR( "Failed to register ptr %p(%d, %llu): ret = %d\n", ptr, fd, size,
                                   ret );
                    fd = -1;
                }
            }
            else
            {
                for ( int i = 0; i < batch; i++ )
                {
                    (void) FadasRegBuf( bufferType,
                                        (uint8_t *) pBuffer->data() + offset + sizeOne * i,
                                        sizeOne );
                }
                fd = 1;   // virtual fd for CPU&GPU pipeline, indicates that the register is
                          // successful, would not be really used.
            }
            memMap[ptr] = { fd, size, offset, batch, ptr, sizeOne };
        }
        else
        {
            if ( pBuffer->buffer.pData != it->second.ptr )
            {
                RIDEHAL_ERROR(
                        "Shared buffer already registered, but stored ptr not match, stored %d "
                        "and given %d",
                        it->second.ptr, pBuffer->buffer.pData );
            }
            else if ( pBuffer->buffer.size != it->second.size )
            {
                RIDEHAL_ERROR( "Shared buffer already registered, but stored size not match, "
                               "stored %d "
                               "and given %d",
                               it->second.size, pBuffer->buffer.size );
            }
            else if ( pBuffer->offset != it->second.offset )
            {
                RIDEHAL_ERROR( "Shared buffer already registered, but stored offset not match, "
                               "stored %d "
                               "and given %d",
                               it->second.offset, pBuffer->offset );
            }
            else if ( pBuffer->imgProps.batchSize != it->second.batch )
            {
                RIDEHAL_ERROR( "Shared buffer already registered, but stored batch not match, "
                               "stored %d "
                               "and given %d",
                               it->second.batch, pBuffer->imgProps.batchSize );
            }
            else
            {
                fd = it->second.fd;
            };
        }
    }

    return fd;
}

void FadasSrv::DeregBuf( void *pBuffer )
{
    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "null buffer!" );
    }
    else
    {
        std::lock_guard<std::mutex> l( s_coreLock[m_processor] );
        auto &memMap = s_memMaps[m_processor];
        auto handle64 = s_handle64[m_processor];
        auto it = memMap.find( pBuffer );
        if ( it != memMap.end() )
        {
            int32_t fd = it->second.fd;
            size_t size = it->second.size;
            size_t offset = it->second.offset;
            uint32_t batch = it->second.batch;
            void *ptr = it->second.ptr;
            size_t sizeOne = it->second.sizeOne;
            memMap.erase( it );

            if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
                 ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
            {
                int extDomainId = 0;
                int domain = CDSP_DOMAIN_ID;
                if ( RIDE_HAL_PROCESSOR_HTP1 == m_processor )
                {
                    domain = CDSP1_DOMAIN_ID;
                }
                int client = s_client;
                extDomainId = get_extended_domains_id( domain, client );
                FadasIface_FadasDeregBuf( handle64, fd, sizeOne, offset, batch );
                FadasIface_munmap( handle64, fd, (uint32_t) size );
                fastrpc_munmap( extDomainId, fd, ptr, size );
                remote_register_buf_v2( extDomainId, ptr, size, -1 );
            }
            else
            {
                for ( int i = 0; i < batch; i++ )
                {
                    FadasDeregBuf( (uint8_t *) pBuffer + sizeOne * i );
                }
            }
        }
    }

    return;
}

FadasRemap::FadasRemap() {}

FadasRemap::~FadasRemap() {}

RideHalError_e FadasRemap::SetRemapParams( uint32_t numOfInputs, uint32_t outputWidth,
                                           uint32_t outputHeight,
                                           RideHal_ImageFormat_e outputFormat,
                                           FadasNormlzParams_t normlzR, FadasNormlzParams_t normlzG,
                                           FadasNormlzParams_t normlzB, bool bEnableUndistortion,
                                           bool bEnableNormalize )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_IMAGE_FORMAT_RGB888 != outputFormat ) &&
         ( RIDE_HAL_IMAGE_FORMAT_BGR888 != outputFormat ) )
    {
        RIDEHAL_ERROR( "Invalid output format!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDE_HAL_MAX_INPUTS <= numOfInputs )
    {
        RIDEHAL_ERROR( "Invalid number of inputs!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        m_handle64 = GetRemoteHandle64();
        m_numOfInputs = numOfInputs;
        m_outputFormat = outputFormat;
        m_outputWidth = outputWidth;
        m_outputHeight = outputHeight;
        m_normlz[0] = normlzR;
        m_normlz[1] = normlzG;
        m_normlz[2] = normlzB;
        m_bEnableUndistortion = bEnableUndistortion;
        m_bEnableNormalize = bEnableNormalize;
    }

    return ret;
};

FadasRemapPipeline_e FadasRemap::RemapGetPipelineCPU( RideHal_ImageFormat_e inputFormat,
                                                      RideHal_ImageFormat_e outputFormat,
                                                      bool bEnableNormalize )
{
    FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX;

    if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
         ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
         ( true == bEnableNormalize ) )   // UYVY to RGB normalize pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to RGB pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // RGB to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_3C888;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to BGR pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_BGR888;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( true == bEnableNormalize ) )   // NV12 to RGB normalize pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to BGR pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888;
    }
    else
    {
        RIDEHAL_ERROR( "Invalid remap pipeline" );
    }

    return pipeline;
}

FadasIface_FadasRemapPipeline_e FadasRemap::RemapGetPipelineDSP( RideHal_ImageFormat_e inputFormat,
                                                                 RideHal_ImageFormat_e outputFormat,
                                                                 bool bEnableNormalize )
{
    FadasIface_FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX_NSP;

    if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
         ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
         ( true == bEnableNormalize ) )   // UYVY to RGB normalize pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to RGB pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // RGB to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_3C888_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to BGR pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_BGR888_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( true == bEnableNormalize ) )   // NV12 to RGB normalize pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8_NSP;
    }
    else if ( ( RIDE_HAL_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( RIDE_HAL_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to BGR pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888_NSP;
    }
    else
    {
        RIDEHAL_ERROR( "Invalid remap pipeline for inputformat = %d, outputfprmat = %d, "
                       "bEnableNormalize = %d ",
                       inputFormat, outputFormat, bEnableNormalize );
    }

    return pipeline;
}

RideHalError_e FadasRemap::CreatRemapTable( uint32_t inputId, uint32_t mapWidth, uint32_t mapHeight,
                                            float *pMapX, float *pMapY )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_mapWidths[inputId] = mapWidth;
    m_mapHeights[inputId] = mapHeight;

    if ( ( true == m_bEnableUndistortion ) && ( ( nullptr == pMapX ) || ( nullptr == pMapY ) ) )
    {
        RIDEHAL_ERROR( "Null remap pointer!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
             ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
        {
            FadasIface_FadasRemapPipeline_e pipeline = RemapGetPipelineDSP(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_NSP == pipeline )
            {
                RIDEHAL_ERROR( "Invalid remap pipelie for DSP!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                uint64 remapPtr = 0;
                uint64 retVal = 0;
                if ( true == m_bEnableUndistortion )
                {
                    retVal = FadasIface_FadasRemap_CreateMapFromMap(
                            m_handle64, &remapPtr, m_inputWidths[inputId], m_inputHeights[inputId],
                            m_mapWidths[inputId], m_mapHeights[inputId], pMapX,
                            m_mapHeights[inputId] * m_inputWidths[inputId], pMapY,
                            m_mapHeights[inputId] * m_inputWidths[inputId],
                            m_inputWidths[inputId] * sizeof( float ), pipeline, 0 );
                }
                else
                {
                    retVal = FadasIface_FadasRemap_CreateMapNoUndistortion(
                            m_handle64, &remapPtr, m_inputWidths[inputId], m_inputHeights[inputId],
                            m_mapWidths[inputId], m_mapHeights[inputId], pipeline, 0 );
                }

                if ( AEE_SUCCESS != retVal )
                {
                    RIDEHAL_ERROR( "Failed to create a remap map for DSP!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    m_remapPtrs[inputId] = remapPtr;
                }
            }
        }
        else
        {
            FadasRemapPipeline_e pipeline = RemapGetPipelineCPU(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );

            if ( FADAS_REMAP_PIPELINE_MAX == pipeline )
            {
                RIDEHAL_ERROR( "Invalid remap pipelie for CPU!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                FadasRemapMap_t *remapPtr = nullptr;
                if ( true == m_bEnableUndistortion )
                {
                    remapPtr = FadasRemap_CreateMapFromMap(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], m_inputWidths[inputId] * sizeof( float ), pMapX,
                            pMapY, pipeline, 0 );
                }
                else
                {
                    remapPtr = FadasRemap_CreateMapNoUndistortion(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], pipeline, 0 );
                }

                if ( remapPtr == nullptr )
                {
                    RIDEHAL_ERROR( "Failed to create a remap map for CPU!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    m_remapPtrs[inputId] = (uint64) remapPtr;
                }
            }
        }
    }

    return ret;
}

RideHalError_e FadasRemap::CreateRemapWorker( uint32_t inputId, RideHal_ImageFormat_e inputFormat,
                                              uint32_t inputWidth, uint32_t inputHeight,
                                              FadasROI_t ROI )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_inputFormats[inputId] = inputFormat;
    m_inputWidths[inputId] = inputWidth;
    m_inputHeights[inputId] = inputHeight;
    m_ROIs[inputId] = ROI;

    if ( ( RIDE_HAL_IMAGE_FORMAT_UYVY != m_inputFormats[inputId] ) &&
         ( RIDE_HAL_IMAGE_FORMAT_RGB888 != m_inputFormats[inputId] ) &&
         ( RIDE_HAL_IMAGE_FORMAT_NV12 != m_inputFormats[inputId] ) )
    {
        RIDEHAL_ERROR( "Invalid input format for inputId = %d ", inputId );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
             ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
        {
            FadasIface_FadasRemapPipeline_e pipeline = RemapGetPipelineDSP(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_NSP == pipeline )
            {
                RIDEHAL_ERROR( "Invalid remap pipelie for DSP!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                uint64 workerPtr = 0;
                auto retVal =
                        FadasIface_FadasRemap_CreateWorkers( m_handle64, &workerPtr, 4, pipeline );
                if ( AEE_SUCCESS != retVal )
                {
                    RIDEHAL_ERROR( "Failed to create a remap worker for DSP!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    m_workerPtrs[inputId] = workerPtr;
                }
            }
        }
        else
        {
            int32_t pThreadsAffinity[] = { 0, 1, 2, 3 };
            FadasRemapPipeline_e pipeline = RemapGetPipelineCPU(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX == pipeline )
            {
                RIDEHAL_ERROR( "Invalid remap pipelie for CPU!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                void *workerPtr = FadasRemap_CreateWorkers( 4, pThreadsAffinity, pipeline );
                if ( workerPtr == nullptr )
                {
                    RIDEHAL_ERROR( "Failed to create a remap worker for CPU!" );
                    ret = RIDE_HAL_ERROR_FAIL;
                }
                else
                {
                    m_workerPtrs[inputId] = (uint64) workerPtr;
                }
            }
        }
    }
    return ret;
}

RideHalError_e FadasRemap::RemapRunCPU( const RideHal_SharedBuffer_t *inputs,
                                        const RideHal_SharedBuffer_t *output )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    /*call unified RegBuf functions even the buffers may have been registered*/
    int32_t srcFds[RIDE_HAL_MAX_INPUTS];
    int32_t dstFd;
    for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
    {
        const RideHal_SharedBuffer_t *input = &inputs[inputId];
        srcFds[inputId] = RegBuf( input, FADAS_BUF_TYPE_IN );
        if ( srcFds[inputId] < 0 )
        {
            RIDEHAL_ERROR( "Input Buffer register failed!" );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
            break;
        }
    }
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        dstFd = RegBuf( output, FADAS_BUF_TYPE_OUT );
        if ( dstFd < 0 )
        {
            RIDEHAL_ERROR( "Output buffer register failed!" );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        size_t outputSize = output->size / output->imgProps.batchSize;
        for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
        {
            uint8_t *pSrc = (uint8_t *) inputs[inputId].data();
            uint8_t *pDst = (uint8_t *) output->data() + inputId * outputSize;

            FadasImage_t srcImg;
            srcImg.props.width = inputs[inputId].imgProps.width;
            srcImg.props.height = inputs[inputId].imgProps.height;
            srcImg.props.numPlanes = inputs[inputId].imgProps.numPlanes;
            for ( int i = 0; i < inputs[inputId].imgProps.numPlanes; i++ )
            {
                srcImg.props.stride[i] = inputs[inputId].imgProps.stride[i];
            }
            srcImg.plane[0] = pSrc;
            if ( RIDE_HAL_IMAGE_FORMAT_UYVY == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_UYVY;
            }
            else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
            }
            else if ( RIDE_HAL_IMAGE_FORMAT_NV12 == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_Y8UV8;
                srcImg.plane[1] =
                        pSrc + srcImg.props.stride[0] * inputs[inputId].imgProps.actualHeight[0];
            }
            else
            {
                RIDEHAL_ERROR( "Invalid input format for inputId = %d!", inputId );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            srcImg.bAllocated = false;

            FadasImage_t rgbImg;
            rgbImg.props.width = output->imgProps.width;
            rgbImg.props.height = output->imgProps.height;
            rgbImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
            rgbImg.props.numPlanes = output->imgProps.numPlanes;
            for ( int i = 0; i < output->imgProps.numPlanes; i++ )
            {
                rgbImg.props.stride[i] = output->imgProps.stride[i];
            }
            rgbImg.plane[0] = pDst;
            rgbImg.bAllocated = false;

            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                FadasROI_t roi = m_ROIs[inputId];
                FadasRemapMap_t *remapPtr = (FadasRemapMap_t *) m_remapPtrs[inputId];
                void *workerPtr = (void *) m_workerPtrs[inputId];

                FadasError_e retFadas;
                if ( false == m_bEnableNormalize )
                {
                    retFadas = FadasRemap_RunMT( workerPtr, remapPtr, &srcImg, &rgbImg, &roi );
                }
                else
                {
                    retFadas = FadasRemap_RunMT( workerPtr, remapPtr, &srcImg, &rgbImg, &roi, 1.0,
                                                 m_normlz );
                }

                if ( FADAS_ERROR_NONE != retFadas )
                {
                    RIDEHAL_ERROR( "Remap888 failed for batch %d: ret = 0x%x", inputId, ret );
                    ret = RIDE_HAL_ERROR_FAIL;
                    break;
                }
            }
        }
    }

    return ret;
}

RideHalError_e FadasRemap::RemapRunDSP( const RideHal_SharedBuffer_t *inputs,
                                        const RideHal_SharedBuffer_t *output )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    /*call unified RegBuf functions even the buffers may have been registered*/
    int32_t srcFds[RIDE_HAL_MAX_INPUTS];
    int32_t dstFd;
    for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
    {
        const RideHal_SharedBuffer_t *input = &inputs[inputId];
        srcFds[inputId] = RegBuf( input, FADAS_BUF_TYPE_IN );
        if ( srcFds[inputId] < 0 )
        {
            RIDEHAL_ERROR( "Input Buffer register failed!" );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
            break;
        }
    }
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        dstFd = RegBuf( output, FADAS_BUF_TYPE_OUT );
        if ( dstFd < 0 )
        {
            RIDEHAL_ERROR( "Output buffer register failed!" );
            ret = RIDE_HAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        size_t outputSize = output->size / output->imgProps.batchSize;
        FadasIface_FadasROI_t ROIs[RIDE_HAL_MAX_INPUTS];
        FadasIface_FadasImgProps_t srcImgProps[RIDE_HAL_MAX_INPUTS];
        uint32_t offsets[RIDE_HAL_MAX_INPUTS];

        for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
        {
            FadasIface_FadasImgProps_t srcImgProp;
            srcImgProp.width = inputs[inputId].imgProps.width;
            srcImgProp.height = inputs[inputId].imgProps.height;
            srcImgProp.numPlanes = inputs[inputId].imgProps.numPlanes;
            for ( int i = 0; i < inputs[inputId].imgProps.numPlanes; i++ )
            {
                srcImgProp.stride[i] = inputs[inputId].imgProps.stride[i];
            }
            for ( int i = 0; i < inputs[inputId].imgProps.numPlanes; i++ )
            {
                srcImgProp.actualHeight[i] = inputs[inputId].imgProps.actualHeight[i];
            }
            if ( RIDE_HAL_IMAGE_FORMAT_UYVY == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_UYVY_NSP;
            }
            else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
            }
            else if ( RIDE_HAL_IMAGE_FORMAT_NV12 == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_Y8UV8_NSP;
            }
            else
            {
                RIDEHAL_ERROR( "Invalid input format for inputId = %d!", inputId );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            srcImgProps[inputId] = srcImgProp;
            offsets[inputId] = 0;
            ROIs[inputId].x = m_ROIs[inputId].x;
            ROIs[inputId].y = m_ROIs[inputId].y;
            ROIs[inputId].width = m_ROIs[inputId].width;
            ROIs[inputId].height = m_ROIs[inputId].height;
        }

        FadasIface_FadasImgProps_t dstImgProp;
        dstImgProp.width = output->imgProps.width;
        dstImgProp.height = output->imgProps.height;
        dstImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
        dstImgProp.numPlanes = output->imgProps.numPlanes;
        for ( int i = 0; i < output->imgProps.numPlanes; i++ )
        {
            dstImgProp.stride[i] = output->imgProps.stride[i];
        }

        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            AEEResult retV;
            if ( false == m_bEnableNormalize )
            {
                retV = FadasIface_FadasRemap_RunMT(
                        m_handle64, m_workerPtrs, m_numOfInputs, m_remapPtrs, m_numOfInputs, srcFds,
                        m_numOfInputs, offsets, m_numOfInputs, srcImgProps, m_numOfInputs, dstFd,
                        outputSize, &dstImgProp, ROIs, m_numOfInputs, nullptr, 0 );
            }
            else
            {
                FadasIface_FadasNormlzParams_t normlz[3];
                normlz[0].sub = m_normlz[0].sub;
                normlz[0].mul = m_normlz[0].mul;
                normlz[0].add = m_normlz[0].add;
                normlz[1].sub = m_normlz[1].sub;
                normlz[1].mul = m_normlz[1].mul;
                normlz[1].add = m_normlz[1].add;
                normlz[2].sub = m_normlz[2].sub;
                normlz[2].mul = m_normlz[2].mul;
                normlz[2].add = m_normlz[2].add;
                retV = FadasIface_FadasRemap_RunMT(
                        m_handle64, m_workerPtrs, m_numOfInputs, m_remapPtrs, m_numOfInputs, srcFds,
                        m_numOfInputs, offsets, m_numOfInputs, srcImgProps, m_numOfInputs, dstFd,
                        outputSize, &dstImgProp, ROIs, m_numOfInputs, normlz, 3 );
            }
            if ( retV != AEE_SUCCESS )
            {
                RIDEHAL_ERROR( "Remap888 failed: ret = 0x%x", retV );
                ret = RIDE_HAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e FadasRemap::RemapRun( const RideHal_SharedBuffer_t *inputs,
                                     const RideHal_SharedBuffer_t *output )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr == inputs )
    {
        RIDEHAL_ERROR( "NULL pointer for input buffers!" );
    }
    else if ( nullptr == output )
    {
        RIDEHAL_ERROR( "NULL pointer for output buffer!" );
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
        {
            if ( m_inputFormats[inputId] != inputs[inputId].imgProps.format )
            {
                RIDEHAL_ERROR( "Format in input buffer and config not match!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            else if ( m_inputWidths[inputId] != inputs[inputId].imgProps.width )
            {
                RIDEHAL_ERROR( "Width in input buffer and config not match!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            else if ( m_inputHeights[inputId] != inputs[inputId].imgProps.height )
            {
                RIDEHAL_ERROR( "Height in input buffer and config not match!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            else if ( 1 != inputs[inputId].imgProps.batchSize )
            {
                RIDEHAL_ERROR( "Batch in input buffer must be 1!" );
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                break;
            }
        }

        if ( m_outputFormat != output->imgProps.format )
        {
            RIDEHAL_ERROR( "Format in output buffer and config not match!" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( m_outputWidth != output->imgProps.width )
        {
            RIDEHAL_ERROR( "Width in output buffer and config not match!" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( m_outputHeight != output->imgProps.height )
        {
            RIDEHAL_ERROR( "Height in output buffer and config not match!" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( m_numOfInputs != output->imgProps.batchSize )
        {
            RIDEHAL_ERROR( "Batch in output buffer and config not match!" );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) ||
                 ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
            {
                ret = RemapRunDSP( inputs, output );
            }
            else
            {
                ret = RemapRunCPU( inputs, output );
            }
        }
    }

    return ret;
}

RideHalError_e FadasRemap::DestroyWorkers()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) || ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
    {
        for ( int i = 0; i < m_numOfInputs; i++ )
        {
            FadasIface_FadasRemap_DestroyWorkers(
                    0, m_workerPtrs[i] );   // handle not really need for DestroyWorkers
        }
    }
    else
    {
        for ( int i = 0; i < m_numOfInputs; i++ )
        {
            FadasRemap_DestroyWorkers( reinterpret_cast<void *>( m_workerPtrs[i] ) );
        }
    }

    return ret;
}

RideHalError_e FadasRemap::DestroyMap()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_PROCESSOR_HTP0 == m_processor ) || ( RIDE_HAL_PROCESSOR_HTP1 == m_processor ) )
    {
        for ( int i = 0; i < m_numOfInputs; i++ )
        {
            FadasIface_FadasRemap_DestroyMap(
                    0, m_remapPtrs[i] );   // handle not really need for DestroyMap
        }
    }
    else
    {
        for ( int i = 0; i < m_numOfInputs; i++ )
        {
            FadasRemap_DestroyMap( reinterpret_cast<FadasRemapMap *>( m_remapPtrs[i] ) );
        }
    }

    return ret;
}

}   // namespace FadasIface
}   // namespace libs
}   // namespace ridehal