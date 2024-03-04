// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.
#ifndef _RIDE_HAL_QNN_RUNTIME_HPP_
#define _RIDE_HAL_QNN_RUNTIME_HPP_

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cinttypes>
#include <functional>
#include <future>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string.h>
#include <thread>
#include <vector>

#include "DynamicLoadUtil.hpp"

using namespace qnn::tools;
using namespace qnn::tools::sample_app;
namespace ride
{
namespace hal
{

typedef enum
{
    QNNRUNTIME_BACKEND_HTP_0 = 0,   // HTP device: 0 core:0
    QNNRUNTIME_BACKEND_HTP_1,       // HTP device: 1 core:0
    QNNRUNTIME_BACKEND_CPU,
    QNNRUNTIME_BACKEND_GPU
} QnnRuntime_Backend_e;

typedef struct
{
    uint64_t qnn;
    uint64_t rpc;
    uint64_t qnnAccelerator;
    uint64_t accelerator;
} QnnRuntime_Perf_t;

typedef struct
{
    std::string modelPath;
    int backendId;
    int backendCoreId = 0;
    Qnn_Priority_t priority = QNN_PRIORITY_DEFAULT;
} QnnRuntime_Config_t;

typedef struct
{
    enum class Tensor_DataType_t : uint8_t
    {
        TENSOR_DATATYPE_UINT8,
        TENSOR_DATATYPE_UINT16,
        TENSOR_DATATYPE_FLOAT32,
        TENSOR_DATATYPE_RAW,
        TENSOR_DATATYPE_RAW_UYVY,
        TENSOR_DATATYPE_RAW_NV12,
        TENSOR_DATATYPE_RAW_P010,
        TENSOR_DATATYPE_RAW_RGB,
        TENSOR_DATATYPE_RAW_GRAY,
        TENSOR_DATATYPE_RAW_OF_MV,     // opticalflow motion vector with paddings accordingly
        TENSOR_DATATYPE_RAW_OF_CONF,   // opticalflow motion vector confidence with paddings
                                       // accordingly
        TENSOR_DATATYPE_UNSPECIFIED
    };

    std::string name;   // name can be optional
    Tensor_DataType_t dataType = Tensor_DataType_t::TENSOR_DATATYPE_RAW;
    // quantization information for DataType::UINT8, can be optional
    float quant_scale = 1.0;
    int32_t quant_offset = 0;
    // For some inference runtime, input and output may combined together,
    // thus tensors may share one FrameBuffer with proper offset
    uint32_t offset = 0;
    uint32_t size;
    // dims can be optional
    std::vector<uint32_t> dims;

    std::string shape()
    {
        std::stringstream ss;
        ss << "[ ";
        for ( auto &dim : dims )
        {
            ss << dim << ", ";
        }
        ss << "]";
        return ss.str();
    }
} QnnRuntime_TensorInfo_t;

class QnnRuntime : public ExecutorIF
{
public:
    QnnRuntime();
    ~QnnRuntime();

    RideHalError_e Init( const char *pName, , const QnnRuntime_Config_t *pConfig, Logger *pLogger );
    RideHalError_e GetInputInfos( std::vector<QnnRuntime_TensorInfo_t> &infos );
    RideHalError_e GetOutputInfos( std::vector<QnnRuntime_TensorInfo_t> &infos );

    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs ) final;

    RideHalError_e Deinit() final;

private:
    RideHalError_e CreateFromModelSo( std::string modelPath );
    RideHalError_e CreateFromBinary( std::string binPath );
    RideHalError_e LoadOpPackages( std::string opPackgesTxtPath );

    Qnn_MemHandle_t GetMemHandleHTP( const RideHal_SharedBuffer_t &buffer,
                                     const Qnn_Tensor_t &tensor );
    Qnn_MemHandle_t GetMemHandle( const RideHal_SharedBuffer_t &buffer,
                                  const Qnn_Tensor_t &tensor );
    void DeResisterMemory();

private:
    void ExtractProfilingEvent( QnnProfile_EventId_t profileEventId, QnnRuntime_Perf_t *perf );
    void GetPerf( QnnRuntime_Perf_t *perf );

private:
    static constexpr size_t CONTEXT_CONFIG_SIZE = 1;
    static constexpr size_t DMA_MEMINFO_MAP_SIZE = 2;

    std::string m_Name;
    hogl::area *m_HoglArea = nullptr;
    int m_BackendId = QnnRuntime_Backend_e::BACKEND_HTP;
    int m_BackendCoreId = 0;
    Qnn_BackendHandle_t m_BackendHandle = nullptr;
    Qnn_DeviceHandle_t m_DeviceHandle = nullptr;
    void *m_ModelHandle = nullptr;
    Qnn_ProfileHandle_t m_ProfileBackendHandle = nullptr;

    Qnn_LogHandle_t m_LogHandle = nullptr;

    QnnFunctionPointers m_QnnFunctionPointers;

    QnnBackend_Config_t **m_BackendConfig = nullptr;
    QnnSystemContext_Handle_t m_SystemContext = nullptr;
    Qnn_ContextHandle_t m_Context = nullptr;
    QnnContext_Config_t *m_ContextConfig[CONTEXT_CONFIG_SIZE + 1] = { nullptr };
    QnnContext_Config_t m_ContextConfigArray[CONTEXT_CONFIG_SIZE];

    bool m_LoadFromCachedBinary = false;
    bool m_BackendInitialized = false;

    qnn_wrapper_api::GraphInfo_t **m_GraphsInfo = nullptr;
    uint32_t m_GraphsCount = 0;

    qnn_wrapper_api::GraphConfigInfo_t **m_GraphConfigsInfo = nullptr;
    uint32_t m_GraphConfigsInfoCount = 0;

    const QnnDevice_PlatformInfo_t *m_PlatformInfo;

    typedef struct
    {
        Qnn_MemHandle_t memHandle;
        size_t size;
    } DmaMemInfo_t;

    // NOTE: this is for now used by HTP backend only, HTP has 2 instance as max
    static uint64_t s_DmaMemInfoMapUseRef[DMA_MEMINFO_MAP_SIZE];
    static std::mutex s_DmaMemInfoMapLock[DMA_MEMINFO_MAP_SIZE];
    static std::map<uint8_t *, DmaMemInfo_t> s_DmaMemInfoMap[DMA_MEMINFO_MAP_SIZE];
};   // QnnRuntime

}   // namespace hal

}   // namespace ride
#endif   // _RIDE_HAL_QNN_RUNTIME_HPP_
