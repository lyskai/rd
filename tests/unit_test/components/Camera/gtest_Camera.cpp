// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/common/Types.hpp"
#include "ridehal/component/Camera.hpp"
#include "gtest/gtest.h"
#include <stdio.h>
#include <string>
#include <unistd.h>


using namespace ridehal;
using namespace ridehal::common;
using namespace ridehal::component;

#define RUNTIME_SECOND ( 3 )
#define BUFFFER_COUNT ( 5 )

static const char *pDumpPath = "/tmp/camera_frame.bin";

static std::FILE *g_Dumpfile = nullptr;

static int DumpFrame( CameraFrame_t *pFrame, const char *path )
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

static void FrameCallBack( CameraFrame_t *pFrame, void *pPrivData )
{
    RideHalError_e ret;

    Camera *pCamera = (Camera *) pPrivData;

#ifdef DUMPFRAME
    uint32_t writeBytes = DumpFrame( pFrame, pDumpPath );
#endif
    if ( RIDEHAL_COMPONENT_STATE_RUNNING == pCamera->GetState() )
    {
        ret = pCamera->ReleaseFrame( pFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
}

static void FrameCallBack_RequestMode( CameraFrame_t *pFrame, void *pPrivData )
{
    RideHalError_e ret;

    Camera *pCamera = (Camera *) pPrivData;

#ifdef DUMPFRAME
    uint32_t writeBytes = DumpFrame( pFrame, pDumpPath );
#endif

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == pCamera->GetState() )
    {
        ret = pCamera->RequestFrame( pFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
}

static void EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    RIDEHAL_LOG_ERROR( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
}

static uint32_t GetIspUserCase( uint32_t inputId )
{
    uint32_t ispCase = 3;

    std::string envName = "RIDEHAL_CAM" + std::to_string( inputId ) + "_ISP_USE_CASE";
    char *envValue = getenv( envName.c_str() );
    if ( nullptr == envValue )
    {
        printf( "no env %s\n", envName.c_str() );
        envName = "RIDEHAL_CAM_ISP_USE_CASE";
        envValue = getenv( envName.c_str() );
    }

    if ( nullptr != envValue )
    {
        ispCase = (uint32_t) std::stoi( envValue );
        printf( "set ISP_USE_CASE from %s=%s\n", envName.c_str(), envValue );
    }
    else
    {
        printf( "no env %s\n", envName.c_str() );
    }

    printf( "set ISP_USE_CASE %u for camera input_id %u\n", ispCase, inputId );

    return ispCase;
}

TEST( Camera, Query_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;
    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    printf( "Number of camera connected: %d\n", camInputs.numInputs );

    delete pCamera;
}

TEST( Camera, SANITY_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    for ( uint32_t i = 0; i < camInputs.numInputs; i++ )
    {
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[i];
        printf( "testing camera input_id %u, resolution %ux%u\n",
                camInputs.pCameraInputs[i].inputId, pCamInputModes->pModes[0].sources[0].width,
                pCamInputModes->pModes[0].sources[0].height );

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[i].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

TEST( Camera, SetBuffer_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    for ( uint32_t i = 0; i < camInputs.numInputs; i++ )
    {
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[i];
        printf( "testing camera input_id %u, resolution %ux%u\n",
                camInputs.pCameraInputs[i].inputId, pCamInputModes->pModes[0].sources[0].width,
                pCamInputModes->pModes[0].sources[0].height );

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = false;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[i].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.streamId = 0;
        RideHal_SharedBuffer_t *pSharedBuffer = new RideHal_SharedBuffer_t[BUFFFER_COUNT];

        for ( int i = 0; i < BUFFFER_COUNT; i++ )
        {
            ret = pSharedBuffer[i].Allocate( camConfig.width, camConfig.height, camConfig.format );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        for ( int i = 0; i < BUFFFER_COUNT; i++ )
        {
            ret = pSharedBuffer[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    delete pCamera;
}

TEST( Camera, PauseResume_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    for ( uint32_t i = 0; i < camInputs.numInputs; i++ )
    {
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[i];
        printf( "testing camera input_id %u, resolution %ux%u\n",
                camInputs.pCameraInputs[i].inputId, pCamInputModes->pModes[0].sources[0].width,
                pCamInputModes->pModes[0].sources[0].height );

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[i].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Pause();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        sleep( RUNTIME_SECOND );

        ret = pCamera->Resume();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

TEST( Camera, RequestMode_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;
    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    for ( uint32_t i = 0; i < camInputs.numInputs; i++ )
    {
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[i];
        printf( "testing camera input_id %u, resolution %ux%u\n",
                camInputs.pCameraInputs[i].inputId, pCamInputModes->pModes[0].sources[0].width,
                pCamInputModes->pModes[0].sources[0].height );

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = true;
        camConfig.inputId = camInputs.pCameraInputs[i].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

TEST( Camera, Coverage_QcarCam )
{
    RideHalError_e ret;
    char componentName[20] = "Camera";

    /* Negative - config is nullptr */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Init( componentName, nullptr, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
        delete pCamera;
    }


    /* Negative - camConfig.bufCnt is 0 */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = 0;
        camConfig.streamId = 0;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - start in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - stop in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - deinit in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - pause in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Pause();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - resume in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Resume();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - GetInputsInfo with null input */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    /* Negative - UYVY */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = 4;
        camConfig.streamId = 0;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_UYVY;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - SetBuffer with nullptr */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = false;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.streamId = 0;
        RideHal_SharedBuffer_t *pSharedBuffer = nullptr;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        delete pCamera;
    }

    /* Negative - SetBuffer in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        RideHal_SharedBuffer_t *pSharedBuffer = new RideHal_SharedBuffer_t[BUFFFER_COUNT];
        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        delete pCamera;
    }

    /* Negative - RequestFrame with nullptr input */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = true;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RequestFrame( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - RequestFrame in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = true;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->RequestFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - unsupport color format */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = 4;
        camConfig.streamId = 0;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_RGB888;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - Init twice */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - ReleaseFrame with nullptr input */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->ReleaseFrame( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - ReleaseFrame in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        Camera_Config_t camConfig = { 0 };
        camConfig.bAllocator = true;
        camConfig.bRequestMode = false;
        camConfig.inputId = camInputs.pCameraInputs[0].inputId;
        camConfig.ispUserCase = GetIspUserCase( camConfig.inputId );
        camConfig.width = pCamInputModes->pModes[0].sources[0].width;
        camConfig.height = pCamInputModes->pModes[0].sources[0].height;
        camConfig.bufCnt = BUFFFER_COUNT;
        camConfig.streamId = 0;
        camConfig.opMode = QCARCAM_OPMODE_OFFLINE_ISP;
        camConfig.format = RIDEHAL_IMAGE_FORMAT_NV12;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->ReleaseFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* 2 camera instances */
    {
        Camera *pCamera = new Camera;
        Camera *pCamera1 = new Camera;
        delete pCamera;
        delete pCamera1;
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
