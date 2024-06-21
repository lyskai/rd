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
#include "QnnSdkBuildId.h"
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
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
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
            RIDEHAL_ERROR( "Failed in composeGraphs(), error is %d", retVal );
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

            if ( QNN_GRAPH_NO_ERROR == m_QnnFunctionPointers.qnnInterface.contextGetBinary(
                                               m_Context,
                                               reinterpret_cast<void *>( saveBuffer.get() ),
                                               binaryBufferSize, &writtenBufferSize ) )
            {
                RIDEHAL_INFO( "saving cached binary(size = %llu)", writtenBufferSize );

                auto dataUtilStatus = tools::datautil::writeBinaryToFile(
                        modelFile, "programGenFromSo.bin", (uint8_t *) saveBuffer.get(),
                        writtenBufferSize );
                if ( tools::datautil::StatusCode::SUCCESS != dataUtilStatus )
                {
                    RIDEHAL_ERROR( "Error while writing binary to file." );
                    ret = RIDEHAL_ERROR_FAIL;
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
                                           int numOfUdoPackages )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( numOfUdoPackages <= 0 )
    {
        RIDEHAL_ERROR( "UdoPackages size is less than 0: %d", numOfUdoPackages );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( pUdoPackages == nullptr )
    {
        RIDEHAL_ERROR( "pUdoPackages is null" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    for ( size_t i = 0; i < numOfUdoPackages; ++i )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            QnnRuntime_UdoPackage_t *pUdoPackage = pUdoPackages + i;
            RIDEHAL_INFO( "Registered Op Package: %s and interface provider: %s",
                          pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider );
            const Qnn_ErrorHandle_t retVal =
                    m_QnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                            m_BackendHandle, (char *) pUdoPackage->udoLibPath,
                            (char *) pUdoPackage->interfaceProvider, nullptr );
            if ( QNN_BACKEND_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "Could not register Op Package: %s and interface provider: %s, "
                               "error is %d",
                               pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider,
                               (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            RIDEHAL_INFO( "Registered Op Package: %s and interface provider: %s",
                          pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider );
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

    std::string modelPath;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        modelPath = std::string( pConfig->modelPath );
        m_LoadFromCachedBinary = ( pConfig->loadType == QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE ||
                                   pConfig->loadType == QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER );
    }


    if ( RIDEHAL_ERROR_NONE == ret )
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

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QnnLog_Error_t logError;
        auto logLevel = QNN_LOG_LEVEL_WARN;

        (void) qnn::log::Logger::createLogger( QnnLog_Callback, logLevel, &logError );

        const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.logCreate(
                QnnLog_Callback, logLevel, &m_LogHandle );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_WARN( "Unable to initialize logging in the backend. error is %d",
                          (int) retVal );
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
        RIDEHAL_INFO( "QNN build version: %s", QNN_SDK_BUILD_ID );
        RIDEHAL_INFO( "QNN running core api version: %u.%u.%u, backend api version: %u.%u.%u",
                      version.coreApiVersion.major, version.coreApiVersion.minor,
                      version.coreApiVersion.patch, version.backendApiVersion.major,
                      version.backendApiVersion.minor, version.backendApiVersion.patch );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->backendType != RIDEHAL_PROCESSOR_CPU )
        {
            const auto returnStatus = m_QnnFunctionPointers.qnnInterface.deviceGetPlatformInfo(
                    m_LogHandle, &m_PlatformInfo );
            if ( QNN_DEVICE_NO_ERROR != returnStatus )
            {
                RIDEHAL_ERROR( "Could not get platform information due to error = %d",
                               returnStatus );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->backendType != RIDEHAL_PROCESSOR_CPU )
        {
            if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_PlatformInfo->version )
            {
                RIDEHAL_INFO( "numHwDevices = %u", m_PlatformInfo->v1.numHwDevices );
                for ( uint32_t i = 0; i < m_PlatformInfo->v1.numHwDevices; i++ )
                {
                    auto &deviceInfo = m_PlatformInfo->v1.hwDevices[i].v1;
                    RIDEHAL_INFO( "deviceId = %u deviceType = %u numCores = %u",
                                  deviceInfo.deviceId, deviceInfo.deviceType, deviceInfo.numCores );
                }

                int deviceId = m_BackendCoreId;
                int core_Id = 0;

                if ( deviceId < (int) m_PlatformInfo->v1.numHwDevices )
                {
                    QnnDevice_HardwareDeviceInfo_t hwDevice =
                            m_PlatformInfo->v1.hwDevices[deviceId];
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
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->numOfUdoPackages == 0 )
        {
            RIDEHAL_INFO( "no op package" );
        }
        else if ( pConfig->numOfUdoPackages > 0 )
        {
            ret = LoadOpPackages( pConfig->pUdoPackages, pConfig->numOfUdoPackages );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "fail to load package" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "UdoPackages size is less than 0: %d", pConfig->numOfUdoPackages );
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

    RIDEHAL_INFO( "init %s with backend %s\n", modelPath.c_str(), s_Backends[m_BackendType] );

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
        const RideHalError_e retVal = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != retVal )
        {
            RIDEHAL_ERROR( "ComponentIF Deinit failed, error is %d", retVal );
        }
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
        m_inputTensorNum = m_GraphsInfo[0]->numInputTensors;
    }

    if ( m_inputTensorNum != 0 )
    {
        m_pInputTensor = new QnnRuntime_TensorInfo_t[m_inputTensorNum];
    }

    RIDEHAL_INFO( "m_pInputTensor size: %d", m_inputTensorNum );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_inputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_GraphsInfo[0]->inputTensors[i];

            m_pInputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( uint32_t j = 0; j < rank; j++ )
            {
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
            pList->num = m_inputTensorNum;
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
        m_outputTensorNum = m_GraphsInfo[0]->numOutputTensors;
    }

    if ( m_outputTensorNum != 0 )
    {
        m_pOutputTensor = new QnnRuntime_TensorInfo_t[m_outputTensorNum];
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_outputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_GraphsInfo[0]->outputTensors[i];

            m_pOutputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( uint32_t j = 0; j < rank; j++ )
            {
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
            pList->num = m_outputTensorNum;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::RegisterBuffer( const RideHal_SharedBuffer_t *pSharedBuffer,
                                           Qnn_MemHandle_t *pMemHandle )
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
    if ( 0 != pSharedBuffer->offset )
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
        auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) pSharedBuffer->data() );
        if ( it == s_DmaMemInfoMap[m_BackendCoreId].end() )
        {
            int domain = CDSP_DOMAIN_ID;
            if ( 1 == m_BackendCoreId )
            {
                domain = CDSP1_DOMAIN_ID;
            }

            Qnn_MemDescriptor_t desc;
            desc.memShape.numDim = pSharedBuffer->tensorProps.numDims;
            desc.memShape.dimSize = (uint32_t *) pSharedBuffer->tensorProps.dims;
            desc.memShape.shapeConfig = nullptr;
            desc.dataType = SwitchToQnnDataType( pSharedBuffer->tensorProps.type );

            int client = 0;   // NOTE: default is 0
            int extDomainId = get_extended_domains_id( domain, client );
#if defined( __QNXNTO__ )
            remote_register_buf_v2( extDomainId, pSharedBuffer->buffer.pData,
                                    pSharedBuffer->buffer.size, 0 );
#else
            remote_register_buf_v2( extDomainId, pSharedBuffer->buffer.pData,
                                    pSharedBuffer->buffer.size,
                                    (int) pSharedBuffer->buffer.dmaHandle );
#endif
            auto fd = rpcmem_to_fd( pSharedBuffer->buffer.pData );
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
            QnnMemHtp_Descriptor_t htpDesc;
            htpDesc.type = QNN_HTP_MEM_SHARED_BUFFER;
            htpDesc.size = pSharedBuffer->buffer.size;
            htpDesc.sharedBufferConfig.fd = fd;
            htpDesc.sharedBufferConfig.offset = pSharedBuffer->offset;

            desc.memType = QNN_MEM_TYPE_CUSTOM;
            desc.customInfo = &htpDesc;
#else
            desc.memType = QNN_MEM_TYPE_ION;
            desc.ionInfo.fd = fd;
#endif

            const Qnn_ErrorHandle_t retVal = m_QnnFunctionPointers.qnnInterface.memRegister(
                    m_Context, &desc, 1, pMemHandle );
            if ( QNN_SUCCESS == retVal )
            {
                QnnRuntime::DmaMemInfo_t info;
                info.memHandle = *pMemHandle;
                info.size = pSharedBuffer->buffer.size;
                info.fd = fd;
                s_DmaMemInfoMap[m_BackendCoreId][(uint8_t *) pSharedBuffer->data()] = info;
                RIDEHAL_INFO( "succeed to register map buffer %p(%d, %u, %u) as %p for core %d",
                              pSharedBuffer->buffer.pData, fd, pSharedBuffer->buffer.size,
                              pSharedBuffer->offset, *pMemHandle, m_BackendCoreId );
            }
            else
            {
                RIDEHAL_ERROR( "failed to map buffer %p(%d, %u, %u) for core %d, error %d\n",
                               pSharedBuffer->buffer.pData, fd, pSharedBuffer->buffer.size,
                               pSharedBuffer->offset, m_BackendCoreId, retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            auto &info = it->second;
            *pMemHandle = info.memHandle;
            RIDEHAL_DEBUG( "already register map buffer %p(%d, %u, %u) as %p for core %d",
                           pSharedBuffer->buffer.pData, info.fd, pSharedBuffer->size,
                           pSharedBuffer->offset, *pMemHandle, m_BackendCoreId );
        }
    }

    return ret;
}


RideHalError_e QnnRuntime::RegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
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
        for ( size_t i = 0; i < numBuffers; ++i )
        {

            Qnn_MemHandle_t memHandle = nullptr;
            ret = RegisterBuffer( &pSharedBuffers[i], &memHandle );
            if ( ret != RIDEHAL_ERROR_NONE )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetMemHandle( const RideHal_SharedBuffer_t *pSharedBuffer,
                                         Qnn_MemHandle_t *pMemHandle )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    *pMemHandle = nullptr;
    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
             RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
        {
            ret = RegisterBuffer( pSharedBuffer, pMemHandle );
        }
    }


    return ret;
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
                // Unsupported qnn event profile type
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

    // Check input tensors
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CheckInputTensors( pInputs, numInputs );
    }

    // Check output tensors
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CheckOutputTensors( pOutputs, numOutputs );
    }


    std::vector<Qnn_Tensor_t> inputs;
    std::vector<Qnn_Tensor_t> outputs;

    auto graphInfo = ( *m_GraphsInfo )[0];

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < graphInfo.numInputTensors; i++ )
        {
            inputs.push_back( graphInfo.inputTensors[i] );
            // HTP
            Qnn_MemHandle_t memHandle = nullptr;
            ret = GetMemHandle( (RideHal_SharedBuffer_t *) ( pInputs + i ), &memHandle );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                QNN_TENSOR_SET_DIMENSIONS( &inputs[i], (uint32_t *) pInputs[i].tensorProps.dims );
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
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < graphInfo.numOutputTensors; i++ )
        {
            outputs.push_back( graphInfo.outputTensors[i] );
            Qnn_MemHandle_t memHandle = nullptr;
            ret = GetMemHandle( (RideHal_SharedBuffer_t *) ( pOutputs + i ), &memHandle );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                QNN_TENSOR_SET_DIMENSIONS( &outputs[i], (uint32_t *) pOutputs[i].tensorProps.dims );
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
        const Qnn_ErrorHandle_t executeStatus = m_QnnFunctionPointers.qnnInterface.graphExecute(
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
            const auto retVal =
                    m_QnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "Failed to DeRegister memory. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
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

RideHalError_e QnnRuntime::DeRegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
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
            auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) pSharedBuffers[i].data() );
            if ( it != s_DmaMemInfoMap[m_BackendCoreId].end() )
            {
                auto ptr = it->first;
                auto &info = it->second;
                const auto retVal =
                        m_QnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
                if ( QNN_SUCCESS != retVal )
                {
                    RIDEHAL_ERROR( "Failed to DeRegister memory. error is %d", (int) retVal );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else
                {
                    RIDEHAL_INFO( "succeed to deregister buffer %p(%d, %u, %u) as %p for core %d",
                                  pSharedBuffers[i].buffer.pData, info.fd, pSharedBuffers[i].size,
                                  pSharedBuffers[i].offset, info.memHandle, m_BackendCoreId );
                }
                if ( RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0 == m_BackendType ||
                     RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1 == m_BackendType )
                {
                    remote_register_buf_v2( extDomainId, (void *) ptr, info.size, -1 );
                }
                (void) s_DmaMemInfoMap[m_BackendCoreId].erase( it );
            }
            else
            {
                RIDEHAL_ERROR( "buffer hasn't been registered yet %p(%u, %u) for core %d",
                               pSharedBuffers[i].buffer.pData, pSharedBuffers[i].size,
                               pSharedBuffers[i].offset, m_BackendCoreId );
                ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            }

            if ( ret != RIDEHAL_ERROR_NONE )
            {
                break;
            }
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
                RIDEHAL_ERROR( "Could not free backend profile handle. error is %d", (int) retVal );
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
                RIDEHAL_ERROR( "Could not free context. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            m_Context = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_SystemContext )
        {
            const auto retVal =
                    m_QnnFunctionPointers.qnnSystemInterface.systemContextFree( m_SystemContext );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "Failed to free system context. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
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
                RIDEHAL_ERROR( "Could not free device handle. Error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            m_DeviceHandle = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr != m_PlatformInfo )
        {
            const auto retVal = m_QnnFunctionPointers.qnnInterface.deviceFreePlatformInfo(
                    m_LogHandle, m_PlatformInfo );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "Failed to free device platform info. error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            m_PlatformInfo = nullptr;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const Qnn_ErrorHandle_t retVal =
                m_QnnFunctionPointers.qnnInterface.backendFree( m_BackendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not terminate backend. Error is %d", (int) retVal );
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
                RIDEHAL_WARN( "Unable to terminate logging in the backend. Error is %d",
                              (int) retVal );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_LoadFromCachedBinary && ( nullptr != m_GraphsInfo ) )
        {
            RIDEHAL_DEBUG( "Cleaning up graph Info structures." );
            const qnn_wrapper_api::ModelError_t retVal =
                    qnn_wrapper_api::freeGraphsInfo( &m_GraphsInfo, m_GraphsCount );
            if ( qnn_wrapper_api::MODEL_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "failed to free graph info. Error is %d", (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
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

RideHal_TensorType_e QnnRuntime::SwitchFromQnnDataType( Qnn_DataType_t dataType )
{
    RideHal_TensorType_e tensorType = RIDEHAL_TENSOR_TYPE_UINT_8;
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

Qnn_DataType_t QnnRuntime::SwitchToQnnDataType( RideHal_TensorType_e tensorType )
{
    Qnn_DataType_t dataType = QNN_DATATYPE_UINT_8;
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

RideHalError_e QnnRuntime::CheckInputTensors( const RideHal_SharedBuffer_t *pInputs,
                                              uint32_t numInputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_inputTensorNum != numInputs )
    {
        RIDEHAL_ERROR( "Input tensors number is not equal to model input tensors number. Input "
                       "tensors number: %u, model input tensors number: %u",
                       numInputs, m_inputTensorNum );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < numInputs; ++i )
        {
            if ( pInputs[i].buffer.pData == nullptr )
            {
                RIDEHAL_ERROR( "buffer %u data pointer is nullptr", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            const auto tensor = &m_GraphsInfo[0]->inputTensors[i];
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            if ( pInputs[i].tensorProps.type != SwitchFromQnnDataType( dataType ) )
            {
                RIDEHAL_ERROR(
                        "Unmatched data type. shared buffer data type: %u, QNN data type: %u",
                        pInputs[i].tensorProps.type, dataType );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }

            const uint32_t rank = QNN_TENSOR_GET_RANK( tensor );
            if ( rank != pInputs[i].tensorProps.numDims )
            {
                RIDEHAL_ERROR( "Input tensors dim is not equal to model input tensors dim. "
                               "Input dim: %u, model input dim: %u",
                               pInputs[i].tensorProps.numDims, rank );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            const auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( size_t j = 1; j < rank; ++j )
            {
                if ( dimensions[j] != pInputs[i].tensorProps.dims[j] )
                {
                    RIDEHAL_ERROR( "Input tensor index %u 's shape  is not equal to model's. "
                                   "Shape layer: %u, input shape: %u, model shape: %u.",
                                   i, j, pInputs[i].tensorProps.dims[j], dimensions[j] );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    break;
                }
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::CheckOutputTensors( const RideHal_SharedBuffer_t *pOutputs,
                                               uint32_t numOutputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_outputTensorNum != numOutputs )
    {
        RIDEHAL_ERROR( "Output tensors number is not equal to model output tensors number. Output "
                       "tensors number: %u, model output tensors number: %u",
                       numOutputs, m_outputTensorNum );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < numOutputs; ++i )
        {
            if ( pOutputs[i].buffer.pData == nullptr )
            {
                RIDEHAL_ERROR( "buffer %u data pointer is nullptr", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }
            const auto tensor = &m_GraphsInfo[0]->outputTensors[i];
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            if ( pOutputs[i].tensorProps.type != SwitchFromQnnDataType( dataType ) )
            {
                RIDEHAL_ERROR(
                        "Unmatched data type. shared buffer data type: %u, QNN data type: %u",
                        pOutputs[i].tensorProps.type, dataType );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }

            const uint32_t rank = QNN_TENSOR_GET_RANK( tensor );
            if ( rank != pOutputs[i].tensorProps.numDims )
            {
                RIDEHAL_ERROR( "Output tensors dim is not equal to model output tensors dim. "
                               "Output dim: %u, model output dim: %u",
                               pOutputs[i].tensorProps.numDims, rank );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                break;
            }

            const auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            for ( size_t j = 1; j < rank; ++j )
            {
                if ( dimensions[j] != pOutputs[i].tensorProps.dims[j] )
                {
                    RIDEHAL_ERROR( "Output tensor index %u 's shape  is not equal to model's. "
                                   "Shape layer: %u, output shape: %u, model shape: %u.",
                                   i, j, pOutputs[i].tensorProps.dims[j], dimensions[j] );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    break;
                }
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal