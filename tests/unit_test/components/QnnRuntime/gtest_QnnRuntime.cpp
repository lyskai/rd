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

    qnnConfig.modelPath = "/var/opt/qride/data/bev4d";
    qnnConfig.backendId = QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP;
    qnnConfig.backendCoreId = 0;

    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    std::vector<RideHal_TensorProps_t> inputInfos;
    // get
    // numDims, type, dimensions, (size)
    ret = qnnRuntime.GetInputInfos( inputInfos );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t inputs[inputInfos.size()];
    for ( int i = 0; i < inputInfos.size(); ++i )
    {
        const auto ret = inputs[i].Allocate( &inputInfos[i] );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    std::vector<RideHal_TensorProps_t> outputInfos;
    ret = qnnRuntime.GetOutputInfos( outputInfos );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    RideHal_SharedBuffer_t outputs[outputInfos.size()];
    for ( int i = 0; i < outputInfos.size(); ++i )
    {
        const auto ret = outputs[i].Allocate( &outputInfos[i] );
        ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
    }

    /// Qnn_ClientBuffer_t: data(), size,
    /// GetMemHandleHTP: offset, sharedBuffer.buffer.pData, sharedBuffer.size, sharedBuffer.handle
    /// graphExecute: data()
    ret = qnnRuntime.Execute( inputs, inputInfos.size(), outputs, outputInfos.size() );
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