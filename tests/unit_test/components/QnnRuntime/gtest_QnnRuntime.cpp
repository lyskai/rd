// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#define QNNRUNTIME_UNIT_TEST
#include "accuracy.hpp"
#include "md5_utils.hpp"
#include "ridehal/component/QnnRuntime.hpp"
#include "gtest/gtest.h"
#include <stdio.h>

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

namespace
{

static void *LoadRaw( std::string path, size_t &size )
{
    void *pOut = nullptr;
    FILE *pFile = fopen( path.c_str(), "rb" );

    if ( nullptr != pFile )
    {
        fseek( pFile, 0, SEEK_END );
        size = ftell( pFile );
        fseek( pFile, 0, SEEK_SET );
        pOut = malloc( size );
        if ( nullptr != pOut )
        {
            auto readSize = fread( pOut, 1, size, pFile );
            (void) readSize;
        }
        fclose( pFile );
        printf( "load raw %s %d\n", path.c_str(), (int) size );
    }
    else
    {
        printf( "no raw file %s\n", path.c_str() );
    }

    return pOut;
}

}   // namespace

TEST( QnnRuntime, SANITY_General )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "SANITY";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, CreateModelFromBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "MODEL_FROM_BUF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP1;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER;
    std::string modelPath = std::string( qnnConfig.modelPath );
    uint64_t bufferSize{ 0 };
    qnn::tools::datautil::StatusCode status{ qnn::tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = qnn::tools::datautil::getFileSize( modelPath );
    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize] );
    qnn::tools::datautil::readBinaryFromFile(
            modelPath, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );

    // buffer is null
    qnnConfig.contextBuffer = nullptr;
    qnnConfig.contextSize = bufferSize;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // buffer size is 0
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 0;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // buffer size is incorrect
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 8;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // correct loading
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = bufferSize;
    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_DEBUG );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, Perf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QPERF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_Perf_t perf;
    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetPerf( (QnnRuntime_Perf_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, RegisterBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "REGBUF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_WARN );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetInputInfo( (QnnRuntime_TensorInfoList_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetOutputInfo( (QnnRuntime_TensorInfoList_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    uint64_t dmaHandle = inputs[0].buffer.dmaHandle;
    void *pData = inputs[0].buffer.pData;
    inputs[0].buffer.dmaHandle = (uint64_t) -1;
    inputs[0].buffer.pData = (void *) 123;
    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    inputs[0].buffer.dmaHandle = dmaHandle;
    inputs[0].buffer.pData = pData;

    ret = qnnRuntime.RegisterBuffers( inputs, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterBuffers( (RideHal_SharedBuffer_t *) nullptr, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    // Register same buffer again
    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.RegisterBuffers( outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    // DeRegister same buffer again
    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.DeRegisterBuffers( (RideHal_SharedBuffer_t *) nullptr, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, LoadModel )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "LOAD_MODEL";

    ret = qnnRuntime.Init( (const char *) nullptr, &qnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    qnnConfig.modelPath = "data/noexistingfile.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP1;

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = nullptr;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    qnnConfig.modelPath = "data/centernet/zero_buffer_size.bin";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // pConfig is null
    ret = qnnRuntime.Init( pName, nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
}

TEST( QnnRuntime, StateMachine )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QnnRuntime qnnRuntime;

    QnnRuntime_TensorInfoList_t infoList;
    ret = qnnRuntime.GetInputInfo( &infoList );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.GetOutputInfo( &infoList );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    RideHal_SharedBuffer_t sharedBuffer[1];
    ret = qnnRuntime.RegisterBuffers( sharedBuffer, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.DeRegisterBuffers( sharedBuffer, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    QnnRuntime_Perf_t perf;
    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( QnnRuntime, LoadOpPackage )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "OP_PKG";

    qnnConfig.modelPath = "data/bevdet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;
    const size_t numOfUdoPackages = 1;
    QnnRuntime_UdoPackage_t udoPackages[numOfUdoPackages];

    // numOfUdoPackages is - 1;
    qnnConfig.numOfUdoPackages = -1;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    //  pUdoPackages is null
    qnnConfig.pUdoPackages = nullptr;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // invalid so
    udoPackages[0].udoLibPath = "invalid.so";
    udoPackages[0].interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    qnnConfig.pUdoPackages = udoPackages;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    udoPackages[0].udoLibPath = "libQnnAutoAiswOpPackage.so";
    udoPackages[0].interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    qnnConfig.pUdoPackages = udoPackages;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, DataType )
{
    QnnRuntime qnnRuntime;
    EXPECT_EQ( QNN_DATATYPE_INT_8, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_16, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_32, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_64, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_8, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_64,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_64,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_8,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_8,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32 ) );


    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_8, qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_32 ) );
}

TEST( QnnRuntime, CreateModelFromSo )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "FROM_SO";

    qnnConfig.processorType = RIDEHAL_PROCESSOR_CPU;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;

    // invalid path
    qnnConfig.modelPath = "invalid.so";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    EXPECT_EQ( RIDEHAL_ERROR_FAIL, ret );

#if defined( __QNXNTO__ )
    qnnConfig.modelPath = "data/centernet/aarch64-qnx/libqride_centernet.so";
#else
    qnnConfig.modelPath = "data/centernet/aarch64-oe-linux-gcc9.3/libqride_centernet.so";
    return;
    /* Note: the build lib complains version `GLIBCXX_3.4.29' not found */
#endif
    qnnConfig.loadType = QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    qnnConfig.processorType = RIDEHAL_PROCESSOR_CPU;
    ret = qnnRuntime.Init( "CNT0", &qnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, DynamicBatchSize )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "DYN_BATCH";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /****** Dynamic dimension test ******/
    uint32_t batchSize = 10;
    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    batchSize = 3;
    for ( int i = 0; i < inputNum; ++i )
    {
        inputs[i].tensorProps.dims[0] = batchSize;
        inputs[i].size = inputs[i].buffer.size / 10 * 3;
    }

    for ( int i = 0; i < outputNum; ++i )
    {
        outputs[i].tensorProps.dims[0] = batchSize;
        outputs[i].size = outputs[i].buffer.size / 10 * 3;
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, BufferFree )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "BUF_FREE";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /****** Dynamic dimension test ******/
    uint32_t batchSize = 3;
    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    batchSize = 10;
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, OneBufferMutipleTensors )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "ONE_BUF_MUL_TS";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];

    size_t outputTotalSize = 0;
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        outputTotalSize += outputs[i].size;
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /**************  one buffer  **************/
    RideHal_SharedBuffer_t outputs1[outputNum];
    RideHal_SharedBuffer_t sharedBuffer;
    size_t offset = 0u;
    sharedBuffer.Allocate( outputTotalSize, RIDEHAL_BUFFER_USAGE_HTP,
                           RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );
    for ( int i = 0; i < outputNum; ++i )
    {
        outputs1[i] = outputs[i];
        outputs1[i].buffer.size = outputTotalSize;
        outputs1[i].buffer.pData = (void *) ( (uint8_t *) sharedBuffer.buffer.pData + offset );
        outputs1[i].buffer.dmaHandle = sharedBuffer.buffer.dmaHandle;
        outputs1[i].offset = offset;
        offset += outputs1[i].size;
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs1, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( int i = 0; i < outputNum; ++i )
    {
        std::string md5OneBuffer = MD5Sum( outputs1[i].buffer.pData, outputs1[i].size );
        std::string md5Output = MD5Sum( outputs[i].buffer.pData, outputs[i].size );
        EXPECT_EQ( md5OneBuffer, md5Output );
    }

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, TestAccuracy )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "ACCURACY";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    std::vector<std::string> inputDataPaths;
    inputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_input.raw" );
    ASSERT_EQ( inputDataPaths.size(), inputNum );
    for ( int i = 0; i < inputNum; ++i )
    {
        size_t inputSize = 0;
        void *pInputData = LoadRaw( inputDataPaths[i], inputSize );
        ASSERT_EQ( inputs[i].size, inputSize );
        memcpy( inputs[i].data(), pInputData, inputSize );
        free( pInputData );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];

    size_t outputTotalSize = 0;
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        outputTotalSize += outputs[i].size;
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    std::vector<std::string> outputDataPaths;
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_0.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_1.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_2.raw" );
    ASSERT_EQ( outputDataPaths.size(), outputNum );

    for ( int i = 0; i < outputNum; ++i )
    {
        size_t outputSize = 0;
        void *pOutputData = LoadRaw( outputDataPaths[i], outputSize );
        ASSERT_EQ( outputs[i].size, outputSize );
        double cosSim = CosineSimilarity( (uint8_t *) pOutputData,
                                          (uint8_t *) outputs[i].buffer.pData, outputSize );
        printf( "output: %d: cosine similarity = %f\n", i, (float) cosSim );
        ASSERT_GT( cosSim, 0.99d );
        free( pOutputData );
    }
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif