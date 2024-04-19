// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "AEEStdErr.h"
#include "FadasIface.h"
#include "HAP_farf.h"
#include "HAP_mem.h"
#include "HAP_perf.h"
#include "HAP_power.h"
#include "qurt.h"
#include "remote.h"
#include <fadas.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    qurt_mutex_t mutex;
} dspContext_t;

// FIXME: Maybe need better way to map those enums
const FadasRemapPipeline_e g_MapImageConversion[FADAS_REMAP_PIPELINE_MAX_NSP] = {
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
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMI8,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_UYVY_TO_BGR888,
};

const FadasImageFormat_e g_MapImageFormat[FADAS_IMAGE_FORMAT_COUNT_NSP] = {
        FADAS_IMAGE_FORMAT_UNKNOWN, FADAS_IMAGE_FORMAT_Y,      FADAS_IMAGE_FORMAT_Y12,
        FADAS_IMAGE_FORMAT_UYVY,    FADAS_IMAGE_FORMAT_UYVY10, FADAS_IMAGE_FORMAT_VYUY,
        FADAS_IMAGE_FORMAT_YUV888,  FADAS_IMAGE_FORMAT_RGB888, FADAS_IMAGE_FORMAT_Y10UV10,
        FADAS_IMAGE_FORMAT_Y8UV8,
};

const FadasBufType_e g_MapBufType[FADAS_BUF_TYPE_MAX_NSP] = {
        FADAS_BUF_TYPE_IN,
        FADAS_BUF_TYPE_OUT,
        FADAS_BUF_TYPE_INOUT,
};

AEEResult SetClocks( remote_handle64 handle )
{
    AEEResult ret = AEE_SUCCESS;
    HAP_power_request_t request;
    memset( &request, 0, sizeof( HAP_power_request_t ) );
    request.type = HAP_power_set_apptype;
    request.apptype = HAP_POWER_COMPUTE_CLIENT_CLASS;
    void *ctx = (void *) ( handle );
    int retVal = HAP_power_set( ctx, &request );

    if ( 0 != retVal )
    {
        ret = AEE_EFAILED;
    }
    else
    {
        memset( &request, 0, sizeof( HAP_power_request_t ) );
        request.type = HAP_power_set_DCVS_v2;

        request.dcvs_v2.dcvs_enable = TRUE;
        request.dcvs_v2.dcvs_params.target_corner = (HAP_dcvs_voltage_corner_t) 7;

        request.dcvs_v2.dcvs_params.min_corner = request.dcvs_v2.dcvs_params.target_corner;
        request.dcvs_v2.dcvs_params.max_corner = request.dcvs_v2.dcvs_params.target_corner;

        request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
        request.dcvs_v2.set_dcvs_params = TRUE;
        request.dcvs_v2.set_latency = TRUE;
        request.dcvs_v2.latency = 100;
        retVal = HAP_power_set( ctx, &request );
    }

    if ( 0 != retVal )
    {
        ret = AEE_EFAILED;
    }
    else
    {
        memset( &request, 0, sizeof( HAP_power_request_t ) );
        request.type = HAP_power_set_HVX;
        request.hvx.power_up = TRUE;
        retVal = HAP_power_set( ctx, &request );
    }

    if ( 0 != retVal )
    {
        FARF( ERROR, "Failed to set clocks!" );
        ret = AEE_EFAILED;
    }

    return ret;
}

void *FadasIface_GetBufPtr( int32_t bufFd )
{
    void *bufPtr = nullptr;
    if ( 0 < bufFd )
    {
        HAP_mmap_get( bufFd, (void **) &bufPtr, NULL );
        HAP_mmap_put( bufFd );
    }
    else
    {
        FARF( ERROR, "bufFd %d!", bufFd );
    }

    return bufPtr;
}

AEEResult FadasIface_open( const char *uri, remote_handle64 *handle )
{
    AEEResult ret = AEE_SUCCESS;
    dspContext_t *dspContext = (dspContext_t *) malloc( sizeof( dspContext_t ) );
    *handle = (remote_handle64) dspContext;
    if ( 0 == *handle )
    {
        FARF( ERROR, "Null handle pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        qurt_mutex_init( &dspContext->mutex );
        ret = SetClocks( *handle );
    }

    if ( AEE_SUCCESS != ret )
    {
        FARF( ERROR, "Failed to do FadasIface_open!" );
    }

    return ret;
}

AEEResult FadasIface_close( remote_handle64 handle )
{
    dspContext_t *dspContext = (dspContext_t *) handle;
    if ( NULL == dspContext )
    {
        FARF( ERROR, "Null handle pointer!" );
    }
    else
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

    strlcpy( reinterpret_cast<char *>( ver_int ), FadasVersion(), ver_intLen );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasDeInit( remote_handle64 handle )
{
    FadasDeInit();

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateMapFromMap( remote_handle64 handle, uint64 *mapPtr,
                                                  uint32_t camWidth, uint32_t camHeight,
                                                  uint32_t mapWidth, uint32_t mapHeight,
                                                  const float *mapX, int mapXLen, const float *mapY,
                                                  int mapYLen, uint32_t mapStride,
                                                  FadasIface_FadasRemapPipeline_e imgFormat,
                                                  uint8_t borderConst )
{
    AEEResult ret = AEE_SUCCESS;
    FadasRemapMap *map = FadasRemap_CreateMapFromMap(
            camWidth, camHeight, mapWidth, mapHeight, mapStride, mapX, mapY,
            g_MapImageConversion[static_cast<int>( imgFormat )], borderConst );

    if ( nullptr == map )
    {
        FARF( ERROR, "Null map pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *mapPtr = reinterpret_cast<uint64>( map );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_CreateMapNoUndistortion( remote_handle64 handle, uint64 *mapPtr,
                                                         uint32_t camWidth, uint32_t camHeight,
                                                         uint32_t mapWidth, uint32_t mapHeight,
                                                         FadasIface_FadasRemapPipeline_e imgFormat,
                                                         uint8_t borderConst )
{
    AEEResult ret = AEE_SUCCESS;
    FadasRemapMap *map = FadasRemap_CreateMapNoUndistortion(
            camWidth, camHeight, mapWidth, mapHeight,
            g_MapImageConversion[static_cast<int>( imgFormat )], borderConst );
    if ( nullptr == map )
    {
        FARF( ERROR, "Null map pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *mapPtr = reinterpret_cast<uint64>( map );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_DestroyMap( remote_handle64 handle, uint64 mapPtr )
{
    FadasRemapMap *map = reinterpret_cast<FadasRemapMap *>( mapPtr );

    FadasRemap_DestroyMap( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateWorkers( remote_handle64 handle, uint64 *worker_ptr,
                                               uint32_t nThreads,
                                               FadasIface_FadasRemapPipeline_e imgFormat )
{
    AEEResult ret = AEE_SUCCESS;
    int32_t pThreadsAffinity[] = { 0, 1, 2, 3 };
    void *worker = FadasRemap_CreateWorkers( nThreads, pThreadsAffinity,
                                             g_MapImageConversion[static_cast<int>( imgFormat )] );
    if ( nullptr == worker )
    {
        FARF( ERROR, "Null worker pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *worker_ptr = reinterpret_cast<uint64>( worker );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_DestroyWorkers( remote_handle64 handle, uint64 worker_ptr )
{
    void *worker = reinterpret_cast<void *>( worker_ptr );
    FadasRemap_DestroyWorkers( worker );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_RunMT( remote_handle64 handle, const uint64 *workerPtrs,
                                       int workerPtrsLen, const uint64 *mapPtrs, int mapPtrsLen,
                                       const int32_t *srcFds, int srcFdsLen,
                                       const uint32_t *offsets, int offsetsLen,
                                       const FadasIface_FadasImgProps_t *srcProps, int srcPropsLen,
                                       int32_t dstFd, uint32_t dstLen,
                                       const FadasIface_FadasImgProps_t *dstProps,
                                       const FadasIface_FadasROI_t *dstROIs, int dstROIsLen,
                                       const FadasIface_FadasNormlzParams_t *normlz, int normlzLen )
{
    AEEResult ret = AEE_SUCCESS;
    dspContext_t *dspContext = (dspContext_t *) handle;
    const uint8_t *src[srcFdsLen];
    uint8_t *dst = (uint8_t *) FadasIface_GetBufPtr( dstFd );
    if ( nullptr == dst )
    {
        FARF( ERROR, "Null dst pointer!" );
        ret = AEE_EFAILED;
    }
    else if ( ( srcFdsLen != offsetsLen ) || ( srcFdsLen != srcPropsLen ) )
    {
        FARF( ERROR, "Fd length not equal to props length" );
        ret = AEE_EFAILED;
    }
    else
    {
        for ( int i = 0; i < srcFdsLen; i++ )
        {
            src[i] = (uint8_t *) FadasIface_GetBufPtr( srcFds[i] );
            if ( nullptr == src[i] )
            {
                FARF( ERROR, "Null src pointer!" );
                ret = AEE_EFAILED;
                break;
            }
            else
            {
                src[i] += offsets[i];
            }
        }
    }

    if ( AEE_SUCCESS == ret )
    {
        FadasError_e retVal;
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
            memcpy( srcImg.props.stride, srcProps[i].stride,
                    srcProps[i].numPlanes * sizeof( uint32_t ) );
            srcImg.props.numPlanes = srcProps[i].numPlanes;
            void *worker = reinterpret_cast<void *>( workerPtrs[0] );
            if ( i < workerPtrsLen )
            {
                worker = reinterpret_cast<void *>( workerPtrs[i] );
            }
            FadasRemapMap *map = reinterpret_cast<FadasRemapMap *>( mapPtrs[0] );
            if ( i < mapPtrsLen )
            {
                map = reinterpret_cast<FadasRemapMap *>( mapPtrs[i] );
            }
            FadasROI_t roiStruct = {};
            roiStruct.x = dstROIs[i].x;
            roiStruct.y = dstROIs[i].y;
            roiStruct.width = dstROIs[i].width;
            roiStruct.height = dstROIs[i].height;
            srcImg.plane[0] = const_cast<uint8_t *>( src[i] );
            if ( FADAS_IMAGE_FORMAT_Y8UV8 == srcImg.props.format )
            {
                srcImg.plane[1] = const_cast<uint8_t *>(
                        src[i] + srcImg.props.stride[0] * srcProps[i].actualHeight[0] );
            }
            dstImg.plane[0] = dst + i * dstLen;
            uint32_t srcLen = srcImg.props.height * srcImg.props.stride[0];
            qurt_mem_cache_clean( (qurt_addr_t) src[i], srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
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
                retVal = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct, 1.0,
                                           normlzParams );
            }
            else
            {
                retVal = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct );
            }
            qurt_mutex_unlock( &dspContext->mutex );

            if ( FADAS_ERROR_NONE != retVal )
            {
                FARF( ERROR, "Failed to do FadasRemap_RunMT" );
                ret = AEE_EOFFSET + retVal;
                break;
            }
        }
    }
    qurt_mem_cache_clean( (qurt_addr_t) dst, dstLen * srcFdsLen, QURT_MEM_CACHE_FLUSH_ALL,
                          QURT_MEM_DCACHE );

    return ret;
}

AEEResult FadasIface_mmap( remote_handle64 handle, int32_t bufFd, uint32_t bufSize )
{
    AEEResult ret = AEE_SUCCESS;
    void *buf = FadasIface_GetBufPtr( bufFd );
    if ( nullptr != buf )
    {
        FARF( ERROR, "Already used buf pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        int32_t prot = HAP_PROT_READ | HAP_PROT_WRITE;
        int32_t flags = 0;
        buf = HAP_mmap( NULL, bufSize, prot, flags, bufFd, 0 );
        if ( ( ( (void *) 0xFFFFFFFF ) == buf ) || ( nullptr == buf ) )
        {
            FARF( ERROR, "Null buf pointer!" );
            ret = AEE_EFAILED;
        }
    }

    return ret;
}

AEEResult FadasIface_munmap( remote_handle64 handle, int32_t bufFd, uint32_t bufSize )
{
    AEEResult ret = AEE_SUCCESS;
    void *buf = nullptr;
    ret = HAP_mmap_get( bufFd, (void **) &buf, NULL );
    if ( AEE_SUCCESS == ret )
    {
        int32_t err = -1;
        do
        {
            // decrement user count to 0
            err = HAP_mmap_put( bufFd );
        } while ( 0 == err );
        ret = HAP_munmap( buf, bufSize );
    }

    if ( AEE_SUCCESS != ret )
    {
        FARF( ERROR, "Failed to do FadasIface_munmap!" );
    }

    return ret;
}

AEEResult FadasIface_FadasRegBuf( remote_handle64 handle, FadasIface_FadasBufType_e bufType,
                                  int32_t bufFd, uint32_t bufSize, uint32_t bufOffset,
                                  uint32_t batch )
{
    AEEResult ret = AEE_SUCCESS;
    uint8_t *ptr = (uint8_t *) FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if ( nullptr == ptr )
    {
        FARF( ERROR, "Null bufFd pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        FadasError_e retVal;
        ptr += bufOffset;
        for ( i = 0; i < batch; i++ )
        {
            retVal = FadasRegBuf( g_MapBufType[bufType], ptr, bufSize );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != retVal )
            {
                FARF( ERROR, "Failed to do FadasRegBuf!" );
                ret = AEE_EFAILED;
                break;
            }
        }
    }

    return ret;
}

AEEResult FadasIface_FadasDeregBuf( remote_handle64 handle, int32_t bufFd, uint32_t bufSize,
                                    uint32_t bufOffset, uint32_t batch )
{
    AEEResult ret = AEE_SUCCESS;
    uint8_t *ptr = (uint8_t *) FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if ( nullptr == ptr )
    {
        FARF( ERROR, "Null bufFd pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        FadasError_e retVal;
        ptr += bufOffset;
        for ( i = 0; i < batch; i++ )
        {
            retVal = FadasDeregBuf( ptr );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != ret )
            {
                FARF( ERROR, "Failed to do FadasDeregBuf!" );
                ret = AEE_EFAILED;
                break;
            }
        }
    }

    return ret;
}
