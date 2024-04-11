// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/QnnRuntime.hpp"

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
    qnnConfig.backendId = QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP;
    qnnConfig.backendCoreId = 0;
    QnnRuntime_UdoPackage_t udoPackage;
    // udoPackage.udoLibPath = "libQnnAutoAiswOpPackage.so";
    // udoPackage.interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    // qnnConfig.udoPackages.push_back( udoPackage );

    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );


    uint32_t inputNum = 0;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfos( nullptr, &inputNum );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfo_t inputInfos[inputNum];
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfos( inputInfos, &inputNum );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        const auto ret = inputs[i].Allocate( &inputInfos[i].properties );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    uint32_t outputNum = 0;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfos( nullptr, &outputNum );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfo_t outputInfos[outputNum];
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfos( outputInfos, &outputNum );
    }
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        const auto ret = outputs[i].Allocate( &outputInfos[i].properties );
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