// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/component/Voxelization.hpp"

namespace ridehal
{
namespace component
{

Voxelization::Voxelization() {}

Voxelization::~Voxelization() {}

RideHalError_e Voxelization::Init( const char *pName, const Voxelization_Config_t *pConfig,
                                   Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    bool bIFInitOK = false;
    bool bFadasInitOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bIFInitOK = true;
        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "pConfig is nullptr!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_plrPre.Init( pConfig->processor, pName, level );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to init FadasPlrPre!" );
        }
        else
        {
            bFadasInitOK = TRUE;
            ret = m_plrPre.SetParams( pConfig->pillarXSize, pConfig->pillarYSize,
                                      pConfig->pillarZSize, pConfig->minXRange, pConfig->minYRange,
                                      pConfig->minZRange, pConfig->maxXRange, pConfig->maxYRange,
                                      pConfig->maxZRange, pConfig->maxNumInPts,
                                      pConfig->numInFeatureDim, pConfig->maxNumPlrs,
                                      pConfig->maxNumPtsPerPlr, pConfig->numOutFeatureDim );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_plrPre.CreatePreProc();
    }


    if ( ret != RIDEHAL_ERROR_NONE )
    { /* do error clean up */
        RIDEHAL_ERROR( "PlrPre Init failed: %d!", ret );

        if ( bFadasInitOK )
        {
            m_plrPre.Deinit();
        }

        if ( bIFInitOK )
        {
            (void) ComponentIF::Deinit();
        }
    }
    else
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
        m_config = *pConfig;
    }

    return ret;
}

RideHalError_e Voxelization::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e Voxelization::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e Voxelization::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        RideHalError_e ret2;
        ret2 = m_plrPre.DestroyPreProc();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPre DestroyPreProc failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_plrPre.Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPre Deinit failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

RideHalError_e Voxelization::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                              uint32_t numBuffers, FadasBufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "PlrPre component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            const RideHal_SharedBuffer_t *pBuf = &pBuffers[i];
            if ( RIDEHAL_BUFFER_TYPE_TENSOR == pBuf->type )
            {
                int32_t fd = m_plrPre.RegBuf( pBuf, bufferType );
                if ( 0 > fd )
                {
                    RIDEHAL_ERROR( "Failed to register buffer[%d]!", i );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
            else
            {
                RIDEHAL_ERROR( "buffer[%d] is not tensor!", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e Voxelization::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                                uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "PlrPre component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            const RideHal_SharedBuffer_t *pBuf = &pBuffers[i];
            if ( RIDEHAL_BUFFER_TYPE_TENSOR == pBuf->type )
            {
                m_plrPre.DeregBuf( pBuf->data() );
            }
            else
            {
                RIDEHAL_ERROR( "buffer[%d] is not tensor!", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e Voxelization::Execute( const RideHal_SharedBuffer_t *pInPts,
                                      const RideHal_SharedBuffer_t *pOutPlrs,
                                      const RideHal_SharedBuffer_t *pOutFeature )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pInPts )
    {
        RIDEHAL_ERROR( "pInPts is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pInPts->type ) ||
              ( nullptr == pInPts->buffer.pData ) || ( 2 != pInPts->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pInPts->tensorProps.type ) ||
              ( m_config.numInFeatureDim != pInPts->tensorProps.dims[1] ) )
    {
        RIDEHAL_ERROR( "pInPts is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pOutPlrs )
    {
        RIDEHAL_ERROR( "pOutPlrs is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pOutPlrs->type ) ||
              ( nullptr == pOutPlrs->buffer.pData ) || ( 2 != pOutPlrs->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pOutPlrs->tensorProps.type ) ||
              ( m_config.maxNumPlrs != pOutPlrs->tensorProps.dims[0] ) ||
              ( VOXELIZATION_PILLAR_COORDS_DIM != pOutPlrs->tensorProps.dims[1] ) )
    {
        RIDEHAL_ERROR( "pOutPlrs is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pOutFeature )
    {
        RIDEHAL_ERROR( "pOutFeature is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pOutFeature->type ) ||
              ( nullptr == pOutFeature->buffer.pData ) ||
              ( 3 != pOutFeature->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pOutFeature->tensorProps.type ) ||
              ( m_config.maxNumPlrs != pOutFeature->tensorProps.dims[0] ) ||
              ( m_config.maxNumPtsPerPlr != pOutFeature->tensorProps.dims[1] ) ||
              ( m_config.numOutFeatureDim != pOutFeature->tensorProps.dims[2] ) )
    {
        RIDEHAL_ERROR( "pOutFeature is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else
    {
        ret = m_plrPre.PointPillarRun( pInPts, pOutPlrs, pOutFeature );
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
