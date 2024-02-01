// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Image.hpp"
#include <apdf.h>

namespace ride
{
namespace hal
{
namespace memory
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

Image::Image() : Buffer()
{
    m_sharedBuffer.type = RIDE_HAL_BUFFER_TYPE_IMAGE;
}

Image::~Image() {}

RideHalError_e Image::Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                                RideHal_ImageFormat_e format, RideHal_BufferFlags_t flags,
                                RideHal_BufferUsage_e usage )
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
    else if ( nullptr != m_sharedBuffer.buffer.pData )
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
            m_sharedBuffer.imgProps.format = format;
            m_sharedBuffer.imgProps.batchSize = batchSize;
            m_sharedBuffer.imgProps.width = width;
            m_sharedBuffer.imgProps.height = height;
            m_sharedBuffer.imgProps.numPlanes = numPlanes;
        }
    }

    Log( Logger::Level_e::VERBOSE, "PlaneDef for image %ux%u format %s\n", width, height,
         s_rideHalFormatToString[format] );
    for ( i = 0; ( i < numPlanes ) && ( RIDE_HAL_ERROR_NONE == ret ); i++ )
    {
        planeDef.nPlaneIndex = i + 1;
        status = PDQueryPlaneDef( eColorFormat, nUsage, &frameRes, &planeDef, 0 );
        if ( PD_OK == status )
        {
            Log( Logger::Level_e::VERBOSE,
                 " plane %u: nMinStride = %u, nMaxstride = %u nStrideMultiples = %u,\n"
                 "  nActualStride = %u nMinPlaneBufHeight = %u, nHeightMultiples = %u,\n"
                 "  nActualPlaneBufHeight = %u nActualBufSizeAlignment =%u,\n"
                 "  nBufAddrAlignment = %u nPlaneBufSize = %u, nPlanePaddingSize = %u\n",
                 i, planeDef.nMinStride, planeDef.nMaxstride, planeDef.nStrideMultiples,
                 planeDef.nActualStride, planeDef.nMinPlaneBufHeight, planeDef.nHeightMultiples,
                 planeDef.nActualPlaneBufHeight, planeDef.nActualBufSizeAlignment,
                 planeDef.nBufAddrAlignment, planeDef.nPlaneBufSize, planeDef.nPlanePaddingSize );
            m_sharedBuffer.imgProps.stride[i] = planeDef.nActualStride;
            m_sharedBuffer.imgProps.actualHeight[i] = planeDef.nActualPlaneBufHeight;
            size += planeDef.nActualStride * planeDef.nActualPlaneBufHeight;
            if ( i == ( numPlanes - 1 ) )
            {
                m_sharedBuffer.imgProps.extraPadding = planeDef.nPlanePaddingSize;
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
        ret = Buffer::Allocate( size, flags, usage );
    }

    return ret;
}

RideHalError_e Image::Allocate( uint32_t width, uint32_t height, RideHal_ImageFormat_e format,
                                RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage )
{
    return Allocate( 1, width, height, format, flags, usage );
}

RideHalError_e Image::Allocate( const RideHal_ImageProps_t *pImgProps, RideHal_BufferFlags_t flags,
                                RideHal_BufferUsage_e usage )
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
              ( 0 == pImgProps->height ) || ( pImgProps->format >= RIDE_HAL_IMAGE_FORMAT_MAX ) )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( s_rideHalFormatToNumPlanes[pImgProps->format] != pImgProps->numPlanes )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != m_sharedBuffer.buffer.pData )
    {
        ret = RIDE_HAL_ERROR_EXISTS;
    }
    else
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
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_sharedBuffer.imgProps = *pImgProps;
        for ( i = 0; i < pImgProps->numPlanes; i++ )
        {
            size += pImgProps->stride[i] * pImgProps->actualHeight[i];
        }
        size += pImgProps->extraPadding;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        size = size * pImgProps->batchSize;
        ret = Buffer::Allocate( size, flags, usage );
    }

    return ret;
}

RideHalError_e Image::GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer )
{
    return Buffer::GetSharedBuffer( pSharedBuffer );
}

RideHalError_e Image::GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer, uint32_t batchOffset,
                                       uint32_t batchSize )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    size_t singleImageSize = 0;

    if ( nullptr == m_sharedBuffer.buffer.pData )
    {
        ret = RIDE_HAL_ERROR_INVALID_BUF;
    }
    else if ( batchOffset >= m_sharedBuffer.imgProps.batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( batchOffset + batchSize ) >= m_sharedBuffer.imgProps.batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        ret = Buffer::GetSharedBuffer( pSharedBuffer );
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


}   // namespace memory
}   // namespace hal
}   // namespace ride
