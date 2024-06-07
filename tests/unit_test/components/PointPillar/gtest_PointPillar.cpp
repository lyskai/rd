// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "ridehal/component/PointPillarPreProc.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>

using namespace ridehal::common;
using namespace ridehal::component;

void SANITY_PointPillarPreProc( RideHal_ProcessorType_e processor )
{
    PointPillarPreProc_Config_t config = {
            processor, 0.16,   0.16, 4.0, /* pillar size: x, y, z */
            0.0,       -39.68, -3.0,      /* min Range, x, y, z */
            69.12,     39.68,  1.0,       /* max Range, x, y, z */
            300000,                       /* maxNumInPts */
            4,                            /* numInFeatureDim*/
            12000,                        /* maxNumPlrs */
            32,                           /* maxNumPtsPerPlr */
            10,                           /* numOutFeatureDim */
    };

    PointPillarPreProc plrPre;
    RideHalError_e ret;

    RideHal_TensorProps_t inPtsTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumPtsIn, config.numInFeatureDim, 0 },
            2,
    };
    RideHal_TensorProps_t outPlrsTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumPlrs, config.numInFeatureDim, 0 },
            2,
    };
    RideHal_TensorProps_t outFeatureTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumPlrs, config.maxNumPtsPerPlr, config.numOutFeatureDim, 0 },
            3,
    };

    RideHal_SharedBuffer_t inPts;
    RideHal_SharedBuffer_t outPlrs;
    RideHal_SharedBuffer_t outFeature;

    ret = inPts.Allocate( &inPtsTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    float *pVoxels = (float *) inPts.data();
    uint32_t numPts = ( config.maxNumPtsIn / 3 ) + rand() % ( 2 * config.maxNumPtsIn / 3 );
    for ( uint32_t i = 0; i < numPts; i++ )
    {
        pVoxels[0] = ( rand() % 10000 ) / 100;
        pVoxels[1] = ( rand() % 10000 ) / 100;
        pVoxels[2] = ( rand() % 300 ) / 100;
        pVoxels[3] = 0.9;
        pVoxels += 4;
    }
    inPts.tensorProps.dims[0] = numPts;

    ret = outPlrs.Allocate( &outPlrsTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = outFeature.Allocate( &outFeatureTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPre.Init( "PLRPRE0", &config, LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPre.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( RIDEHAL_PROCESSOR_CPU != processor )
    {
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    else
    {
        printf( "skip for CPU as execute core dump issue\n" );
    }

    ret = plrPre.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPre.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( FadasPlr, SANITY_PointPillarPreProcCPU )
{
    SANITY_PointPillarPreProc( RIDEHAL_PROCESSOR_CPU );
}

TEST( FadasPlr, SANITY_PointPillarPreProcDSP )
{
    SANITY_PointPillarPreProc( RIDEHAL_PROCESSOR_HTP0 );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif