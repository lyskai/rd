// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <cinttypes>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "C2D_Impl.hpp"
#include "ridehal/component/C2D.hpp"

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

std::vector<std::unique_ptr<C2DImpl>> g_Impls;

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
        RIDEHAL_ERROR( "ComponentIF::Init failed" );
    }

    else
    {
        m_batchSize = pConfig->batchSize;

        /* Initialize input parameters */
        for ( uint32_t i = 0; i < m_batchSize; i++ )
        {
            std::array<uint32_t, 2> inputRes;
            inputRes[0] = pConfig->inputConfigs[i].inputResolution.width;
            inputRes[1] = pConfig->inputConfigs[i].inputResolution.height;
            m_inputResolutions.push_back( inputRes );
            m_inputFormats.push_back( pConfig->inputConfigs[i].inputFormat );
            size_t inputSize = (size_t) ( m_inputResolutions[i][0] * m_inputResolutions[i][1] *
                                          GetFormatDepthSize( m_inputFormats[i] ) );
            m_inputSizes.push_back( inputSize );

            std::array<uint32_t, 4> inputROI;
            inputROI[0] = pConfig->inputConfigs->ROI.topX;
            inputROI[1] = pConfig->inputConfigs->ROI.topY;
            inputROI[2] = pConfig->inputConfigs->ROI.width;
            inputROI[3] = pConfig->inputConfigs->ROI.height;
            m_rois.push_back( inputROI );
        }

        /* Initialize output parameters */
        m_outputResolution[0] = pConfig->outputResolution.width;
        m_outputResolution[1] = pConfig->outputResolution.height;
        m_outputFormat = pConfig->outputFormat;
        uint32_t stride = m_outputResolution[0] * GetFormatDepthSize( m_outputFormat );
        m_stride = ALIGN_S( stride, m_align );
        m_outputSize = (size_t) ( m_stride * m_outputResolution[1] * m_batchSize );

        /* Initialize C2DImpl parameters */
        for ( uint32_t i = 0; i < m_batchSize; i++ )
        {
            std::unique_ptr<C2DImpl> c2dImpl = std::make_unique<C2DImpl>();
            ret = c2dImpl->init( m_inputResolutions[i], m_inputFormats[i], m_outputResolution,
                                 m_outputFormat, m_rois[i], m_align );
            if ( ret != RIDE_HAL_ERROR_NONE )
            {
                RIDEHAL_ERROR( "Failed to initialize C2DImpl for input %u\n", i );
                return ret;
            }
            g_Impls.push_back( std::move( c2dImpl ) );
        }

        /* Complete initialization */
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
        RIDEHAL_INFO( "Component C2D is initialized\n" );
    }

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

    /* Deinitialize C2DImpl vector */
    for ( uint32_t i = 0; i < m_batchSize; i++ )
    {
        ret = g_Impls[i]->deInit();
        if ( RIDE_HAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "g_Impls %u is deinitialized\n", i );
        }
    }

    /* Complete initialization */
    m_state = RIDE_HAL_COMPONENT_STATE_READY;
    RIDEHAL_INFO( "Component C2D is initialized\n" );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // DO deinit
        m_inputResolutions.clear();
        m_inputFormats.clear();
        m_inputSizes.clear();
        m_rois.clear();
        g_Impls.clear();

        m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;
        RIDEHAL_INFO( "Component C2D is deinitialized\n" );
    }

    return ret;
}

RideHalError_e C2D::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                             const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( numInputs != m_batchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "batch size not correct: %u != %u\n", m_batchSize, numInputs );
        return ret;
    }

    for ( size_t i = 0; i < numInputs; i++ )
    {
        void *inputDataPtr = pInputs[i].data();
        void *outputDataPtr = (void *) ( (uintptr_t) pOutputs[i].data() + i * m_outputSize );
        if ( m_inputFormats[i] != pInputs[i].imgProps.format )
        {
            ret = RIDE_HAL_ERROR_TYPE;
            RIDEHAL_ERROR( "Input %u format not correct, expected: %d , input: %d\n ", i,
                           (int) m_inputFormats[i], (int) pInputs[i].imgProps.format );
            return ret;
        }

        ret = g_Impls[i]->draw( inputDataPtr, outputDataPtr );
        if ( ret != RIDE_HAL_ERROR_NONE )
        {
            RIDEHAL_ERROR( "Failed to draw blit objects for input %U\n: ", i );
            return ret;
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal

