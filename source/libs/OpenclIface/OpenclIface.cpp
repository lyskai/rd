//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "OpenclIface.hpp"

namespace ridehal
{
namespace libs
{
namespace OpenclIface
{

RideHalError_e OpenclSrv::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to create logger for OpenclSrv %s: ret = %d\n",
                        pName, ret );
        ret = RIDEHAL_ERROR_NONE; /* ignore logger init error */
    }

    retCL = clGetPlatformIDs( 1, &m_platformID, NULL );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to get Platforms, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( CL_SUCCESS == retCL )
    {
        clGetDeviceIDs( m_platformID, CL_DEVICE_TYPE_GPU, 1, &m_deviceID, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to get OpenCL compatible GPU device, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        m_context = clCreateContext( NULL, 1, &m_deviceID, NULL, NULL, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create context, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        m_commandQueue = clCreateCommandQueueWithProperties( m_context, m_deviceID, NULL, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create command queue, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::LoadFromSource( const char *pSourceFile, const char *pKernelName )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    m_program =
            clCreateProgramWithSource( m_context, 1, (const char **) &pSourceFile, NULL, &retCL );
    if ( ret != CL_SUCCESS )
    {
        RIDEHAL_ERROR( "Unable to create program with source, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        m_sourceFile = pSourceFile;
    }

    if ( CL_SUCCESS == retCL )
    {
        retCL = clBuildProgram( m_program, 1, &m_deviceID, NULL, NULL, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to build program, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        m_kernel = clCreateKernel( m_program, pKernelName, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_kernelName = pKernelName;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::LoadFromBinary( const char *pBinaryFile, const char *pKernelName )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    m_program = clCreateProgramWithBinary( m_context, 1, &m_deviceID, NULL,
                                           (const unsigned char **) &pBinaryFile, NULL, &retCL );
    if ( ret != CL_SUCCESS )
    {
        RIDEHAL_ERROR( "Unable to create program with binary, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        m_binaryFile = pBinaryFile;
    }

    if ( CL_SUCCESS == retCL )
    {
        retCL = clBuildProgram( m_program, 1, &m_deviceID, NULL, NULL, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to build program, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        m_kernel = clCreateKernel( m_program, pKernelName, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_kernelName = pKernelName;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    retCL = clReleaseKernel( m_kernel );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release kernel, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    retCL = clReleaseProgram( m_program );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release program, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    for ( auto &it : m_memMap )
    {
        ret = DeregBuf( it.first );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Unable to deregister buffer %d", it.first );
        }
    }
    m_memMap.clear();

    ret = RIDEHAL_LOGGER_DEINIT();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for OpenclSrv: ret = %d\n",
                        ret );
        ret = RIDEHAL_ERROR_NONE; /* ignore logger deinit error */
    }

    return ret;
}

RideHalError_e OpenclSrv::RegBuf( void *pBufferHost, size_t size, cl_mem *pBufferCL )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    if ( nullptr == pBufferHost )
    {
        RIDEHAL_ERROR( "null host buffer!" );
    }
    else
    {
        auto it = m_memMap.find( pBufferHost );
        if ( it == m_memMap.end() )
        {
            *pBufferCL = clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                                         size, pBufferHost, &retCL );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to create CL buffer, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                m_memMap[pBufferHost] = *pBufferCL;
            }
        }
        else
        {
            *pBufferCL = it->second;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::DeregBuf( void *pBufferHost )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    if ( nullptr == pBufferHost )
    {
        RIDEHAL_ERROR( "null host buffer!" );
    }
    else
    {
        auto it = m_memMap.find( pBufferHost );
        if ( it != m_memMap.end() )
        {
            retCL = clReleaseMemObject( it->second );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to release CL buffer, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                (void) m_memMap.erase( it );
            }
        }
    }

    return ret;
}


RideHalError_e OpenclSrv::Execute( const OpenclIfcae_Arg_t *pArgs, size_t numOfArgs,
                                   const OpenclIface_WorkParams_t *pWorkParam )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    for ( int i = 0; i < numOfArgs; i++ )
    {
        retCL = clSetKernelArg( m_kernel, i, pArgs[i].argSize, pArgs[i].pArg );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to set number %d argument, retCL = %d", i, retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retCL = clEnqueueNDRangeKernel( m_commandQueue, m_kernel, pWorkParam->workDim,
                                        pWorkParam->pGlobalWorkOffset, pWorkParam->pGlobalWorkSize,
                                        pWorkParam->pLocalWorkSize, 0, NULL, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to enqueue range kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            retCL = clFinish( m_commandQueue );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to finish command queue, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

}   // namespace OpenclIface
}   // namespace libs
}   // namespace ridehal
