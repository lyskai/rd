// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "ridehal/component/PostCenterPoint.hpp"
#include "ridehal/component/Voxelization.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace ridehal::common;
using namespace ridehal::component;

static Voxelization_Config_t plrPreConfig0 = {
        RIDEHAL_PROCESSOR_HTP0,
        0.16,
        0.16,
        4.0, /* pillar size: x, y, z */
        0.0,
        -39.68,
        -3.0, /* min Range, x, y, z */
        69.12,
        39.68,
        1.0,    /* max Range, x, y, z */
        300000, /* maxNumInPts */
        4,      /* numInFeatureDim*/
        12000,  /* maxNumPlrs */
        32,     /* maxNumPtsPerPlr */
        10,     /* numOutFeatureDim */
};

static Voxelization_Config_t plrPreConfig1 = {
        RIDEHAL_PROCESSOR_HTP0,
        0.2,
        0.2,
        5.0, /* pillar size: x, y, z */
        -2.0,
        -40.0,
        -2.0, /* min Range, x, y, z */
        150.0,
        40.0,
        3.0,    /* max Range, x, y, z */
        300000, /* maxNumInPts */
        4,      /* numInFeatureDim*/
        40000,  /* maxNumPlrs */
        32,     /* maxNumPtsPerPlr */
        10,     /* numOutFeatureDim */
};

static PostCenterPoint_Config_t plrPostConfig0 = {
        RIDEHAL_PROCESSOR_HTP0,
        0.16,
        0.16, /* pillar size: x, y */
        0.0,
        -39.68, /* min Range, x, y */
        69.12,
        39.68,                                        /* max Range, x, y */
        3,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

static PostCenterPoint_Config_t plrPostConfig1 = {
        RIDEHAL_PROCESSOR_HTP0,
        0.2,
        0.2, /* pillar size: x, y */
        -2.0,
        -40.0, /* min Range, x, y */
        150,
        40,                                           /* max Range, x, y */
        8,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

static void RandomGenPoints( float *pVoxels, uint32_t numPts )
{
    for ( uint32_t i = 0; i < numPts; i++ )
    {
        pVoxels[0] = ( rand() % 10000 ) / 100;
        pVoxels[1] = ( rand() % 10000 ) / 100;
        pVoxels[2] = ( rand() % 300 ) / 100;
        pVoxels[3] = 0.9;
        pVoxels += 4;
    }
}

static void LoadPoints( void *pData, uint32_t size, uint32_t &numPts, const char *pcdFile )
{
    FILE *pFile = fopen( pcdFile, "rb" );
    ASSERT_NE( nullptr, pFile );

    fseek( pFile, 0, SEEK_END );
    int length = ftell( pFile );
    ASSERT_LT( length, size );
    numPts = length / 16;
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, numPts * 16, pFile );
    ASSERT_EQ( r, length );
    printf( "load %u points from %s\n", numPts, pcdFile );
    fclose( pFile );

    ASSERT_NE( 0, numPts );
}

static void LoadRaw( void *pData, uint32_t length, const char *rawFile )
{
    printf( "load raw from %s\n", rawFile );
    FILE *pFile = fopen( rawFile, "rb" );
    ASSERT_NE( nullptr, pFile );
    fseek( pFile, 0, SEEK_END );
    int size = ftell( pFile );
    ASSERT_EQ( size, length );
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, length, pFile );
    ASSERT_EQ( r, length );
    fclose( pFile );
}

static void SaveRaw( std::string path, void *pData, size_t size )
{
    FILE *pFile = fopen( path.c_str(), "wb" );
    if ( nullptr != pFile )
    {
        fwrite( pData, 1, size, pFile );
        fclose( pFile );
        printf( "save raw %s\n", path.c_str() );
    }
}


static void SANITY_Voxelization( RideHal_ProcessorType_e processor, Voxelization_Config_t &cfg,
                                 const char *pcdFile = nullptr, bool bDumpOutput = false )
{
    Voxelization_Config_t config = cfg;
    config.processor = processor;
    Voxelization plrPre;
    RideHalError_e ret;

    RideHal_TensorProps_t inPtsTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumInPts, config.numInFeatureDim, 0 },
            2,
    };
    RideHal_TensorProps_t outPlrsTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
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

    uint32_t numPts = 0;
    if ( nullptr == pcdFile )
    {
        numPts = ( config.maxNumInPts / 3 ) + rand() % ( 2 * config.maxNumInPts / 3 );
        RandomGenPoints( (float *) inPts.data(), numPts );
    }
    else
    {
        LoadPoints( inPts.data(), inPts.size, numPts, pcdFile );
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

    ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( bDumpOutput )
    {
        SaveRaw( "/tmp/coords.raw", outPlrs.data(), outPlrs.size );
        SaveRaw( "/tmp/features.raw", outFeature.data(), outFeature.size );
    }

    ret = plrPre.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPre.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( FadasPlr, SANITY_VoxelizationCPU )
{
    SANITY_Voxelization( RIDEHAL_PROCESSOR_CPU, plrPreConfig0 );
    SANITY_Voxelization( RIDEHAL_PROCESSOR_CPU, plrPreConfig1 );

    SANITY_Voxelization( RIDEHAL_PROCESSOR_CPU, plrPreConfig0, "data/test/plr/pointcloud.bin" );
    SANITY_Voxelization( RIDEHAL_PROCESSOR_CPU, plrPreConfig1, "data/test/plr/pointcloud.bin" );
}

TEST( FadasPlr, SANITY_VoxelizationDSP )
{
    SANITY_Voxelization( RIDEHAL_PROCESSOR_HTP0, plrPreConfig0 );
    SANITY_Voxelization( RIDEHAL_PROCESSOR_HTP0, plrPreConfig1 );


    SANITY_Voxelization( RIDEHAL_PROCESSOR_HTP0, plrPreConfig0, "data/test/plr/pointcloud.bin" );
    SANITY_Voxelization( RIDEHAL_PROCESSOR_HTP0, plrPreConfig1, "data/test/plr/pointcloud.bin" );
}


void SANITY_PostCenterPoint( RideHal_ProcessorType_e processor, PostCenterPoint_Config_t &cfg,
                             const char *pcdFile, const char *hmFile, const char *xyFile,
                             const char *zFile, const char *sizeFile, const char *thetaFile,
                             bool bDumpOutput = false )
{
    PostCenterPoint_Config_t config = cfg;
    PostCenterPoint plrPost;
    config.processor = processor;
    RideHalError_e ret;

    uint32_t numCellsX =
            ( uint32_t )( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
    uint32_t numCellsY =
            ( uint32_t )( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

    uint32_t width = numCellsX / config.stride;
    uint32_t height = numCellsY / config.stride;
    RideHal_TensorProps_t inPtsTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumInPts, config.numInFeatureDim, 0 },
            2,
    };
    RideHal_TensorProps_t hmTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, config.numClass },
            4,
    };
    RideHal_TensorProps_t xyTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 2 },
            4,
    };
    RideHal_TensorProps_t zTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 1 },
            4,
    };
    RideHal_TensorProps_t sizeTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 3 },
            4,
    };
    RideHal_TensorProps_t thetaTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 2 },
            4,
    };
    RideHal_TensorProps_t detTsProp = {
            RIDEHAL_TENSOR_TYPE_FLOAT_32,
            { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM },
            2,
    };

    RideHal_SharedBuffer_t inPts;
    RideHal_SharedBuffer_t hm;
    RideHal_SharedBuffer_t xy;
    RideHal_SharedBuffer_t z;
    RideHal_SharedBuffer_t size;
    RideHal_SharedBuffer_t theta;
    RideHal_SharedBuffer_t det;

    ret = inPts.Allocate( &inPtsTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    uint32_t numPts = 0;
    LoadPoints( inPts.data(), inPts.size, numPts, pcdFile );
    inPts.tensorProps.dims[0] = numPts;

    ret = hm.Allocate( &hmTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    LoadRaw( hm.data(), hm.size, hmFile );

    ret = xy.Allocate( &xyTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    LoadRaw( xy.data(), xy.size, xyFile );

    ret = z.Allocate( &zTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    LoadRaw( z.data(), z.size, zFile );

    ret = size.Allocate( &sizeTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    LoadRaw( size.data(), size.size, sizeFile );

    ret = theta.Allocate( &thetaTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    LoadRaw( theta.data(), theta.size, thetaFile );

    ret = det.Allocate( &detTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPost.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    det.tensorProps.dims[0] = config.maxNumDetOut;
    ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &det );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
    for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
    {
        printf( "[%d] class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
                "theta=%.3f, mean=[%.3f %.3f %.3f %.3f]x%u\n",
                i, pObj->label, pObj->score, pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                pObj->height, pObj->theta, pObj->meanPtX, pObj->meanPtY, pObj->meanPtZ,
                pObj->meanIntensity, pObj->numPts );
        pObj++;
    }

    if ( bDumpOutput )
    {
        /* dump in a python list format for visualization with vis3d.py */
        PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
        for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
        {
            printf( "[%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %d],\n", pObj->x, pObj->y,
                    pObj->z, pObj->length, pObj->width, pObj->height, pObj->theta, pObj->score,
                    pObj->label );
            pObj++;
        }
    }

    ret = plrPost.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = plrPost.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

extern int PtPlr_PostProc( void );
TEST( FadasPlr, SANITY_PostCenterPointCPU )
{
    SANITY_PostCenterPoint( RIDEHAL_PROCESSOR_CPU, plrPostConfig0, "data/test/plr/CFG0/000008.bin",
                            "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
    SANITY_PostCenterPoint( RIDEHAL_PROCESSOR_CPU, plrPostConfig1, "data/test/plr/pointcloud.bin",
                            "data/test/plr/hm-activation-0-inf-1.bin",
                            "data/test/plr/reg-activation-0-inf-1.bin",
                            "data/test/plr/height-activation-0-inf-1.bin",
                            "data/test/plr/dim-activation-0-inf-1.bin",
                            "data/test/plr/rot-activation-0-inf-1.bin" );
}

TEST( FadasPlr, SANITY_PostCenterPointDSP )
{
    SANITY_PostCenterPoint( RIDEHAL_PROCESSOR_HTP0, plrPostConfig0, "data/test/plr/CFG0/000008.bin",
                            "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
    SANITY_PostCenterPoint( RIDEHAL_PROCESSOR_HTP0, plrPostConfig1, "data/test/plr/pointcloud.bin",
                            "data/test/plr/hm-activation-0-inf-1.bin",
                            "data/test/plr/reg-activation-0-inf-1.bin",
                            "data/test/plr/height-activation-0-inf-1.bin",
                            "data/test/plr/dim-activation-0-inf-1.bin",
                            "data/test/plr/rot-activation-0-inf-1.bin", true );
}


static void FadasPlr_E2E_PreProc( RideHal_ProcessorType_e processor, Voxelization_Config_t &cfg )
{
    int exist = access( "/tmp/pointcloud.bin", F_OK );
    if ( 0 == exist )
    {
        SANITY_Voxelization( processor, cfg, "/tmp/pointcloud.bin", true );
    }
    else
    {
        printf( "skip E2E_PreProc\n" );
    }
}

static void FadasPlr_E2E_PostProc( RideHal_ProcessorType_e processor,
                                   PostCenterPoint_Config_t &cfg )
{
    int exist = access( "/tmp/pointcloud.bin", F_OK );
    exist |= access( "/tmp/hm.raw", F_OK );
    if ( 0 == exist )
    {
        SANITY_PostCenterPoint( processor, cfg, "/tmp/pointcloud.bin", "/tmp/hm.raw",
                                "/tmp/center.raw", "/tmp/center_z.raw", "/tmp/dim_exp.raw",
                                "/tmp/rot.raw", true );
    }
    else
    {
        printf( "skip E2E_PostProc\n" );
    }
}

TEST( FadasPlr, E2E_CFG0_PreProcCPU )
{
    FadasPlr_E2E_PreProc( RIDEHAL_PROCESSOR_CPU, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PostProcCPU )
{
    FadasPlr_E2E_PostProc( RIDEHAL_PROCESSOR_CPU, plrPostConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PreProcDSP )
{
    FadasPlr_E2E_PreProc( RIDEHAL_PROCESSOR_HTP0, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PostProcDSP )
{
    FadasPlr_E2E_PostProc( RIDEHAL_PROCESSOR_HTP0, plrPostConfig0 );
}

TEST( FadasPlr, E2E_CFG1_PreProcCPU )
{
    FadasPlr_E2E_PreProc( RIDEHAL_PROCESSOR_CPU, plrPreConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PostProcCPU )
{
    FadasPlr_E2E_PostProc( RIDEHAL_PROCESSOR_CPU, plrPostConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PreProcDSP )
{
    FadasPlr_E2E_PreProc( RIDEHAL_PROCESSOR_HTP0, plrPreConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PostProcDSP )
{
    FadasPlr_E2E_PostProc( RIDEHAL_PROCESSOR_HTP0, plrPostConfig1 );
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif