// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <c2d2.h>
#include <inttypes.h>
#include <unistd.h>

#include "C2D_Impl.hpp"

using namespace ride::hal;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

RideHal_ImageFormat_e GetDataType( const std::string &formatStr )
{
    RideHal_ImageFormat_e dataType;

    if ( formatStr == "uyvy" )
    {
        dataType = RIDE_HAL_IMAGE_FORMAT_UYVY;
    }
    else if ( formatStr == "nv12" )
    {
        dataType = RIDE_HAL_IMAGE_FORMAT_NV12;
    }
    else if ( formatStr == "p010" )
    {
        dataType = RIDE_HAL_IMAGE_FORMAT_P010;
    }
    else if ( formatStr == "rgb" )
    {
        dataType = RIDE_HAL_IMAGE_FORMAT_RGB888;
    }
    else
    {
        dataType = RIDE_HAL_IMAGE_FORMAT_MAX;
    }

    return dataType;
}

float GetFormatDepthSize( RideHal_ImageFormat_e format )
{
    float depthSize = 3.0;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
            depthSize = 2.0;
            break;
        case RIDE_HAL_IMAGE_FORMAT_NV12:
            depthSize = 1.5;
            break;
        case RIDE_HAL_IMAGE_FORMAT_P010:
            depthSize = 3.0;
            break;
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
            depthSize = 3.0;
            break;
        default:
            break;
    }

    return depthSize;
}

uint32_t GetC2DFormatType( RideHal_ImageFormat_e format )
{
    uint32_t c2dFormat = (uint32_t) -1;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
            c2dFormat = C2D_COLOR_FORMAT_422_UYVY;
            break;
        case RIDE_HAL_IMAGE_FORMAT_NV12:
            c2dFormat = C2D_COLOR_FORMAT_420_NV12;
            break;
        case RIDE_HAL_IMAGE_FORMAT_P010:
            c2dFormat = C2D_COLOR_FORMAT_420_P010;
            break;
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB | C2D_FORMAT_SWAP_ENDIANNESS );
            break;
        default:
            break;
    }
    return c2dFormat;
}

C2DImpl::C2DImpl() {}

C2DImpl::~C2DImpl()
{
    if ( m_SourceDef )
    {
        free( m_SourceDef );
    }
    if ( m_TargetDef )
    {
        free( m_TargetDef );
    }

    if ( 0 != m_SourceSurface )
    {
        c2dDestroySurface( m_SourceSurface );
    }
    if ( 0 != m_TargetSurface )
    {
        c2dDestroySurface( m_TargetSurface );
    }
}

RideHalError_e C2DImpl::init( std::array<uint32_t, 2> &inputResolution,
                              RideHal_ImageFormat_e inputFormat,
                              std::array<uint32_t, 2> &outputResolution,
                              RideHal_ImageFormat_e outputFormat, std::array<uint32_t, 4> &roi,
                              uint32_t align )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_InputFormat = inputFormat;
    m_InputResolution = inputResolution;
    m_OutputFormat = outputFormat;
    m_OutputResolution = outputResolution;
    m_ROI = roi;
    m_Align = align;

    /* create source surface */
    ret = createSurface( m_SourceDef, &m_SourceSurface, inputFormat, inputResolution, true );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        // RIDEHAL_ERROR( "Failed to create surface for source\n" );
        return ret;
    }
    if ( nullptr == m_SourceDef )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Source surface is null\n" );
        return ret;
    }

    /* create target surface */
    ret = createSurface( m_TargetDef, &m_TargetSurface, outputFormat, outputResolution, false );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        // RIDEHAL_ERROR( "Failed to create surface for target\n" );
        return ret;
    }
    if ( nullptr == m_TargetDef )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Target surface is null\n" );
        return ret;
    }

    /* allocate shared buffer for input image */
    ret = m_SharedBuffer.Allocate( inputResolution[0], inputResolution[1], inputFormat );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to allocate shared dma buffer for image\n" );
        return ret;
    }

    m_C2dObject.surface_id = m_SourceSurface;   // blit from source
    if ( ( m_ROI[0] != 0 ) || ( m_ROI[1] != 0 ) || ( m_ROI[2] != 0 ) || ( m_ROI[3] != 0 ) )
    {
        m_C2dObject.source_rect.height = m_ROI[3] << 16;
        m_C2dObject.source_rect.width = m_ROI[2] << 16;
        m_C2dObject.source_rect.x = m_ROI[0] << 16;
        m_C2dObject.source_rect.y = m_ROI[1] << 16;
    }
    else
    {
        m_C2dObject.source_rect.height = inputResolution[1] << 16;
        m_C2dObject.source_rect.width = inputResolution[0] << 16;
        m_C2dObject.source_rect.x = 0 << 16;
        m_C2dObject.source_rect.y = 0 << 16;
    }

    m_C2dObject.config_mask = C2D_SOURCE_RECT_BIT;
    m_C2dObject.config_mask |= C2D_NO_BILINEAR_BIT;
    m_C2dObject.config_mask |= C2D_NO_ANTIALIASING_BIT;
    return ret;
}

RideHalError_e C2DImpl::draw( void *input, void *output )
{
    RideHalError_e ret = updateSurface( m_SourceDef, m_SourceSurface, input, m_InputFormat,
                                        m_InputResolution, true );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to update source surface\n" );
        return ret;
    }

    ret = updateSurface( m_TargetDef, m_TargetSurface, output, m_OutputFormat, m_OutputResolution,
                         false );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to update target surface\n" );
        return ret;
    }

    auto c2dStatus = c2dDraw( m_TargetSurface, C2D_TARGET_ROTATE_0, 0, 0, 0, &m_C2dObject, 1 );
    if ( C2D_STATUS_OK != c2dStatus )
    {

        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to draw: %d\n", c2dStatus );
        return ret;
    }

    c2dStatus = c2dFinish( m_TargetSurface );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to finish: %d\n", c2dStatus );
        return ret;
    }

    return ret;
}

RideHalError_e C2DImpl::createYUVSurface( void *surface, uint32_t *surfaceId,
                                          RideHal_ImageFormat_e format,
                                          std::array<uint32_t, 2> &resolution, bool isSource )
{
    surface = nullptr;
    uint32_t width = resolution[0];
    uint32_t height = resolution[1];
    uint32_t bytesPerPixel = 1;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( format == RIDE_HAL_IMAGE_FORMAT_P010 ) || ( format == RIDE_HAL_IMAGE_FORMAT_UYVY ) )
    {
        bytesPerPixel = 2;
    }
    uint32_t stride = width * bytesPerPixel;

    C2D_YUV_SURFACE_DEF *surfaceDef =
            (C2D_YUV_SURFACE_DEF *) malloc( sizeof( C2D_YUV_SURFACE_DEF ) );
    if ( surfaceDef != nullptr )
    {
        memset( surfaceDef, 0, sizeof( C2D_YUV_SURFACE_DEF ) );

        surfaceDef->plane0 = m_SharedBuffer.data();
        surfaceDef->format = GetC2DFormatType( format );
        surfaceDef->height = height;
        surfaceDef->width = width;
        surfaceDef->stride0 = stride;
        surfaceDef->phys0 = (void *) 1;   // any nonzero value
        switch ( surfaceDef->format )
        {
            case C2D_COLOR_FORMAT_420_NV12:
            case C2D_COLOR_FORMAT_420_P010:
                surfaceDef->plane1 = (void *) ( (uint8_t *) surfaceDef->plane0 + stride * height );
                surfaceDef->stride1 = stride;
                surfaceDef->phys1 = (void *) 1;   // any nonzero value
                break;
            default:
                break;
        }
        auto c2dStatus = c2dCreateSurface(
                surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
                static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS ),
                surfaceDef );
        if ( C2D_STATUS_OK != c2dStatus )
        {
            free( surfaceDef );
            // RIDEHAL_ERROR( "Failed to create %s YUV surface for format %d resoultion %d x %d:
            // %d\n",
            //    isSource ? "source" : "target", (int) format, width, height, (int) ret );
        }
        else
        { /* create surface successfully */
            surface = surfaceDef;
        }
    }
    else
    {
        free( surfaceDef );
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Failed to allocate memory for YUV surfaceDef\n" );
        return ret;
    }
    return ret;
}

RideHalError_e C2DImpl::createRGBSurface( void *surface, uint32_t *surfaceId,
                                          RideHal_ImageFormat_e format,
                                          std::array<uint32_t, 2> &resolution, bool isSource )
{
    surface = nullptr;
    uint32_t width = resolution[0];
    uint32_t height = resolution[1];
    uint32_t stride = width * 3;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( false == isSource )
    {
        /* only do this for RGB output */
        stride = ALIGN_S( stride, m_Align );
    }

    C2D_RGB_SURFACE_DEF *surfaceDef =
            (C2D_RGB_SURFACE_DEF *) malloc( sizeof( C2D_RGB_SURFACE_DEF ) );
    if ( surfaceDef != nullptr )
    {
        memset( surfaceDef, 0, sizeof( C2D_RGB_SURFACE_DEF ) );

        surfaceDef->buffer = m_SharedBuffer.data();
        surfaceDef->format = GetC2DFormatType( format );
        surfaceDef->height = height;
        surfaceDef->width = width;
        surfaceDef->stride = stride;
        surfaceDef->phys = (void *) 1;   // any nonzero value
        auto c2dStatus = c2dCreateSurface(
                surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
                static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ),
                surfaceDef );
        if ( C2D_STATUS_OK != c2dStatus )
        {
            free( surfaceDef );
            // RIDEHAL_ERROR( "Failed to create %s RGB surface for format %d resoultion %dx%d:
            // %d\n",
            //    isSource ? "source" : "target", (int) format, width, height, (int) ret );
        }
        else
        { /* create surface successfully */
            surface = surfaceDef;
        }
    }
    else
    {
        free( surfaceDef );
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Failed to allocate memory for RGB surfaceDef\n" );
        return ret;
    }
    return ret;
}

RideHalError_e C2DImpl::createSurface( void *surface, uint32_t *surfaceId,
                                       RideHal_ImageFormat_e format,
                                       std::array<uint32_t, 2> &resolution, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
            ret = createRGBSurface( surface, surfaceId, format, resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to create RGB Surface\n" );
            }
            break;
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
            ret = createYUVSurface( surface, surfaceId, format, resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to create YUV Surface\n" );
            }
            break;
        default:
            break;
    }
    return ret;
}

RideHalError_e C2DImpl::updateYUVSurface( C2D_YUV_SURFACE_DEF *surfaceDef, uint32_t surfaceId,
                                          void *ptr, RideHal_ImageFormat_e format,
                                          std::array<uint32_t, 2> &resolution, bool isSource )
{
    uint32_t width = resolution[0];
    uint32_t height = resolution[1];
    uint32_t bytesPerPixel = 1;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( format == RIDE_HAL_IMAGE_FORMAT_P010 )
    {
        bytesPerPixel = 2;
    }
    uint32_t stride = width * bytesPerPixel;
    surfaceDef->plane0 = ptr;
    surfaceDef->plane1 = (void *) ( (uint8_t *) surfaceDef->plane0 + stride * height );
    auto c2dStatus = c2dUpdateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            ( C2D_SURFACE_TYPE )( C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS ), surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to update %s YUV surface: %d\n", isSource ? "source" : "target",
        //    (int) ret );
        //    return ret;
    }
    return ret;
}

RideHalError_e C2DImpl::updateRGBSurface( C2D_RGB_SURFACE_DEF *surfaceDef, uint32_t surfaceId,
                                          void *ptr, RideHal_ImageFormat_e format,
                                          std::array<uint32_t, 2> &resolution, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    surfaceDef->buffer = ptr;
    auto c2dStatus = c2dUpdateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            ( C2D_SURFACE_TYPE )( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ), surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to update %s RGB surface: %d\n", isSource ? "source" : "target",
        //    (int) ret );
        //    return ret;
    }
    return ret;
}

RideHalError_e C2DImpl::updateSurface( void *surfaceDef, uint32_t surfaceId, void *ptr,
                                       RideHal_ImageFormat_e format,
                                       std::array<uint32_t, 2> &resolution, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
            ret = updateRGBSurface( (C2D_RGB_SURFACE_DEF *) surfaceDef, surfaceId, ptr, format,
                                    resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to update RGB Surface\n" );
            }
            break;
        default:
            ret = updateYUVSurface( (C2D_YUV_SURFACE_DEF *) surfaceDef, surfaceId, ptr, format,
                                    resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to update YUV Surface\n" );
            }
            break;
    }
    return ret;
}
