// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/component/QnnRuntime.hpp"
#include "DataUtil.hpp"
#include "HTP/QnnHtpCommon.h"
#include "HTP/QnnHtpMem.h"
#include "HTP/QnnHtpProfile.h"
#include "Logger.hpp"
#include "QnnProfile.h"
#include "QnnSampleAppUtils.hpp"
#include "QnnTypeMacros.hpp"
#include <unistd.h>

using namespace qnn;
using namespace qnn::tools;

extern "C"
{
#include "fastrpc_api.h"
}
// #include "AEEStdErr.h"
#include "rpcmem.h"
#pragma weak remote_session_control
// #include "remote.h"

extern "C"
{
    int get_extended_domains_id( int domain, int session );
    void remote_register_buf_v2( int ext_domain_id, void *buf, int size, int fd );
    void remote_register_buf_attr_v2( int ext_domain_id, void *buf, int size, int fd, int attr );
}

namespace ridehal
{
namespace component
{

static std::map<RideHal_ProcessorType_e, const char *> s_Backends = {
        { RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0, "libQnnHtp.so" },
        { RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1, "libQnnHtp.so" },
        { RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_CPU, "libQnnCpu.so" },
        { RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_GPU, "libQnnGpu.so" } };

std::mutex QnnRuntime::s_DmaMemInfoMapLock[QnnRuntime::DMA_MEMINFO_MAP_SIZE];
std::map<uint8_t *, QnnRuntime::DmaMemInfo_t>
        QnnRuntime::s_DmaMemInfoMap[QnnRuntime::DMA_MEMINFO_MAP_SIZE];
uint64_t QnnRuntime::s_DmaMemInfoMapUseRef[QnnRuntime::DMA_MEMINFO_MAP_SIZE] = { 0, 0 };


QnnRuntime::QnnRuntime() {}

void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp, va_list args )
{
    switch ( logLevel )
    {
        case QNN_LOG_LEVEL_VERBOSE:
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
            break;
        case QNN_LOG_LEVEL_DEBUG:
            Logger::GetDefault().Log( LOGGER_LEVEL_DEBUG, fmt, args );
            break;
        case QNN_LOG_LEVEL_INFO:
            Logger::GetDefault().Log( LOGGER_LEVEL_INFO, fmt, args );
            break;
        case QNN_LOG_LEVEL_WARN:
            Logger::GetDefault().Log( LOGGER_LEVEL_WARN, fmt, args );
            break;
        case QNN_LOG_LEVEL_ERROR:
            Logger::GetDefault().Log( LOGGER_LEVEL_ERROR, fmt, args );
            break;
        default:
            break;
    }
}

RideHalError_e QnnRuntime::CreateFromModelSo( std::string modelFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( -1 == access( modelFile.c_str(), F_OK ) )
    {
        RIDEHAL_ERROR( "No existing file: %s", modelFile.c_str() );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.contextCreate(
                m_BackendHandle, m_DeviceHandle, (const QnnContext_Config_t **) m_ContextConfig,
                &m_Context );
        if ( QNN_CONTEXT_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not create context, error is %d", (int) retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const qnn_wrapper_api::ModelError_t retVal = m_QnnFunctionPointers.composeGraphsFnHandle(
                m_BackendHandle, m_QnnFunctionPointers.qnnInterface, m_Context,
                (const qnn_wrapper_api::GraphConfigInfo_t **) m_GraphConfigsInfo,
                m_GraphConfigsInfoCount, &m_GraphsInfo, &m_GraphsCount, false, QnnLog_Callback,
                QNN_LOG_LEVEL_ERROR );
        if ( qnn_wrapper_api::ModelError_t::MODEL_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failed in composeGraphs()" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_GraphsCount != 1 )
        {
            RIDEHAL_ERROR( "too much graphs" );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.graphFinalize(
                ( *m_GraphsInfo )[0].graph, m_ProfileBackendHandle, nullptr );
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failed in graphFinalize()" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Qnn_ContextBinarySize_t binaryBufferSize;
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.contextGetBinarySize(
                m_Context, &binaryBufferSize );
        if ( QNN_GRAPH_NO_ERROR == retVal )
        {
            std::unique_ptr<uint8_t[]> saveBuffer( new uint8_t[binaryBufferSize] );
            Qnn_ContextBinarySize_t writtenBufferSize;
            if ( nullptr != saveBuffer )
            {
                if ( QNN_GRAPH_NO_ERROR == m_QnnFunctionPointers.qnnInterface.contextGetBinary(
                                                   m_Context,
                                                   reinterpret_cast<void *>( saveBuffer.get() ),
                                                   binaryBufferSize, &writtenBufferSize ) )
                {
                    RIDEHAL_INFO( "saving cached binary(size = %llu)", writtenBufferSize );

                    auto dataUtilStatus = tools::datautil::writeBinaryToFile(
                            modelFile, "program.bin", (uint8_t *) saveBuffer.get(),
                            writtenBufferSize );
                    if ( tools::datautil::StatusCode::SUCCESS != dataUtilStatus )
                    {
                        RIDEHAL_ERROR( "Error while writing binary to file." );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::CreateFromBinaryBuffer( uint8_t *pBuffer, uint64_t bufferSize )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( 0 == bufferSize )
    {
        RIDEHAL_ERROR( "Failed to create binary, Buffer size is 0" );
        ret = RIDEHAL_ERROR_FAIL;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pBuffer == nullptr )
        {
            RIDEHAL_ERROR( "Buffer is null." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    const QnnSystemContext_BinaryInfo_t *binaryInfo{ nullptr };
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Qnn_ContextBinarySize_t binaryInfoSize{ 0 };
        const Qnn_ErrorHandle_t retVal =
                m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
                        m_SystemContext, static_cast<void *>( pBuffer ), bufferSize, &binaryInfo,
                        &binaryInfoSize );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Failed to get context binary info." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( !copyMetadataToGraphsInfo( binaryInfo, m_GraphsInfo, m_GraphsCount ) )
        {
            RIDEHAL_ERROR( "Failed to copy metadata." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary )
        {
            RIDEHAL_ERROR( "contextCreateFromBinaryFnHandle is nullptr." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary(
                m_BackendHandle, m_DeviceHandle, (const QnnContext_Config_t **) m_ContextConfig,
                static_cast<void *>( pBuffer ), bufferSize, &m_Context, m_ProfileBackendHandle );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Could not create context from binary. Error is %d ", retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    for ( size_t graphIdx = 0; graphIdx < m_GraphsCount; graphIdx++ )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            if ( nullptr == m_QnnFunctionPointers.qnnInterface.graphRetrieve )
            {
                RIDEHAL_ERROR( "graphRetrieveFnHandle is nullptr." );
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.graphRetrieve(
                    m_Context, ( *m_GraphsInfo )[graphIdx].graphName,
                    &( ( *m_GraphsInfo )[graphIdx].graph ) );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "Unable to retrieve graph handle for graph Idx: %d, error is %d",
                               graphIdx, (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}


RideHalError_e QnnRuntime::CreateFromBinaryFile( std::string modelFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( -1 == access( modelFile.c_str(), F_OK ) )
    {
        RIDEHAL_ERROR( "No existing file: %s", modelFile.c_str() );
        ret = RIDEHAL_ERROR_FAIL;
    }

    uint64_t bufferSize{ 0 };
    std::shared_ptr<uint8_t> buffer{ nullptr };
    // read serialized binary into a byte buffer
    tools::datautil::StatusCode status{ tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = tools::datautil::getFileSize( modelFile );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( 0 == bufferSize )
        {
            RIDEHAL_ERROR( "Received path to an empty file. Nothing to deserialize." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize], std::default_delete<uint8_t[]>() );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( !buffer )
        {
            RIDEHAL_ERROR( "Failed to allocate memory." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        status = tools::datautil::readBinaryFromFile(
                modelFile, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );
        if ( status != tools::datautil::StatusCode::SUCCESS )
        {
            RIDEHAL_ERROR( "Failed to read binary data." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CreateFromBinaryBuffer( buffer.get(), bufferSize );
    }

    return ret;
}

RideHalError_e QnnRuntime::LoadOpPackages( QnnRuntime_UdoPackage_t *pUdoPackages,
                                           size_t numOfUdoPackages )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( numOfUdoPackages <= 0 )
    {
        RIDEHAL_ERROR( "UdoPackages size is less than 0: %d "
                       "pUdoPackages: %d ",
                       numOfUdoPackages );
        ret = RIDEHAL_ERROR_FAIL;
    }

    for ( size_t i = 0; i < numOfUdoPackages; ++i )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            QnnRuntime_UdoPackage_t udoPackage = pUdoPackages[i];
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                            m_BackendHandle, (char *) udoPackage.udoLibPath,
                            (char *) udoPackage.interfaceProvider, nullptr );
            if ( QNN_BACKEND_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "Could not register Op Package: %s and interface provider: %s, "
                               "error is %d",
                               udoPackage.udoLibPath, udoPackage.interfaceProvider, (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            RIDEHAL_INFO( "Registered Op Package: %s and interface provider: %s",
                          udoPackage.udoLibPath, udoPackage.interfaceProvider );
        }
    }
    return ret;
}

RideHalError_e QnnRuntime::Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                                 Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init component!" );
    }

    if ( nullptr == pConfig )
    {
        RIDEHAL_ERROR( "QnnRuntime Config is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_BackendType = pConfig->backendType;
        if ( m_BackendType == RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 )
        {
            m_BackendCoreId = 1;
        }
        else
        {
            m_BackendCoreId = 0;
        }
    }

    const std::string modelPath = std::string( pConfig->modelPath );
    m_LoadFromCachedBinary = ( pConfig->loadType == QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE ||
                               pConfig->loadType == QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( (int) m_BackendType < (int) RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_MAX )
        {
            auto statusCode = dynamicloadutil::getQnnFunctionPointers(
                    s_Backends[m_BackendType], modelPath, &m_QnnFunctionPointers, &m_BackendHandle,
                    !m_LoadFromCachedBinary, &m_ModelHandle );
            if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
            {
                RIDEHAL_ERROR( "failed to get qnn function pointers from model %s(%s), error is %d",
                               modelPath.c_str(), s_Backends[m_BackendType], statusCode );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "invalid backend type %d", (int) m_BackendType );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QnnLog_Error_t logError;
        auto logLevel = QNN_LOG_LEVEL_WARN;

        qnn::log::Logger::createLogger( QnnLog_Callback, logLevel, &logError );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.logCreate(
                    QnnLog_Callback, logLevel, &m_LogHandle );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_WARN( "Unable to initialize logging in the backend. error is %d",
                              (int) retVal );
            }
        }
    }

    auto statusCode = dynamicloadutil::getQnnSystemFunctionPointers( "libQnnSystem.so",
                                                                     &m_QnnFunctionPointers );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
        {
            RIDEHAL_ERROR( "Error initializing QNN System Function Pointers" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate ||
             nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ||
             nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextFree )
        {
            RIDEHAL_ERROR( "QNN System function pointers are not populated." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal =
                m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate( &m_SystemContext );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Could not create system handle. error is %d", (int) retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const auto returnStatus = m_QnnFunctionPointers.qnnInterface.backendCreate(
                m_LogHandle, (const QnnBackend_Config_t **) m_BackendConfig, &m_BackendHandle );
        if ( QNN_BACKEND_NO_ERROR != returnStatus )
        {
            RIDEHAL_ERROR( "Could not initialize backend due to error = %d", returnStatus );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Qnn_ApiVersion_t version;
        const auto returnStatus =
                m_QnnFunctionPointers.qnnInterface.backendGetApiVersion( &version );
        if ( QNN_BACKEND_NO_ERROR != returnStatus )
        {
            RIDEHAL_ERROR( "Could not get backend version due to error = %d", returnStatus );
            ret = RIDEHAL_ERROR_FAIL;
        }
        RIDEHAL_INFO( "QNN version: %u.%u.%u %u.%u.%u", version.coreApiVersion.major,
                      version.coreApiVersion.minor, version.coreApiVersion.patch,
                      version.backendApiVersion.major, version.backendApiVersion.minor,
                      version.backendApiVersion.patch );
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const auto returnStatus = m_QnnFunctionPointers.qnnInterface.deviceGetPlatformInfo(
                m_LogHandle, &m_PlatformInfo );
        if ( QNN_BACKEND_NO_ERROR != returnStatus )
        {
            RIDEHAL_ERROR( "Could not get platform information due to error = %d", returnStatus );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_PlatformInfo->version )
        {
            RIDEHAL_INFO( "numHwDevices = %u", m_PlatformInfo->v1.numHwDevices );
            for ( uint32_t i = 0; i < m_PlatformInfo->v1.numHwDevices; i++ )
            {
                auto &deviceInfo = m_PlatformInfo->v1.hwDevices[i].v1;
                RIDEHAL_INFO( "deviceId = %u deviceType = %u numCores = %u", deviceInfo.deviceId,
                              deviceInfo.deviceType, deviceInfo.numCores );
            }

            int deviceId = m_BackendCoreId;
            int core_Id = 0;

            if ( deviceId < (int) m_PlatformInfo->v1.numHwDevices )
            {
                QnnDevice_HardwareDeviceInfo_t hwDevice = m_PlatformInfo->v1.hwDevices[deviceId];
                QnnDevice_PlatformInfo_t platformInfo = {
                        .version = QNN_DEVICE_PLATFORM_INFO_VERSION_1,
                        .v1 =
                                {
                                        .numHwDevices = 1,
                                        .hwDevices = &hwDevice,
                                },
                };
                QnnDevice_Config_t deviceConfig = {
                        .option = QNN_DEVICE_CONFIG_OPTION_PLATFORM_INFO,
                        .hardwareInfo = &platformInfo,
                };
                const QnnDevice_Config_t *configs[] = { &deviceConfig, nullptr };
                const auto returnStatus = m_QnnFunctionPointers.qnnInterface.deviceCreate(
                        m_LogHandle, configs, &m_DeviceHandle );
                if ( QNN_BACKEND_NO_ERROR != returnStatus )
                {
                    RIDEHAL_ERROR( "Could not create device due to error = %d", returnStatus );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
            else
            {
                RIDEHAL_ERROR( "invalid backend device id = %d", deviceId );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->numOfUdoPackages == 0 )
        {
            RIDEHAL_INFO( "no op package" );
        }
        else if ( pConfig->numOfUdoPackages > 0 )
        {
            RideHalError_e ret = LoadOpPackages( pConfig->pUdoPackages, pConfig->numOfUdoPackages );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "fail to load package" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "UdoPackages size is less than 0: %d "
                           "pUdoPackages: %d ",
                           pConfig->numOfUdoPackages );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
             RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
        {   // set up context priority
            m_ContextConfigArray[0].option = QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
            m_ContextConfigArray[0].priority = pConfig->priority;
            m_ContextConfig[0] = &m_ContextConfigArray[0];
            m_ContextConfig[1] = nullptr;
            RIDEHAL_INFO( "set context priority = %d", pConfig->priority );
        }

        switch ( pConfig->loadType )
        {
            case QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE:
            {
                ret = CreateFromModelSo( modelPath );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "fail to create from model so." );
                }
                break;
            }
            case QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER:
            {
                ret = CreateFromBinaryBuffer( pConfig->contextBuffer, pConfig->contextSize );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to create from binary buffer %d", pConfig->contextSize );
                }
                break;
            }

            case QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE:
            default:
            {
                ret = CreateFromBinaryFile( modelPath );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "fail to create from binary file." );
                }
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetInputInfo();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetOutputInfo();
    }

    RIDEHAL_INFO( "init %s with backend %s\n", modelPath, s_Backends[m_BackendType] );

    if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
         RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        s_DmaMemInfoMapUseRef[m_BackendCoreId]++;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        ComponentIF::Deinit();
    }

    return ret;
}

QnnRuntime::~QnnRuntime() {}

RideHalError_e QnnRuntime::GetInputInfo()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_GraphsInfo == nullptr )
    {
        RIDEHAL_ERROR( "m_GraphsInfo is nullptr" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_pInputTensorNum = m_GraphsInfo[0]->numInputTensors;
    }

    if ( m_pInputTensorNum != 0 )
    {
        m_pInputTensor = new QnnRuntime_TensorInfo_t[m_pInputTensorNum];
    }

    RIDEHAL_INFO( "m_pInputTensor size: %d", m_pInputTensorNum );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_pInputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_GraphsInfo[0]->inputTensors[i];

            m_pInputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            size_t sz = 1;
            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( uint32_t j = 0; j < rank; j++ )
            {
                sz *= dimensions[j];
                tensorProp.dims[j] = dimensions[j];
            }
            tensorProp.numDims = rank;

            auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
            if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
            {
                m_pInputTensor[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                m_pInputTensor[i].quantOffset = quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "input %s: quantize encoding %d not supported",
                              m_pInputTensor[i].pName, quantizeParams.quantizationEncoding );
            }
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            tensorProp.type = SwitchFromQnnDataType( dataType );
            m_pInputTensor[i].properties = tensorProp;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetInputInfo( QnnRuntime_TensorInfoList_t *pList )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pInputTensor == nullptr )
        {
            RIDEHAL_ERROR( "Input tensor is nullptr!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            pList->pInfo = m_pInputTensor;
            pList->num = m_pInputTensorNum;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetOutputInfo()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_GraphsInfo == nullptr )
    {
        RIDEHAL_ERROR( "m_GraphsInfo is nullptr" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_pOutputTensorNum = m_GraphsInfo[0]->numOutputTensors;
    }

    if ( m_pOutputTensorNum != 0 )
    {
        m_pOutputTensor = new QnnRuntime_TensorInfo_t[m_pOutputTensorNum];
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_pOutputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_GraphsInfo[0]->outputTensors[i];

            m_pOutputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            size_t sz = 1;
            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( uint32_t j = 0; j < rank; j++ )
            {
                sz *= dimensions[j];
                tensorProp.dims[j] = dimensions[j];
            }
            tensorProp.numDims = rank;

            auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
            if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
            {
                m_pOutputTensor[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                m_pOutputTensor[i].quantOffset = quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "input %s: quantize encoding %d not supported",
                              m_pOutputTensor[i].pName, quantizeParams.quantizationEncoding );
            }
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            tensorProp.type = SwitchFromQnnDataType( dataType );
            m_pOutputTensor[i].properties = tensorProp;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetOutputInfo( QnnRuntime_TensorInfoList_t *pList )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pOutputTensor == nullptr )
        {
            RIDEHAL_ERROR( "Input tensor is nullptr!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            pList->pInfo = m_pOutputTensor;
            pList->num = m_pOutputTensorNum;
        }
    }

    return ret;
}

Qnn_MemHandle_t QnnRuntime::GetMemHandleHTP( const RideHal_SharedBuffer_t &sharedBuffer,
                                             const Qnn_Tensor_t &tensor )
{

    Qnn_MemHandle_t memHandle = nullptr;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    // #warning QnnRuntime build with old version QNN SDK that do not support DMA buffer with
    // offset.
    if ( 0 != sharedBuffer.offset )
    {
        RIDEHAL_ERROR( "Tensor offset is not zero in qnn ealier version!" );
        ret = RIDEHAL_ERROR_FAIL;
    }
#endif

    if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
    {
        RIDEHAL_ERROR( "Backend core id is out of dma memery info map size!" );
        ret = RIDEHAL_ERROR_UNSUPPORTED; /* for safety */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) sharedBuffer.data() );
        if ( it == s_DmaMemInfoMap[m_BackendCoreId].end() )
        {
            int domain = CDSP_DOMAIN_ID;
            if ( 1 == m_BackendCoreId )
            {
                domain = CDSP1_DOMAIN_ID;
            }

            Qnn_MemDescriptor_t desc;
            desc.memShape.numDim = QNN_TENSOR_GET_RANK( &tensor );
            desc.memShape.dimSize = QNN_TENSOR_GET_DIMENSIONS( &tensor );
            desc.dataType = QNN_TENSOR_GET_DATA_TYPE( &tensor );

            int client = 0;   // NOTE: default is 0
            int extDomainId = get_extended_domains_id( domain, client );
#if defined( __QNXNTO__ )
            remote_register_buf_v2( extDomainId, sharedBuffer.buffer.pData, sharedBuffer.size, 0 );
#else
            remote_register_buf_v2( extDomainId, sharedBuffer.buffer.pData, sharedBuffer.size,
                                    (int) sharedBuffer.buffer.dmaHandle );
#endif
            auto fd = rpcmem_to_fd( sharedBuffer.buffer.pData );
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
            QnnMemHtp_Descriptor_t htpDesc;
            htpDesc.type = QNN_HTP_MEM_SHARED_BUFFER;
            htpDesc.size = sharedBuffer.size;
            htpDesc.sharedBufferConfig.fd = fd;
            htpDesc.sharedBufferConfig.offset = sharedBuffer.offset;

            desc.memShape.shapeConfig = nullptr;
            desc.memType = QNN_MEM_TYPE_CUSTOM;
            desc.customInfo = &htpDesc;
#else
            desc.memShape.shapeConfig = nullptr;
            desc.memType = QNN_MEM_TYPE_ION;
            desc.ionInfo.fd = fd;
#endif

            const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.memRegister(
                    m_Context, &desc, 1, &memHandle );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "map buffer %p(%d, %u, %u) for core %d, error %d\n",
                               sharedBuffer.buffer.pData, fd, sharedBuffer.size,
                               sharedBuffer.offset, m_BackendCoreId, retVal );
            }
            else
            {
                QnnRuntime::DmaMemInfo_t info;
                info.memHandle = memHandle;
                info.size = sharedBuffer.size;
                s_DmaMemInfoMap[m_BackendCoreId][(uint8_t *) sharedBuffer.data()] = info;
                QNN_INFO( "map buffer %p(%d, %u, %u) as %p for core %d", sharedBuffer.buffer.pData,
                          fd, sharedBuffer.size, sharedBuffer.offset, memHandle, m_BackendCoreId );
            }
        }
        else
        {
            auto &info = it->second;
            memHandle = info.memHandle;
        }
    }

    return memHandle;
}


RideHalError_e QnnRuntime::RegisterBuffers( RideHal_SharedBuffer_t *sharedBuffer,
                                            uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    // #warning QnnRuntime build with old version QNN SDK that do not support DMA buffer with
    // offset.
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( 0 != sharedBuffer[i].offset )
        {
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
#endif
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
        {
            ret = RIDEHAL_ERROR_FAIL;
        }
    }


    Qnn_MemHandle_t memHandle = nullptr;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        for ( size_t i = 0; i < numBuffers; ++i )
        {
            auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) sharedBuffer[i].data() );
            if ( it == s_DmaMemInfoMap[m_BackendCoreId].end() )
            {
                int domain = CDSP_DOMAIN_ID;
                if ( 1 == m_BackendCoreId )
                {
                    domain = CDSP1_DOMAIN_ID;
                }

                Qnn_MemDescriptor_t desc;
                desc.memShape.numDim = sharedBuffer[i].tensorProps.numDims;
                desc.memShape.dimSize = sharedBuffer[i].tensorProps.dims;
                desc.dataType = SwitchToQnnDataType( sharedBuffer[i].tensorProps.type );

                int client = 0;   // NOTE: default is 0
                int extDomainId = get_extended_domains_id( domain, client );
#if defined( __QNXNTO__ )
                remote_register_buf_v2( extDomainId, sharedBuffer[i].buffer.pData,
                                        sharedBuffer[i].size, 0 );
#else
                remote_register_buf_v2( extDomainId, sharedBuffer[i].buffer.pData,
                                        sharedBuffer[i].size,
                                        (int) sharedBuffer[i].buffer.dmaHandle );
#endif
                auto fd = rpcmem_to_fd( sharedBuffer[i].buffer.pData );
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
                QnnMemHtp_Descriptor_t htpDesc;
                htpDesc.type = QNN_HTP_MEM_SHARED_BUFFER;
                htpDesc.size = sharedBuffer[i].size;
                htpDesc.sharedBufferConfig.fd = fd;
                htpDesc.sharedBufferConfig.offset = sharedBuffer[i].offset;

                desc.memShape.shapeConfig = nullptr;
                desc.memType = QNN_MEM_TYPE_CUSTOM;
                desc.customInfo = &htpDesc;
#else
                desc.memShape.shapeConfig = nullptr;
                desc.memType = QNN_MEM_TYPE_ION;
                desc.ionInfo.fd = fd;
#endif

                const Qnn_ErrorHandle_t memRegisterRet =
                        m_QnnFunctionPointers.qnnInterface.memRegister( m_Context, &desc, 1,
                                                                        &memHandle );
                if ( QNN_SUCCESS != memRegisterRet )
                {
                    RIDEHAL_ERROR( "map buffer %p(%d, %u, %u) for core %d, error %d\n",
                                   sharedBuffer[i].buffer.pData, fd, sharedBuffer[i].size,
                                   sharedBuffer[i].offset, m_BackendCoreId, memRegisterRet );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else
                {
                    QnnRuntime::DmaMemInfo_t info;
                    info.memHandle = memHandle;
                    info.size = sharedBuffer[i].size;
                    s_DmaMemInfoMap[m_BackendCoreId][(uint8_t *) sharedBuffer[i].data()] = info;
                    RIDEHAL_INFO( "map buffer %p(%d, %u, %u) as %p for core %d",
                                  sharedBuffer[i].buffer.pData, fd, sharedBuffer[i].size,
                                  sharedBuffer[i].offset, memHandle, m_BackendCoreId );
                }
            }
            else
            {
                auto &info = it->second;
                memHandle = info.memHandle;
            }
        }
    }

    return ret;
}

Qnn_MemHandle_t QnnRuntime::GetMemHandle( const RideHal_SharedBuffer_t &sharedBuffer,
                                          const Qnn_Tensor_t &tensor )
{

    Qnn_MemHandle_t memHandle = nullptr;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
    }

    if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
         RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
    {
        memHandle = GetMemHandleHTP( sharedBuffer, tensor );
    }


    return memHandle;
}

RideHalError_e QnnRuntime::ExtractProfilingEvent( QnnProfile_EventId_t profileEventId,
                                                  QnnRuntime_Perf_t *pPerf )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pPerf == nullptr )
        {
            RIDEHAL_ERROR( "Pointer of perf is nullptr!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QnnProfile_EventData_t eventData;
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.profileGetEventData(
                profileEventId, &eventData );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failure in profile get event type. error is %d", (int) retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        RIDEHAL_DEBUG( "Printing Event Info - Event Type: [%d], Event Value: [%" PRIu64
                       "], Event Identifier: [%s], Event Unit: [%d]",
                       eventData.type, eventData.value, eventData.identifier, eventData.unit );
        switch ( eventData.type )
        {
            case QNN_PROFILE_EVENTTYPE_EXECUTE:
                pPerf->entireExecTime = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HOST_RPC_TIME_MICROSEC:
                pPerf->rpcExecTimeCPU = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HTP_RPC_TIME_MICROSEC:
                pPerf->rpcExecTimeHTP = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_ACCEL_TIME_MICROSEC:
                pPerf->rpcExecTimeAcc = eventData.value;
                break;
            default:
            {
                RIDEHAL_INFO( "Unsupported qnn event profile type: %d!", (int) eventData.type );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GeneratePerf()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    const QnnProfile_EventId_t *profileEvents{ nullptr };
    uint32_t numEvents{ 0 };

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.profileGetEvents(
                m_ProfileBackendHandle, &profileEvents, &numEvents );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failure in profile get events." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    RIDEHAL_DEBUG( "ProfileEvents: numEvents: [%u]", numEvents );
    for ( size_t event = 0; event < numEvents; event++ )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = ExtractProfilingEvent( *( profileEvents + event ), &m_perf );
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                    const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    std::vector<Qnn_Tensor_t> inputs;
    std::vector<Qnn_Tensor_t> outputs;

    Qnn_ErrorHandle_t executeStatus = QNN_GRAPH_NO_ERROR;
    auto graphInfo = ( *m_GraphsInfo )[0];

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < graphInfo.numInputTensors; i++ )
        {
            inputs.push_back( graphInfo.inputTensors[i] );
            // HTP
            Qnn_MemHandle_t memHandle = GetMemHandle( pInputs[i], inputs[i] );
            if ( nullptr != memHandle )
            {
                QNN_TENSOR_SET_MEM_TYPE( &inputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                QNN_TENSOR_SET_MEM_HANDLE( &inputs[i], memHandle );
            }
            else
            {
                QNN_TENSOR_SET_MEM_TYPE( &inputs[i], QNN_TENSORMEMTYPE_RAW );
                Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pInputs[i].data(),
                                                    (uint32_t) pInputs[i].size };
                QNN_TENSOR_SET_CLIENT_BUF( &inputs[i], clientBuffer );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < graphInfo.numOutputTensors; i++ )
        {
            outputs.push_back( graphInfo.outputTensors[i] );
            Qnn_MemHandle_t memHandle = GetMemHandle( pOutputs[i], outputs[i] );
            if ( nullptr != memHandle )
            {
                QNN_TENSOR_SET_MEM_TYPE( &outputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                QNN_TENSOR_SET_MEM_HANDLE( &outputs[i], memHandle );
            }
            else
            {
                QNN_TENSOR_SET_MEM_TYPE( &outputs[i], QNN_TENSORMEMTYPE_RAW );
                Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pOutputs[i].data(),
                                                    (uint32_t) pOutputs[i].size };
                QNN_TENSOR_SET_CLIENT_BUF( &outputs[i], clientBuffer );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( m_bEnabelPerf ) && ( nullptr == m_ProfileBackendHandle ) )
        {
            const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.profileCreate(
                    m_BackendHandle, QNN_PROFILE_LEVEL_BASIC, &m_ProfileBackendHandle );
            if ( QNN_GRAPH_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "failed to create profile. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        executeStatus = m_QnnFunctionPointers.qnnInterface.graphExecute(
                graphInfo.graph, inputs.data(), graphInfo.numInputTensors, outputs.data(),
                graphInfo.numOutputTensors, m_ProfileBackendHandle, nullptr );
        if ( QNN_GRAPH_NO_ERROR != executeStatus )
        {
            RIDEHAL_ERROR( "QNN failed %d", executeStatus );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            if ( ( m_bEnabelPerf ) && ( nullptr != m_ProfileBackendHandle ) )
            {
                ret = GeneratePerf();
            }
        }
    }
    return ret;
}

RideHalError_e QnnRuntime::DeRegisterBuffers()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
        {
            ret = RIDEHAL_ERROR_FAIL; /* for safety */
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        constexpr int client = 0;   // NOTE: default is 0
        int domain = CDSP_DOMAIN_ID;
        if ( 1 == m_BackendCoreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int extDomainId = get_extended_domains_id( domain, client );
        for ( auto &kv : s_DmaMemInfoMap[m_BackendCoreId] )
        {
            auto ptr = kv.first;
            auto &info = kv.second;
            m_QnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
            if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
                 RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
            {
                remote_register_buf_v2( extDomainId, (void *) ptr, info.size, -1 );
            }
        }
        s_DmaMemInfoMap[m_BackendCoreId].clear();
    }

    return ret;
}

RideHalError_e QnnRuntime::DeRegisterBuffers( RideHal_SharedBuffer_t *sharedBuffer,
                                              uint32_t numBuffers )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
        {
            ret = RIDEHAL_ERROR_FAIL; /* for safety */
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {

        constexpr int client = 0;   // NOTE: default is 0
        int domain = CDSP_DOMAIN_ID;
        if ( 1 == m_BackendCoreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int extDomainId = get_extended_domains_id( domain, client );

        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        for ( size_t i = 0; i < numBuffers; ++i )
        {
            auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) sharedBuffer[i].data() );
            if ( it == s_DmaMemInfoMap[m_BackendCoreId].end() )
            {
                auto ptr = it->first;
                auto &info = it->second;
                m_QnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
                if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
                     RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
                {
                    remote_register_buf_v2( extDomainId, (void *) ptr, info.size, -1 );
                }
            }

            s_DmaMemInfoMap[m_BackendCoreId].erase( it );
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::Deinit()
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
             RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
        {
            std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
            if ( s_DmaMemInfoMapUseRef[m_BackendCoreId] > 0 )
            {
                s_DmaMemInfoMapUseRef[m_BackendCoreId]--;
                if ( 0 == s_DmaMemInfoMapUseRef[m_BackendCoreId] )
                {
                    ret = DeRegisterBuffers();
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_ProfileBackendHandle )
        {
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.profileFree( m_ProfileBackendHandle );
            if ( QNN_PROFILE_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "%s:Could not free backend profile handle. error is %d",
                               (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_Context )
        {
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.contextFree( m_Context, nullptr );
            if ( QNN_CONTEXT_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "%s:Could not free context. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            m_Context = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_SystemContext )
        {
            m_QnnFunctionPointers.qnnSystemInterface.systemContextFree( m_SystemContext );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_DeviceHandle )
        {
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.deviceFree( m_DeviceHandle );
            if ( QNN_CONTEXT_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "%s:Could not free device handle. Error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            m_DeviceHandle = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_PlatformInfo )
        {
            m_QnnFunctionPointers.qnnInterface.deviceFreePlatformInfo( m_LogHandle,
                                                                       m_PlatformInfo );
            m_PlatformInfo = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal =
                m_QnnFunctionPointers.qnnInterface.backendFree( m_BackendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "%s:Could not terminate backend. Error is %d", (int) retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_LogHandle != nullptr )
        {
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.logFree( m_LogHandle );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_WARN( "%s:Unable to terminate logging in the backend. Error is %d",
                              (int) retVal );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_LoadFromCachedBinary && ( nullptr != m_GraphsInfo ) )
        {
            RIDEHAL_DEBUG( "Cleaning up graph Info structures." );
            qnn_wrapper_api::freeGraphsInfo( &m_GraphsInfo, m_GraphsCount );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pInputTensor != nullptr )
        {
            delete[] m_pInputTensor;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pOutputTensor != nullptr )
        {
            delete[] m_pOutputTensor;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ComponentIF::Deinit();
    }

    return ret;
}

RideHalError_e QnnRuntime::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "QnnRuntime component start failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e QnnRuntime::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state ) ||
         ( RIDEHAL_COMPONENT_STATE_ERROR == m_state ) )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "QnnRuntime component stop failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e QnnRuntime::EnablePerf()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    m_bEnabelPerf = true;
    return ret;
}

RideHalError_e QnnRuntime::DisablePerf()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    m_bEnabelPerf = false;
    return ret;
}

RideHalError_e QnnRuntime::GetPerf( QnnRuntime_Perf_t *pPerf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    *pPerf = m_perf;
    return ret;
}

inline RideHal_TensorType_e QnnRuntime::SwitchFromQnnDataType( Qnn_DataType_t dataType )
{
    RideHal_TensorType_e tensorType;
    switch ( dataType )
    {
        case QNN_DATATYPE_INT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_8;
            break;

        case QNN_DATATYPE_INT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_16;
            break;

        case QNN_DATATYPE_INT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_32;
            break;

        case QNN_DATATYPE_INT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_64;
            break;

        case QNN_DATATYPE_UINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_8;
            break;

        case QNN_DATATYPE_UINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_16;
            break;

        case QNN_DATATYPE_UINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_32;
            break;

        case QNN_DATATYPE_UINT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_64;
            break;

        case QNN_DATATYPE_FLOAT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_16;
            break;

        case QNN_DATATYPE_FLOAT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_32;
            break;

        case QNN_DATATYPE_FLOAT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_64;
            break;

        case QNN_DATATYPE_SFIXED_POINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8;
            break;

        case QNN_DATATYPE_SFIXED_POINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16;
            break;

        case QNN_DATATYPE_SFIXED_POINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32;
            break;

        case QNN_DATATYPE_UFIXED_POINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16;
            break;

        case QNN_DATATYPE_UFIXED_POINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32;
            break;

        default:
            RIDEHAL_ERROR( "unsupported qnn data type: %d", (int) dataType );
            break;
    }
    return tensorType;
}

inline Qnn_DataType_t QnnRuntime::SwitchToQnnDataType( RideHal_TensorType_e tensorType )
{
    Qnn_DataType_t dataType;
    switch ( tensorType )
    {
        case RIDEHAL_TENSOR_TYPE_INT_8:
            dataType = QNN_DATATYPE_INT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_16:
            dataType = QNN_DATATYPE_INT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_32:
            dataType = QNN_DATATYPE_INT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_64:
            dataType = QNN_DATATYPE_INT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_8:
            dataType = QNN_DATATYPE_UINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_16:
            dataType = QNN_DATATYPE_UINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_32:
            dataType = QNN_DATATYPE_UINT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_64:
            dataType = QNN_DATATYPE_UINT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_16:
            dataType = QNN_DATATYPE_FLOAT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_32:
            dataType = QNN_DATATYPE_FLOAT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_64:
            dataType = QNN_DATATYPE_FLOAT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8:
            dataType = QNN_DATATYPE_SFIXED_POINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16:
            dataType = QNN_DATATYPE_SFIXED_POINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32:
            dataType = QNN_DATATYPE_SFIXED_POINT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8:
            dataType = QNN_DATATYPE_UFIXED_POINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16:
            dataType = QNN_DATATYPE_UFIXED_POINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32:
            dataType = QNN_DATATYPE_UFIXED_POINT_32;
            break;

        default:
            RIDEHAL_ERROR( "unsupported ridehal tensor type: %d", (int) tensorType );
            break;
    }
    return dataType;
}

}   // namespace component
}   // namespace ridehal