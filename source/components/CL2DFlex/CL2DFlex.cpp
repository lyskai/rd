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
        else if ( RIDEHAL_MAX_INPUTS < pConfig->numOfInputs )
        {
            RIDEHAL_ERROR( "Invalid inputs number!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            for ( uint32_t inputId = 0; inputId < pConfig->numOfInputs; inputId++ )
            {
                if ( ( 0 != ( pConfig->inputWidths[inputId] % 2 ) ) ||
                     ( 0 == pConfig->inputWidths[inputId] ) )
                {
                    RIDEHAL_ERROR( "Invalid input width!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( 0 != ( pConfig->inputHeights[inputId] % 2 ) ) ||
                          ( 0 == pConfig->inputHeights[inputId] ) )
                {
                    RIDEHAL_ERROR( "Invalid input height!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputWidths[inputId] <
                          ( pConfig->ROIs[inputId].x + pConfig->ROIs[inputId].width ) )
                {
                    RIDEHAL_ERROR( "Invalid ROI values, (ROI.x + ROI.width) > inputWidth!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputHeights[inputId] <
                          ( pConfig->ROIs[inputId].y + pConfig->ROIs[inputId].height ) )
                {
                    RIDEHAL_ERROR( "Invalid ROI values, (ROI.y + ROI.height) > inputHeight!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
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
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceCL2DFlex );
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Load program from source file s_pSourceCL2DFlex failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < pConfig->numOfInputs; inputId++ )
                {
                    if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                         ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                    {
                        if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                             ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ConvertNV12ToRGB" );
                        }
                        else
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ResizeNV12ToRGB" );
                        }
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                    {
                        if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                             ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ConvertUYVYToRGB" );
                        }
                        else
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ResizeUYVYToRGB" );
                        }
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
                    {
                        if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                             ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ConvertUYVYToNV12" );
                        }
                        else
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ResizeUYVYToNV12" );
                        }
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Invalid CL2DFlex pipeline for inputId=%d!", inputId );
                        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    }
                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Create kernel failed for inputId=%d!", inputId );
                        break;
                    }
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
            ret = m_OpenclSrvObj.RegBuf( &( pBuffers[i].buffer ), &bufferCL );
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
            ret = m_OpenclSrvObj.DeregBuf( &( pBuffers[i].buffer ) );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister buffer for number %d!", i );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel,
                                               cl_mem bufferSrc, uint32_t srcOffset,
                                               cl_mem bufferDst, uint32_t dstOffset,
                                               const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 10;
    OpenclIfcae_Arg_t OpenclArgs[10];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[7].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[inputId].y / 2;
    OpenclArgs[8].pArg = (void *) &( kernelROIX );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( kernelROIY );
    OpenclArgs[9].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( pOutput->imgProps.width ) / 2,
                                 ( pOutput->imgProps.height ) / 2 };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromUYVYToRGB( uint32_t inputId, cl_kernel *pKernel,
                                               cl_mem bufferSrc, uint32_t srcOffset,
                                               cl_mem bufferDst, uint32_t dstOffset,
                                               const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 8;
    OpenclIfcae_Arg_t OpenclArgs[8];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[inputId].y;
    OpenclArgs[6].pArg = (void *) &( kernelROIX );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( kernelROIY );
    OpenclArgs[7].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( pOutput->imgProps.width ) / 2, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert UYVY to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::ConvertFromUYVYToNV12( uint32_t inputId, cl_kernel *pKernel,
                                                cl_mem bufferSrc, uint32_t srcOffset,
                                                cl_mem bufferDst, uint32_t dstOffset,
                                                const RideHal_SharedBuffer_t *pInput,
                                                const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 10;
    OpenclIfcae_Arg_t OpenclArgs[10];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[1] );
    OpenclArgs[7].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[inputId].y / 2;
    OpenclArgs[8].pArg = (void *) &( kernelROIX );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( kernelROIY );
    OpenclArgs[9].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( pOutput->imgProps.width ) / 2,
                                 ( pOutput->imgProps.height ) / 2 };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert UYVY to NV12 OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel,
                                              cl_mem bufferSrc, uint32_t srcOffset,
                                              cl_mem bufferDst, uint32_t dstOffset,
                                              const RideHal_SharedBuffer_t *pInput,
                                              const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 14;
    OpenclIfcae_Arg_t OpenclArgs[14];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[inputId].x / m_config.ROIs[inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[inputId].y / m_config.ROIs[inputId].height * m_config.outputHeight;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromUYVYToRGB( uint32_t inputId, cl_kernel *pKernel,
                                              cl_mem bufferSrc, uint32_t srcOffset,
                                              cl_mem bufferDst, uint32_t dstOffset,
                                              const RideHal_SharedBuffer_t *pInput,
                                              const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 12;
    OpenclIfcae_Arg_t OpenclArgs[12];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[inputId].x / m_config.ROIs[inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[inputId].y / m_config.ROIs[inputId].height * m_config.outputHeight;
    OpenclArgs[10].pArg = (void *) &( kernelROIX );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( kernelROIY );
    OpenclArgs[11].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromUYVYToNV12( uint32_t inputId, cl_kernel *pKernel,
                                               cl_mem bufferSrc, uint32_t srcOffset,
                                               cl_mem bufferDst, uint32_t dstOffset,
                                               const RideHal_SharedBuffer_t *pInput,
                                               const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 16;
    OpenclIfcae_Arg_t OpenclArgs[16];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[1] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[inputId].x / m_config.ROIs[inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[inputId].y / m_config.ROIs[inputId].height * m_config.outputHeight;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );
    OpenclArgs[14].pArg = (void *) &( pOutput->imgProps.height );
    OpenclArgs[14].argSize = sizeof( cl_int );
    OpenclArgs[15].pArg = (void *) &( pOutput->imgProps.width );
    OpenclArgs[15].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DFlex::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                  const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not initialized!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pOutput )
    {
        RIDEHAL_ERROR( "Output buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInputs )
    {
        RIDEHAL_ERROR( "Input buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( numInputs != m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "number of inputs not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        RIDEHAL_ERROR( "Output buffer is not image type!" );
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
    else if ( numInputs != pOutput->imgProps.batchSize )
    {
        RIDEHAL_ERROR( "Output image batch not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        cl_mem bufferDst;
        ret = m_OpenclSrvObj.RegBuf( &( pOutput->buffer ), &bufferDst );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register output buffer!" );
        }
        else
        {
            for ( uint32_t inputId = 0; inputId < numInputs; inputId++ )
            {
                if ( nullptr == pInputs[inputId].data() )
                {
                    RIDEHAL_ERROR( "Input buffer data is null for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pInputs[inputId].type )
                {
                    RIDEHAL_ERROR( "Input buffer is not image type for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( m_config.inputFormats[inputId] != pInputs[inputId].imgProps.format )
                {
                    RIDEHAL_ERROR( "Input image format not match for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( m_config.inputWidths[inputId] != pInputs[inputId].imgProps.width )
                {
                    RIDEHAL_ERROR( "Input image width not match for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( m_config.inputHeights[inputId] != pInputs[inputId].imgProps.height )
                {
                    RIDEHAL_ERROR( "Input image height not match for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else
                {
                    cl_mem bufferSrc;
                    ret = m_OpenclSrvObj.RegBuf( &( pInputs[inputId].buffer ), &bufferSrc );
                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Failed to register input buffer for inputId=%d!", inputId );
                    }
                    else
                    {
                        uint32_t srcOffset = pInputs[inputId].offset;
                        uint32_t sizeOne = ( pOutput->size ) / ( pOutput->imgProps.batchSize );
                        uint32_t dstOffset = pOutput->offset + inputId * sizeOne;
                        if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                             ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                        {
                            if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                                 ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                            {
                                ret = ConvertFromNV12ToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                            srcOffset, bufferDst, dstOffset,
                                                            &pInputs[inputId], pOutput );
                            }
                            else
                            {
                                ret = ResizeFromNV12ToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                           srcOffset, bufferDst, dstOffset,
                                                           &pInputs[inputId], pOutput );
                            }
                        }

                        else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                                  ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
                        {
                            if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                                 ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                            {
                                ret = ConvertFromUYVYToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                            srcOffset, bufferDst, dstOffset,
                                                            &pInputs[inputId], pOutput );
                            }
                            else
                            {
                                ret = ResizeFromUYVYToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                           srcOffset, bufferDst, dstOffset,
                                                           &pInputs[inputId], pOutput );
                            }
                        }

                        else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                                  ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
                        {
                            if ( ( m_config.ROIs[inputId].width == m_config.outputWidth ) &&
                                 ( m_config.ROIs[inputId].height == m_config.outputHeight ) )
                            {
                                ret = ConvertFromUYVYToNV12( inputId, &m_kernel[inputId], bufferSrc,
                                                             srcOffset, bufferDst, dstOffset,
                                                             &pInputs[inputId], pOutput );
                            }
                            else
                            {
                                ret = ResizeFromUYVYToNV12( inputId, &m_kernel[inputId], bufferSrc,
                                                            srcOffset, bufferDst, dstOffset,
                                                            &pInputs[inputId], pOutput );
                            }
                        }
                        else
                        {
                            RIDEHAL_ERROR( "Invalid CL2DFlex pipeline!" );
                            RIDEHAL_ERROR_BAD_ARGUMENTS;
                        }
                    }
                }
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to run OpenCL kernel for inputId=%d!", inputId );
                    break;
                }
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
