// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

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

    qnnConfig.modelPath = "/var/opt/qride/data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP0;
    QnnRuntime_UdoPackage_t udoPackage;
    // udoPackage.udoLibPath = "libQnnAutoAiswOpPackage.so";
    // udoPackage.interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    // qnnConfig.udoPackages.push_back( udoPackage );

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

    /****** Dynamic dimension test ******/
    const uint32_t batchSize = 3;
    for ( int i = 0; i < inputNum; ++i )
    {
        auto ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        tensorInputList.pInfo[i].properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( int i = 0; i < outputNum; ++i )
    {
        auto ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        tensorOutputList.pInfo[i].properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    /****** Dynamic dimension test ******/


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

    qnnConfig.modelPath = "/var/opt/qride/data/centernet/program.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER;
    std::string modelPath = std::string( qnnConfig.modelPath );
    uint64_t bufferSize{ 0 };
    qnn::tools::datautil::StatusCode status{ qnn::tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = qnn::tools::datautil::getFileSize( modelPath );
    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize] );
    qnn::tools::datautil::readBinaryFromFile(
            modelPath, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = bufferSize;

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

TEST( QnnRuntime, Perf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "/var/opt/qride/data/centernet/program.bin";
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

    qnnConfig.modelPath = "/var/opt/qride/data/centernet/program.bin";
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

    qnnConfig.modelPath = "/var/noexistingfile.bin";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDEHAL_PROCESSOR_HTP1;

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = "/var/opt/qride/data/centernet/zero_buffer_size.bin";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = "/var/opt/qride/data/centernet";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );
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


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif