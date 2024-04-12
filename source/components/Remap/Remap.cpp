// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ridehal/component/Remap.hpp"

#include <map>
#include <vector>

namespace ridehal
{
namespace component
{

Remap::Remap() {}

Remap::~Remap() {}

RideHalError_e Remap::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( RIDE_HAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDE_HAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        m_state = RIDE_HAL_COMPONENT_STATE_ERROR;
        RIDEHAL_ERROR( "Remap component start failed due to wrong state!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    return ret;
}

RideHalError_e Remap::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_COMPONENT_STATE_RUNNING == m_state ) ||
         ( RIDE_HAL_COMPONENT_STATE_ERROR == m_state ) )
    {
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
    }
    else
    {
        m_state = RIDE_HAL_COMPONENT_STATE_ERROR;
        RIDEHAL_ERROR( "Remap component stop failed due to wrong state!" );
        ret = RIDE_HAL_ERROR_STATE;
    }

    return ret;
}

RideHalError_e Remap::Init( const char *pName, const Remap_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init component!" );
    }
    else if ( nullptr == pConfig )
    {
        RIDEHAL_ERROR( "Empty config pointer!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        m_config = *pConfig;
        ret = m_fadasRemapObj.Init( m_config.processor, pName, level );
    }

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init fadas remap!" );
    }
    else
    {
        ret = m_fadasRemapObj.SetRemapParams(
                m_config.numOfInputs, m_config.outputWidth, m_config.outputHeight,
                m_config.outputFormat, m_config.normlzR, m_config.normlzG, m_config.normlzB,
                m_config.bEnableUndistortion, m_config.bEnableNormalize );
    }

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to set parameters!" );
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
        {
            ret = m_fadasRemapObj.CreateRemapWorker( inputId,
                                                     m_config.inputConfigs[inputId].inputFormat,
                                                     m_config.inputConfigs[inputId].inputWidth,
                                                     m_config.inputConfigs[inputId].inputHeight,
                                                     m_config.inputConfigs[inputId].ROI );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Create worker fail at inputId = %d", inputId );
                break;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
        {
            ret = m_fadasRemapObj.CreatRemapTable(
                    inputId, m_config.inputConfigs[inputId].mapWidth,
                    m_config.inputConfigs[inputId].mapHeight,
                    m_config.inputConfigs[inputId].remapTable.pMapX,
                    m_config.inputConfigs[inputId].remapTable.pMapY );
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Create remap table fail at inputId = %d", inputId );
                break;
            }
        }
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_state = RIDE_HAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e Remap::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_fadasRemapObj.Deinit();

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to deinit fadas remap!" );
    }
    else
    {
        ret = ComponentIF::Deinit();
    }

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to deinit component!" );
    }

    return ret;
}

RideHalError_e Remap::RegBuf( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                              FadasBufType_e bufferType )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not in ready or running status!" );
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numBuffers; inputId++ )
        {
            const RideHal_SharedBuffer_t *input = &pBuffers[inputId];
            int32_t fd = m_fadasRemapObj.RegBuf( input, bufferType );
            if ( 0 > fd )
            {
                RIDEHAL_ERROR( "Failed to register buffer for inputId = %d!", inputId );
                ret = RIDE_HAL_ERROR_FAIL;
                break;
            }
        }
    }

    return ret;
}

RideHalError_e Remap::DeregBuf( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not in ready or running status!" );
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDE_HAL_ERROR_NULL_PTR;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numBuffers; inputId++ )
        {
            m_fadasRemapObj.DeregBuf( pBuffers[inputId].data() );
        }
    }

    return ret;
}

RideHalError_e Remap::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                               const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( ( RIDE_HAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDE_HAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not initialized!" );
        ret = RIDE_HAL_ERROR_STATE;
    }
    else if ( m_config.numOfInputs != numInputs )
    {
        RIDEHAL_ERROR( "Number of input buffers not equal to config value!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 1 != numOutputs )
    {
        RIDEHAL_ERROR( "Number of output buffers not equal to 1!" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        ret = m_fadasRemapObj.RemapRun( pInputs, pOutputs );
    }

    if ( RIDE_HAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to run remap component!" );
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal