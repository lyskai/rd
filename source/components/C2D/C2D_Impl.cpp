// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <c2d2.h>
#include <inttypes.h>
#include <unistd.h>

#include "C2D_Impl.hpp"

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
        case RIDE_HAL_IMAGE_FORMAT_BGR888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB );
            break;
        default:
            break;
    }
    return c2dFormat;
}

C2DImpl::C2DImpl() {}

C2DImpl::~C2DImpl() {}

RideHalError_e C2DImpl::init( std::array<uint32_t, 2> &inputResolution,
                              RideHal_ImageFormat_e inputFormat,
                              std::array<uint32_t, 2> &outputResolution,
                              RideHal_ImageFormat_e outputFormat, std::array<uint32_t, 4> &roi,
                              uint32_t align )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_inputFormat = inputFormat;
    m_inputResolution = inputResolution;
    m_outputFormat = outputFormat;
    m_outputResolution = outputResolution;
    m_roi = roi;
    m_align = align;

    /* allocate shared buffer for input buffer */
    ret = m_inputBuffer.Allocate( m_inputResolution[0], m_inputResolution[1], m_inputFormat );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to allocate shared dma buffer for image\n" );
        std::cout << "Failed to allocate memory for input buffer" << std::endl;
        return ret;
    }

    /* allocate shared buffer for output buffer */
    ret = m_outputBuffer.Allocate( m_outputResolution[0], m_outputResolution[1], m_outputFormat );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to allocate shared dma buffer for image\n" );
        std::cout << "Failed to allocate memory for output buffer" << std::endl;
        return ret;
    }

    /* create source surface */
    m_sourceDef = createSurface( &m_sourceSurface, m_inputFormat, &m_inputBuffer, m_inputResolution,
                                 true );
    if ( nullptr == m_sourceDef )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Source surface is null\n" );
        std::cout << "Source surface is null" << std::endl;
        return ret;
    }

    /* create target surface */
    m_targetDef = createSurface( &m_targetSurface, m_outputFormat, &m_outputBuffer,
                                 m_outputResolution, false );
    if ( nullptr == m_targetDef )
    {
        ret = RIDE_HAL_ERROR_NULL_PTR;
        // RIDEHAL_ERROR( "Target surface is null\n" );
        std::cout << "Target surface is null" << std::endl;
        return ret;
    }

    m_c2dObject.surface_id = m_sourceSurface;   // blit from source
    if ( ( m_roi[0] != 0 ) || ( m_roi[1] != 0 ) || ( m_roi[2] != 0 ) || ( m_roi[3] != 0 ) )
    {
        m_c2dObject.source_rect.height = m_roi[3] << 16;
        m_c2dObject.source_rect.width = m_roi[2] << 16;
        m_c2dObject.source_rect.x = m_roi[0] << 16;
        m_c2dObject.source_rect.y = m_roi[1] << 16;
    }
    else
    {
        m_c2dObject.source_rect.height = inputResolution[1] << 16;
        m_c2dObject.source_rect.width = inputResolution[0] << 16;
        m_c2dObject.source_rect.x = 0 << 16;
        m_c2dObject.source_rect.y = 0 << 16;
    }

    m_c2dObject.config_mask = C2D_SOURCE_RECT_BIT;
    m_c2dObject.config_mask |= C2D_NO_BILINEAR_BIT;
    m_c2dObject.config_mask |= C2D_NO_ANTIALIASING_BIT;
    return ret;
}

RideHalError_e C2DImpl::deInit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( m_sourceDef )
    {
        free( m_sourceDef );
    }
    if ( m_targetDef )
    {
        free( m_targetDef );
    }

    if ( 0 != m_sourceSurface )
    {
        c2dDestroySurface( m_sourceSurface );
    }
    if ( 0 != m_targetSurface )
    {
        c2dDestroySurface( m_targetSurface );
    }

    ret = m_inputBuffer.Free();
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        std::cout << "Failed to free input buffer" << std::endl;
    }
    ret = m_outputBuffer.Free();
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        std::cout << "Failed to free output buffer" << std::endl;
    }

    return ret;
}

RideHalError_e C2DImpl::draw( void *input, void *output )
{
    RideHalError_e ret = updateSurface( m_sourceDef, m_sourceSurface, input, m_inputFormat,
                                        m_inputResolution, true );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to update source surface\n" );
        std::cout << "Failed to update source surface" << std::endl;
        return ret;
    }

    ret = updateSurface( m_targetDef, m_targetSurface, output, m_outputFormat, m_outputResolution,
                         false );
    if ( ret != RIDE_HAL_ERROR_NONE )
    {
        // RIDEHAL_ERROR( "Failed to update target surface\n" );
        std::cout << "Failed to update target surface" << std::endl;
        return ret;
    }

    auto c2dStatus = c2dDraw( m_targetSurface, C2D_TARGET_ROTATE_0, 0, 0, 0, &m_c2dObject, 1 );
    if ( C2D_STATUS_OK != c2dStatus )
    {

        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to draw: %d\n", c2dStatus );
        std::cout << "Failed to draw, status =  " << c2dStatus << std::endl;
        return ret;
    }

    c2dStatus = c2dFinish( m_targetSurface );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to finish: %d\n", c2dStatus );
        std::cout << "Failed to finish, status =  " << c2dStatus << std::endl;
        return ret;
    }

    return ret;
}

void *C2DImpl::createYUVSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                 RideHal_SharedBuffer_t *buffer,
                                 std::array<uint32_t, 2> &resolution, bool isSource )
{
    void *surface = nullptr;
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

        surfaceDef->plane0 = buffer->data();
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
            // RIDEHAL_ERROR(
            //         "Failed to create %s YUV surface for format %d resoultion %d x %d:% d\n ",
            //         isSource ? "source" : "target", (int) format, width, height, (int) ret );
            std::cout << "Failed to create"
                      << " YUV surface, c2dStatus wrong" << std::endl;
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
        std::cout << "Failed to allocate memory for YUV surfaceDef" << std::endl;
        return surface;
    }
    return surface;
}

void *C2DImpl::createRGBSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                 RideHal_SharedBuffer_t *buffer,
                                 std::array<uint32_t, 2> &resolution, bool isSource )
{
    void *surface = nullptr;
    uint32_t width = resolution[0];
    uint32_t height = resolution[1];
    uint32_t stride = width * 3;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( false == isSource )
    {
        /* only do this for RGB output */
        stride = ALIGN_S( stride, m_align );
    }

    C2D_RGB_SURFACE_DEF *surfaceDef =
            (C2D_RGB_SURFACE_DEF *) malloc( sizeof( C2D_RGB_SURFACE_DEF ) );
    if ( surfaceDef != nullptr )
    {
        memset( surfaceDef, 0, sizeof( C2D_RGB_SURFACE_DEF ) );

        surfaceDef->buffer = buffer->data();
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
            // RIDEHAL_ERROR( "Failed to create %s RGB surface for format %d resoultion %dx%d: % d\n
            //                ", isSource ? " source "  : " target ", (int) format, width, height,
            //                (int) ret );
            std::cout << "Failed to create"
                      << " RGB surface" << std::endl;
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
        std::cout << "Failed to allocate memory for RGB surfaceDef" << std::endl;
        return surface;
    }
    return surface;
}

void *C2DImpl::createSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                              RideHal_SharedBuffer_t *buffer, std::array<uint32_t, 2> &resolution,
                              bool isSource )
{
    void *surface = nullptr;
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_RGB888: /* no break */
        case RIDE_HAL_IMAGE_FORMAT_BGR888:
            surface = createRGBSurface( surfaceId, format, buffer, resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to create RGB Surface\n" );
                std::cout << "Failed to create RGB Surface" << std::endl;
            }
            break;
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
        case RIDE_HAL_IMAGE_FORMAT_NV12:
        case RIDE_HAL_IMAGE_FORMAT_P010:
            surface = createYUVSurface( surfaceId, format, buffer, resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to create YUV Surface\n" );
                std::cout << "Failed to create YUV Surface" << std::endl;
            }
            break;
        default:
            std::cout << "Unsupported image format" << std::endl;
            break;
    }
    return surface;
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
            (C2D_SURFACE_TYPE) ( C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS ), surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to update %s YUV surface: %d\n", isSource ? "source" : "target",
        //                (int) ret );
        std::cout << "Failed to update"
                  << " YUV surface" << std::endl;
        return ret;
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
            (C2D_SURFACE_TYPE) ( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ), surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        // RIDEHAL_ERROR( "Failed to update %s RGB surface: %d\n", isSource ? "source" : "target",
        //                (int) ret );
        std::cout << "Failed to update"
                  << " YUV surface" << std::endl;
        return ret;
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
                std::cout << "Failed to update RGB Surface" << std::endl;
            }
            break;
        default:
            ret = updateYUVSurface( (C2D_YUV_SURFACE_DEF *) surfaceDef, surfaceId, ptr, format,
                                    resolution, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                // RIDEHAL_ERROR( "Failed to update YUV Surface\n" );
                std::cout << "Failed to update YUV Surface" << std::endl;
            }
            break;
    }
    return ret;
}

