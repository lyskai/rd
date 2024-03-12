// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "gtest/gtest.h"
#include <stdio.h>

#include "ridehal/common/Types.hpp"
#include "ridehal/component/Camera.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

#define RUNTIME_SECOND ( 5 )

const char *pDumpPath = "/tmp/camera_frame.bin";

using namespace ridehal;

std::FILE *g_Dumpfile = nullptr;

int DumpFrame( Camera_Frame_t *pFrame, const char *path )
{
    int ret = 0;

    if ( nullptr == g_Dumpfile )
    {
        g_Dumpfile = std::fopen( path, "wb+" );
    }

    if ( nullptr != g_Dumpfile )
    {
        int frameSize = pFrame->sharedBuffer.size;
        void *buffer = pFrame->sharedBuffer.data();

        ret = std::fwrite( buffer, frameSize, 1, g_Dumpfile );
    }

    return ret;
}

void FrameCallBack( Camera_Frame_t *pFrame, void *pPrivData )
{
    RideHalError_e ret;

    printf( "FrameCallBack Index: %d stream id: %d, pPrivData: %p\n", pFrame->frameIndex,
            pFrame->streamId, pPrivData );
    Camera *pCamera = (Camera *) pPrivData;

#ifdef DUMPFRAME
    uint32_t writeBytes = DumpFrame( pFrame, pDumpPath );
#endif
    ret = pCamera->ReleaseFrame( pFrame->streamId, pFrame->frameIndex );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
}

void EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
}

TEST( Camera, SANITY_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    char componentName[20] = "Camera";
    Camera_Config_t camConfig;
    camConfig.isAllocator = true;
    camConfig.requestMode = false;
    camConfig.inputId = 0;
    camConfig.width = 1928;
    camConfig.height = 1208;
    camConfig.bufCnt = 5;
    camConfig.format = RIDE_HAL_IMAGE_FORMAT_NV12;

    ret = pCamera->Init( componentName, camConfig );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );


    ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    ret = pCamera->Start();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

    // sanity test to run few seconds and then stop
    sleep( RUNTIME_SECOND );

    ret = pCamera->Stop();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );

#if 1   // deinit crash
    ret = pCamera->Deinit();
    ASSERT_EQ( RIDE_HAL_ERROR_NONE, ret );
#endif

    delete pCamera;
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
