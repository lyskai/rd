// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <c2d2.h>
#include <cinttypes>
#include <memory>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "ridehal/component/C2D.hpp"

namespace ridehal
{
namespace component
{

C2D::C2D() {}

C2D::~C2D() {}

RideHalError_e C2D::Init( const char *pName, const C2D_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "ComponentIF::Init failed\n" );
        return ret;
    }

    m_numOfInputs = pConfig->numOfInputs;
    m_numOfOutputs = pConfig->numOfOutputs;
    m_batchSize = pConfig->batchSize;
    m_outputFormat = pConfig->outputFormat;
    m_outputWidth = pConfig->outputResolution.width;
    m_outputHeight = pConfig->outputResolution.height;

    for ( uint32_t i = 0; i < m_numOfInputs; i++ )
    {
        m_inputResolutions[i].width = pConfig->inputConfigs->inputResolution.width;
        m_inputResolutions[i].height = pConfig->inputConfigs->inputResolution.height;
        m_inputFormats[i] = pConfig->inputConfigs->inputFormat;

        m_rois[i].topX = pConfig->inputConfigs->ROI.topX;
        m_rois[i].topY = pConfig->inputConfigs->ROI.topY;
        m_rois[i].width = pConfig->inputConfigs->ROI.width;
        m_rois[i].height = pConfig->inputConfigs->ROI.height;
    }

    /* Complete initialization */
    m_state = RIDE_HAL_COMPONENT_STATE_READY;
    RIDEHAL_INFO( "Component C2D is initialized\n" );
    return ret;
}


RideHalError_e C2D::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    if ( RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // DO start
        m_state = RIDE_HAL_COMPONENT_STATE_RUNNING;
        RIDEHAL_INFO( "Component C2D start to run\n" );
    }

    return ret;
}

RideHalError_e C2D::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // DO stop
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
        RIDEHAL_INFO( "Component C2D is stopped\n" );
    }

    return ret;
}

RideHalError_e C2D::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDE_HAL_ERROR_STATE;
    }


    m_inputBufferSurfaceMap.clear();
    m_outputBufferSurfaceMap.clear();

    /* Complete initialization */
    m_state = RIDE_HAL_COMPONENT_STATE_READY;
    RIDEHAL_INFO( "Component C2D is initialized\n" );

    return ret;
}

RideHalError_e C2D::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                             const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( numInputs != m_numOfInputs )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of inputs not correct: %u != %u\n", m_numOfInputs, numInputs );
        return ret;
    }

    if ( numOutputs != m_numOfOutputs )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of outputs not correct: %u != %u\n", m_numOfOutputs, numOutputs );
        return ret;
    }

    void *inputBufAddr = nullptr;
    void *outputBufAddr = nullptr;
    void *targetSurfaceDef = nullptr;
    uint32_t targetSurfaceId = 0;
    C2D_OBJECT c2dObject;

    /* Check if input buffer is registered */
    for ( size_t i = 0; i < m_numOfInputs; i++ )
    {
        if ( m_inputBufferSurfaceMap.find( pInputs[i].data() ) == m_inputBufferSurfaceMap.end() )
        {
            ret = RegisterInputBuffers( &pInputs[i], 1 );
        }
    }

    /* Check if output buffer is registered */
    for ( size_t i = 0; i < numOutputs; i++ )
    {
        if ( m_outputBufferSurfaceMap.find( pOutputs[i].data() ) == m_outputBufferSurfaceMap.end() )
        {
            ret = RegisterOutputBuffers( &pOutputs[i], 1 );
        }
    }

    for ( size_t i = 0; i < m_numOfInputs; i++ )
    {
        inputBufAddr = pInputs[i].data();
        c2dObject = m_inputBufferSurfaceMap[inputBufAddr];

        outputBufAddr = (void *) ( (uintptr_t) pOutputs->data() + i * m_outputSize );
        targetSurfaceId = m_outputBufferSurfaceMap[outputBufAddr];

        auto c2dStatus = c2dDraw( targetSurfaceId, C2D_TARGET_ROTATE_0, 0, 0, 0, &c2dObject, 1 );
        if ( C2D_STATUS_OK != c2dStatus )
        {
            ret = RIDE_HAL_ERROR_FAIL;
            RIDEHAL_ERROR( "Failed to draw blit objects for input %u\n: ", i );
            return ret;
        }

        c2dStatus = c2dFinish( targetSurfaceId );
        if ( C2D_STATUS_OK != c2dStatus )
        {
            ret = RIDE_HAL_ERROR_FAIL;
            RIDEHAL_ERROR( "Failed to finish target, status = %d\n", c2dStatus );
            return ret;
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                          uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    for ( size_t i = 0; i < numOfInputBuffers; i++ )
    {
        /* Check input image format */
        if ( pInputBuffer->imgProps.format != m_inputFormats[i] )
        {
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Failed to register input buffer %u, input format not correct\n", i );
            return ret;
        }

        /* Check input image resolution */
        if ( pInputBuffer->imgProps.width != m_inputResolutions[i].width ||
             pInputBuffer->imgProps.height != m_inputResolutions[i].height )
        {
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Failed to register input buffer %u, input resolution not correct\n",
                           i );
            return ret;
        }
    }

    void *bufferAddr = nullptr;
    bool isSource = true;
    uint32_t sourceSurfaceId = 0;
    C2D_OBJECT c2dObj;

    for ( size_t i = 0; i < numOfInputBuffers; i++ )
    {
        bufferAddr = pInputBuffer[i].data();
        if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
        {
            continue;
        }
        else
        {
            for ( uint8_t k = 0; k < RIDE_HAL_NUM_IMAGE_PLANES; k++ )
            {
                m_stride[k] = pInputBuffer[i].imgProps.stride[k];
                m_actualHeight[k] = pInputBuffer[i].imgProps.actualHeight[k];
            }

            ret = createSurface( &sourceSurfaceId, m_inputFormats[i], bufferAddr,
                                 m_inputResolutions[i].width, m_inputResolutions[i].height,
                                 m_stride, m_actualHeight, isSource );
            if ( ret == RIDE_HAL_ERROR_NONE )
            {
                c2dObj.surface_id = sourceSurfaceId;
                if ( ( m_rois[i].topX != 0 ) || ( m_rois[i].topY != 0 ) ||
                     ( m_rois[i].width != 0 ) || ( m_rois[i].height != 0 ) )
                {
                    c2dObj.source_rect.x = m_rois[i].topX << 16;
                    c2dObj.source_rect.y = m_rois[i].topY << 16;
                    c2dObj.source_rect.width = m_rois[i].width << 16;
                    c2dObj.source_rect.height = m_rois[i].height << 16;
                }
                else
                {
                    c2dObj.source_rect.x = 0 << 16;
                    c2dObj.source_rect.y = 0 << 16;
                    c2dObj.source_rect.width = m_rois[i].width << 16;
                    c2dObj.source_rect.height = m_rois[i].height << 16;
                }

                c2dObj.config_mask = C2D_SOURCE_RECT_BIT;
                c2dObj.config_mask |= C2D_NO_BILINEAR_BIT;
                c2dObj.config_mask |= C2D_NO_ANTIALIASING_BIT;
                m_inputBufferSurfaceMap[bufferAddr] = c2dObj;
            }
            else
            {
                RIDEHAL_ERROR( "Failed to register input buffer %u\n: ", i );
                return ret;
            }
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                           uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    /* Check output batch size */
    if ( pOutputBuffer->imgProps.batchSize != m_batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Failed to register output buffer, output batch size not correct\n " );
        return ret;
    }

    /* Check output image format */
    if ( pOutputBuffer->imgProps.format != m_outputFormat )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Failed to register output buffer, output format not correct\n" );
        return ret;
    }

    /* Check output image resolution */
    if ( pOutputBuffer->imgProps.width != m_outputWidth ||
         pOutputBuffer->imgProps.height != m_outputHeight )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Failed to register output buffer, output resolution not correct\n" );
        return ret;
    }

    void *bufferAddr = nullptr;
    uint32_t targetSurfaceId = 0;
    bool isSource = false;

    for ( size_t i = 0; i < numOfOutputBuffers; i++ )
    {
        for ( uint8_t j = 0; j < RIDE_HAL_NUM_IMAGE_PLANES; j++ )
        {
            m_stride[j] = pOutputBuffer[i].imgProps.stride[j];
            m_actualHeight[j] = pOutputBuffer[i].imgProps.actualHeight[j];
        }

        m_outputSize = pOutputBuffer[i].size / m_batchSize;
        for ( size_t k = 0; k < m_batchSize; k++ )
        {
            bufferAddr = (void *) ( (uintptr_t) pOutputBuffer[i].data() + k * m_outputSize );
            if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
            {
                continue;
            }
            else
            {
                ret = createSurface( &targetSurfaceId, m_outputFormat, bufferAddr, m_outputWidth,
                                     m_outputHeight, m_stride, m_actualHeight, isSource );
                if ( ret == RIDE_HAL_ERROR_NONE )
                {
                    m_outputBufferSurfaceMap[bufferAddr] = targetSurfaceId;
                }
                else
                {
                    RIDEHAL_ERROR( "Failed to register output buffer %u for batch %u\n: ", i, k );
                    return ret;
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                            uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    for ( size_t i = 0; i < numOfInputBuffers; i++ )
    {
        bufferAddr = pInputBuffer[i].data();
        if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
        {
            m_inputBufferSurfaceMap.erase( bufferAddr );
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                             uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    for ( size_t i = 0; i < numOfOutputBuffers; i++ )
    {
        for ( size_t k = 0; k < m_batchSize; k++ )
        {
            bufferAddr = (void *) ( (uintptr_t) pOutputBuffer[i].data() + k * m_outputSize );
            if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
            {
                m_outputBufferSurfaceMap.erase( bufferAddr );
            }
        }
    }

    return ret;
}

RideHalError_e C2D::createSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                   void *bufferAddr, uint32_t width, uint32_t height,
                                   uint32_t *stride, uint32_t *actualHeight, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    switch ( format )
    {
        case RIDE_HAL_IMAGE_FORMAT_RGB888:
        case RIDE_HAL_IMAGE_FORMAT_BGR888:
            ret = createRGBSurface( surfaceId, format, bufferAddr, width, height, stride,
                                    isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to create RGB Surface\n" );
            }
            break;
        case RIDE_HAL_IMAGE_FORMAT_UYVY:
        case RIDE_HAL_IMAGE_FORMAT_NV12:
        case RIDE_HAL_IMAGE_FORMAT_P010:
            ret = createYUVSurface( surfaceId, format, bufferAddr, width, height, stride,
                                    actualHeight, isSource );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to create YUV Surface\n" );
            }
            break;
        default:
            RIDEHAL_ERROR( "Unsupported image format\n" );
            break;
    }
    return ret;
}

RideHalError_e C2D::createYUVSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                      void *bufferAddr, uint32_t width, uint32_t height,
                                      uint32_t *stride, uint32_t *actualHeight, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    C2D_YUV_SURFACE_DEF surfaceDef;
    memset( &surfaceDef, 0, sizeof( C2D_YUV_SURFACE_DEF ) );

    surfaceDef.plane0 = bufferAddr;
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.height = height;
    surfaceDef.width = width;
    surfaceDef.stride0 = stride[0];
    surfaceDef.stride1 = stride[1];
    surfaceDef.stride2 = stride[2];
    surfaceDef.phys0 = (void *) 1;   // any nonzero value
    surfaceDef.phys1 = (void *) 1;   // any nonzero value

    switch ( surfaceDef.format )
    {
        case C2D_COLOR_FORMAT_420_NV12:
        case C2D_COLOR_FORMAT_420_P010:
            surfaceDef.plane1 =
                    (void *) ( (uint8_t *) surfaceDef.plane0 + stride[0] * actualHeight[0] );
            break;
        default:
            break;
    }
    auto c2dStatus = c2dCreateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS ),
            (void *) &surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDE_HAL_ERROR_FAIL;
        RIDEHAL_ERROR( "Failed to create YUV surface, c2dStatus wrong\n" );
    }

    return ret;
}

RideHalError_e C2D::createRGBSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                      void *bufferAddr, uint32_t width, uint32_t height,
                                      uint32_t *stride, bool isSource )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    C2D_RGB_SURFACE_DEF surfaceDef;
    memset( &surfaceDef, 0, sizeof( C2D_RGB_SURFACE_DEF ) );

    surfaceDef.buffer = bufferAddr;
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.height = height;
    surfaceDef.width = width;
    surfaceDef.stride = stride[0];
    surfaceDef.phys = (void *) 1;   // any nonzero value
    auto c2dStatus = c2dCreateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ),
            (void *) &surfaceDef );

    if ( C2D_STATUS_OK != c2dStatus )
    {
        RIDEHAL_ERROR( "Failed to create RGB surface, c2dStatus wrong\n" );
    }

    return ret;
}

uint32_t C2D::GetC2DFormatType( RideHal_ImageFormat_e format )
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

}   // namespace component
}   // namespace ridehal

