// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/component/C2D.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align


TEST( C2D, SANITY_C2D_ConvertUYVYtoRGB )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_UYVY;
    C2DConfig.inputConfigs->inputResolution.width = 3840;
    C2DConfig.inputConfigs->inputResolution.height = 2160;
    C2DConfig.inputConfigs->ROI.topX = 100;
    C2DConfig.inputConfigs->ROI.topY = 100;
    C2DConfig.inputConfigs->ROI.width = 600;
    C2DConfig.inputConfigs->ROI.height = 600;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    C2DConfig.outputResolution.width = 600;
    C2DConfig.outputResolution.height = 600;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertUYVYtoBGR )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_UYVY;
    C2DConfig.inputConfigs->inputResolution.width = 2500;
    C2DConfig.inputConfigs->inputResolution.height = 3000;
    C2DConfig.inputConfigs->ROI.topX = 200;
    C2DConfig.inputConfigs->ROI.topY = 350;
    C2DConfig.inputConfigs->ROI.width = 900;
    C2DConfig.inputConfigs->ROI.height = 900;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_BGR888;
    C2DConfig.outputResolution.width = 900;
    C2DConfig.outputResolution.height = 900;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertRGBtoUYVY )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    C2DConfig.inputConfigs->inputResolution.width = 3840;
    C2DConfig.inputConfigs->inputResolution.height = 2160;
    C2DConfig.inputConfigs->ROI.topX = 320;
    C2DConfig.inputConfigs->ROI.topY = 200;
    C2DConfig.inputConfigs->ROI.width = 1080;
    C2DConfig.inputConfigs->ROI.height = 720;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_UYVY;
    C2DConfig.outputResolution.width = 1080;
    C2DConfig.outputResolution.height = 720;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertBGRtoUYVY )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_BGR888;
    C2DConfig.inputConfigs->inputResolution.width = 3000;
    C2DConfig.inputConfigs->inputResolution.height = 3000;
    C2DConfig.inputConfigs->ROI.topX = 200;
    C2DConfig.inputConfigs->ROI.topY = 350;
    C2DConfig.inputConfigs->ROI.width = 1080;
    C2DConfig.inputConfigs->ROI.height = 720;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_UYVY;
    C2DConfig.outputResolution.width = 1080;
    C2DConfig.outputResolution.height = 720;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertNV12toRGB )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    C2DConfig.inputConfigs->inputResolution.width = 3840;
    C2DConfig.inputConfigs->inputResolution.height = 2160;
    C2DConfig.inputConfigs->ROI.topX = 200;
    C2DConfig.inputConfigs->ROI.topY = 150;
    C2DConfig.inputConfigs->ROI.width = 2000;
    C2DConfig.inputConfigs->ROI.height = 1000;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    C2DConfig.outputResolution.width = 2000;
    C2DConfig.outputResolution.height = 1000;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertNV12toBGR )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_NV12;
    C2DConfig.inputConfigs->inputResolution.width = 3200;
    C2DConfig.inputConfigs->inputResolution.height = 2000;
    C2DConfig.inputConfigs->ROI.topX = 400;
    C2DConfig.inputConfigs->ROI.topY = 650;
    C2DConfig.inputConfigs->ROI.width = 1000;
    C2DConfig.inputConfigs->ROI.height = 1000;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_BGR888;
    C2DConfig.outputResolution.width = 1000;
    C2DConfig.outputResolution.height = 1000;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertP010toRGB )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_P010;
    C2DConfig.inputConfigs->inputResolution.width = 3840;
    C2DConfig.inputConfigs->inputResolution.height = 2160;
    C2DConfig.inputConfigs->ROI.topX = 300;
    C2DConfig.inputConfigs->ROI.topY = 450;
    C2DConfig.inputConfigs->ROI.width = 800;
    C2DConfig.inputConfigs->ROI.height = 800;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_RGB888;
    C2DConfig.outputResolution.width = 800;
    C2DConfig.outputResolution.height = 800;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

TEST( C2D, SANITY_C2D_ConvertP010toBGR )
{
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";

    C2DConfig.batchSize = 1;
    C2DConfig.inputConfigs->inputFormat = RIDE_HAL_IMAGE_FORMAT_P010;
    C2DConfig.inputConfigs->inputResolution.width = 3840;
    C2DConfig.inputConfigs->inputResolution.height = 2160;
    C2DConfig.inputConfigs->ROI.topX = 200;
    C2DConfig.inputConfigs->ROI.topY = 100;
    C2DConfig.inputConfigs->ROI.width = 1080;
    C2DConfig.inputConfigs->ROI.height = 720;
    C2DConfig.outputFormat = RIDE_HAL_IMAGE_FORMAT_BGR888;
    C2DConfig.outputResolution.width = 1080;
    C2DConfig.outputResolution.height = 720;

    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = C2DObj.Init( pName, pC2DConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = C2DObj.Deinit();
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

