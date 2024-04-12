//-----------------------------------------------------------------------------
//
// Qualcomm Technologies, Inc. Proprietary
// (c) 2021-2024 Qualcomm Technologies, Inc. All rights reserved.
//
// All data and information contained in or disclosed by this document are
// confidential and proprietary information of Qualcomm Technologies, Inc., and
// all rights therein are expressly reserved. By accepting this material, the
// recipient agrees that this material and the information contained therein
// are held in confidence and in trust and will not be used, copied, reproduced
// in whole or in part, nor its contents revealed in any manner to others
// without the express written permission of Qualcomm Technologies, Inc.
//
// This software may be subject to U.S. and international export, re-export, or
// transfer ("export") laws.  Diversion contrary to U.S. and international law
// is strictly prohibited.
//-----------------------------------------------------------------------------

#include "ridehal/component/QnnRuntime.hpp"
#include "DataUtil.hpp"
#include "HTP/QnnHtpProfile.h"
#include "Logger.hpp"
#include "QnnProfile.h"
#include "QnnSampleAppUtils.hpp"
#include "QnnTypeMacros.hpp"
#include <HTP/QnnHtpCommon.h>
#include <HTP/QnnHtpMem.h>
#include <fstream>
#include <stdarg.h>
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

#define QNN_BACKEND_NUM ( sizeof( s_Backends ) / sizeof( char * ) )

static const char *const s_Backends[] = {
        "libQnnHtp.so", "libQnnHta.so", "libQnnCpu.so", "libQnnHtpMcp.so", "libQnnGpu.so",
};

std::mutex QnnRuntime::s_DmaMemInfoMapLock[QnnRuntime::DMA_MEMINFO_MAP_SIZE];
std::map<uint8_t *, QnnRuntime::DmaMemInfo_t>
        QnnRuntime::s_DmaMemInfoMap[QnnRuntime::DMA_MEMINFO_MAP_SIZE];
uint64_t QnnRuntime::s_DmaMemInfoMapUseRef[QnnRuntime::DMA_MEMINFO_MAP_SIZE] = { 0, 0 };

QnnRuntime::QnnRuntime() {}

void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp, va_list args )
{
    char msg[512];

    Logger_Level_e level = Logger_Level_e::LOGGER_LEVEL_INFO;
    switch ( logLevel )
    {
        case QNN_LOG_LEVEL_DEBUG:
        case QNN_LOG_LEVEL_VERBOSE:
            level = Logger_Level_e::LOGGER_LEVEL_DEBUG;
            break;
        case QNN_LOG_LEVEL_INFO:
            level = Logger_Level_e::LOGGER_LEVEL_INFO;
            break;
        case QNN_LOG_LEVEL_WARN:
            level = Logger_Level_e::LOGGER_LEVEL_WARN;
            break;
        case QNN_LOG_LEVEL_ERROR:
            level = Logger_Level_e::LOGGER_LEVEL_ERROR;
            break;
        default:
            break;
    }

    vprintf( fmt, args );
    // m_pLogger->Log( level, "%s", msg );
}

RideHalError_e QnnRuntime::CreateFromModelSo( std::string modelFile )
{
    if ( -1 == access( modelFile.c_str(), F_OK ) )
    {
        RIDEHAL_ERROR( "No existing file: %s", modelFile.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_CONTEXT_NO_ERROR != m_QnnFunctionPointers.qnnInterface.contextCreate(
                                         m_BackendHandle, m_DeviceHandle,
                                         (const QnnContext_Config_t **) m_ContextConfig,
                                         &m_Context ) )
    {
        RIDEHAL_ERROR( "%s: Could not create context", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( qnn_wrapper_api::ModelError_t::MODEL_NO_ERROR !=
         m_QnnFunctionPointers.composeGraphsFnHandle(
                 m_BackendHandle, m_QnnFunctionPointers.qnnInterface, m_Context,
                 (const qnn_wrapper_api::GraphConfigInfo_t **) m_GraphConfigsInfo,
                 m_GraphConfigsInfoCount, &m_GraphsInfo, &m_GraphsCount, false, QnnLog_Callback,
                 QNN_LOG_LEVEL_ERROR ) )
    {
        RIDEHAL_ERROR( "%s: Failed in composeGraphs()", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( m_GraphsCount != 1 )
    {
        RIDEHAL_ERROR( "%s: too much graphs", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( QNN_GRAPH_NO_ERROR !=
         m_QnnFunctionPointers.qnnInterface.graphFinalize( ( *m_GraphsInfo )[0].graph,
                                                           m_ProfileBackendHandle, nullptr ) )
    {
        RIDEHAL_ERROR( "%s: Failed in graphFinalize()", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    Qnn_ContextBinarySize_t binaryBufferSize;
    if ( QNN_GRAPH_NO_ERROR ==
         m_QnnFunctionPointers.qnnInterface.contextGetBinarySize( m_Context, &binaryBufferSize ) )
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
                RIDEHAL_INFO( "%s: saving cached binary(size = %llu)", m_Name.c_str(),
                              writtenBufferSize );

                auto dataUtilStatus = tools::datautil::writeBinaryToFile(
                        modelFile, "program.bin", (uint8_t *) saveBuffer.get(), writtenBufferSize );
                if ( tools::datautil::StatusCode::SUCCESS != dataUtilStatus )
                {
                    RIDEHAL_ERROR( "%s: Error while writing binary to file.", m_Name.c_str() );
                    return RideHalError_e::RIDE_HAL_ERROR_FAIL;
                }
            }
        }
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

RideHalError_e QnnRuntime::CreateFromBinary( uint8_t *buffer, uint64_t bufferSize )
{
    if ( 0 == bufferSize )
    {
        RIDEHAL_ERROR( "%s: Failed to create binary, Buffer size is 0", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( !buffer )
    {
        RIDEHAL_ERROR( "%s: Buffer is null.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    const QnnSystemContext_BinaryInfo_t *binaryInfo{ nullptr };
    Qnn_ContextBinarySize_t binaryInfoSize{ 0 };
    if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
                                m_SystemContext, static_cast<void *>( buffer ), bufferSize,
                                &binaryInfo, &binaryInfoSize ) )
    {
        RIDEHAL_ERROR( "%s: Failed to get context binary info", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    if ( !copyMetadataToGraphsInfo( binaryInfo, m_GraphsInfo, m_GraphsCount ) )
    {
        RIDEHAL_ERROR( "%s: Failed to copy metadata.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( nullptr == m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary )
    {
        RIDEHAL_ERROR( "%s: contextCreateFromBinaryFnHandle is nullptr.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary(
                 m_BackendHandle, m_DeviceHandle, (const QnnContext_Config_t **) m_ContextConfig,
                 static_cast<void *>( buffer ), bufferSize, &m_Context, m_ProfileBackendHandle ) )
    {
        RIDEHAL_ERROR( "%s: Could not create context from binary.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    for ( size_t graphIdx = 0; graphIdx < m_GraphsCount; graphIdx++ )
    {
        if ( nullptr == m_QnnFunctionPointers.qnnInterface.graphRetrieve )
        {
            RIDEHAL_ERROR( "%s: graphRetrieveFnHandle is nullptr.", m_Name.c_str() );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            break;
        }
        if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnInterface.graphRetrieve(
                                    m_Context, ( *m_GraphsInfo )[graphIdx].graphName,
                                    &( ( *m_GraphsInfo )[graphIdx].graph ) ) )
        {
            RIDEHAL_ERROR( "%s: Unable to retrieve graph handle for graph Idx: %d", m_Name.c_str(),
                           graphIdx );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}


RideHalError_e QnnRuntime::CreateFromBinary( std::string modelFile )
{
    if ( -1 == access( modelFile.c_str(), F_OK ) )
    {
        RIDEHAL_ERROR( "No existing file: %s", modelFile.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    uint64_t bufferSize{ 0 };
    std::shared_ptr<uint8_t> buffer{ nullptr };
    // read serialized binary into a byte buffer
    tools::datautil::StatusCode status{ tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = tools::datautil::getFileSize( modelFile );
    if ( 0 == bufferSize )
    {
        RIDEHAL_ERROR( "%s: Received path to an empty file. Nothing to deserialize.",
                       m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize], std::default_delete<uint8_t[]>() );
    if ( !buffer )
    {
        RIDEHAL_ERROR( "%s: Failed to allocate memory.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    status = tools::datautil::readBinaryFromFile(
            modelFile, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );
    if ( status != tools::datautil::StatusCode::SUCCESS )
    {
        RIDEHAL_ERROR( "%s: Failed to read binary data.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    return CreateFromBinary( buffer.get(), bufferSize );
}

RideHalError_e QnnRuntime::LoadOpPackages( const std::vector<QnnRuntime_UdoPackage_t> &udoPackages )
{

    for ( auto const &udoPackage : udoPackages )
    {

        if ( QNN_BACKEND_NO_ERROR != m_QnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                                             m_BackendHandle, (char *) udoPackage.udoLibPath,
                                             (char *) udoPackage.interfaceProvider, nullptr ) )
        {
            RIDEHAL_ERROR( "%s: Could not register Op Package: %s and interface provider: %s",
                           m_Name.c_str(), udoPackage.udoLibPath, udoPackage.interfaceProvider );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
        RIDEHAL_INFO( "%s: Registered Op Package: %s and interface provider: %s", m_Name.c_str(),
                      udoPackage.udoLibPath, udoPackage.interfaceProvider );
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

RideHalError_e QnnRuntime::Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                                 Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    ret = ComponentIF::Init( pName, level );

    m_Name = pName;

    m_BackendId = pConfig->backendId;
    m_BackendCoreId = pConfig->backendCoreId;
    auto modelPath = pConfig->modelPath;
    QnnLog_Error_t logError;
    auto logLevel = QNN_LOG_LEVEL_WARN;

    qnn::log::Logger::createLogger( QnnLog_Callback, logLevel, &logError );

    std::string soPath = modelPath + "/program.so";
    std::string binPath = modelPath + "/program.bin";
    m_LoadFromCachedBinary = ( pConfig->loadType == LOAD_CONTEXT_BIN_FROM_FILE ||
                               pConfig->loadType == LOAD_CONTEXT_BIN_FROM_BUFFER );
    if ( m_BackendId < (int) QNN_BACKEND_NUM )
    {
        auto statusCode = dynamicloadutil::getQnnFunctionPointers(
                s_Backends[m_BackendId], soPath, &m_QnnFunctionPointers, &m_BackendHandle,
                !m_LoadFromCachedBinary, &m_ModelHandle );
        if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
        {
            RIDEHAL_ERROR( "%s: failed to get qnn function pointers from model %s(%s), error "
                           "is %d",
                           m_Name.c_str(), soPath.c_str(), s_Backends[m_BackendId], statusCode );
            std::cout << "failed to get qnn function pointers from model, statusCode: "
                      << (int) statusCode << std::endl
                      << std::flush;
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "%s: invalid backend id %d", m_Name.c_str(), m_BackendId );
        std::cout << "invalid backend id" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_SUCCESS !=
         m_QnnFunctionPointers.qnnInterface.logCreate( QnnLog_Callback, logLevel, &m_LogHandle ) )
    {
        RIDEHAL_WARN( "%s: Unable to initialize logging in the backend.", m_Name.c_str() );
        std::cout << "Unable to initialize logging in the backend." << std::endl << std::flush;
    }


    auto statusCode = dynamicloadutil::getQnnSystemFunctionPointers( "libQnnSystem.so",
                                                                     &m_QnnFunctionPointers );
    if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
    {
        RIDEHAL_ERROR( "%s: Error initializing QNN System Function Pointers", m_Name.c_str() );
        std::cout << "Error initializing QNN System Function Pointers" << std::endl << std::flush;
    }

    if ( nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate ||
         nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ||
         nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextFree )
    {
        RIDEHAL_ERROR( "%s: QNN System function pointers are not populated.", m_Name.c_str() );
        std::cout << "QNN System function pointers are not populated" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_SUCCESS !=
         m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate( &m_SystemContext ) )
    {
        RIDEHAL_ERROR( "%s: Could not create system handle.", m_Name.c_str() );
        std::cout << "Could not create system handle" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    auto returnStatus = m_QnnFunctionPointers.qnnInterface.backendCreate(
            m_LogHandle, (const QnnBackend_Config_t **) m_BackendConfig, &m_BackendHandle );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        RIDEHAL_ERROR( "%s: Could not initialize backend due to error = %d", m_Name.c_str(),
                       returnStatus );
        std::cout << "Could not initialize backend due to errore" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    Qnn_ApiVersion_t version;
    returnStatus = m_QnnFunctionPointers.qnnInterface.backendGetApiVersion( &version );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        RIDEHAL_ERROR( "%s: Could not get backend version due to error = %d", m_Name.c_str(),
                       returnStatus );
        std::cout << "Could not get backend version due to error" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    m_BackendInitialized = true;
    RIDEHAL_INFO( "%s: QNN version: %u.%u.%u %u.%u.%u", m_Name.c_str(),
                  version.coreApiVersion.major, version.coreApiVersion.minor,
                  version.coreApiVersion.patch, version.backendApiVersion.major,
                  version.backendApiVersion.minor, version.backendApiVersion.patch );

    returnStatus = m_QnnFunctionPointers.qnnInterface.deviceGetPlatformInfo( m_LogHandle,
                                                                             &m_PlatformInfo );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        RIDEHAL_ERROR( "%s: Could not get platform information due to error = %d", m_Name.c_str(),
                       returnStatus );
        std::cout << "Could not get platform information due to error" << std::endl << std::flush;
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_PlatformInfo->version )
    {
        RIDEHAL_INFO( "%s: numHwDevices = %u", m_Name.c_str(), m_PlatformInfo->v1.numHwDevices );
        for ( uint32_t i = 0; i < m_PlatformInfo->v1.numHwDevices; i++ )
        {
            auto &deviceInfo = m_PlatformInfo->v1.hwDevices[i].v1;
            RIDEHAL_INFO( "%s: deviceId = %u deviceType = %u numCores = %u", m_Name.c_str(),
                          deviceInfo.deviceId, deviceInfo.deviceType, deviceInfo.numCores );
        }

        int deviceId = m_BackendCoreId;
        int core_Id = 0;
        if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP_MCP == m_BackendId )
        { /* NOTE: this assume that only has one HTP_MCP device */
            deviceId = 0;
            core_Id = m_BackendCoreId;
        }

        if ( deviceId < (int) m_PlatformInfo->v1.numHwDevices )
        {
            QnnDevice_HardwareDeviceInfo_t hwDevice = m_PlatformInfo->v1.hwDevices[deviceId];
            if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP_MCP == m_BackendId )
            {
                if ( core_Id < (int) hwDevice.v1.numCores )
                {
                    hwDevice.v1.numCores = 1;
                    hwDevice.v1.cores = &hwDevice.v1.cores[core_Id];
                }
                else
                {
                    RIDEHAL_ERROR( "%s: invalid backend core id = %d", m_Name.c_str(),
                                   m_BackendCoreId );
                    std::cout << "invalid backend core id" << std::endl << std::flush;

                    return RideHalError_e::RIDE_HAL_ERROR_FAIL;
                }
            }
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
            returnStatus = m_QnnFunctionPointers.qnnInterface.deviceCreate( m_LogHandle, configs,
                                                                            &m_DeviceHandle );
            if ( QNN_BACKEND_NO_ERROR != returnStatus )
            {
                RIDEHAL_ERROR( "%s: Could not create device due to error = %d", m_Name.c_str(),
                               returnStatus );
                std::cout << "Could not create device due to error" << std::endl << std::flush;
                return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "%s: invalid backend device id = %d", m_Name.c_str(), deviceId );
            std::cout << "invalid backend device id" << std::endl << std::flush;
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( pConfig->udoPackages.size() > 0 )
    {
        RideHalError_e ret = LoadOpPackages( pConfig->udoPackages );
        if ( RideHalError_e::RIDE_HAL_ERROR_NONE != ret )
        {
            QNN_ERROR( "%s: fail to load package", m_Name.c_str() );
            return ret;
        }
    }
    else
    {
        QNN_INFO( "No opPackage!" );
    }

    if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP == m_BackendId )
    {   // set up context priority
        m_ContextConfigArray[0].option = QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
        m_ContextConfigArray[0].priority = pConfig->priority;
        m_ContextConfig[0] = &m_ContextConfigArray[0];
        m_ContextConfig[1] = nullptr;
        RIDEHAL_INFO( "%s: set context priority = %d", m_Name.c_str(), pConfig->priority );
    }

    switch ( pConfig->loadType )
    {
        case LOAD_SHARED_LIBRARY:
        {
            if ( RIDE_HAL_ERROR_NONE != CreateFromModelSo( soPath ) )
            {
                std::cout << "fail to create from model so" << std::endl << std::flush;
                return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            }
            break;
        }
        case LOAD_CONTEXT_BIN_FROM_BUFFER:
        {
            if ( RIDE_HAL_ERROR_NONE !=
                 CreateFromBinary( pConfig->contextBuffer, pConfig->contextSize ) )
            {
                RIDEHAL_ERROR( "Failed to create from binary buffer %d", pConfig->contextSize );
                return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            }
            break;
            break;
        }

        case LOAD_CONTEXT_BIN_FROM_FILE:
        default:
        {
            if ( RIDE_HAL_ERROR_NONE != CreateFromBinary( binPath ) )
            {
                std::cout << "fail to create from binary" << std::endl << std::flush;
                return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            }
            break;
        }
    }

    RIDEHAL_INFO( "%s: init %s with backend %s\n", m_Name.c_str(), modelPath.c_str(),
                  s_Backends[m_BackendId] );

    if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP == m_BackendId )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        s_DmaMemInfoMapUseRef[m_BackendCoreId]++;
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

QnnRuntime::~QnnRuntime() {}

RideHalError_e QnnRuntime::GetInputInfo( QnnRuntime_TensorInfo_t *pInfos, uint32_t *pNum )
{
    // only update tensor numbers if pInfos is nullptr
    if ( pInfos == nullptr )
    {
        *pNum = m_GraphsInfo[0]->numInputTensors;
        return RideHalError_e::RIDE_HAL_ERROR_NONE;
    }
    else
    {
        RideHalError_e ret = RideHalError_e::RIDE_HAL_ERROR_FAIL;
        for ( uint32_t i = 0; i < *pNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            QnnRuntime_TensorInfo_t tensorInfo;
            auto tensor = &m_GraphsInfo[0]->inputTensors[i];

            auto name = QNN_TENSOR_GET_NAME( tensor );
            if ( nullptr != name )
            {
                pInfos[i].pName = name;
            }
            else
            {
                pInfos[i].pName = std::to_string( i ).c_str();
            }

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
                pInfos[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                pInfos[i].quantOffset = -quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "%s: input %s: quantize encoding %d not supported", m_Name.c_str(),
                              pInfos[i].pName, quantizeParams.quantizationEncoding );
            }
            auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            switch ( dataType )
            {
                case QNN_DATATYPE_UFIXED_POINT_8:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT8;
                    break;
                case QNN_DATATYPE_UFIXED_POINT_16:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT16;
                    break;
                case QNN_DATATYPE_FLOAT_32:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_FLOAT32;
                    break;
                default:
                    break;
            }
            pInfos[i].properties = tensorProp;
            ret = RideHalError_e::RIDE_HAL_ERROR_NONE;
        }
        return ret;
    }
}

RideHalError_e QnnRuntime::GetOutputInfo( QnnRuntime_TensorInfo_t *pInfos, uint32_t *pNum )
{
    // only update tensor numbers if pInfos is nullptr
    if ( pInfos == nullptr )
    {
        *pNum = m_GraphsInfo[0]->numOutputTensors;
        return RideHalError_e::RIDE_HAL_ERROR_NONE;
    }
    else
    {
        RideHalError_e ret = RideHalError_e::RIDE_HAL_ERROR_FAIL;
        for ( uint32_t i = 0; i < *pNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            QnnRuntime_TensorInfo_t tensorInfo;
            auto tensor = &m_GraphsInfo[0]->outputTensors[i];

            auto name = QNN_TENSOR_GET_NAME( tensor );
            if ( nullptr != name )
            {
                pInfos[i].pName = name;
            }
            else
            {
                pInfos[i].pName = std::to_string( i ).c_str();
            }

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
                pInfos[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                pInfos[i].quantOffset = -quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "%s: input %s: quantize encoding %d not supported", m_Name.c_str(),
                              pInfos[i].pName, quantizeParams.quantizationEncoding );
            }
            auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            switch ( dataType )
            {
                case QNN_DATATYPE_UFIXED_POINT_8:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT8;
                    break;
                case QNN_DATATYPE_UFIXED_POINT_16:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT16;
                    break;
                case QNN_DATATYPE_FLOAT_32:
                    tensorProp.type = RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_FLOAT32;
                    break;
                default:
                    break;
            }
            pInfos[i].properties = tensorProp;
            ret = RideHalError_e::RIDE_HAL_ERROR_NONE;
        }
        return ret;
    }
}

Qnn_MemHandle_t QnnRuntime::GetMemHandleHTP( const RideHal_SharedBuffer_t &sharedBuffer,
                                             const Qnn_Tensor_t &tensor )
{
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    // #warning QnnRuntime build with old version QNN SDK that do not support DMA buffer with
    // offset.
    if ( 0 != sharedBuffer.offset )
    {
        return nullptr;
    }
#endif

    if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
    {
        return nullptr; /* for safety */
    }

    Qnn_MemHandle_t memHandle = nullptr;

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
                                (int) sharedBuffer.handle );
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

        auto ret =
                m_QnnFunctionPointers.qnnInterface.memRegister( m_Context, &desc, 1, &memHandle );
        if ( QNN_SUCCESS != ret )
        {
            QNN_ERROR( "%s: map buffer %p(%d, %u, %u) for core %d, error %d\n", m_Name.c_str(),
                       sharedBuffer.buffer.pData, fd, sharedBuffer.size, sharedBuffer.offset,
                       m_BackendCoreId, ret );
        }
        else
        {
            QnnRuntime::DmaMemInfo_t info;
            info.memHandle = memHandle;
            info.size = sharedBuffer.size;
            s_DmaMemInfoMap[m_BackendCoreId][(uint8_t *) sharedBuffer.data()] = info;
            QNN_INFO( "%s: map buffer %p(%d, %u, %u) as %p for core %d", m_Name.c_str(),
                      sharedBuffer.buffer.pData, fd, sharedBuffer.size, sharedBuffer.offset,
                      memHandle, m_BackendCoreId );
        }
    }
    else
    {
        auto &info = it->second;
        memHandle = info.memHandle;
    }

    return memHandle;
}


RideHalError_e QnnRuntime::RegisterMemoryBuffer( RideHal_SharedBuffer_t &sharedBuffer )
{
#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    // #warning QnnRuntime build with old version QNN SDK that do not support DMA buffer with
    // offset.
    if ( 0 != sharedBuffer.offset )
    {
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
#endif

    if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
    {
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    Qnn_MemHandle_t memHandle = nullptr;

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
        desc.memShape.numDim = sharedBuffer.tensorProps.numDims;
        desc.memShape.dimSize = sharedBuffer.tensorProps.dims;
        switch ( sharedBuffer.tensorProps.type )
        {
            case RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT8:
                desc.dataType = QNN_DATATYPE_UFIXED_POINT_8;
                break;
            case RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_UINT16:
                desc.dataType = QNN_DATATYPE_UFIXED_POINT_16;
                break;
            case RideHal_TensorType_e::RIDE_HAL_TENSOR_TYPE_FLOAT32:
                desc.dataType = QNN_DATATYPE_FLOAT_32;
                break;
            default:
                break;
        }

        int client = 0;   // NOTE: default is 0
        int extDomainId = get_extended_domains_id( domain, client );
#if defined( __QNXNTO__ )
        remote_register_buf_v2( extDomainId, sharedBuffer.buffer.pData, sharedBuffer.size, 0 );
#else
        remote_register_buf_v2( extDomainId, sharedBuffer.buffer.pData, sharedBuffer.size,
                                (int) sharedBuffer.handle );
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

        auto ret =
                m_QnnFunctionPointers.qnnInterface.memRegister( m_Context, &desc, 1, &memHandle );
        if ( QNN_SUCCESS != ret )
        {
            RIDEHAL_ERROR( "%s: map buffer %p(%d, %u, %u) for core %d, error %d\n", m_Name.c_str(),
                           sharedBuffer.buffer.pData, fd, sharedBuffer.size, sharedBuffer.offset,
                           m_BackendCoreId, ret );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            QnnRuntime::DmaMemInfo_t info;
            info.memHandle = memHandle;
            info.size = sharedBuffer.size;
            s_DmaMemInfoMap[m_BackendCoreId][(uint8_t *) sharedBuffer.data()] = info;
            RIDEHAL_INFO( "%s: map buffer %p(%d, %u, %u) as %p for core %d", m_Name.c_str(),
                          sharedBuffer.buffer.pData, fd, sharedBuffer.size, sharedBuffer.offset,
                          memHandle, m_BackendCoreId );
        }
    }
    else
    {
        auto &info = it->second;
        memHandle = info.memHandle;
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

Qnn_MemHandle_t QnnRuntime::GetMemHandle( const RideHal_SharedBuffer_t &sharedBuffer,
                                          const Qnn_Tensor_t &tensor )
{

    if ( m_BackendId == QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP )
    {
        return GetMemHandleHTP( sharedBuffer, tensor );
    }


    return nullptr;
}

void QnnRuntime::ExtractProfilingEvent( QnnProfile_EventId_t profileEventId,
                                        QnnRuntime_Perf_t *perf )
{
    QnnProfile_EventData_t eventData;
    if ( QNN_PROFILE_NO_ERROR !=
         m_QnnFunctionPointers.qnnInterface.profileGetEventData( profileEventId, &eventData ) )
    {
        RIDEHAL_ERROR( "Failure in profile get event type." );
        return;
    }
    RIDEHAL_DEBUG( "Printing Event Info - Event Type: [%d], Event Value: [%" PRIu64
                   "], Event Identifier: [%s], Event Unit: [%d]",
                   eventData.type, eventData.value, eventData.identifier, eventData.unit );
    switch ( eventData.type )
    {
        case QNN_PROFILE_EVENTTYPE_EXECUTE:
            perf->qnn = eventData.value;
            break;
        case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HOST_RPC_TIME_MICROSEC:
            perf->rpc = eventData.value;
            break;
        case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HTP_RPC_TIME_MICROSEC:
            perf->qnnAccelerator = eventData.value;
            break;
        case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_ACCEL_TIME_MICROSEC:
            perf->accelerator = eventData.value;
            break;
        default:
            break;
    }
}

void QnnRuntime::GeneratePerf()
{
    const QnnProfile_EventId_t *profileEvents{ nullptr };
    uint32_t numEvents{ 0 };
    if ( QNN_PROFILE_NO_ERROR != m_QnnFunctionPointers.qnnInterface.profileGetEvents(
                                         m_ProfileBackendHandle, &profileEvents, &numEvents ) )
    {
        RIDEHAL_ERROR( "Failure in profile get events." );
        return;
    }
    RIDEHAL_DEBUG( "ProfileEvents: numEvents: [%u]", numEvents );
    for ( size_t event = 0; event < numEvents; event++ )
    {
        ExtractProfilingEvent( *( profileEvents + event ), &m_perf );
    }
}

RideHalError_e QnnRuntime::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                    const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    std::vector<Qnn_Tensor_t> inputs;
    std::vector<Qnn_Tensor_t> outputs;

    Qnn_ErrorHandle_t executeStatus = QNN_GRAPH_NO_ERROR;
    auto graphInfo = ( *m_GraphsInfo )[0];
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

    if ( ( m_bEnabelPerf ) && ( nullptr == m_ProfileBackendHandle ) )
    {
        auto ret = m_QnnFunctionPointers.qnnInterface.profileCreate(
                m_BackendHandle, QNN_PROFILE_LEVEL_BASIC, &m_ProfileBackendHandle );
        if ( QNN_GRAPH_NO_ERROR != ret )
        {
            RIDEHAL_ERROR( "%s: failed to create profile", m_Name.c_str() );
        }
    }

    executeStatus = m_QnnFunctionPointers.qnnInterface.graphExecute(
            graphInfo.graph, inputs.data(), graphInfo.numInputTensors, outputs.data(),
            graphInfo.numOutputTensors, m_ProfileBackendHandle, nullptr );
    if ( QNN_GRAPH_NO_ERROR != executeStatus )
    {
        RIDEHAL_ERROR( "%s: QNN failed %d", m_Name.c_str(), executeStatus );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    else
    {
        if ( ( m_bEnabelPerf ) && ( nullptr != m_ProfileBackendHandle ) )
        {
            GeneratePerf();
        }
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

void QnnRuntime::DeRegisterMemory()
{
    if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
    {
        return; /* for safety */
    }

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
        if ( m_BackendId == QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP )
        {
            remote_register_buf_v2( extDomainId, (void *) ptr, info.size, -1 );
        }
    }
    s_DmaMemInfoMap[m_BackendCoreId].clear();
}

void QnnRuntime::DeRegisterMemory( const RideHal_SharedBuffer_t &sharedBuffer )
{
    if ( m_BackendCoreId >= (int) DMA_MEMINFO_MAP_SIZE )
    {
        return; /* for safety */
    }

    constexpr int client = 0;   // NOTE: default is 0
    int domain = CDSP_DOMAIN_ID;
    if ( 1 == m_BackendCoreId )
    {
        domain = CDSP1_DOMAIN_ID;
    }
    int extDomainId = get_extended_domains_id( domain, client );

    std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
    auto it = s_DmaMemInfoMap[m_BackendCoreId].find( (uint8_t *) sharedBuffer.data() );
    if ( it == s_DmaMemInfoMap[m_BackendCoreId].end() )
    {
        auto ptr = it->first;
        auto &info = it->second;
        m_QnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
        if ( m_BackendId == QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP )
        {
            remote_register_buf_v2( extDomainId, (void *) ptr, info.size, -1 );
        }
    }

    s_DmaMemInfoMap[m_BackendCoreId].erase( it );
}

RideHalError_e QnnRuntime::Deinit()
{
    if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP == m_BackendId )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        if ( s_DmaMemInfoMapUseRef[m_BackendCoreId] > 0 )
        {
            s_DmaMemInfoMapUseRef[m_BackendCoreId]--;
            if ( 0 == s_DmaMemInfoMapUseRef[m_BackendCoreId] )
            {
                DeRegisterMemory();
            }
        }
    }

    if ( nullptr != m_ProfileBackendHandle )
    {
        if ( QNN_PROFILE_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.profileFree( m_ProfileBackendHandle ) )
        {
            RIDEHAL_ERROR( "%s:Could not free backend profile handle.", m_Name.c_str() );
        }
    }

    if ( nullptr != m_Context )
    {
        if ( QNN_CONTEXT_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.contextFree( m_Context, nullptr ) )
        {
            RIDEHAL_ERROR( "%s:Could not free context", m_Name.c_str() );
        }
        m_Context = nullptr;
    }

    if ( nullptr != m_SystemContext )
    {
        m_QnnFunctionPointers.qnnSystemInterface.systemContextFree( m_SystemContext );
    }

    if ( nullptr != m_DeviceHandle )
    {
        if ( QNN_CONTEXT_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.deviceFree( m_DeviceHandle ) )
        {
            RIDEHAL_ERROR( "%s:Could not free device handle", m_Name.c_str() );
        }
        m_DeviceHandle = nullptr;
    }

    if ( nullptr != m_PlatformInfo )
    {
        m_QnnFunctionPointers.qnnInterface.deviceFreePlatformInfo( m_LogHandle, m_PlatformInfo );
        m_PlatformInfo = nullptr;
    }

    if ( m_BackendInitialized )
    {
        if ( QNN_BACKEND_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.backendFree( m_BackendHandle ) )
        {
            RIDEHAL_ERROR( "%s:Could not terminate backend", m_Name.c_str() );
        }
        m_BackendInitialized = false;
    }

    if ( m_LogHandle != nullptr )
    {
        if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnInterface.logFree( m_LogHandle ) )
        {
            RIDEHAL_WARN( "%s:Unable to terminate logging in the backend.", m_Name.c_str() );
        }
    }

    if ( m_LoadFromCachedBinary && ( nullptr != m_GraphsInfo ) )
    {
        RIDEHAL_DEBUG( "%s:Cleaning up graph Info structures.", m_Name.c_str() );
        qnn_wrapper_api::freeGraphsInfo( &m_GraphsInfo, m_GraphsCount );
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

}   // namespace component
}   // namespace ridehal