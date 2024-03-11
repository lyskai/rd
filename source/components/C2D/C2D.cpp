// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include <cinttypes>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "C2D_Impl.hpp"
#include "ride/hal/C2D.hpp"

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

std::vector<std::unique_ptr<C2DImpl>> g_Impls;

namespace ride
{
namespace hal
{
namespace component
{
using namespace ride::hal;

C2D::C2D() {}

C2D::~C2D() {}

RideHalError_e C2D::Init( const char *pName, const C2D_Config_t *pConfig, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    // ret = ComponentIF::Init( pName, pLogger );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_BatchSize = pConfig->batchSize;

        /* Initialize input parameters */
        for ( uint32_t i = 0; i < m_BatchSize; i++ )
        {
            std::array<uint32_t, 2> inputRes;
            inputRes[0] = pConfig->inputConfigs[i].inputResolution.width;
            inputRes[1] = pConfig->inputConfigs[i].inputResolution.height;
            m_InputResolutions.push_back( inputRes );
            m_InputFormats.push_back( pConfig->inputConfigs[i].inputFormat );
            size_t inputSize = ( size_t )( m_InputResolutions[i][0] * m_InputResolutions[i][1] *
                                           GetFormatDepthSize( m_InputFormats[i] ) );
            m_InputSizes.push_back( inputSize );

            std::array<uint32_t, 4> inputROI;
            inputROI[0] = pConfig->inputConfigs->ROI.topX;
            inputROI[1] = pConfig->inputConfigs->ROI.topY;
            inputROI[2] = pConfig->inputConfigs->ROI.width;
            inputROI[3] = pConfig->inputConfigs->ROI.height;
            m_ROIs.push_back( inputROI );
        }

        /* Initialize output parameters */
        m_OutputResolution[0] = pConfig->outputResolution.width;
        m_OutputResolution[1] = pConfig->outputResolution.height;
        m_OutputFormat = pConfig->outputFormat;
        uint32_t stride = m_OutputResolution[0] * GetFormatDepthSize( m_OutputFormat );
        m_Stride = ALIGN_S( stride, m_Align );
        m_OutputSize = ( size_t )( m_Stride * m_OutputResolution[1] * m_BatchSize );

        /* Initialize C2DImpl parameters */
        for ( uint32_t i = 0; i < m_BatchSize; i++ )
        {
            std::unique_ptr<C2DImpl> c2dImpl = std::make_unique<C2DImpl>();
            ret = c2dImpl->init( m_InputResolutions[i], m_InputFormats[i], m_OutputResolution,
                                 m_OutputFormat, m_ROIs[i], m_Align );
            if ( ret != RIDE_HAL_ERROR_NONE )
            {
                // RIDEHAL_ERROR( "Failed to initialize C2DImpl for input %u\n", i );
                return ret;
            }
            g_Impls.push_back( std::move( c2dImpl ) );
        }

        /* Complete initialization */
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
        // RIDEHAL_INFO( "Component C2D is initialized\n" );
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
        // RIDEHAL_INFO( "Component C2D start to run\n" );
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
        // RIDEHAL_INFO( "Component C2D is stopped\n" );
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

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // DO deinit
        m_InputResolutions.clear();
        m_InputFormats.clear();
        m_InputSizes.clear();
        m_ROIs.clear();
        g_Impls.clear();

        m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;
        // RIDEHAL_INFO( "Component C2D is deinitialized\n" );
    }

    return ret;
}

RideHalError_e C2D::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                             const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( numInputs != m_BatchSize )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        // RIDEHAL_ERROR( "batch size not correct: %u != %u\n", m_BatchSize, numInputs );
        return ret;
    }

    for ( size_t i = 0; i < numInputs; i++ )
    {
        void *inputDataPtr = pInputs[i].data();
        void *outputDataPtr = (void *) ( (uintptr_t) pOutputs[i].data() + i * m_OutputSize );
        if ( m_InputFormats[i] != pInputs[i].imgProps.format )
        {
            ret = RIDE_HAL_ERROR_TYPE;
            // RIDEHAL_ERROR( "Input %u format not correct, expected: %d , input: %d\n ", i,
            //    (int) m_InputFormats[i], (int) pInputs[i].imgProps.format );
            return ret;
        }

        ret = g_Impls[i]->draw( inputDataPtr, outputDataPtr );
        if ( ret != RIDE_HAL_ERROR_NONE )
        {
            // RIDEHAL_ERROR( "Failed to draw blit objects for input %U\n: ", i );
            return ret;
        }
    }

    return ret;
}

}   // namespace component
}   // namespace hal
}   // namespace ride
