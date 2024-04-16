// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.
#ifndef _RIDE_HAL_QNN_RUNTIME_HPP_
#define _RIDE_HAL_QNN_RUNTIME_HPP_

#include <map>
#include <string.h>
#include <vector>

#include "DynamicLoadUtil.hpp"
#include "ridehal/component/ComponentIF.hpp"

using namespace qnn::tools;
using namespace qnn::tools::sample_app;
using namespace ridehal::common;

namespace ridehal
{
namespace component
{

typedef struct
{
    uint64_t qnn;
    uint64_t rpc;
    uint64_t qnnAccelerator;
    uint64_t accelerator;
} QnnRuntime_Perf_t;

typedef struct
{
    const char *interfaceProvider;
    const char *udoLibPath;
} QnnRuntime_UdoPackage_t;

typedef enum
{
    LOAD_CONTEXT_BIN_FROM_FILE,
    LOAD_CONTEXT_BIN_FROM_BUFFER,   // cotext encrypted
    LOAD_SHARED_LIBRARY
} QnnRuntime_LoadType_e;

typedef struct
{
    QnnRuntime_LoadType_e loadType = LOAD_CONTEXT_BIN_FROM_FILE;
    std::string modelPath;
    uint8_t *contextBuffer;
    uint64_t contextSize;
    RideHal_ProcessorType_e backendType;
    Qnn_Priority_t priority = QNN_PRIORITY_DEFAULT;
    std::vector<QnnRuntime_UdoPackage_t> udoPackages;
} QnnRuntime_Config_t;

typedef struct
{
    const char *pName;
    RideHal_TensorProps_t properties;
    float quantScale;
    int32_t quantOffset;
} QnnRuntime_TensorInfo_t;


class QnnRuntime : public ComponentIF
{
public:
    QnnRuntime();
    ~QnnRuntime();

    /// @brief Initialize QnnRuntime component
    /// @param pName component name
    /// @param pConfig QnnRuntime configuration
    /// @param level Logger level
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Get Input tensor information
    /// @param pInfo tensor info struct
    /// @param pNum number of input tensors
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetInputInfo( QnnRuntime_TensorInfo_t *pInfo, uint32_t *pNum );

    /// @brief Get Input tensor information
    /// @param pInfo tensor info struct
    /// @param pNum number of output tensors
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GetOutputInfo( QnnRuntime_TensorInfo_t *pInfo, uint32_t *pNum );

    /// @brief Execute qnn model with input and output buffer
    /// @param pInputs input shared buffer
    /// @param numInputs number of input shared buffer
    /// @param pOutputs output shared buffer
    /// @param numOutputs number of output shared buffer
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs );

    /// @brief Deinit the QnnRuntime object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit() final;

    /// @brief Start the QnnRuntime object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start() final { return RideHalError_e::RIDE_HAL_ERROR_NONE; };

    /// @brief Stop the QnnRuntime object
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop() final { return RideHalError_e::RIDE_HAL_ERROR_NONE; };

    /// @brief Enable qnn performance calculation
    void EnablePerf() { m_bEnabelPerf = true; };

    /// @brief Disable qnn performance calculation
    void DisablePerf() { m_bEnabelPerf = false; };

    /// @brief Get qnn latest performance data
    /// @return QnnRuntime performance structure
    QnnRuntime_Perf_t GetPerf() { return m_perf; };

    /// @brief Rigister memory with specific shared buffer
    /// @param sharedBuffer specific shared buffer
    RideHalError_e RegisterMemoryBuffer( RideHal_SharedBuffer_t &sharedBuffer );

    /// @brief DeRigister memory with specific shared buffer
    /// @param sharedBuffer specific shared buffer
    void DeRegisterMemory( const RideHal_SharedBuffer_t &sharedBuffer );

private:
    /// @brief Create qnn model from .so file
    /// @param modelPath model path
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e CreateFromModelSo( std::string modelPath );

    /// @brief Create qnn model from .bin file
    /// @param modelPath model path
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e CreateFromBinary( std::string modelPath );

    /// @brief Create qnn model from binary buffer
    /// @param buffer buffer pointer
    /// @param bufferSize buffer size
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e CreateFromBinary( uint8_t *buffer, uint64_t bufferSize );

    /// @brief Load customer op package
    /// @param udoPackages set of udo packages information
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e LoadOpPackages( const std::vector<QnnRuntime_UdoPackage_t> &udoPackages );

    /// @brief Get memory handle for HTP
    /// @param sharedBuffer shared buffer
    /// @param tensor Qnn defined tensor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    Qnn_MemHandle_t GetMemHandleHTP( const RideHal_SharedBuffer_t &sharedBuffer,
                                     const Qnn_Tensor_t &tensor );

    /// @brief Get memory handle
    /// @param sharedBuffer shared buffer
    /// @param tensor Qnn defined tensor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    Qnn_MemHandle_t GetMemHandle( const RideHal_SharedBuffer_t &sharedBuffer,
                                  const Qnn_Tensor_t &tensor );

    /// @brief DeRegister memory
    void DeRegisterMemory();

private:
    /// @brief Extract qnn profiling event
    /// @param profileEventId profiling event id
    /// @param perf Qnn performance info
    void ExtractProfilingEvent( QnnProfile_EventId_t profileEventId, QnnRuntime_Perf_t *perf );

    /// @brief Generate qnn performance
    void GeneratePerf();

private:
    static constexpr size_t CONTEXT_CONFIG_SIZE = 1;
    static constexpr size_t DMA_MEMINFO_MAP_SIZE = 2;

    std::string m_Name;
    Logger *m_pLogger = nullptr;
    RideHal_ProcessorType_e m_BackendType;
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
    QnnRuntime_Perf_t m_perf;
    bool m_bEnabelPerf = true;
};   // QnnRuntime

}   // namespace component
}   // namespace ridehal
#endif   // _RIDE_HAL_QNN_RUNTIME_HPP_