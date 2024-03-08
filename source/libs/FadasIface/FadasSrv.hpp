//  Copyright 2020-2022 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#ifndef _QRIDE_FADAS_SRV_HPP_
#define _QRIDE_FADAS_SRV_HPP_

#include <fadas.h>
#include <map>
#include <mutex>
#include <vector>
#pragma weak remote_session_control
#include "AEEStdErr.h"
#include <remote.h>
extern "C"
{
#include "FadasIface.h"
#include "fastrpc_api.h"
}

namespace ride
{
namespace hal
{
namespace libs
{
namespace FadasIface
{

#define FADAS_CLIENT_ID 1
#define FADAS_CLIENT_URI "&_session=1"

class FadasSrv final
{
public:
    enum Core
    {
        DSP0,
        DSP1,
        CPU,
        MAX
    };

    static bool Initialize( Core coreId );
    static void DeInitialize( Core coreId );
    static std::recursive_mutex &GetLock( Core coreId );
    static remote_handle64 GetRemoteHandle64( Core coreId );

    static int32_t RegBuf( Core coreId, void *ptr, size_t size, size_t offset = 0,
                           size_t batch = 1 );
    static void DeRegBuf( Core coreId, void *ptr );

private:
    struct MemInfo
    {
        int32_t fd;
        size_t size;
        size_t offset;
        size_t batch;
    };

private:
    static bool init_cpu();
    static bool init_dsp( Core coreId );
    static std::recursive_mutex s_CoreLock[Core::MAX];
    static std::mutex s_FadasLock;
    static remote_handle64 s_Handle64[Core::MAX];
    static uint64_t s_UseRef[Core::MAX];
    static bool s_Initialized[Core::MAX];
    static std::map<void *, MemInfo> s_MemMaps[Core::MAX];
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace hal
}   // namespace ride

#endif   // _QRIDE_FADAS_SRV_HPP_
