/***************************************************************************//**
@brief
   Program to run each feature of FastADAS once.

@internal
   Copyright (c) 2020 Qualcomm Technologies, Inc.
   All Rights Reserved.
   Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#include "AEEStdErr.h"
#include <stdio.h>
#include <string.h>
#include <fadas.h>
#include "FadasIface.h"
#include "HAP_farf.h"
#include "HAP_mem.h"
#include "remote.h"
#include "HAP_perf.h"
#include "HAP_power.h"
#include "qurt.h"

#define ALIGN_128(x) ((((x) + 127)>>7)<<7)

typedef struct
{
    qurt_mutex_t mutex;
} dspContext_t;

// FIXME: Maybe need better way to map those enums
const FadasRemapPipeline_e g_MapImageConversion[FADAS_REMAP_PIPELINE_MAX_NSP] =
{
        FADAS_REMAP_PIPELINE_1C8,
        FADAS_REMAP_PIPELINE_1C8_ROISCALE,
        FADAS_REMAP_PIPELINE_3C888,
        FADAS_REMAP_PIPELINE_3C888_ROISCALE,
        FADAS_REMAP_PIPELINE_YUV888_TO_RGB888,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888,
        FADAS_REMAP_PIPELINE_VYUY_TO_RGB888,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMI8,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8,
};


const FadasCvtYUVPipeline_e g_MapCvtYUVPipeline[FADAS_CVTYUV_PIPELINE_COUNT_NSP] =
{
    FADAS_CVTYUV_PIPELINE_DownscaleUYVYAndRGB888,
    FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2AndRGB888,
    FADAS_CVTYUV_PIPELINE_DownscaleYUV888AndRGB888,
    FADAS_CVTYUV_PIPELINE_DownscaleYUV888By2AndRGB888,
    FADAS_CVTYUV_PIPELINE_DownscaleY8UV8By2,
    FADAS_CVTYUV_PIPELINE_DownscaleY10UV10By2,
};

const FadasImageFormat_e g_MapImageFormat[FADAS_IMAGE_FORMAT_COUNT_NSP] =
{
    FADAS_IMAGE_FORMAT_UNKNOWN,
    FADAS_IMAGE_FORMAT_Y,
    FADAS_IMAGE_FORMAT_Y12,
    FADAS_IMAGE_FORMAT_UYVY,
    FADAS_IMAGE_FORMAT_UYVY10,
    FADAS_IMAGE_FORMAT_VYUY,
    FADAS_IMAGE_FORMAT_YUV888,
    FADAS_IMAGE_FORMAT_RGB888,
    FADAS_IMAGE_FORMAT_Y10UV10,
    FADAS_IMAGE_FORMAT_Y8UV8,
};

const FadasBufType_e g_MapBufType[FADAS_BUF_TYPE_MAX_NSP] =
{
    FADAS_BUF_TYPE_IN,
    FADAS_BUF_TYPE_OUT,
    FADAS_BUF_TYPE_INOUT,
};

AEEResult setClocks( remote_handle64 handle )
{
    HAP_power_request_t request;
    memset( &request, 0, sizeof( HAP_power_request_t ) );
    request.type = HAP_power_set_apptype;
    request.apptype = HAP_POWER_COMPUTE_CLIENT_CLASS;

    void *ctx = (void *)( handle );
    int retval = HAP_power_set( ctx, &request );
    if ( retval )
        return AEE_EFAILED;

    memset( &request, 0, sizeof( HAP_power_request_t ) );
    request.type = HAP_power_set_DCVS_v2;

    request.dcvs_v2.dcvs_enable = TRUE;
    request.dcvs_v2.dcvs_params.target_corner = (HAP_dcvs_voltage_corner_t)7;

    request.dcvs_v2.dcvs_params.min_corner = request.dcvs_v2.dcvs_params.target_corner;
    request.dcvs_v2.dcvs_params.max_corner = request.dcvs_v2.dcvs_params.target_corner;

    request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
    request.dcvs_v2.set_dcvs_params = TRUE;
    request.dcvs_v2.set_latency = TRUE;
    request.dcvs_v2.latency = 100;
    retval = HAP_power_set( ctx, &request );
    if ( retval )
        return AEE_EFAILED;

    memset( &request, 0, sizeof( HAP_power_request_t ) );
    request.type = HAP_power_set_HVX;
    request.hvx.power_up = TRUE;
    retval = HAP_power_set( ctx, &request );
    if ( retval )
        return AEE_EFAILED;

    return AEE_SUCCESS;
}

void *FadasIface_GetBufPtr( int32_t bufFd )
{
    void *bufPtr = nullptr;

    if ( 0 < bufFd )
    {
        HAP_mmap_get( bufFd, (void **)&bufPtr, NULL );
        HAP_mmap_put( bufFd );
    }

    return bufPtr;
}

AEEResult FadasIface_open( const char *uri, remote_handle64 *handle )
{
    AEEResult status = AEE_EFAILED;
    dspContext_t *dspContext = (dspContext_t *)malloc( sizeof( dspContext_t ) );
    *handle = (remote_handle64)dspContext;
    if ( *handle != 0 )
    {
        qurt_mutex_init( &dspContext->mutex );
        status = setClocks( *handle );
    }

    return status;
}

AEEResult FadasIface_close( remote_handle64 handle )
{
    dspContext_t *dspContext = (dspContext_t *)handle;
    if ( dspContext != NULL )
    {
        free( dspContext );
    }

    HAP_power_destroy( NULL );

    return AEE_SUCCESS;
}



AEEResult FadasIface_FadasInit( remote_handle64 handle, int32_t *status )
{
    *status = static_cast<int32_t>( FadasInit( nullptr ) );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasVersion( remote_handle64 handle, uint8_t *ver_int, int ver_intLen )
{

    strlcpy( reinterpret_cast<char*>( ver_int ), FadasVersion(), ver_intLen );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasDeInit( remote_handle64 handle )
{
    FadasDeInit();

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_UYVYtoRGB(
        remote_handle64 handle,
        const uint8_t* src,
        int srcLen,
        const FadasIface_FadasImgProps_t* srcProps,
        uint8_t* dst,
        int dstLen,
        uint32_t dstStride )
{
    FadasImgProps_t srcImgProps;
    srcImgProps.width = srcProps->width;
    srcImgProps.height = srcProps->height;
    srcImgProps.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImgProps.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImgProps.numPlanes = srcProps->numPlanes;

    if( FadasCvtYUV_UYVYtoRGB( src, srcImgProps, dst, dstStride ) != FADAS_ERROR_NONE )
    {
        return AEE_EFAILED;
    }

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_UYVYtoYUV(
        remote_handle64 handle,
        const uint8_t* src,
        int srcLen,
        const FadasIface_FadasImgProps_t* srcProps,
        uint8_t* dst,
        int dstLen,
        uint32_t dstStride)
{
    FadasImgProps_t srcImgProps;
    srcImgProps.width = srcProps->width;
    srcImgProps.height = srcProps->height;
    srcImgProps.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImgProps.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImgProps.numPlanes = srcProps->numPlanes;

    if( FadasCvtYUV_UYVYtoYUV( src, srcImgProps, dst, dstStride ) != FADAS_ERROR_NONE )
    {
        return AEE_EFAILED;
    }

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_UYVYtoNV12(
        remote_handle64 handle,
        int32_t srcFd,
        uint32_t srcLen,
        uint32_t offset,
        const FadasIface_FadasImgProps_t* srcProps,
        int32_t dstFd,
        uint32_t dstLen,
        uint32_t dstStride)
{
    dspContext_t *dspContext = (dspContext_t *)handle;
    const uint8_t *src = (uint8_t *)FadasIface_GetBufPtr( srcFd );
    uint8_t *dst = (uint8_t *)FadasIface_GetBufPtr( dstFd );
    if ( ( nullptr == src ) || ( nullptr == dst ) )
    {
        return AEE_EFAILED;
    }

    src += offset;
    FadasError_e retval;
    FadasImgProps_t srcImgProps;
    srcImgProps.width = srcProps->width;
    srcImgProps.height = srcProps->height;
    srcImgProps.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImgProps.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImgProps.numPlanes = srcProps->numPlanes;

    uint8_t* dstY = dst;
    uint32_t dstYStride = dstStride;
    uint8_t* dstUV = dst + dstStride * srcProps->height;
    uint32_t dstUVStride = dstStride;

    qurt_mem_cache_clean( (qurt_addr_t)src, srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
                          QURT_MEM_DCACHE );
    FadasRegBuf( FADAS_BUF_TYPE_OUT, dstUV, dstStride * srcProps->height );
    qurt_mutex_lock( &dspContext->mutex );
    retval = FadasCvtYUV_UYVYtoNV12( src, srcImgProps, dstY, dstYStride, dstUV, dstUVStride );
    qurt_mutex_unlock( &dspContext->mutex );
    FadasDeregBuf( dstUV );
    if( retval != FADAS_ERROR_NONE )
    {
        FARF( ALWAYS, "FAILED:  FadasCvtYUV_UYVYtoNV12" );
        return  AEE_EOFFSET + retval;
    }
    else
    {
        qurt_mem_cache_clean( (qurt_addr_t)dst, dstLen, QURT_MEM_CACHE_FLUSH_ALL,
                                QURT_MEM_DCACHE );
    }
    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_CreateWorkers(
        remote_handle64 handle,
        uint64_t* addr,
        int32_t threads,
        FadasIface_FadasCvtYUVPipeline_e type )
{
    int32_t               pThreadsAffinity[] = {0,1,2,3};
    void* wrkrs = FadasCvtYUV_CreateWorkers( 0, pThreadsAffinity, g_MapCvtYUVPipeline[static_cast<int>( type )] );
    *addr = ( uint64_t )wrkrs;
    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_DestroyWorkers( remote_handle64 handle, uint64_t addr )
{
    void* wrkrs = reinterpret_cast<void*>( addr );
    FadasCvtYUV_DestroyWorkers( wrkrs );
    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasCvtYUV_RunMT(
        remote_handle64 handle,
        uint64 addr,
        uint8_t* src,
        int srcLen,
        uint8_t* dst,
        int dstLen,
        FadasIface_FadasImgProps_t* srcProps,
        FadasIface_FadasImgProps_t *dstProps,
        FadasIface_FadasROI_t *roi )
{
    void* wrkrs = ( void* )addr;
    FadasImage_t srcImg;
    FadasImage_t dstImg;
    FadasROI_t roi_ptr;

    srcImg.plane[0] = src;
    srcImg.props.width = srcProps->width;
    srcImg.props.height = srcProps->height;
    srcImg.props.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImg.props.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImg.props.numPlanes = srcProps->numPlanes;

    dstImg.plane[0] = dst;
    dstImg.props.width = dstProps->width;
    dstImg.props.height = dstProps->height;
    dstImg.props.format = g_MapImageFormat[dstProps->format];
    memcpy( dstImg.props.stride, dstProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    dstImg.props.numPlanes = dstProps->numPlanes;

    roi_ptr.x = roi->x;
    roi_ptr.y = roi->y;
    roi_ptr.width = roi->width;
    roi_ptr.height = roi->height;

    if( FadasCvtYUV_RunMT( wrkrs, &srcImg, &dstImg, &roi_ptr ) != FADAS_ERROR_NONE )
    {
        return AEE_EFAILED;
    }

    dstProps->width = dstImg.props.width;
    dstProps->height = dstImg.props.height;
    memcpy( dstProps->stride, dstImg.props.stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    dstProps->format = static_cast<FadasIface_FadasImageFormat_e>( dstImg.props.format );
    dstProps->numPlanes = dstImg.props.numPlanes;

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateMapFromMap(
        remote_handle64 handle,
        uint64* mapPtr,
        uint32_t camWidth,
        uint32_t camHeight,
        uint32_t mapWidth,
        uint32_t mapHeight,
        const float* mapX,
        int mapXLen,
        const float* mapY,
        int mapYLen,
        uint32_t mapStride,
        FadasIface_FadasRemapPipeline_e imgFormat,
        uint8_t borderConst)
{
    FadasRemapMap* map = FadasRemap_CreateMapFromMap( camWidth, camHeight, mapWidth, mapHeight,
            mapStride, mapX, mapY,  g_MapImageConversion[static_cast<int>( imgFormat )],
            borderConst );

    if( map == nullptr )
    {
        return AEE_EFAILED;
    }

    *mapPtr = reinterpret_cast<uint64>( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateMapFromFisheyeCalib(
        remote_handle64 handle,
        uint64* mapPtr,
        uint32_t camWidth,
        uint32_t camHeight,
        const FadasIface_FadasCameraProps_t* camProps,
        const FadasIface_FadasDistCoeffs_t* distCoeffs,
        FadasIface_FadasRemapPipeline_e imgFormat,
        uint32_t mapWidth,
        uint32_t mapHeight,
        uint8_t borderConst)
{
    FadasCameraProps_t cameraProps;
    cameraProps.focalLengthX = static_cast<float64_t>( camProps->focalLengthX );
    cameraProps.focalLengthY = static_cast<float64_t>( camProps->focalLengthY );
    cameraProps.principalPointX = static_cast<float64_t>( camProps->principalPointX );
    cameraProps.principalPointY = static_cast<float64_t>( camProps->principalPointY );

    FadasDistCoeffs_t dCoeffs;
    dCoeffs.k1 = static_cast<float64_t>( distCoeffs->k1 );
    dCoeffs.k2 = static_cast<float64_t>( distCoeffs->k2 );
    dCoeffs.k3 = static_cast<float64_t>( distCoeffs->k3 );
    dCoeffs.k4 = static_cast<float64_t>( distCoeffs->k4 );

    FadasRemapMap* map = FadasRemap_CreateMapFromFisheyeCalib( camWidth, camHeight, cameraProps,
            dCoeffs,  g_MapImageConversion[static_cast<int32_t>( imgFormat )],  mapWidth, mapHeight,
            borderConst );
    if( map == nullptr )
    {
        return AEE_EFAILED;
    }

    *mapPtr = reinterpret_cast<uint64>( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateMapNoUndistortion(
        remote_handle64 handle,
        uint64* mapPtr,
        uint32_t camWidth,
        uint32_t camHeight,
        uint32_t mapWidth,
        uint32_t mapHeight,
        FadasIface_FadasRemapPipeline_e imgFormat,
        uint8_t borderConst)
{
    FadasRemapMap* map = FadasRemap_CreateMapNoUndistortion( camWidth, camHeight, mapWidth,
            mapHeight, g_MapImageConversion[static_cast<int>( imgFormat )], borderConst );
    if( map == nullptr )
    {
        return AEE_EFAILED;
    }

    *mapPtr = reinterpret_cast<uint64>( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_DestroyMap( remote_handle64 handle, uint64 mapPtr )
{
    FadasRemapMap* map = reinterpret_cast<FadasRemapMap*>( mapPtr );

    FadasRemap_DestroyMap( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateWorkers(
        remote_handle64 handle,
        uint64* worker_ptr,
        uint32_t nThreads,
        FadasIface_FadasRemapPipeline_e imgFormat)
{
    int32_t              pThreadsAffinity[]={0,1,2,3};
    void* worker = FadasRemap_CreateWorkers( nThreads,pThreadsAffinity,
            g_MapImageConversion[static_cast<int>( imgFormat )] );
    if( worker == nullptr )
    {
        return AEE_EFAILED;
    }

    *worker_ptr = reinterpret_cast<uint64>( worker );
    return AEE_SUCCESS;

}

AEEResult FadasIface_FadasRemap_DestroyWorkers( remote_handle64 handle, uint64 worker_ptr )
{
    void* worker = reinterpret_cast<void*>( worker_ptr );
    FadasRemap_DestroyWorkers( worker );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_Run(
        remote_handle64 handle,
        uint64 mapPtr,
        const uint8_t* src,
        int srcLen,
        const FadasIface_FadasImgProps_t* srcProps,
        uint8_t* __restrict dst,
        int dstLen,
        const FadasIface_FadasImgProps_t* dstProps,
        FadasIface_FadasROI_t *dstROI)
{

    FadasError_e retval;
    FadasRemapMap* map = reinterpret_cast<FadasRemapMap*>( mapPtr );

    FadasROI_t roiStruct = {};
    roiStruct.x = dstROI->x;
    roiStruct.y = dstROI->y;
    roiStruct.width = dstROI->width;
    roiStruct.height = dstROI->height;

    FadasImage_t srcImg = {};
    srcImg.plane[0] = const_cast<uint8_t*>( src );
    srcImg.bAllocated = false;
    srcImg.props.width = static_cast<uint32_t>( srcProps->width );
    srcImg.props.height = static_cast<uint32_t>( srcProps->height );
    srcImg.props.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImg.props.stride, srcProps->stride, srcProps->numPlanes * sizeof( uint32_t ) );
    srcImg.props.numPlanes = srcProps->numPlanes;

    FadasImage_t dstImg = {};
    dstImg.plane[0] = dst;
    dstImg.bAllocated = false;
    dstImg.props.width = static_cast<uint32_t>( dstProps->width );
    dstImg.props.height = static_cast<uint32_t>( dstProps->height );
    dstImg.props.format = g_MapImageFormat[dstProps->format];
    memcpy( dstImg.props.stride, dstProps->stride, dstProps->numPlanes * sizeof( uint32_t ) );
    dstImg.props.numPlanes = dstProps->numPlanes;

    // FIXME: Is it the correct usage of those functions??
    FadasRegBuf( FADAS_BUF_TYPE_IN, src, srcLen );
    FadasRegBuf( FADAS_BUF_TYPE_OUT, dst, dstLen );

    retval = FadasRemap_Run( map, &srcImg, &dstImg, &roiStruct );

    FadasDeregBuf( src );
    FadasDeregBuf( dst );

    if( retval != FADAS_ERROR_NONE )
    {
        FARF( ALWAYS, "FAILED:  FadasRemap_Run" );
        return  AEE_EFAILED + retval;
    }

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_RunMT(
        remote_handle64 handle,
        const uint64* workerPtrs, int workerPtrsLen,
        const uint64* mapPtrs, int mapPtrsLen,
        const int32_t* srcFds, int srcFdsLen,
        const uint32_t* offsets, int offsetsLen,
        const FadasIface_FadasImgProps_t* srcProps, int srcPropsLen,
        int32_t dstFd,
        uint32_t dstLen,
        const FadasIface_FadasImgProps_t* dstProps,
        const FadasIface_FadasROI_t* dstROIs, int dstROIsLen,
        const FadasIface_FadasNormlzParams_t* normlz, int normlzLen )
{
    dspContext_t *dspContext = (dspContext_t *)handle;
    const uint8_t *src[srcFdsLen];
    uint8_t *dst = (uint8_t *)FadasIface_GetBufPtr( dstFd );
    if ( nullptr == dst )
    {
        return AEE_EFAILED;
    }

    if ( ( srcFdsLen != offsetsLen ) || ( srcFdsLen != srcPropsLen ) )
    {
        return AEE_EFAILED;
    }

    for ( int i = 0; i < srcFdsLen; i++ )
    {
        src[i] = (uint8_t *)FadasIface_GetBufPtr( srcFds[i] );
        if ( nullptr == src[i] )
        {
            return AEE_EFAILED;
        }
        src[i] += offsets[i];
    }

    FadasError_e retval;

    FadasImage_t srcImg = {};
    FadasImage_t dstImg = {};
    dstImg.bAllocated = false;
    dstImg.props.width = static_cast<uint32_t>( dstProps->width );
    dstImg.props.height = static_cast<uint32_t>( dstProps->height );
    dstImg.props.format = g_MapImageFormat[dstProps->format];
    memcpy( dstImg.props.stride, dstProps->stride, dstProps->numPlanes * sizeof( uint32_t ) );
    dstImg.props.numPlanes = dstProps->numPlanes;

     for ( int i = 0; i < srcFdsLen; i++ )
     {
        srcImg.bAllocated = false;
        srcImg.props.width = static_cast<uint32_t>( srcProps[i].width );
        srcImg.props.height = static_cast<uint32_t>( srcProps[i].height );
        srcImg.props.format = g_MapImageFormat[srcProps[i].format];
        memcpy( srcImg.props.stride, srcProps[i].stride, srcProps[i].numPlanes * sizeof( uint32_t ) );
        srcImg.props.numPlanes = srcProps[i].numPlanes;

        void* worker = reinterpret_cast<void*>( workerPtrs[0] );
        if ( i < workerPtrsLen )
        {
            worker = reinterpret_cast<void*>( workerPtrs[i] );
        }

        FadasRemapMap* map = reinterpret_cast<FadasRemapMap*>( mapPtrs[0] );
        if ( i < mapPtrsLen )
        {
            map = reinterpret_cast<FadasRemapMap*>( mapPtrs[i] );
        }
        FadasROI_t roiStruct = {};
        roiStruct.x = dstROIs[i].x;
        roiStruct.y = dstROIs[i].y;
        roiStruct.width = dstROIs[i].width;
        roiStruct.height = dstROIs[i].height;
        srcImg.plane[0] = const_cast<uint8_t *>( src[i] );
        dstImg.plane[0] = dst + i * dstLen;
        uint32_t srcLen = srcImg.props.height * srcImg.props.stride[0];
        qurt_mem_cache_clean( (qurt_addr_t)src[i], srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
                            QURT_MEM_DCACHE );
        qurt_mutex_lock( &dspContext->mutex );
        if ( 3 == normlzLen )
        {
            FadasNormlzParams_t normlzParams[3];
            normlzParams[0].sub = normlz[0].sub;
            normlzParams[0].mul = normlz[0].mul;
            normlzParams[0].add = normlz[0].add;
            normlzParams[1].sub = normlz[1].sub;
            normlzParams[1].mul = normlz[1].mul;
            normlzParams[1].add = normlz[1].add;
            normlzParams[2].sub = normlz[2].sub;
            normlzParams[2].mul = normlz[2].mul;
            normlzParams[2].add = normlz[2].add;
            retval = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct, 1.0, normlzParams );
        }
        else
        {
            retval = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct );
        }
        qurt_mutex_unlock( &dspContext->mutex );
        if( retval != FADAS_ERROR_NONE )
        {
            FARF( ALWAYS, "FAILED:  FadasRemap_RunMT" );
            return  AEE_EOFFSET + retval;
        }
    }

    qurt_mem_cache_clean( (qurt_addr_t)dst, dstLen * srcFdsLen, QURT_MEM_CACHE_FLUSH_ALL,
                                    QURT_MEM_DCACHE );

    return AEE_SUCCESS;
}

AEEResult FadasIface_mmap( remote_handle64 handle,
                           int32_t bufFd,
                           uint32_t bufSize )
{
    AEEResult status = AEE_SUCCESS;
    void *buf = FadasIface_GetBufPtr(bufFd);
    if ( nullptr == buf )
    {
        int32_t prot = HAP_PROT_READ | HAP_PROT_WRITE;
        int32_t flags = 0;
        buf = HAP_mmap( NULL, bufSize, prot, flags, bufFd, 0 );
        if ( ( ( (void *)0xFFFFFFFF ) == buf) || ( nullptr == buf ) )
        {
            status = AEE_EFAILED;
        }
    }

    return status;
}

AEEResult FadasIface_munmap( remote_handle64 handle,
                             int32_t bufFd,
                             uint32_t bufSize )
{
    AEEResult status = AEE_EFAILED;
    int32_t retVal = -1;
    void *buf = nullptr;
    status = HAP_mmap_get( bufFd, (void **)&buf, NULL );
    if ( AEE_SUCCESS == status )
    {
        int32_t err = -1;
        do
        {
            // decrement user count to 0
            err = HAP_mmap_put( bufFd );
        } while ( 0 == err );
        retVal = HAP_munmap( buf, bufSize );

        if ( AEE_SUCCESS != retVal )
        {
            status = AEE_EFAILED;
        }
        else
        {
            status = AEE_SUCCESS;
        }
    }

    return status;
}

AEEResult FadasIface_FadasRegBuf( remote_handle64 handle,
                                  FadasIface_FadasBufType_e bufType,
                                  int32_t bufFd,
                                  uint32_t bufSize,
                                  uint32_t bufOffset,
                                  uint32_t batch )
{
    FadasError_e ret = FADAS_ERROR_UNKNOWN;
    uint8_t* ptr = (uint8_t*)FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if (nullptr != ptr) {
        for ( i = 0; i < batch; i++ ) {
            ptr += bufOffset;
            ret = FadasRegBuf( g_MapBufType[bufType], ptr, bufSize );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != ret ) {
                break;
            }
        }
    }

    return (AEEResult)ret;
}

AEEResult FadasIface_FadasDeregBuf( remote_handle64 handle,
                                    int32_t bufFd,
                                    uint32_t bufSize,
                                    uint32_t bufOffset,
                                    uint32_t batch )
{
    FadasError_e ret = FADAS_ERROR_UNKNOWN;
    uint8_t* ptr = (uint8_t*)FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if (nullptr != ptr) {
        for ( i = 0; i < batch; i++ ) {
            ptr += bufOffset;
            ret = FadasDeregBuf( ptr );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != ret ) {
                break;
            }
        }
    }
    return (AEEResult)ret;
}


AEEResult FadasIface_FadasCvtYUV_Renormalize888( remote_handle64 handle,
                                                 int32_t srcFd,
                                                 const FadasIface_FadasImgProps_t* srcProps,
                                                 const FadasIface_FadasNormlzParams_t* normlzR,
                                                 const FadasIface_FadasNormlzParams_t* normlzG,
                                                 const FadasIface_FadasNormlzParams_t* normlzB,
                                                 int32_t dstFd,
                                                 uint32_t dstStride,
                                                 int32_t batchSize )
{
    dspContext_t *dspContext = (dspContext_t *)handle;
    const uint8_t *src = (uint8_t *)FadasIface_GetBufPtr( srcFd );
    uint8_t *dst = (uint8_t *)FadasIface_GetBufPtr( dstFd );
    if ( ( nullptr == src ) || ( nullptr == dst ) )
    {
        return AEE_EFAILED;
    }

    FadasImgProps_t srcImgProps;
    srcImgProps.width = srcProps->width;
    srcImgProps.height = srcProps->height;
    srcImgProps.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImgProps.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImgProps.numPlanes = srcProps->numPlanes;

    uint32_t srcLen = srcProps->height * srcProps->stride[0];
    uint32_t dstLen = srcProps->height * dstStride;

    FadasNormlzParams_t normlzParamsR, normlzParamsG, normlzParamsB;
    normlzParamsR.sub = normlzR->sub;
    normlzParamsR.mul = normlzR->mul;
    normlzParamsR.add = normlzR->add;
    normlzParamsG.sub = normlzG->sub;
    normlzParamsG.mul = normlzG->mul;
    normlzParamsG.add = normlzG->add;
    normlzParamsB.sub = normlzB->sub;
    normlzParamsB.mul = normlzB->mul;
    normlzParamsB.add = normlzB->add;

    for ( int i = 0; i < batchSize; i++ )
    {
        const uint8_t *srcPtr = src + srcLen * i;
        uint8_t *dstPtr = dst + dstLen * i;
        qurt_mem_cache_clean( (qurt_addr_t)srcPtr, srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
                            QURT_MEM_DCACHE );
        qurt_mutex_lock( &dspContext->mutex );
        auto retval = FadasCvtYUV_Renormalize888( srcPtr, srcImgProps, normlzParamsR, normlzParamsG,
                                                normlzParamsB, dstPtr, dstStride );
        qurt_mutex_unlock( &dspContext->mutex );
        if( retval != FADAS_ERROR_NONE )
        {
            FARF( ALWAYS, "FAILED:  FadasCvtYUV_Renormalize888 for batch %d", i );
            return  AEE_EOFFSET + retval;
        }
        else
        {
            qurt_mem_cache_clean( (qurt_addr_t)dstPtr, dstLen, QURT_MEM_CACHE_FLUSH_ALL,
                                QURT_MEM_DCACHE );
        }
    }

    return AEE_SUCCESS;
}


AEEResult FadasIface_FadasCvtYUV_Renormalize888u8f32( remote_handle64 handle,
                                                      int32_t srcFd,
                                                      const FadasIface_FadasImgProps_t* srcProps,
                                                      const FadasIface_FadasNormlzParams_t* normlzR,
                                                      const FadasIface_FadasNormlzParams_t* normlzG,
                                                      const FadasIface_FadasNormlzParams_t* normlzB,
                                                      int32_t dstFd,
                                                      uint32_t dstStride,
                                                      int32_t batchSize )
{
    dspContext_t *dspContext = (dspContext_t *)handle;
    const uint8_t *src = (uint8_t *)FadasIface_GetBufPtr( srcFd );
    float *dst = (float *)FadasIface_GetBufPtr( dstFd );
    if ( ( nullptr == src ) || ( nullptr == dst ) )
    {
        return AEE_EFAILED;
    }

    FadasImgProps_t srcImgProps;
    srcImgProps.width = srcProps->width;
    srcImgProps.height = srcProps->height;
    srcImgProps.format = g_MapImageFormat[srcProps->format];
    memcpy( srcImgProps.stride, srcProps->stride, FADAS_NUM_IMAGE_PLANES*sizeof( uint32_t ) );
    srcImgProps.numPlanes = srcProps->numPlanes;

    uint32_t srcLen = srcProps->height * srcProps->stride[0];
    uint32_t dstLen = srcProps->height * dstStride;

    FadasNormlzParams_t normlzParamsR, normlzParamsG, normlzParamsB;
    normlzParamsR.sub = normlzR->sub;
    normlzParamsR.mul = normlzR->mul;
    normlzParamsR.add = normlzR->add;
    normlzParamsG.sub = normlzG->sub;
    normlzParamsG.mul = normlzG->mul;
    normlzParamsG.add = normlzG->add;
    normlzParamsB.sub = normlzB->sub;
    normlzParamsB.mul = normlzB->mul;
    normlzParamsB.add = normlzB->add;

    for ( int i = 0; i < batchSize; i++ )
    {
        const uint8_t *srcPtr = src + srcLen * i;
        float *dstPtr = dst + dstLen * i;
        qurt_mem_cache_clean( (qurt_addr_t)srcPtr, srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
                            QURT_MEM_DCACHE );
        qurt_mutex_lock( &dspContext->mutex );
        auto retval = FadasCvtYUV_Renormalize888u8f32( srcPtr, srcImgProps, normlzParamsR, normlzParamsG,
                                                    normlzParamsB, dstPtr, dstStride );
        qurt_mutex_unlock( &dspContext->mutex );
        if( retval != FADAS_ERROR_NONE )
        {
            FARF( ALWAYS, "FAILED:  FadasCvtYUV_Renormalize888u8f32" );
            return  AEE_EOFFSET + retval;
        }
        else
        {
            qurt_mem_cache_clean( (qurt_addr_t)dstPtr, dstLen, QURT_MEM_CACHE_FLUSH_ALL,
                                QURT_MEM_DCACHE );
        }
    }

    return AEE_SUCCESS;
}

