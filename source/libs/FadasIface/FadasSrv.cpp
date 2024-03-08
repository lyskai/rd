//  Copyright 2020-2022 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#include "FadasSrv.hpp"

#include <rpcmem.h>

namespace ride
{
namespace hal
{
namespace libs
{
namespace FadasIface
{

#ifndef CDSP_DOMAIN
#define CDSP_DOMAIN "&_dom=cdsp0"
#endif

#ifndef CDSP1_DOMAIN
#define CDSP1_DOMAIN "&_dom=cdsp1"
#endif

std::recursive_mutex FadasSrv::s_CoreLock[FadasSrv::Core::MAX];
std::mutex FadasSrv::s_FadasLock;
remote_handle64 FadasSrv::s_Handle64[FadasSrv::Core::MAX] = { 0, 0, 0 };
bool FadasSrv::s_Initialized[FadasSrv::Core::MAX] = { false, false, false };
uint64_t FadasSrv::s_UseRef[FadasSrv::Core::MAX] = { 0, 0, 0 };

std::map<void *, FadasSrv::MemInfo> FadasSrv::s_MemMaps[FadasSrv::Core::MAX];

extern "C"
{
    int get_extended_domains_id( int domain, int session );
    void remote_register_buf_v2( int ext_domain_id, void *buf, int size, int fd );
    void remote_register_buf_attr_v2( int ext_domain_id, void *buf, int size, int fd, int attr );
}

bool FadasSrv::init_cpu()
{
    if ( FadasCvtYUV_Init( nullptr ) != FADAS_ERROR_NONE )
    {
        // hogl::post( s_HoglArea, s_HoglArea->ERROR, "FadasCvtYUV_Init failed" );
        return false;
    }

    if ( FadasRemap_Init( nullptr ) != FADAS_ERROR_NONE )
    {
        // hogl::post( s_HoglArea, s_HoglArea->ERROR, "FadasRemap_Init failed" );
        return false;
    }

    // hogl::post( s_HoglArea, s_HoglArea->INFO, "Initialized FastADAS(CPU, %s)
    // successfully",FadasVersion() );
    return true;
}

bool FadasSrv::init_dsp( Core coreId )
{
    remote_handle64 handle64 = 0;

    const char *uri = FadasIface_URI CDSP_DOMAIN FADAS_CLIENT_URI;
    int domain = CDSP_DOMAIN_ID;
    if ( FadasSrv::Core::DSP1 == coreId )
    {
        uri = FadasIface_URI CDSP1_DOMAIN FADAS_CLIENT_URI;
        domain = CDSP1_DOMAIN_ID;
    }

    if ( 0 == s_Handle64[coreId] )
    {
        // Use unsigned PD for DSP
        if ( remote_session_control )
        {
            struct remote_rpc_control_unsigned_module data;
            data.enable = 1;
            data.domain = domain;
            int nErr = remote_session_control( DSPRPC_CONTROL_UNSIGNED_MODULE,
                                               reinterpret_cast<void *>( &data ), sizeof( data ) );
            // hogl::post(s_HoglArea, s_HoglArea->INFO,"remote_session_control returned 0x%x for
            // configuringunsigned PD on domain %d.",nErr, data.domain );
        }
        else
        {
            // hogl::post( s_HoglArea, s_HoglArea->WARN, "Unsigned PD not supported on this device."
            // );
        }

        auto ret = FadasIface_open( uri, &handle64 );
        if ( AEE_SUCCESS != ret )
        {
            // hogl::post( s_HoglArea, s_HoglArea->ERROR, "Failed to open fadas: %d", ret );
            return false;
        }

        s_Handle64[coreId] = handle64;
    }
    else
    {
        handle64 = s_Handle64[coreId];
    }

    int32_t ans = 0xFFFFFFFF;
    FadasIface_FadasInit( handle64, &ans );
    if ( FADAS_ERROR_NONE != ans )
    {
        // hogl::post( s_HoglArea, s_HoglArea->ERROR, "FAILED:  FadasIface_FadasInit - 0x%x", ans );
        return false;
    }

    constexpr size_t FADAS_VERSION_LEN = 64;
    char version[FADAS_VERSION_LEN] = { 0 };
    auto result = FadasIface_FadasVersion( handle64, reinterpret_cast<uint8_t *>( version ),
                                           FADAS_VERSION_LEN );
    if ( result != AEE_SUCCESS )
    {
        // hogl::post( s_HoglArea, s_HoglArea->ERROR, "FAILED: FadasIface_FadasVersion -
        // 0x%x",result );
        return false;
    }

    // hogl::post( s_HoglArea, s_HoglArea->INFO, "Initialized FastADAS(DSP, %s)
    // successfully",version );
    return true;
}

bool FadasSrv::Initialize( Core coreId )
{
    bool ret = true;

    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( false == s_Initialized[coreId] )
    {
        if ( coreId == FadasSrv::Core::CPU )
        {
            ret = init_cpu();
        }
        else
        {
            ret = init_dsp( coreId );
        }
        if ( true == ret )
        {
            s_Initialized[coreId] = true;
        }
    }

    if ( true == ret )
    {
        s_UseRef[coreId]++;
    }

    return ret;
}

void FadasSrv::DeInitialize( Core coreId )
{
    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( s_Initialized[coreId] && ( s_UseRef[coreId] > 0 ) )
    {
        s_UseRef[coreId]--;
        if ( 0 == s_UseRef[coreId] )
        {
            if ( coreId == FadasSrv::Core::CPU )
            {
                // do nothing
            }
            else
            {
                std::lock_guard<std::recursive_mutex> l( s_CoreLock[coreId] );
                auto &memMap = s_MemMaps[coreId];
                std::vector<void *> ptrs;
                for ( auto &it : memMap )
                {
                    ptrs.push_back( it.first );
                }
                for ( auto ptr : ptrs )
                {
                    DeRegBuf( coreId, ptr );
                }
                FadasIface_FadasDeInit( s_Handle64[coreId] );
            }
            s_Initialized[coreId] = false;
        }
    }
}

std::recursive_mutex &FadasSrv::GetLock( Core coreId )
{
    return s_CoreLock[coreId];
}

#ifdef USE_FADAS_DSP
remote_handle64 FadasSrv::GetRemoteHandle64( Core coreId )
{
    return s_Handle64[coreId];
}
#endif

int32_t FadasSrv::RegBuf( Core coreId, void *ptr, size_t size, size_t offset, size_t batch )
{
    int32_t fd = -1;
    if ( coreId == FadasSrv::Core::CPU )
    {
        return -1;   // not necessary for CPU backend
    }

    std::lock_guard<std::recursive_mutex> l( s_CoreLock[coreId] );
    auto &memMap = s_MemMaps[coreId];
    auto handle64 = s_Handle64[coreId];
    auto it = memMap.find( ptr );
    if ( it == memMap.end() )
    {
        int extDomainId = 0;
        int domain = CDSP_DOMAIN_ID;
        if ( FadasSrv::Core::DSP1 == coreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int client = FADAS_CLIENT_ID;
        extDomainId = get_extended_domains_id( domain, client );

        fd = rpcmem_to_fd( (void *) ptr );
        if ( fd < 0 )
        {
            remote_register_buf_v2( extDomainId, ptr, size * batch, 0 );
            fd = rpcmem_to_fd( (void *) ptr );
            if ( fd < 0 )
            {
                // hogl::post( s_HoglArea, s_HoglArea->ERROR, "rpcmem_to_fd failed, fd = %d", fd );
                return -1;
            }
        }

        auto nErr = fastrpc_mmap( extDomainId, fd, ptr, 0, size * batch, FASTRPC_MAP_FD_DELAYED );
        if ( ( AEE_EALREADY != nErr ) && ( AEE_SUCCESS != nErr ) )
        {
            // hogl::post( s_HoglArea, s_HoglArea->ERROR,"Failed to fastrpc_mmap ptr %p(%d, %llu):
            // ret = %d\n", ptr, fd, size,nErr );
            return -1;
        }

        auto ret = FadasIface_mmap( handle64, fd, (uint32_t) size * batch );
        if ( AEE_SUCCESS != ret )
        {
            // hogl::post( s_HoglArea, s_HoglArea->ERROR, "Failed to map ptr %p(%d, %llu): ret =
            // %d\n",ptr, fd, size, ret );
            return -1;
        }

        uint32_t status = 0;
        // to make things simple, always register as IN and OUT type
        ret = FadasIface_FadasRegBuf( handle64, FADAS_BUF_TYPE_INOUT_NSP, fd, size - offset, offset,
                                      batch );
        if ( ( AEE_SUCCESS != ret ) || ( FADAS_ERROR_NONE != (FadasError_e) status ) )
        {
            // hogl::post( s_HoglArea, s_HoglArea->ERROR,"Failed to register ptr %p(%d, %llu): ret =
            // %d\n", ptr, fd, size, ret );
            return -1;
        }

        // hogl::post( s_HoglArea, s_HoglArea->INFO, "RegBuf %p(%llu) with offset %llu as %d OK",
        // ptr, size, offset, fd );
        memMap[ptr] = { fd, size, offset, batch };
    }
    else
    {
        fd = it->second.fd;
    }

    return fd;
}

void FadasSrv::DeRegBuf( Core coreId, void *ptr )
{
    if ( coreId == FadasSrv::Core::CPU )
    {
        return;   // not necessary for CPU backend
    }

    std::lock_guard<std::recursive_mutex> l( s_CoreLock[coreId] );
    auto &memMap = s_MemMaps[coreId];
    auto handle64 = s_Handle64[coreId];
    auto it = memMap.find( ptr );
    if ( it != memMap.end() )
    {
        auto fd = it->second.fd;
        auto ptr = it->first;
        auto size = it->second.size;
        auto offset = it->second.offset;
        auto batch = it->second.batch;
        memMap.erase( it );

        int extDomainId = 0;
        int domain = CDSP_DOMAIN_ID;
        if ( FadasSrv::Core::DSP1 == coreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int client = FADAS_CLIENT_ID;
        extDomainId = get_extended_domains_id( domain, client );
        FadasIface_FadasDeregBuf( handle64, fd, size - offset, offset, batch );
        FadasIface_munmap( handle64, fd, (uint32_t) size * batch );
        fastrpc_munmap( extDomainId, fd, ptr, size * batch );
        remote_register_buf_v2( extDomainId, ptr, size * batch, -1 );

        // hogl::post( s_HoglArea, s_HoglArea->INFO, "DeRegBuf %p(%llu) with offset %llu as %d
        // OK",ptr, size, offset, fd );
    }
}

}   // namespace FadasIface
}   // namespace libs
}   // namespace hal
}   // namespace ride