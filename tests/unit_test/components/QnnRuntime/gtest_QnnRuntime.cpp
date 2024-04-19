// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "DataUtil.hpp"
#include "ridehal/component/QnnRuntime.hpp"
#include "gtest/gtest.h"
#include <stdio.h>

using namespace ridehal::common;
using namespace ridehal::component;

TEST( QnnRuntime, SANITY_General )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "/var/opt/qride/data/centernet";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDE_HAL_PROCESSOR_HTP0;
    QnnRuntime_UdoPackage_t udoPackage;
    // udoPackage.udoLibPath = "libQnnAutoAiswOpPackage.so";
    // udoPackage.interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    // qnnConfig.udoPackages.push_back( udoPackage );

    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( QnnRuntime, CreateModelFromBuffer )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QnnRuntime";

    qnnConfig.modelPath = "/var/opt/qride/data/centernet";
    qnnConfig.backendType = RideHal_ProcessorType_e::RIDE_HAL_PROCESSOR_HTP0;
    qnnConfig.loadType = QnnRuntime_LoadType_e::LOAD_CONTEXT_BIN_FROM_BUFFER;
    std::string modelFile = std::string( qnnConfig.modelPath ) + "/program.bin";
    uint64_t bufferSize{ 0 };
    qnn::tools::datautil::StatusCode status{ qnn::tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = qnn::tools::datautil::getFileSize( modelFile );
    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize] );
    qnn::tools::datautil::readBinaryFromFile(
            modelFile, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = bufferSize;

    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif