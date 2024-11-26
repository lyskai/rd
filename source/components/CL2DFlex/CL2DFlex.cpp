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
                else
                {
                    // empty else block
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
                         ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                         ( CL2DFLEX_WORK_MODE_CONVERT == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ConvertNV12ToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ResizeNV12ToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_CONVERT == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ConvertUYVYToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ResizeUYVYToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_CONVERT == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                           "ConvertUYVYToNV12" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ResizeRGBToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId], "ResizeUYVYToNV12" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST ==
                                m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] = CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB;
                        ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                           "LetterboxNV12ToRGB" );
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE ==
                                m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] =
                                CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB_MULTIPLE;
                        RideHal_TensorProps_t roiProp = {
                                RIDEHAL_TENSOR_TYPE_INT_32,
                                { ( uint32_t )( RIDEHAL_CL2DFLEX_ROI_NUMBER_MAX * 4 ), 0 },
                                1,
                        };
                        ret = m_roiBuffer.Allocate( &roiProp );
                        if ( RIDEHAL_ERROR_NONE != ret )
                        {
                            RIDEHAL_ERROR( "Failed to allocate roi buffer!" );
                        }
                        else
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "LetterboxNV12ToRGBMultiple" );
                        }
                    }

                    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[inputId] ) &&
                              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) &&
                              ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE ==
                                m_config.workModes[inputId] ) )
                    {
                        m_pipelines[inputId] =
                                CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB_MULTIPLE;
                        RideHal_TensorProps_t roiProp = {
                                RIDEHAL_TENSOR_TYPE_INT_32,
                                { ( uint32_t )( RIDEHAL_CL2DFLEX_ROI_NUMBER_MAX * 4 ), 0 },
                                1,
                        };
                        ret = m_roiBuffer.Allocate( &roiProp );
                        if ( RIDEHAL_ERROR_NONE != ret )
                        {
                            RIDEHAL_ERROR( "Failed to allocate roi buffer!" );
                        }
                        else
                        {
                            ret = m_OpenclSrvObj.CreateKernel( &m_kernel[inputId],
                                                               "ResizeNV12ToRGBMultiple" );
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

        if ( nullptr != m_roiBuffer.buffer.pData )
        {
            retVal = m_roiBuffer.Free();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deallocate roi buffer!" );
            }
        }

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
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 ( size_t )( pOutput->imgProps.height ) / 2 };
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
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 pOutput->imgProps.height };
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
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 ( size_t )( pOutput->imgProps.height ) / 2 };
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

RideHalError_e CL2DFlex::LetterboxFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel,
                                                 cl_mem bufferSrc, uint32_t srcOffset,
                                                 cl_mem bufferDst, uint32_t dstOffset,
                                                 const RideHal_SharedBuffer_t *pInput,
                                                 const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 17;
    OpenclIfcae_Arg_t OpenclArgs[17];
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
    uint32_t kernelROIX = m_config.ROIs[inputId].x;
    uint32_t kernelROIY = m_config.ROIs[inputId].y;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );
    float inputRatio = (float) m_config.ROIs[inputId].height / (float) m_config.ROIs[inputId].width;
    float outputRatio = (float) ( pOutput->imgProps.height ) / (float) ( pOutput->imgProps.width );
    OpenclArgs[14].pArg = (void *) &( inputRatio );
    OpenclArgs[14].argSize = sizeof( cl_float );
    OpenclArgs[15].pArg = (void *) &( outputRatio );
    OpenclArgs[15].argSize = sizeof( cl_float );
    OpenclArgs[16].pArg = (void *) &( m_config.letterboxPaddingValue );
    OpenclArgs[16].argSize = sizeof( cl_int );

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

RideHalError_e CL2DFlex::ResizeFromRGBToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                             uint32_t srcOffset, cl_mem bufferDst,
                                             uint32_t dstOffset,
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

RideHalError_e CL2DFlex::LetterboxFromNV12ToRGBMultiple( uint32_t numROIs, cl_kernel *pKernel,
                                                         cl_mem bufferSrc, uint32_t srcOffset,
                                                         cl_mem bufferDst, uint32_t dstOffset,
                                                         const RideHal_SharedBuffer_t *pInput,
                                                         const RideHal_SharedBuffer_t *pOutput,
                                                         const CL2DFlex_ROIConfig_t *pROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    for ( int i = 0; i < numROIs; i++ )
    {
        if ( ( ( pROIs[i].x + pROIs[i].width ) <= pInput->imgProps.width ) &&
             ( ( pROIs[i].y + pROIs[i].height ) <= pInput->imgProps.height ) )
        {
            ( (int *) m_roiBuffer.data() )[i * 4 + 0] = pROIs[i].x;
            ( (int *) m_roiBuffer.data() )[i * 4 + 1] = pROIs[i].y;
            ( (int *) m_roiBuffer.data() )[i * 4 + 2] = pROIs[i].width;
            ( (int *) m_roiBuffer.data() )[i * 4 + 3] = pROIs[i].height;
        }
        else
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Invalid roi parameter for inputId=%d\n!", i );
            break;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        cl_mem roiBufferCL;
        ret = m_OpenclSrvObj.RegBuf( &( m_roiBuffer.buffer ), &roiBufferCL );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register roi buffer!" );
        }
        else
        {
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
            OpenclArgs[4].pArg = (void *) &roiBufferCL;
            OpenclArgs[4].argSize = sizeof( cl_mem );
            OpenclArgs[5].pArg = (void *) &( m_config.outputHeight );
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( m_config.outputWidth );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );
            OpenclArgs[8].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
            OpenclArgs[8].argSize = sizeof( cl_int );
            OpenclArgs[9].pArg = (void *) &( pInput->imgProps.stride[1] );
            OpenclArgs[9].argSize = sizeof( cl_int );
            OpenclArgs[10].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[10].argSize = sizeof( cl_int );
            OpenclArgs[11].pArg = (void *) &( m_config.letterboxPaddingValue );
            OpenclArgs[11].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 3;
            size_t globalWorkSize[3] = { numROIs, pOutput->imgProps.width,
                                         pOutput->imgProps.height };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[3] = { 0, 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }

            ret = m_OpenclSrvObj.DeregBuf( &( m_roiBuffer.buffer ) );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister roi buffer!" );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ResizeFromNV12ToRGBMultiple( uint32_t numROIs, cl_kernel *pKernel,
                                                      cl_mem bufferSrc, uint32_t srcOffset,
                                                      cl_mem bufferDst, uint32_t dstOffset,
                                                      const RideHal_SharedBuffer_t *pInput,
                                                      const RideHal_SharedBuffer_t *pOutput,
                                                      const CL2DFlex_ROIConfig_t *pROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    for ( int i = 0; i < numROIs; i++ )
    {
        if ( ( ( pROIs[i].x + pROIs[i].width ) <= pInput->imgProps.width ) &&
             ( ( pROIs[i].y + pROIs[i].height ) <= pInput->imgProps.height ) )
        {
            ( (int *) m_roiBuffer.data() )[i * 4 + 0] = pROIs[i].x;
            ( (int *) m_roiBuffer.data() )[i * 4 + 1] = pROIs[i].y;
            ( (int *) m_roiBuffer.data() )[i * 4 + 2] = pROIs[i].width;
            ( (int *) m_roiBuffer.data() )[i * 4 + 3] = pROIs[i].height;
        }
        else
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Invalid roi parameter for inputId=%d\n!", i );
            break;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        cl_mem roiBufferCL;
        ret = m_OpenclSrvObj.RegBuf( &( m_roiBuffer.buffer ), &roiBufferCL );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register roi buffer!" );
        }
        else
        {
            size_t numOfArgs = 11;
            OpenclIfcae_Arg_t OpenclArgs[11];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &srcOffset;
            OpenclArgs[1].argSize = sizeof( cl_int );
            OpenclArgs[2].pArg = (void *) &bufferDst;
            OpenclArgs[2].argSize = sizeof( cl_mem );
            OpenclArgs[3].pArg = (void *) &dstOffset;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &roiBufferCL;
            OpenclArgs[4].argSize = sizeof( cl_mem );
            OpenclArgs[5].pArg = (void *) &( m_config.outputHeight );
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( m_config.outputWidth );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );
            OpenclArgs[8].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
            OpenclArgs[8].argSize = sizeof( cl_int );
            OpenclArgs[9].pArg = (void *) &( pInput->imgProps.stride[1] );
            OpenclArgs[9].argSize = sizeof( cl_int );
            OpenclArgs[10].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[10].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 3;
            size_t globalWorkSize[3] = { numROIs, pOutput->imgProps.width,
                                         pOutput->imgProps.height };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[3] = { 0, 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }

            ret = m_OpenclSrvObj.DeregBuf( &( m_roiBuffer.buffer ) );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister roi buffer!" );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::Execute( const RideHal_SharedBuffer_t *pInputs, const uint32_t numInputs,
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
                        uint32_t sizeOne =
                                ( uint32_t )( pOutput->size ) / ( pOutput->imgProps.batchSize );
                        uint32_t dstOffset = ( uint32_t )( pOutput->offset ) + inputId * sizeOne;
                        if ( CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB == m_pipelines[inputId] )
                        {
                            ret = ConvertFromNV12ToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                        srcOffset, bufferDst, dstOffset,
                                                        &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB ==
                                  m_pipelines[inputId] )
                        {
                            ret = ResizeFromNV12ToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                       srcOffset, bufferDst, dstOffset,
                                                       &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB == m_pipelines[inputId] )
                        {
                            ret = ConvertFromUYVYToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                        srcOffset, bufferDst, dstOffset,
                                                        &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB ==
                                  m_pipelines[inputId] )
                        {
                            ret = ResizeFromUYVYToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                       srcOffset, bufferDst, dstOffset,
                                                       &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12 == m_pipelines[inputId] )
                        {
                            ret = ConvertFromUYVYToNV12( inputId, &m_kernel[inputId], bufferSrc,
                                                         srcOffset, bufferDst, dstOffset,
                                                         &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12 ==
                                  m_pipelines[inputId] )
                        {
                            ret = ResizeFromUYVYToNV12( inputId, &m_kernel[inputId], bufferSrc,
                                                        srcOffset, bufferDst, dstOffset,
                                                        &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB ==
                                  m_pipelines[inputId] )
                        {
                            ret = LetterboxFromNV12ToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                          srcOffset, bufferDst, dstOffset,
                                                          &pInputs[inputId], pOutput );
                        }
                        else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB ==
                                  m_pipelines[inputId] )
                        {
                            ret = ResizeFromRGBToRGB( inputId, &m_kernel[inputId], bufferSrc,
                                                      srcOffset, bufferDst, dstOffset,
                                                      &pInputs[inputId], pOutput );
                        }
                        else
                        {
                            RIDEHAL_ERROR( "Invalid CL2DFlex pipeline for inputId=%d!", inputId );
                            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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

RideHalError_e CL2DFlex::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput,
                                         const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs )
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
    else if ( nullptr == pROIs )
    {
        RIDEHAL_ERROR( "ROI configurations is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 1 != m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "number of inputs not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput->data() )
    {
        RIDEHAL_ERROR( "Output buffer data is null" );
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
    else if ( numROIs != pOutput->imgProps.batchSize )
    {
        RIDEHAL_ERROR( "Output image batch not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInput->data() )
    {
        RIDEHAL_ERROR( "Input buffer data is null" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pInput->type )
    {
        RIDEHAL_ERROR( "Input buffer is not image type" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputFormats[0] != pInput->imgProps.format )
    {
        RIDEHAL_ERROR( "Input image format not match" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputWidths[0] != pInput->imgProps.width )
    {
        RIDEHAL_ERROR( "Input image width not match" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputHeights[0] != pInput->imgProps.height )
    {
        RIDEHAL_ERROR( "Input image height not match" );
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
            cl_mem bufferSrc;
            ret = m_OpenclSrvObj.RegBuf( &( pInput->buffer ), &bufferSrc );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to register input buffer" );
            }
            else
            {
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    if ( CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB_MULTIPLE ==
                         m_pipelines[0] )
                    {
                        uint32_t srcOffset = pInput->offset;
                        uint32_t dstOffset = pOutput->offset;
                        ret = LetterboxFromNV12ToRGBMultiple( numROIs, &m_kernel[0], bufferSrc,
                                                              srcOffset, bufferDst, dstOffset,
                                                              pInput, pOutput, pROIs );
                    }
                    else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB_MULTIPLE ==
                              m_pipelines[0] )
                    {
                        uint32_t srcOffset = pInput->offset;
                        uint32_t dstOffset = pOutput->offset;
                        ret = ResizeFromNV12ToRGBMultiple( numROIs, &m_kernel[0], bufferSrc,
                                                           srcOffset, bufferDst, dstOffset, pInput,
                                                           pOutput, pROIs );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Invalid CL2DFlex pipeline!" );
                        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    }

                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Failed to run OpenCL kernel!" );
                    }
                }
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
