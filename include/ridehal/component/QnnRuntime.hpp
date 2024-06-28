// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.
#ifndef RIDEHAL_QNN_RUNTIME_HPP
#define RIDEHAL_QNN_RUNTIME_HPP

#include <map>
#include <string.h>
#include <string>
#include <vector>

#include "DataUtil.hpp"
#include "DynamicLoadUtil.hpp"
#include "ridehal/component/ComponentIF.hpp"

using namespace qnn::tools;
using namespace qnn::tools::sample_app;
using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/** @brief QnnRuntime performation information */
typedef struct
{
    uint64_t entireExecTime; /**<qnn model entire execution time (ms) */
    uint64_t rpcExecTimeCPU; /**<execution time(ms) of remote procedure call on the CPU processor
                              * when client invokes QnnGraph_execute or
                              * QnnGraph_executeAsync.*/
    uint64_t rpcExecTimeHTP; /**<execution time(ms) of remote procedure call on the HTP processor
                              * when client invokes QnnGraph_execute or
                              * QnnGraph_executeAsync.*/
    uint64_t rpcExecTimeAcc; /**<execution time(ms) of remote procedure call on the
                              * accelerator when client invokes QnnGraph_execute or
                              * QnnGraph_executeAsync.*/
} QnnRuntime_Perf_t;

/** @brief UDO package information */
typedef struct
{
    const char *interfaceProvider; /**<name of interface provider*/
    const char *udoLibPath;        /**<uod library path*/
} QnnRuntime_UdoPackage_t;

/** @brief Qnn model loading type */
typedef enum
{
    QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE,   /**<load qnn model from binary file*/
    QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER, /**<load qnn model from bin buffer*/
    QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE /**<load qnn model from .so file*/
} QnnRuntime_LoadType_e;

/** @brief QnnRuntime configuration*/
typedef struct
{
    QnnRuntime_LoadType_e loadType =
            QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE;   /**<Qnn model loading type*/
    const char *modelPath;                           /**<Qnn model path*/
    uint8_t *contextBuffer;                          /**<Pointer to qnn model context buffer */
    uint64_t contextSize;                            /**<qnn model context buffer size */
    RideHal_ProcessorType_e backendType;             /**<Preprocessor type */
    Qnn_Priority_t priority = QNN_PRIORITY_DEFAULT;  /**<Qnn priority */
    QnnRuntime_UdoPackage_t *pUdoPackages = nullptr; /**<The pointer to QnnRuntime udo package */
    int numOfUdoPackages = 0;                        /**<The number of udo packages */
} QnnRuntime_Config_t;

/** @brief QnnRuntime tensor information */
typedef struct
{
    const char *pName;                /**<The name of tensor*/
    RideHal_TensorProps_t properties; /**<The property of tensor*/
    float quantScale;                 /**<The value of quantization scale*/
    int32_t quantOffset;              /**<The value of quantization offset*/
} QnnRuntime_TensorInfo_t;

/** @brief The list of QnnRuntime tensor information */
typedef struct
{
    QnnRuntime_TensorInfo_t *pInfo; /**<Pointer to QnnRuntime tensor information*/
    uint32_t num;                   /**<The number of tensors*/
} QnnRuntime_TensorInfoList_t;

/*=================================================================================================
** API Functions
=================================================================================================*/

/** @addtogroup QnnRuntime Functions
@{ */

class QnnRuntime : public ComponentIF
{
public:
    QnnRuntime();
    ~QnnRuntime();

    /**
     * @cond QnnRuntime::Init @endcond
     * @brief Initialize QnnRuntime component
     * @param[in] pName Component name
     * @param[in] pConfig QnnRuntime configuration
     * @param [in] level Logger level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @cond QnnRuntime::GetInputInfo @endcond
     * @brief Get Input tensor information
     * @param[out] pList Pointer to tensor info list
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetInputInfo( QnnRuntime_TensorInfoList_t *pList );

    /**
     * @cond QnnRuntime::GetOutputInfo @endcond
     * @brief Get output tensor information
     * @param[out] pList Pointer to tensor info list
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetOutputInfo( QnnRuntime_TensorInfoList_t *pList );

    /**
     * @cond QnnRuntime::Execute @endcond
     * @brief Execute qnn model with input and output buffer
     * @param[in] pInputs Pointer to input shared buffer
     * @param[in] numInputs The number of input shared buffers
     * @param[out] pOutputs Pointer to output shared buffer
     * @param[out] numOutputs The number of output shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs );

    /**
     * @cond QnnRuntime::Deinit @endcond
     * @brief Deinit the QnnRuntime object
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit() final;

    /**
     * @cond QnnRuntime::Start @endcond
     * @brief Start the QnnRuntime object
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start() final;

    /**
     * @cond QnnRuntime::Stop @endcond
     * @brief Stop the QnnRuntime object
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop() final;

    /**
     * @cond QnnRuntime::EnablePerf @endcond
     * @brief Enable qnn performance calculation
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e EnablePerf();

    /**
     * @cond QnnRuntime::DisablePerf @endcond
     * @brief Disable qnn performance calculation
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DisablePerf();

    /**
     * @cond QnnRuntime::GetPerf @endcond
     * @brief Get qnn latest performance data
     * @param[out] pPerf Pointer to QnnRuntime perf structure
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetPerf( QnnRuntime_Perf_t *pPerf );

    /**
     * @cond QnnRuntime::RegisterBuffers @endcond
     * @brief Rigister memory with specific shared buffers
     * @param[in] pSharedBuffers Pointer to shared buffers
     * @param[in] numBuffers The number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
                                    uint32_t numBuffers );

    /**
     * @cond QnnRuntime::DeRegisterBuffers @endcond
     * @brief DeRigister memory with specific shared buffers
     * @param[in] pSharedBuffers Pointer to shared buffers
     * @param[in] numBuffers The number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
                                      uint32_t numBuffers );

private:
    /**
     * @cond QnnRuntime::CreateFromModelSo @endcond
     * @brief Create qnn model from .so file
     * @param[in] modelFile model path
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e CreateFromModelSo( std::string modelFile );

    /**
     * @cond QnnRuntime::CreateFromBinary @endcond
     * @brief Create qnn model from .bin file
     * @param[in] modelFile model path
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e CreateFromBinaryFile( std::string modelFile );

    /**
     * @cond QnnRuntime::CreateFromBinary @endcond
     * @brief Create qnn model from binary buffer
     * @param[in] pBuffer The pointer ro buffer
     * @param[in] bufferSize The size of buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e CreateFromBinaryBuffer( uint8_t *pBuffer, uint64_t bufferSize );

    /**
     * @cond QnnRuntime::LoadOpPackages @endcond
     * @brief Load customer op package
     * @param[in] pUdoPackages The pointer to udo packages information
     * @param[in] numOfUdoPackages The number of udo packages
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e LoadOpPackages( QnnRuntime_UdoPackage_t *pUdoPackages, int numOfUdoPackages );

    /**
     * @cond QnnRuntime::RegisterBuffer @endcond
     * @brief Register Buffer on HTP memory and get the handle
     * @param[in] pSharedBuffer pointer shared buffer
     * @param[out] pMemHandle pointer to HTP memory handle
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffer( const RideHal_SharedBuffer_t *pSharedBuffer,
                                   Qnn_MemHandle_t *pMemHandle );

    /**
     * @cond QnnRuntime::GetMemHandle @endcond
     * @brief Get HTP memory handle
     * @param[in] pSharedBuffer pointer shared buffer
     * @param[out] pMemHandle pointer to HTP memory handle
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetMemHandle( const RideHal_SharedBuffer_t *pSharedBuffer,
                                 Qnn_MemHandle_t *pMemHandle );

    /**
     * @cond QnnRuntime::DeRegisterBuffers @endcond
     * @brief DeRegister memory buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers();

    /**
     * @cond QnnRuntime::GetInputInfo @endcond
     * @brief get input tensor info internally
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetInputInfo();

    /**
     * @cond QnnRuntime::GetOutputInfo @endcond
     * @brief get output tensor info internally
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetOutputInfo();

    /**
     * @cond QnnRuntime::ExtractProfilingEvent @endcond
     * @brief Extract qnn profiling event
     * @param[in] profileEventId profiling event id
     * @param[in] pPerf Pointer to QnnRuntime perf structure
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e ExtractProfilingEvent( QnnProfile_EventId_t profileEventId,
                                          QnnRuntime_Perf_t *pPerf );

    /**
     * @cond QnnRuntime::GeneratePerf @endcond
     * @brief Generate qnn performance
     * @param[in] profileEventId profiling event id
     * @param[in] pPerf Pointer to QnnRuntime perf structure
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GeneratePerf();

    RideHalError_e CheckInputTensors( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs );

    RideHalError_e CheckOutputTensors( const RideHal_SharedBuffer_t *pOutputs,
                                       uint32_t numOutputs );


#ifdef QNNRUNTIME_UNIT_TEST
public:
#endif

    /**
     * @cond QnnRuntime::SwitchFromQnnDataType @endcond
     * @brief Stwich qnn defined data type to ridehal defined tensor type
     * @param[in] dataType qnn defined data type
     * @return RideHal_TensorType_e ridehal defined tensor type
     */
    RideHal_TensorType_e SwitchFromQnnDataType( Qnn_DataType_t dataType );

    /**
     * @cond QnnRuntime::SwitchFromQnnDataType @endcond
     * @brief Stwich ridehal defined tensor type to qnn defined data type
     * @param[in] tensorType ridehal defined tensor type
     * @return Qnn_DataType_t qnn defined data type
     */
    Qnn_DataType_t SwitchToQnnDataType( RideHal_TensorType_e tensorType );


private:
    static constexpr size_t CONTEXT_CONFIG_SIZE = 1;
    static constexpr size_t DMA_MEMINFO_MAP_SIZE = 2;

    Logger *m_pLogger = nullptr;
    RideHal_ProcessorType_e m_BackendType;
    int m_BackendCoreId = 0;
    Qnn_BackendHandle_t m_BackendHandle = nullptr;
    Qnn_DeviceHandle_t m_DeviceHandle = nullptr;
    void *m_ModelHandle = nullptr;
    Qnn_ProfileHandle_t m_ProfileBackendHandle = nullptr;

    Qnn_LogHandle_t m_LogHandle = nullptr;

    QnnFunctionPointers m_QnnFunctionPointers;

    const QnnBackend_Config_t **m_BackendConfig = nullptr;
    QnnSystemContext_Handle_t m_SystemContext = nullptr;
    Qnn_ContextHandle_t m_Context = nullptr;
    QnnContext_Config_t *m_ContextConfig[CONTEXT_CONFIG_SIZE + 1] = { nullptr };
    QnnContext_Config_t m_ContextConfigArray[CONTEXT_CONFIG_SIZE];

    bool m_LoadFromCachedBinary = false;

    qnn_wrapper_api::GraphInfo_t **m_GraphsInfo = nullptr;
    uint32_t m_GraphsCount = 0;

    const qnn_wrapper_api::GraphConfigInfo_t **m_GraphConfigsInfo = nullptr;
    uint32_t m_GraphConfigsInfoCount = 0;

    const QnnDevice_PlatformInfo_t *m_PlatformInfo;

    typedef struct
    {
        Qnn_MemHandle_t memHandle;
        size_t size;
        int32_t fd;
    } DmaMemInfo_t;

    // NOTE: this is for now used by HTP backend only, HTP has 2 instance as max
    static uint64_t s_DmaMemInfoMapUseRef[DMA_MEMINFO_MAP_SIZE];
    static std::mutex s_DmaMemInfoMapLock[DMA_MEMINFO_MAP_SIZE];
    static std::map<uint8_t *, DmaMemInfo_t> s_DmaMemInfoMap[DMA_MEMINFO_MAP_SIZE];
    QnnRuntime_Perf_t m_perf;
    bool m_bEnabelPerf = false;
    QnnRuntime_TensorInfo_t *m_pInputTensor = nullptr;
    size_t m_inputTensorNum = 0;
    QnnRuntime_TensorInfo_t *m_pOutputTensor = nullptr;
    size_t m_outputTensorNum = 0;
};   // QnnRuntime

}   // namespace component
}   // namespace ridehal
#endif   // RIDEHAL_QNN_RUNTIME_HPP