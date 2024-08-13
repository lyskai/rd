// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/CL2DFlex.hpp"
#include "CL2DFlex.cl.h"

namespace ridehal
{
namespace component
{

CL2DFlex::CL2DFlex() {}

CL2DFlex::~CL2DFlex() {}

RideHalError_e CL2DFlex::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "CL2DFlex component start failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e CL2DFlex::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "CL2DFlex component stop failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e CL2DFlex::Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                               Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init component!" );
    }
    else
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;

        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "Empty config pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( RIDEHAL_IMAGE_FORMAT_RGB888 != pConfig->outputFormat ) &&
                  ( RIDEHAL_IMAGE_FORMAT_NV12 != pConfig->outputFormat ) )
        {
            RIDEHAL_ERROR( "Invalid output format!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 != pConfig->inputFormat ) &&
                  ( RIDEHAL_IMAGE_FORMAT_UYVY != pConfig->inputFormat ) )
        {
            RIDEHAL_ERROR( "Invalid input format!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( 0 != ( pConfig->inputWidth % 2 ) ) || ( 0 == pConfig->inputWidth ) )
        {
            RIDEHAL_ERROR( "Invalid input width!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( 0 != ( pConfig->inputHeight % 2 ) ) || ( 0 == pConfig->inputHeight ) )
        {
            RIDEHAL_ERROR( "Invalid input height!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            m_config = *pConfig;
            ret = m_OpenclSrvObj.Init( pName, level );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Init OpenCL failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormat ) &&
                     ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                {
                    if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                         ( m_config.inputHeight == m_config.outputHeight ) )
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceConvertNV12ToRGB,
                                                             "ConvertNV12ToRGB" );
                    }
                    else
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceResizeNV12ToRGB,
                                                             "ResizeNV12ToRGB" );
                    }

                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Load kernel from source for NV12 to RGB failed!" );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }

                else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormat ) &&
                          ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                {
                    if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                         ( m_config.inputHeight == m_config.outputHeight ) )
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceConvertUYVYToRGB,
                                                             "ConvertUYVYToRGB" );
                    }
                    else
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceResizeUYVYToRGB,
                                                             "ResizeUYVYToRGB" );
                    }

                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Load kernel from source for UYVY to RGB failed!" );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }

                else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormat ) &&
                          ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
                {
                    if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                         ( m_config.inputHeight == m_config.outputHeight ) )
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceConvertUYVYToNV12,
                                                             "ConvertUYVYToNV12" );
                    }
                    else
                    {
                        ret = m_OpenclSrvObj.LoadFromSource( s_pSourceResizeUYVYToNV12,
                                                             "ResizeUYVYToNV12" );
                    }

                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Load kernel from source for UYVY to NV12 failed!" );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }
                else
                {
                    RIDEHAL_ERROR( "Invalid CL2DFlex pipeline!" );
                    ret = RIDEHAL_ERROR_FAIL;
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
            RideHalError_e retVal;
            retVal = ComponentIF::Deinit();
            if ( RIDEHAL_ERROR_NONE != retVal )
            {
                RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        RideHalError_e retVal;

        retVal = m_OpenclSrvObj.Deinit();
        if ( RIDEHAL_ERROR_NONE != retVal )
        {
            RIDEHAL_ERROR( "Release CL resources failed!" );
            ret = RIDEHAL_ERROR_FAIL;
        }

        retVal = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != retVal )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}


RideHalError_e CL2DFlex::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                          uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready or running status!" );
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
            cl_mem bufferCL;
            ret = m_OpenclSrvObj.RegBuf( pBuffers[i].data(), pBuffers[i].size,
                                         pBuffers[i].buffer.dmaHandle, &bufferCL );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to register buffer for number %d!", i );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                            uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready or running status!" );
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
            ret = m_OpenclSrvObj.DeregBuf( pBuffers[i].data() );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister buffer for number %d!", i );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromNV12ToRGB( const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 8;
            OpenclIfcae_Arg_t OpenclArgs[8];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &( pInput->imgProps.actualHeight[0] );
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[1] );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.inputWidth / 2, m_config.inputHeight / 2 };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromUYVYToRGB( const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 6;
            OpenclIfcae_Arg_t OpenclArgs[8];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[5].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.inputWidth / 2, m_config.inputHeight };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert UYVY to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromUYVYToNV12( const RideHal_SharedBuffer_t *pInput,
                                                const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 8;
            OpenclIfcae_Arg_t OpenclArgs[8];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( pOutput->imgProps.actualHeight[0] );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[1] );
            OpenclArgs[7].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.inputWidth / 2, m_config.inputHeight / 2 };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert UYVY to NV12 OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromNV12ToRGB( const RideHal_SharedBuffer_t *pInput,
                                              const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 10;
            OpenclIfcae_Arg_t OpenclArgs[10];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &m_config.outputHeight;
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &m_config.outputWidth;
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pInput->imgProps.actualHeight[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );
            OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[1] );
            OpenclArgs[8].argSize = sizeof( cl_int );
            OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[9].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.outputWidth, m_config.outputHeight };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromUYVYToRGB( const RideHal_SharedBuffer_t *pInput,
                                              const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 8;
            OpenclIfcae_Arg_t OpenclArgs[8];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &m_config.outputHeight;
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &m_config.outputWidth;
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.outputWidth, m_config.outputHeight };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromUYVYToNV12( const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferSrc;
    cl_mem bufferDst;
    ret = m_OpenclSrvObj.RegBuf( pInput->data(), pInput->size, pInput->buffer.dmaHandle,
                                 &bufferSrc );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input buffer!" );
    }
    else
    {
        ret = m_OpenclSrvObj.RegBuf( pOutput->data(), pOutput->size, pOutput->buffer.dmaHandle,
                                     &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            size_t numOfArgs = 10;
            OpenclIfcae_Arg_t OpenclArgs[10];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &bufferDst;
            OpenclArgs[1].argSize = sizeof( cl_mem );
            OpenclArgs[2].pArg = (void *) &m_config.inputHeight;
            OpenclArgs[2].argSize = sizeof( cl_int );
            OpenclArgs[3].pArg = (void *) &m_config.inputWidth;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &m_config.outputHeight;
            OpenclArgs[4].argSize = sizeof( cl_int );
            OpenclArgs[5].pArg = (void *) &m_config.outputWidth;
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );
            OpenclArgs[8].pArg = (void *) &( pOutput->imgProps.actualHeight[0] );
            OpenclArgs[8].argSize = sizeof( cl_int );
            OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[1] );
            OpenclArgs[9].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 2;
            size_t globalWorkSize[2] = { m_config.outputWidth, m_config.outputHeight };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[2] = { 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::Execute( const RideHal_SharedBuffer_t *pInput,
                                  const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not initialized!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pInput )
    {
        RIDEHAL_ERROR( "Input buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput )
    {
        RIDEHAL_ERROR( "Output buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pInput->type )
    {
        RIDEHAL_ERROR( "Input buffer is not image type!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        RIDEHAL_ERROR( "Output buffer is not image type!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputFormat != pInput->imgProps.format )
    {
        RIDEHAL_ERROR( "Input image format not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputWidth != pInput->imgProps.width )
    {
        RIDEHAL_ERROR( "Input image width not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputHeight != pInput->imgProps.height )
    {
        RIDEHAL_ERROR( "Input image height not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputFormat != pOutput->imgProps.format )
    {
        RIDEHAL_ERROR( "Output image format not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputWidth != pOutput->imgProps.width )
    {
        RIDEHAL_ERROR( "Output image width not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputHeight != pOutput->imgProps.height )
    {
        RIDEHAL_ERROR( "Output image height not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormat ) &&
             ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
        {
            if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                 ( m_config.inputHeight == m_config.outputHeight ) )
            {
                ret = ConvertFromNV12ToRGB( pInput, pOutput );
            }
            else
            {
                ret = ResizeFromNV12ToRGB( pInput, pOutput );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to run OpenCL kernel from NV12 to RGB!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }

        else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormat ) &&
                  ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
        {
            if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                 ( m_config.inputHeight == m_config.outputHeight ) )
            {
                ret = ConvertFromUYVYToRGB( pInput, pOutput );
            }
            else
            {
                ret = ResizeFromUYVYToRGB( pInput, pOutput );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to run OpenCL kernel from UYVY to RGB!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }

        else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormat ) &&
                  ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
        {
            if ( ( m_config.inputWidth == m_config.outputWidth ) &&
                 ( m_config.inputHeight == m_config.outputHeight ) )
            {
                ret = ConvertFromUYVYToNV12( pInput, pOutput );
            }
            else
            {
                ret = ResizeFromUYVYToNV12( pInput, pOutput );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to run OpenCL kernel from UYVY to NV12!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "Invalid CL2DFlex pipeline!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
