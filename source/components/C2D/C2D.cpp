// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <c2d2.h>
#include <cinttypes>
#include <cstring>
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
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "ComponentIF::Init failed\n" );
    }
    else
    {
        m_numOfInputs = pConfig->numOfInputs;
        if ( m_numOfInputs > RIDEHAL_MAX_INPUTS )
        {
            ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            RIDEHAL_ERROR( "Number of Inputs exceeds maximum limit" );
        }


        if ( RIDEHAL_ERROR_NONE == ret )
        {
            for ( uint32_t i = 0; i < m_numOfInputs; i++ )
            {
                m_inputResolutions[i].width = pConfig->inputConfigs->inputResolution.width;
                m_inputResolutions[i].height = pConfig->inputConfigs->inputResolution.height;
                m_inputFormats[i] = pConfig->inputConfigs->inputFormat;

                if ( pConfig->inputConfigs->ROI.topX >= 0 &&
                     pConfig->inputConfigs->ROI.topX <= m_inputResolutions[i].width )
                {
                    m_rois[i].topX = pConfig->inputConfigs->ROI.topX;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topX of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs->ROI.topY >= 0 &&
                     pConfig->inputConfigs->ROI.topY <= m_inputResolutions[i].height )
                {
                    m_rois[i].topY = pConfig->inputConfigs->ROI.topY;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topY of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs->ROI.width >= 0 &&
                     pConfig->inputConfigs->ROI.width <=
                             m_inputResolutions[i].width - m_rois[i].topX )
                {
                    m_rois[i].width = pConfig->inputConfigs->ROI.width;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI width of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs->ROI.height >= 0 &&
                     pConfig->inputConfigs->ROI.height <=
                             m_inputResolutions[i].height - m_rois[i].topY )
                {
                    m_rois[i].height = pConfig->inputConfigs->ROI.height;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI height of input %u is out of range", i );
                    break;
                }
            }

            /* Complete initialization */
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                m_state = RIDEHAL_COMPONENT_STATE_READY;
                RIDEHAL_INFO( "Component C2D is initialized\n" );
            }
        }
    }

    return ret;
}

RideHalError_e C2D::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO start
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
        RIDEHAL_INFO( "Component C2D start to run\n" );
    }

    return ret;
}

RideHalError_e C2D::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO stop
        m_state = RIDEHAL_COMPONENT_STATE_READY;
        RIDEHAL_INFO( "Component C2D is stopped\n" );
    }

    return ret;
}

RideHalError_e C2D::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_inputBufferSurfaceMap.clear();
        m_outputBufferSurfaceMap.clear();

        ret = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            /* Complete deinitialization */
            m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            RIDEHAL_INFO( "Component C2D is deinitialized\n" );
        }
        else
        {
            RIDEHAL_ERROR( "ComponentIF::Deinit failed\n" );
        }
    }

    return ret;
}

RideHalError_e C2D::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                             const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *inputBufAddr = nullptr;
    void *outputBufAddr = pOutput->data();
    void *targetSurfaceDef = nullptr;
    uint32_t targetSurfaceId = 0;
    uint32_t outputSize = pOutput->size / m_numOfInputs;
    C2D_OBJECT c2dObject;

    if ( numInputs != m_numOfInputs )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of inputs not correct: %u != %u\n", m_numOfInputs, numInputs );
    }

    /* Check if output buffer is registered */
    if ( m_outputBufferSurfaceMap.find( outputBufAddr ) == m_outputBufferSurfaceMap.end() )
    {
        ret = RegisterOutputBuffers( pOutput, 1 );
    }

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            inputBufAddr = pInputs[i].data();

            /* Check if input buffer is registered */
            if ( m_inputBufferSurfaceMap.find( inputBufAddr ) == m_inputBufferSurfaceMap.end() )
            {
                ret = RegisterInputBuffers( &pInputs[i], 1 );
            }

            c2dObject = m_inputBufferSurfaceMap[inputBufAddr];
            outputBufAddr = (void *) ( (uintptr_t) pOutput->data() + i * outputSize );
            targetSurfaceId = m_outputBufferSurfaceMap[outputBufAddr];

            auto c2dStatus =
                    c2dDraw( targetSurfaceId, C2D_TARGET_ROTATE_0, 0, 0, 0, &c2dObject, 1 );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to draw blit objects for input %u\n: ", i );
                break;
            }

            c2dStatus = c2dFinish( targetSurfaceId );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to finish target, status = %d\n", c2dStatus );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                          uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    bool isSource = true;
    uint32_t sourceSurfaceId = 0;
    uint32_t batchIdx = 0;
    C2D_OBJECT c2dObj;

    for ( size_t i = 0; i < numOfInputBuffers; i++ )
    {
        /* Check input image format */
        if ( pInputBuffer->imgProps.format != m_inputFormats[i] )
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Failed to register input buffer %u, format not correct\n", i );
            break;
        }

        /* Check input image resolution */
        if ( pInputBuffer->imgProps.width != m_inputResolutions[i].width ||
             pInputBuffer->imgProps.height != m_inputResolutions[i].height )
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Failed to register input buffer %u, resolution not correct\n", i );
            break;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfInputBuffers; i++ )
        {
            bufferAddr = pInputBuffer[i].data();
            if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
            {
                continue;
            }
            else
            {
                ret = createSurface( &sourceSurfaceId, batchIdx, pInputBuffer, isSource );

                if ( ret == RIDEHAL_ERROR_NONE )
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
                    break;
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                           uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    uint32_t targetSurfaceId = 0;
    bool isSource = false;
    uint32_t outputSize = pOutputBuffer->size / pOutputBuffer->imgProps.batchSize;

    if ( m_numOfInputs != pOutputBuffer->imgProps.batchSize )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Failed to register output buffer, numOfInputs %u != batchSize %u",
                       m_numOfInputs, pOutputBuffer->imgProps.batchSize );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfOutputBuffers; i++ )
        {
            for ( size_t k = 0; k < m_numOfInputs; k++ )
            {
                bufferAddr = (void *) ( (uintptr_t) pOutputBuffer[i].data() + k * outputSize );
                if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
                {
                    continue;
                }
                else
                {
                    ret = createSurface( &targetSurfaceId, k, pOutputBuffer, isSource );
                    if ( ret == RIDEHAL_ERROR_NONE )
                    {
                        m_outputBufferSurfaceMap[bufferAddr] = targetSurfaceId;
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Failed to register output buffer %u for batch %u\n: ", i,
                                       k );
                        break;
                    }
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                            uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    if ( numOfInputBuffers > m_inputBufferSurfaceMap.size() )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfInputBuffers; i++ )
        {
            bufferAddr = pInputBuffer[i].data();
            if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
            {
                m_inputBufferSurfaceMap.erase( bufferAddr );
            }
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                             uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    uint32_t outputSize = pOutputBuffer->size / pOutputBuffer->imgProps.batchSize;

    if ( numOfOutputBuffers > m_outputBufferSurfaceMap.size() )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfOutputBuffers; i++ )
        {
            for ( size_t k = 0; k < m_numOfInputs; k++ )
            {
                bufferAddr = (void *) ( (uintptr_t) pOutputBuffer[i].data() + k * outputSize );
                if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
                {
                    m_outputBufferSurfaceMap.erase( bufferAddr );
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::createSurface( uint32_t *surfaceId, uint32_t batchIdx,
                                   const RideHal_SharedBuffer_t *pSharedBuffer, bool isSource )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint32_t width = pSharedBuffer->imgProps.width;
    uint32_t height = pSharedBuffer->imgProps.height;
    RideHal_ImageFormat_e format = pSharedBuffer->imgProps.format;

    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_RGB888:
        case RIDEHAL_IMAGE_FORMAT_BGR888:
            ret = createRGBSurface( surfaceId, batchIdx, pSharedBuffer, isSource );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to create RGB Surface\n" );
            }
            break;
        case RIDEHAL_IMAGE_FORMAT_UYVY:
        case RIDEHAL_IMAGE_FORMAT_NV12:
        case RIDEHAL_IMAGE_FORMAT_P010:
            ret = ret = createYUVSurface( surfaceId, batchIdx, pSharedBuffer, isSource );
            if ( RIDEHAL_ERROR_NONE != ret )
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

RideHalError_e C2D::createYUVSurface( uint32_t *surfaceId, uint32_t batchIdx,
                                      const RideHal_SharedBuffer_t *pSharedBuffer, bool isSource )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    RideHal_ImageFormat_e format = pSharedBuffer->imgProps.format;
    C2D_YUV_SURFACE_DEF surfaceDef;

    memset( &surfaceDef, 0, sizeof( C2D_YUV_SURFACE_DEF ) );

    if ( isSource )
    {
        bufferAddr = pSharedBuffer->data();
    }
    else
    {
        uint32_t outputSize = pSharedBuffer->size / pSharedBuffer->imgProps.batchSize;
        bufferAddr = (void *) ( (uintptr_t) pSharedBuffer->data() + batchIdx * outputSize );
    }

    surfaceDef.plane0 = bufferAddr;
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.height = pSharedBuffer->imgProps.height;
    surfaceDef.width = pSharedBuffer->imgProps.width;
    surfaceDef.stride0 = pSharedBuffer->imgProps.stride[0];
    surfaceDef.stride1 = pSharedBuffer->imgProps.stride[1];
    surfaceDef.phys0 = (void *) 1;   // any nonzero value
    surfaceDef.phys1 = (void *) 1;   // any nonzero value

    switch ( surfaceDef.format )
    {
        case C2D_COLOR_FORMAT_420_NV12:
        case C2D_COLOR_FORMAT_420_P010:
            surfaceDef.plane1 =
                    (void *) ( (uint8_t *) surfaceDef.plane0 +
                               surfaceDef.stride0 * pSharedBuffer->imgProps.actualHeight[0] );
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
        ret = RIDEHAL_ERROR_FAIL;
        RIDEHAL_ERROR( "Failed to create %s YUV surface, format: %d, width: %u, height: %u, "
                       "c2dStatus wrong",
                       isSource ? "source" : "target", (int) format, surfaceDef.width,
                       surfaceDef.height );
    }

    return ret;
}

RideHalError_e C2D::createRGBSurface( uint32_t *surfaceId, uint32_t batchIdx,
                                      const RideHal_SharedBuffer_t *pSharedBuffer, bool isSource )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    RideHal_ImageFormat_e format = pSharedBuffer->imgProps.format;
    C2D_RGB_SURFACE_DEF surfaceDef;

    memset( &surfaceDef, 0, sizeof( C2D_RGB_SURFACE_DEF ) );

    if ( isSource )
    {
        bufferAddr = pSharedBuffer->data();
    }
    else
    {
        uint32_t outputSize = pSharedBuffer->size / pSharedBuffer->imgProps.batchSize;
        bufferAddr = (void *) ( (uintptr_t) pSharedBuffer->data() + batchIdx * outputSize );
    }

    surfaceDef.buffer = bufferAddr;
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.height = pSharedBuffer->imgProps.height;
    surfaceDef.width = pSharedBuffer->imgProps.width;
    surfaceDef.stride = pSharedBuffer->imgProps.stride[0];
    surfaceDef.phys = (void *) 1;   // any nonzero value
    auto c2dStatus = c2dCreateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ),
            (void *) &surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDEHAL_ERROR_FAIL;
        RIDEHAL_ERROR( "Failed to create %s RGB surface, format: %d, width: %u, height: %u, "
                       "c2dStatus wrong",
                       isSource ? "source" : "target", (int) format, surfaceDef.width,
                       surfaceDef.height );
    }

    return ret;
}

uint32_t C2D::GetC2DFormatType( RideHal_ImageFormat_e format )
{
    uint32_t c2dFormat = (uint32_t) RIDEHAL_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
            c2dFormat = C2D_COLOR_FORMAT_422_UYVY;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12:
            c2dFormat = C2D_COLOR_FORMAT_420_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            c2dFormat = C2D_COLOR_FORMAT_420_P010;
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB | C2D_FORMAT_SWAP_ENDIANNESS );
            break;
        case RIDEHAL_IMAGE_FORMAT_BGR888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB );
            break;
        default:
            RIDEHAL_ERROR( "Unsupported C2D image format" );
            break;
    }

    return c2dFormat;
}

}   // namespace component
}   // namespace ridehal

