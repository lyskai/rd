// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#define QNNRUNTIME_UNIT_TEST
#include "ridehal/component/QnnRuntime.hpp"
#include "gtest/gtest.h"
#include <stdio.h>

using namespace ridehal::common;
using namespace ridehal::component;

TEST( QnnRuntime, SANITY_General )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;

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
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
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
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
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
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1;
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
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // buffer size is 0
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 0;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // buffer size is incorrect
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 8;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // correct loading
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = bufferSize;
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
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
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
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, Perf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;

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
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
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
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_Perf_t perf;
    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

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
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;

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
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
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
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


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
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/noexistingfile.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1;

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = "data/centernet/zero_buffer_size.bin";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = "data/centernet";
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
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/bevdet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;
    const size_t numOfUdoPackages = 1;
    QnnRuntime_UdoPackage_t udoPackages[numOfUdoPackages];

    // numOfUdoPackages is - 1;
    qnnConfig.numOfUdoPackages = -1;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    //  pUdoPackages is null
    qnnConfig.pUdoPackages = nullptr;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

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
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
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
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
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
    char pName[20] = "QnnRuntime";

    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_CPU;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    std::string modelPath = std::string( qnnConfig.modelPath );

    // invalid path
    qnnConfig.modelPath = "invalid.so";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    EXPECT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = "data/centernet/libqride_centernet.so";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    EXPECT_EQ( RIDEHAL_ERROR_FAIL, ret );
}

TEST( QnnRuntime, DynamicBatchSize )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;

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
        tensorInputList.pInfo[i].properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        tensorOutputList.pInfo[i].properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
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

    /****** Dynamic dimension test ******/


    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif