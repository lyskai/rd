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
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "Remap component start failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e Remap::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "Remap component stop failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e Remap::Init( const char *pName, const Remap_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init component!" );
    }
    else
    {
        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "Empty config pointer!" );
            ComponentIF::Deinit();
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            m_config = *pConfig;
            m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;
            ret = m_fadasRemapObj.Init( m_config.processor, pName, level );
            if ( RIDEHAL_ERROR_NONE != ret )
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

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to set parameters!" );
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
                {
                    ret = m_fadasRemapObj.CreateRemapWorker(
                            inputId, m_config.inputConfigs[inputId].inputFormat,
                            m_config.inputConfigs[inputId].inputWidth,
                            m_config.inputConfigs[inputId].inputHeight,
                            m_config.inputConfigs[inputId].ROI );
                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        m_fadasRemapObj.DestroyWorkers();
                        RIDEHAL_ERROR( "Create worker fail at inputId = %d", inputId );
                        break;
                    }
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
                {
                    ret = m_fadasRemapObj.CreatRemapTable(
                            inputId, m_config.inputConfigs[inputId].mapWidth,
                            m_config.inputConfigs[inputId].mapHeight,
                            m_config.inputConfigs[inputId].remapTable.pMapX,
                            m_config.inputConfigs[inputId].remapTable.pMapY );
                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        m_fadasRemapObj.DestroyMap();
                        m_fadasRemapObj.DestroyWorkers();
                        RIDEHAL_ERROR( "Create remap table fail at inputId = %d", inputId );
                        break;
                    }
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                m_state = RIDEHAL_COMPONENT_STATE_READY;
            }
            else
            {
                m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
                m_fadasRemapObj.Deinit();
                ComponentIF::Deinit();
            }
        }
    }

    return ret;
}

RideHalError_e Remap::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "Remap component not in ready status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        ret = m_fadasRemapObj.DestroyMap();

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to destroy map!" );
        }
        else
        {
            ret = m_fadasRemapObj.DestroyWorkers();
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to destroy worker!" );
        }
        else
        {
            ret = m_fadasRemapObj.Deinit();
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to deinit fadas remap!" );
        }
        else
        {
            ret = ComponentIF::Deinit();
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to deinit component!" );
        }
    }

    return ret;
}

RideHalError_e Remap::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                                       FadasBufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
        }
    }

    return ret;
}

RideHalError_e Remap::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                         uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Remap component not initialized!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( m_config.numOfInputs != numInputs )
    {
        RIDEHAL_ERROR( "Number of input buffers not equal to config value!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        ret = m_fadasRemapObj.RemapRun( pInputs, pOutput );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to run remap component!" );
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal