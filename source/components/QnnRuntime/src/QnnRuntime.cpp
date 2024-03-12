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

// #ifdef QNN_WARN
// #undef QNN_WARN
// #define QNN_WARN( fmt, ... ) m_pLogger->Log( Logger_Level_e::LOGGER_LEVEL_WARN, fmt,
// ##__VA_ARGS__ ) #endif

// #ifdef QNN_INFO
// #undef QNN_INFO
// #define QNN_INFO( fmt, ... ) m_pLogger->Log( Logger_Level_e::LOGGER_LEVEL_INFO, fmt,
// ##__VA_ARGS__ ) #endif

// #ifdef QNN_DEBUG
// #undef QNN_DEBUG
// #define QNN_DEBUG( fmt, ... ) \
//     m_pLogger->Log( Logger_Level_e::LOGGER_LEVEL_DEBUG, fmt, ##__VA_ARGS__ )
// #endif

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

    // hogl::area *m_HoglArea = hogl::add_area( "QNN-RUNTIME" );
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

    vsnprintf( msg, sizeof( msg ), fmt, args );
    // m_pLogger->Log( level, "%s", msg );
}

RideHalError_e QnnRuntime::CreateFromModelSo( std::string modelPath )
{
    if ( QNN_CONTEXT_NO_ERROR != m_QnnFunctionPointers.qnnInterface.contextCreate(
                                         m_BackendHandle, m_DeviceHandle,
                                         (const QnnContext_Config_t **) m_ContextConfig,
                                         &m_Context ) )
    {
        QNN_ERROR( "%s: Could not create context", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( qnn_wrapper_api::ModelError_t::MODEL_NO_ERROR !=
         m_QnnFunctionPointers.composeGraphsFnHandle(
                 m_BackendHandle, m_QnnFunctionPointers.qnnInterface, m_Context,
                 (const qnn_wrapper_api::GraphConfigInfo_t **) m_GraphConfigsInfo,
                 m_GraphConfigsInfoCount, &m_GraphsInfo, &m_GraphsCount, false, QnnLog_Callback,
                 QNN_LOG_LEVEL_ERROR ) )
    {
        QNN_ERROR( "%s: Failed in composeGraphs()", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( m_GraphsCount != 1 )
    {
        QNN_ERROR( "%s: too much graphs", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( QNN_GRAPH_NO_ERROR !=
         m_QnnFunctionPointers.qnnInterface.graphFinalize( ( *m_GraphsInfo )[0].graph,
                                                           m_ProfileBackendHandle, nullptr ) )
    {
        QNN_ERROR( "%s: Failed in graphFinalize()", m_Name.c_str() );
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
                QNN_INFO( "%s: saving cached binary(size = %llu)", m_Name.c_str(),
                          writtenBufferSize );

                auto dataUtilStatus = tools::datautil::writeBinaryToFile(
                        modelPath, "program.bin", (uint8_t *) saveBuffer.get(), writtenBufferSize );
                if ( tools::datautil::StatusCode::SUCCESS != dataUtilStatus )
                {
                    QNN_ERROR( "%s: Error while writing binary to file.", m_Name.c_str() );
                    return RideHalError_e::RIDE_HAL_ERROR_FAIL;
                }
            }
        }
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

RideHalError_e QnnRuntime::CreateFromBinary( std::string binPath )
{
    uint64_t bufferSize{ 0 };
    std::shared_ptr<uint8_t> buffer{ nullptr };
    // read serialized binary into a byte buffer
    tools::datautil::StatusCode status{ tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = tools::datautil::getFileSize( binPath );
    if ( 0 == bufferSize )
    {
        QNN_ERROR( "%s: Received path to an empty file. Nothing to deserialize.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize], std::default_delete<uint8_t[]>() );
    if ( !buffer )
    {
        QNN_ERROR( "%s: Failed to allocate memory.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    status = tools::datautil::readBinaryFromFile(
            binPath, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );
    if ( status != tools::datautil::StatusCode::SUCCESS )
    {
        QNN_ERROR( "%s: Failed to read binary data.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    const QnnSystemContext_BinaryInfo_t *binaryInfo{ nullptr };
    Qnn_ContextBinarySize_t binaryInfoSize{ 0 };
    if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
                                m_SystemContext, static_cast<void *>( buffer.get() ), bufferSize,
                                &binaryInfo, &binaryInfoSize ) )
    {
        QNN_ERROR( "%s: Failed to get context binary info", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    if ( !copyMetadataToGraphsInfo( binaryInfo, m_GraphsInfo, m_GraphsCount ) )
    {
        QNN_ERROR( "%s: Failed to copy metadata.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( nullptr == m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary )
    {
        QNN_ERROR( "%s: contextCreateFromBinaryFnHandle is nullptr.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( m_QnnFunctionPointers.qnnInterface.contextCreateFromBinary(
                 m_BackendHandle, m_DeviceHandle, (const QnnContext_Config_t **) m_ContextConfig,
                 static_cast<void *>( buffer.get() ), bufferSize, &m_Context,
                 m_ProfileBackendHandle ) )
    {
        QNN_ERROR( "%s: Could not create context from binary.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    for ( size_t graphIdx = 0; graphIdx < m_GraphsCount; graphIdx++ )
    {
        if ( nullptr == m_QnnFunctionPointers.qnnInterface.graphRetrieve )
        {
            QNN_ERROR( "%s: graphRetrieveFnHandle is nullptr.", m_Name.c_str() );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            break;
        }
        if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnInterface.graphRetrieve(
                                    m_Context, ( *m_GraphsInfo )[graphIdx].graphName,
                                    &( ( *m_GraphsInfo )[graphIdx].graph ) ) )
        {
            QNN_ERROR( "%s: Unable to retrieve graph handle for graph Idx: %d", m_Name.c_str(),
                       graphIdx );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

RideHalError_e QnnRuntime::LoadOpPackages( std::string opPackgesTxtPath )
{
    const size_t pathIdx = 0;
    const size_t interfaceProviderIdx = 1;
    std::vector<std::string> opPackagePaths;
    std::ifstream is( opPackgesTxtPath );
    std::string path;
    while ( is >> path )
    {
        opPackagePaths.push_back( path );
    }

    for ( auto const &opPackagePath : opPackagePaths )
    {
        std::vector<std::string> opPackage;
        split( opPackage, opPackagePath, ':' );
        QNN_DEBUG( "%s: opPackagePath: %s", m_Name.c_str(), opPackagePath.c_str() );
        if ( opPackage.size() != 2 )
        {
            QNN_ERROR( "%s: Malformed opPackageString provided: %s", m_Name.c_str(),
                       opPackagePath.c_str() );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
        if ( nullptr == m_QnnFunctionPointers.qnnInterface.backendRegisterOpPackage )
        {
            QNN_ERROR( "%s: backendRegisterOpPackageFnHandle is nullptr.", m_Name.c_str() );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
        if ( QNN_BACKEND_NO_ERROR != m_QnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                                             m_BackendHandle, (char *) opPackage[pathIdx].c_str(),
                                             (char *) opPackage[interfaceProviderIdx].c_str(),
                                             nullptr ) )
        {
            QNN_ERROR( "%s: Could not register Op Package: %s and interface provider: %s",
                       m_Name.c_str(), opPackage[pathIdx].c_str(),
                       opPackage[interfaceProviderIdx].c_str() );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
        QNN_INFO( "%s: Registered Op Package: %s and interface provider: %s", m_Name.c_str(),
                  opPackage[pathIdx].c_str(), opPackage[interfaceProviderIdx].c_str() );
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

RideHalError_e QnnRuntime::Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                                 Logger *pLogger )
{
    m_Name = pName;
    m_pLogger = pLogger;
    pLogger->Init( pName );
    m_BackendId = pConfig->backendId;
    m_BackendCoreId = pConfig->backendCoreId;
    auto modelPath = pConfig->modelPath;
    QnnLog_Error_t logError;
    auto logLevel = QNN_LOG_LEVEL_WARN;

    qnn::log::Logger::createLogger( QnnLog_Callback, logLevel, &logError );

    std::string soPath = modelPath + "/program.so";
    std::string binPath = modelPath + "/program.bin";
    if ( 0 == access( binPath.c_str(), F_OK ) )
    {
        m_LoadFromCachedBinary = true;
    }
    if ( m_BackendId < (int) QNN_BACKEND_NUM )
    {
        auto statusCode = dynamicloadutil::getQnnFunctionPointers(
                s_Backends[m_BackendId], soPath, &m_QnnFunctionPointers, &m_BackendHandle,
                !m_LoadFromCachedBinary, &m_ModelHandle );
        if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
        {
            QNN_ERROR( "%s: failed to get qnn function pointers from model %s(%s), error "
                       "is %d",
                       m_Name.c_str(), soPath.c_str(), s_Backends[m_BackendId], statusCode );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }
    else
    {
        QNN_ERROR( "%s: invalid backend id %d", m_Name.c_str(), m_BackendId );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_SUCCESS !=
         m_QnnFunctionPointers.qnnInterface.logCreate( QnnLog_Callback, logLevel, &m_LogHandle ) )
    {
        QNN_WARN( "%s: Unable to initialize logging in the backend.", m_Name.c_str() );
    }


    auto statusCode = dynamicloadutil::getQnnSystemFunctionPointers( "libQnnSystem.so",
                                                                     &m_QnnFunctionPointers );
    if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
    {
        QNN_ERROR( "%s: Error initializing QNN System Function Pointers", m_Name.c_str() );
    }

    if ( nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate ||
         nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ||
         nullptr == m_QnnFunctionPointers.qnnSystemInterface.systemContextFree )
    {
        QNN_ERROR( "%s: QNN System function pointers are not populated.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_SUCCESS !=
         m_QnnFunctionPointers.qnnSystemInterface.systemContextCreate( &m_SystemContext ) )
    {
        QNN_ERROR( "%s: Could not create system handle.", m_Name.c_str() );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    auto returnStatus = m_QnnFunctionPointers.qnnInterface.backendCreate(
            m_LogHandle, (const QnnBackend_Config_t **) m_BackendConfig, &m_BackendHandle );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        QNN_ERROR( "%s: Could not initialize backend due to error = %d", m_Name.c_str(),
                   returnStatus );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    Qnn_ApiVersion_t version;
    returnStatus = m_QnnFunctionPointers.qnnInterface.backendGetApiVersion( &version );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        QNN_ERROR( "%s: Could not get backend version due to error = %d", m_Name.c_str(),
                   returnStatus );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    m_BackendInitialized = true;
    QNN_INFO( "%s: QNN version: %u.%u.%u %u.%u.%u", m_Name.c_str(), version.coreApiVersion.major,
              version.coreApiVersion.minor, version.coreApiVersion.patch,
              version.backendApiVersion.major, version.backendApiVersion.minor,
              version.backendApiVersion.patch );

    returnStatus = m_QnnFunctionPointers.qnnInterface.deviceGetPlatformInfo( m_LogHandle,
                                                                             &m_PlatformInfo );
    if ( QNN_BACKEND_NO_ERROR != returnStatus )
    {
        QNN_ERROR( "%s: Could not get platform information due to error = %d", m_Name.c_str(),
                   returnStatus );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }

    if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_PlatformInfo->version )
    {
        QNN_INFO( "%s: numHwDevices = %u", m_Name.c_str(), m_PlatformInfo->v1.numHwDevices );
        for ( uint32_t i = 0; i < m_PlatformInfo->v1.numHwDevices; i++ )
        {
            auto &deviceInfo = m_PlatformInfo->v1.hwDevices[i].v1;
            QNN_INFO( "%s: deviceId = %u deviceType = %u numCores = %u", m_Name.c_str(),
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
                    QNN_ERROR( "%s: invalid backend core id = %d", m_Name.c_str(),
                               m_BackendCoreId );
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
                QNN_ERROR( "%s: Could not create device due to error = %d", m_Name.c_str(),
                           returnStatus );
                return RideHalError_e::RIDE_HAL_ERROR_FAIL;
            }
        }
        else
        {
            QNN_ERROR( "%s: invalid backend device id = %d", m_Name.c_str(), deviceId );
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    std::string opPackgesTxtPath = modelPath + "/OpPackges.txt";
    if ( 0 == access( opPackgesTxtPath.c_str(), F_OK ) )
    {
        if ( false == LoadOpPackages( opPackgesTxtPath ) )
        {
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP == m_BackendId )
    {   // set up context priority
        m_ContextConfigArray[0].option = QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
        m_ContextConfigArray[0].priority = pConfig->priority;
        m_ContextConfig[0] = &m_ContextConfigArray[0];
        m_ContextConfig[1] = nullptr;
        QNN_INFO( "%s: set context priority = %d", m_Name.c_str(), pConfig->priority );
    }

    if ( !m_LoadFromCachedBinary )
    {
        if ( false == CreateFromModelSo( modelPath ) )
        {
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }
    else
    {
        if ( false == CreateFromBinary( binPath ) )
        {
            return RideHalError_e::RIDE_HAL_ERROR_FAIL;
        }
    }

    QNN_INFO( "%s: init %s with backend %s\n", m_Name.c_str(), modelPath.c_str(),
              s_Backends[m_BackendId] );

    if ( QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP == m_BackendId )
    {
        std::lock_guard<std::mutex> l( s_DmaMemInfoMapLock[m_BackendCoreId] );
        s_DmaMemInfoMapUseRef[m_BackendCoreId]++;
    }

    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

QnnRuntime::~QnnRuntime() {}

RideHalError_e QnnRuntime::GetInputInfos( std::vector<QnnRuntime_TensorInfo_t> &infos )
{
    RideHalError_e ret = RideHalError_e::RIDE_HAL_ERROR_FAIL;
    for ( uint32_t i = 0; i < m_GraphsInfo[0]->numInputTensors; i++ )
    {
        QnnRuntime_TensorInfo_t info;
        auto tensor = &m_GraphsInfo[0]->inputTensors[i];
        auto name = QNN_TENSOR_GET_NAME( tensor );
        if ( nullptr != name )
        {
            info.name = name;
        }
        else
        {
            info.name = "input:" + std::to_string( i );
        }
        size_t sz = 1;
        auto rank = QNN_TENSOR_GET_RANK( tensor );
        auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
        for ( uint32_t j = 0; j < rank; j++ )
        {
            sz *= dimensions[j];
            info.dims.push_back( dimensions[j] );
        }
        info.offset = 0;
        info.size = sz;
        auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
        if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
        {
            info.quant_scale = quantizeParams.scaleOffsetEncoding.scale;
            info.quant_offset = -quantizeParams.scaleOffsetEncoding.offset;
        }
        else
        {
            QNN_WARN( "%s: input %s: quantize encoding %d not supported", m_Name.c_str(),
                      info.name.c_str(), quantizeParams.quantizationEncoding );
        }
        auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
        switch ( dataType )
        {
            case QNN_DATATYPE_UFIXED_POINT_8:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_UINT8;
                break;
            case QNN_DATATYPE_UFIXED_POINT_16:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_UINT16;
                info.size *= sizeof( uint16_t );
                break;
            case QNN_DATATYPE_FLOAT_32:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_FLOAT32;
                info.size *= sizeof( float );
                break;
            default:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_RAW;
                switch ( dataType & 0xFF )
                {
                    case 0x08:
                        break;
                    case 0x16:
                        info.size *= 2;
                        break;
                    case 0x32:
                        info.size *= 4;
                        break;
                    case 0x64:
                        info.size *= 8;
                        break;
                    default:
                        QNN_ERROR( "%s: input %s: invalid data type=%x \n", m_Name.c_str(),
                                   info.name.c_str(), dataType );
                        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
                        break;
                }
                break;
        }
        QNN_INFO( "%s: input %s: shape = %s, size=%u, scale=%f, offset=%d, type=%x\n",
                  m_Name.c_str(), info.name.c_str(), info.shape().c_str(), sz, info.quant_scale,
                  info.quant_offset, dataType );
        infos.push_back( info );
        ret = RideHalError_e::RIDE_HAL_ERROR_NONE;
    }
    return ret;
}

RideHalError_e QnnRuntime::GetOutputInfos( std::vector<QnnRuntime_TensorInfo_t> &infos )
{
    RideHalError_e ret = RideHalError_e::RIDE_HAL_ERROR_FAIL;

    for ( uint32_t i = 0; i < m_GraphsInfo[0]->numOutputTensors; i++ )
    {
        QnnRuntime_TensorInfo_t info;
        auto tensor = &m_GraphsInfo[0]->outputTensors[i];
        auto name = QNN_TENSOR_GET_NAME( tensor );
        if ( nullptr != name )
        {
            info.name = name;
        }
        else
        {
            info.name = "output:" + std::to_string( i );
        }

        size_t sz = 1;
        auto rank = QNN_TENSOR_GET_RANK( tensor );
        auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
        for ( uint32_t j = 0; j < rank; j++ )
        {
            sz *= dimensions[j];
            info.dims.push_back( dimensions[j] );
        }
        info.offset = 0;
        info.size = sz;
        auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
        if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
        {
            info.quant_scale = quantizeParams.scaleOffsetEncoding.scale;
            info.quant_offset = -quantizeParams.scaleOffsetEncoding.offset;
        }
        else
        {
            QNN_WARN( "%s: output %s: quantize encoding %d not supported", m_Name.c_str(),
                      info.name.c_str(), quantizeParams.quantizationEncoding );
        }
        auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
        switch ( dataType )
        {
            case QNN_DATATYPE_UFIXED_POINT_8:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_UINT8;
                break;
            case QNN_DATATYPE_UFIXED_POINT_16:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_UINT16;
                info.size *= sizeof( uint16_t );
                break;
            case QNN_DATATYPE_FLOAT_32:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_FLOAT32;
                info.size *= sizeof( float );
                break;
            default:
                info.dataType = QnnRuntime_TensorInfo_t::Tensor_DataType_t::TENSOR_DATATYPE_RAW;
                switch ( dataType & 0xFF )
                {
                    case 0x08:
                        break;
                    case 0x16:
                        info.size *= 2;
                        break;
                    case 0x32:
                        info.size *= 4;
                        break;
                    case 0x64:
                        info.size *= 8;
                        break;
                    default:
                        QNN_ERROR( "%s: output %s: invalid data type=%x \n", m_Name.c_str(),
                                   info.name.c_str(), dataType );
                        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
                        break;
                }
                break;
        }
        QNN_INFO( "%s: output %s: shape = %s, size=%u, scale=%f, offset=%d, type=%x\n",
                  m_Name.c_str(), info.name.c_str(), info.shape().c_str(), sz, info.quant_scale,
                  info.quant_offset, dataType );
        infos.push_back( info );
        ret = RideHalError_e::RIDE_HAL_ERROR_NONE;
    }

    return ret;
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
        QNN_ERROR( "Failure in profile get event type." );
        return;
    }
    QNN_DEBUG( "Printing Event Info - Event Type: [%d], Event Value: [%" PRIu64
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

void QnnRuntime::GetPerf( QnnRuntime_Perf_t *perf )
{
    const QnnProfile_EventId_t *profileEvents{ nullptr };
    uint32_t numEvents{ 0 };
    if ( QNN_PROFILE_NO_ERROR != m_QnnFunctionPointers.qnnInterface.profileGetEvents(
                                         m_ProfileBackendHandle, &profileEvents, &numEvents ) )
    {
        QNN_ERROR( "Failure in profile get events." );
        return;
    }
    QNN_DEBUG( "ProfileEvents: numEvents: [%u]", numEvents );
    for ( size_t event = 0; event < numEvents; event++ )
    {
        ExtractProfilingEvent( *( profileEvents + event ), perf );
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

    if ( ( nullptr != m_pPerf ) && ( nullptr == m_ProfileBackendHandle ) )
    {
        auto ret = m_QnnFunctionPointers.qnnInterface.profileCreate(
                m_BackendHandle, QNN_PROFILE_LEVEL_BASIC, &m_ProfileBackendHandle );
        if ( QNN_GRAPH_NO_ERROR != ret )
        {
            QNN_ERROR( "%s: failed to create profile", m_Name.c_str() );
        }
    }

    executeStatus = m_QnnFunctionPointers.qnnInterface.graphExecute(
            graphInfo.graph, inputs.data(), graphInfo.numInputTensors, outputs.data(),
            graphInfo.numOutputTensors, m_ProfileBackendHandle, nullptr );
    if ( QNN_GRAPH_NO_ERROR != executeStatus )
    {
        QNN_ERROR( "%s: QNN failed %d", m_Name.c_str(), executeStatus );
        return RideHalError_e::RIDE_HAL_ERROR_FAIL;
    }
    else
    {
        if ( ( nullptr != m_pPerf ) && ( nullptr != m_ProfileBackendHandle ) )
        {
            GetPerf( m_pPerf );
        }
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

void QnnRuntime::DeResisterMemory()
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
                DeResisterMemory();
            }
        }
    }

    if ( nullptr != m_ProfileBackendHandle )
    {
        if ( QNN_PROFILE_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.profileFree( m_ProfileBackendHandle ) )
        {
            QNN_ERROR( "%s:Could not free backend profile handle.", m_Name.c_str() );
        }
    }

    if ( nullptr != m_Context )
    {
        if ( QNN_CONTEXT_NO_ERROR !=
             m_QnnFunctionPointers.qnnInterface.contextFree( m_Context, nullptr ) )
        {
            QNN_ERROR( "%s:Could not free context", m_Name.c_str() );
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
            QNN_ERROR( "%s:Could not free device handle", m_Name.c_str() );
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
            QNN_ERROR( "%s:Could not terminate backend", m_Name.c_str() );
        }
        m_BackendInitialized = false;
    }

    if ( m_LogHandle != nullptr )
    {
        if ( QNN_SUCCESS != m_QnnFunctionPointers.qnnInterface.logFree( m_LogHandle ) )
        {
            QNN_WARN( "%s:Unable to terminate logging in the backend.", m_Name.c_str() );
        }
    }

    if ( m_LoadFromCachedBinary && ( nullptr != m_GraphsInfo ) )
    {
        QNN_DEBUG( "%s:Cleaning up graph Info structures.", m_Name.c_str() );
        qnn_wrapper_api::freeGraphsInfo( &m_GraphsInfo, m_GraphsCount );
    }
    return RideHalError_e::RIDE_HAL_ERROR_NONE;
}

}   // namespace component
}   // namespace ridehal