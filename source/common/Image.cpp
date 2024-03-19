// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include <apdf.h>

namespace ridehal
{
namespace common
{
#define PLANEDEF_HW_USAGE_FLAGS                                                                    \
    ( WFD_USAGE_OPENGL_ES2 | WFD_USAGE_OPENGL_ES3 | WFD_USAGE_CAPTURE | WFD_USAGE_VIDEO |          \
      WFD_USAGE_DISPLAY | WFD_USAGE_NATIVE )

static PDColorFormat_e s_rideHalFormatToApdfFormat[RIDE_HAL_IMAGE_FORMAT_MAX] = {
        PD_FORMAT_RGB888, /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
        PD_FORMAT_RGB888, /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
        PD_FORMAT_UYVY,   /* RIDE_HAL_IMAGE_FORMAT_UYVY */
        PD_FORMAT_NV12,   /* RIDE_HAL_IMAGE_FORMAT_NV12 */
        PD_FORMAT_P010    /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

static uint32_t s_rideHalFormatToBytesPerPixel[RIDE_HAL_IMAGE_FORMAT_MAX] = {
        3, /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
        3, /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
        2, /* RIDE_HAL_IMAGE_FORMAT_UYVY */
        1, /* RIDE_HAL_IMAGE_FORMAT_NV12 */
        1  /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

static uint32_t s_rideHalFormatToNumPlanes[RIDE_HAL_IMAGE_FORMAT_MAX] = {
        1, /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
        1, /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
        1, /* RIDE_HAL_IMAGE_FORMAT_UYVY */
        2, /* RIDE_HAL_IMAGE_FORMAT_NV12 */
        2  /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

static uint32_t s_rideHalFormatToHeightDividerPerPlanes
        [RIDE_HAL_IMAGE_FORMAT_MAX][RIDE_HAL_NUM_IMAGE_PLANES] = {
                { 1, 0, 0, 0 }, /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
                { 1, 0, 0, 0 }, /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
                { 1, 0, 0, 0 }, /* RIDE_HAL_IMAGE_FORMAT_UYVY */
                { 1, 2, 0, 0 }, /* RIDE_HAL_IMAGE_FORMAT_NV12 */
                { 1, 2, 0, 0 }  /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

static const char *s_rideHalFormatToString[RIDE_HAL_IMAGE_FORMAT_MAX] = {
        "RGB888", /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
        "BGR888", /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
        "UYVU",   /* RIDE_HAL_IMAGE_FORMAT_UYVY */
        "NV12",   /* RIDE_HAL_IMAGE_FORMAT_NV12 */
        "P010"    /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

RideHalError_e RideHal_SharedBuffer::Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                                               RideHal_ImageFormat_e format,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    size_t size = 0;
    uint32_t nUsage = PLANEDEF_HW_USAGE_FLAGS;
    FrameRes_t frameRes;
    PlaneDef_t planeDef;
    uint32_t numPlanes = 0;
    PDColorFormat_e eColorFormat = PD_FORMAT_MAX;
    PDStatus_e status;
    uint32_t i = 0;

    if ( ( 0 == batchSize ) || ( 0 == width ) || ( 0 == height ) ||
         ( format >= RIDE_HAL_IMAGE_FORMAT_MAX ) )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        ret = RIDE_HAL_ERROR_EXISTS;
    }
    else
    {
        eColorFormat = s_rideHalFormatToApdfFormat[format];
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        status = PDQueryNumPlanes( eColorFormat, nUsage, &numPlanes, 0 );
        if ( ( PD_OK != status ) || ( 0 == numPlanes ) ||
             ( numPlanes > RIDE_HAL_NUM_IMAGE_PLANES ) )
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
        else
        {
            frameRes.nWidthInPixels = width;
            frameRes.nHeightInPixels = height;
            this->imgProps.format = format;
            this->imgProps.batchSize = batchSize;
            this->imgProps.width = width;
            this->imgProps.height = height;
            this->imgProps.numPlanes = numPlanes;
            this->type = RIDE_HAL_BUFFER_TYPE_IMAGE;
        }
    }

    RIDEHAL_LOG_VERBOSE( "PlaneDef for image %ux%u format %s\n", width, height,
                         s_rideHalFormatToString[format] );
    for ( i = 0; ( i < numPlanes ) && ( RIDE_HAL_ERROR_NONE == ret ); i++ )
    {
        planeDef.nPlaneIndex = i + 1;
        status = PDQueryPlaneDef( eColorFormat, nUsage, &frameRes, &planeDef, 0 );
        if ( PD_OK == status )
        {
            RIDEHAL_LOG_VERBOSE(
                    " plane %u: nMinStride = %u, nMaxstride = %u nStrideMultiples = %u", i,
                    planeDef.nMinStride, planeDef.nMaxstride, planeDef.nStrideMultiples );
            RIDEHAL_LOG_VERBOSE(
                    "   nActualStride = %u nMinPlaneBufHeight = %u, nHeightMultiples = %u",
                    planeDef.nActualStride, planeDef.nMinPlaneBufHeight,
                    planeDef.nHeightMultiples );
            RIDEHAL_LOG_VERBOSE( "   nActualPlaneBufHeight = %u nActualBufSizeAlignment =%u,",
                                 planeDef.nActualPlaneBufHeight, planeDef.nActualBufSizeAlignment );
            RIDEHAL_LOG_VERBOSE(
                    "   nBufAddrAlignment = %u nPlaneBufSize = %u, nPlanePaddingSize = %u",
                    planeDef.nBufAddrAlignment, planeDef.nPlaneBufSize,
                    planeDef.nPlanePaddingSize );
            this->imgProps.stride[i] = planeDef.nActualStride;
            this->imgProps.actualHeight[i] = planeDef.nActualPlaneBufHeight;
            size += planeDef.nActualStride * planeDef.nActualPlaneBufHeight;
            if ( i == ( numPlanes - 1 ) )
            {
                this->imgProps.extraPadding = planeDef.nPlanePaddingSize;
                size += planeDef.nPlanePaddingSize;
            }
            else
            {
                if ( 0 != planeDef.nPlanePaddingSize )
                {
                    ret = RIDE_HAL_ERROR_UNSUPPORTED;
                }
            }
        }
        else
        {
            ret = RIDE_HAL_ERROR_FAIL;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        size = size * batchSize;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::Allocate( uint32_t width, uint32_t height,
                                               RideHal_ImageFormat_e format,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    return Allocate( 1, width, height, format, usage, flags );
}

RideHalError_e RideHal_SharedBuffer::Allocate( const RideHal_ImageProps_t *pImgProps,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    size_t size = 0;
    uint32_t i = 0;
    uint32_t bpp = 0;
    uint32_t div = 0;

    if ( nullptr == pImgProps )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else if ( ( 0 == pImgProps->batchSize ) || ( 0 == pImgProps->width ) ||
              ( 0 == pImgProps->height ) )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( pImgProps->format >= RIDE_HAL_IMAGE_FORMAT_MAX ) &&
              ( pImgProps->format < RIDE_HAL_IMAGE_FORMAT_COMPRESSED_MIN ) )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( pImgProps->format >= RIDE_HAL_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        ret = RIDE_HAL_ERROR_EXISTS;
    }
    else
    {
        if ( pImgProps->format < RIDE_HAL_IMAGE_FORMAT_MAX )
        { /* check properties for uncompressed image */
            if ( s_rideHalFormatToNumPlanes[pImgProps->format] != pImgProps->numPlanes )
            {
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
        }
        else
        { /* check properties for compressed image */
            if ( 0 == pImgProps->compressedSize )
            {
                ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        if ( pImgProps->format < RIDE_HAL_IMAGE_FORMAT_MAX )
        {
            bpp = s_rideHalFormatToBytesPerPixel[pImgProps->format];
            /* check each plane def is reasonable */
            for ( i = 0; ( i < pImgProps->numPlanes ) && ( RIDE_HAL_ERROR_NONE == ret ); i++ )
            {
                div = s_rideHalFormatToHeightDividerPerPlanes[pImgProps->format][i];
                if ( ( pImgProps->width * bpp ) < pImgProps->stride[i] )
                {
                    ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( pImgProps->height * bpp ) < pImgProps->actualHeight[i] )
                {
                    ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
                }
                else
                {
                }
            }
            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                for ( i = 0; i < pImgProps->numPlanes; i++ )
                {
                    size += pImgProps->stride[i] * pImgProps->actualHeight[i];
                }
                size += pImgProps->extraPadding;
                size = size * pImgProps->batchSize;
            }
        }
        else
        {
            size = pImgProps->compressedSize;
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        this->imgProps = *pImgProps;
        this->type = RIDE_HAL_BUFFER_TYPE_IMAGE;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer,
                                                      uint32_t batchOffset, uint32_t batchSize )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    size_t singleImageSize = 0;

    if ( nullptr == this->buffer.pData )
    {
        ret = RIDE_HAL_ERROR_INVALID_BUF;
    }
    else if ( RIDE_HAL_BUFFER_TYPE_IMAGE != this->type )
    {
        ret = RIDE_HAL_ERROR_UNSUPPORTED;
    }
    else if ( batchOffset >= this->imgProps.batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( batchOffset + batchSize ) >= this->imgProps.batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        *pSharedBuffer = *this;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        singleImageSize = pSharedBuffer->buffer.size / pSharedBuffer->imgProps.batchSize;
        pSharedBuffer->imgProps.batchSize = batchSize;
        pSharedBuffer->size = batchSize * singleImageSize;
        pSharedBuffer->offset = batchOffset * singleImageSize;
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
