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

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"
using namespace ridehal::common;

#define ALIGN_128( x ) ( ( ( ( x ) + 127 ) >> 7 ) << 7 )

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

#define FADAS_CLIENT_ID 1
#define FADAS_CLIENT_URI "&_session=1"

class FadasSrv
{
public:
    RideHalError_e Init( RideHal_ProcessorType_e coreId, const char *pName, Logger_Level_e level );
    RideHalError_e Deinit();
    int32_t RegBuf( const RideHal_SharedBuffer_t *pBuffer, FadasBufType_e bufferType );
    void DeregBuf( void *pBuffer );
    remote_handle64 GetRemoteHandle64();

protected:
    struct MemInfo
    {
        int32_t fd;
        size_t size;
        size_t offset;
        uint32_t batch;
        void *ptr;
        size_t sizeOne;
    };
    RideHal_ProcessorType_e m_processor;

private:
    RideHalError_e InitCPU();
    RideHalError_e InitDSP( RideHal_ProcessorType_e coreId );
    static std::mutex s_coreLock[RIDE_HAL_PROCESSOR_MAX];
    static std::mutex s_FadasLock;
    static remote_handle64 s_handle64[RIDE_HAL_PROCESSOR_MAX];
    static uint64_t s_useRef[RIDE_HAL_PROCESSOR_MAX];
    static bool s_initialized[RIDE_HAL_PROCESSOR_MAX];
    static std::map<void *, MemInfo> s_memMaps[RIDE_HAL_PROCESSOR_MAX];

protected:
    RIDEHAL_DECLARE_LOGGER();
};

class FadasRemap : public FadasSrv
{
public:
    FadasRemap();
    ~FadasRemap();
    RideHalError_e SetRemapParams( uint32_t numOfInputs, uint32_t outputWidth,
                                   uint32_t outputHeight, RideHal_ImageFormat_e outputFormat,
                                   FadasNormlzParams_t normlzR, FadasNormlzParams_t normlzG,
                                   FadasNormlzParams_t normlzB, bool bEnableUndistortion,
                                   bool bEnableNormalize );
    RideHalError_e CreatRemapTable( uint32_t inputId, uint32_t mapWidth, uint32_t mapHeight,
                                    float *pMapX, float *pMapY );
    RideHalError_e CreateRemapWorker( uint32_t inputId, RideHal_ImageFormat_e inputFormat,
                                      uint32_t inputWidth, uint32_t inputHeight, FadasROI_t ROI );
    RideHalError_e RemapRun( const RideHal_SharedBuffer_t *inputs,
                             const RideHal_SharedBuffer_t *outputs );
    RideHalError_e DestroyWorkers();
    RideHalError_e DestroyMap();

private:
    RideHalError_e RemapRunCPU( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *outputs );
    RideHalError_e RemapRunDSP( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *outputs );
    FadasRemapPipeline_e RemapGetPipelineCPU( RideHal_ImageFormat_e inputFormat,
                                              RideHal_ImageFormat_e outputFormat,
                                              bool bEnableNormalize );
    FadasIface_FadasRemapPipeline_e RemapGetPipelineDSP( RideHal_ImageFormat_e inputFormat,
                                                         RideHal_ImageFormat_e outputFormat,
                                                         bool bEnableNormalize );

private:
    remote_handle64 m_handle64;
    uint32_t m_numOfInputs;
    RideHal_ImageFormat_e m_inputFormats[RIDE_HAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_outputFormat;
    uint32_t m_inputWidths[RIDE_HAL_MAX_INPUTS];
    uint32_t m_inputHeights[RIDE_HAL_MAX_INPUTS];
    uint32_t m_mapWidths[RIDE_HAL_MAX_INPUTS];
    uint32_t m_mapHeights[RIDE_HAL_MAX_INPUTS];
    FadasROI_t m_ROIs[RIDE_HAL_MAX_INPUTS];
    uint64 m_workerPtrs[RIDE_HAL_MAX_INPUTS];
    uint64 m_remapPtrs[RIDE_HAL_MAX_INPUTS];
    FadasNormlzParams_t m_normlz[3];
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    bool m_bEnableUndistortion;
    bool m_bEnableNormalize;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace ridehal

#endif   // _QRIDE_FADAS_SRV_HPP_
