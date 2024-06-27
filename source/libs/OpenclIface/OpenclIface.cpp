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
        retCL = clGetDeviceIDs( m_platformID, CL_DEVICE_TYPE_GPU, 1, &m_deviceID, NULL );
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
    if ( retCL != CL_SUCCESS )
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
            size_t len;
            (void) clGetProgramBuildInfo( m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL,
                                          &len );
            std::vector<char> logs;
            logs.resize( len );
            char *pBuffer = logs.data();
            if ( nullptr != pBuffer )
            {
                (void) clGetProgramBuildInfo( m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, len,
                                              pBuffer, NULL );
                RIDEHAL_INFO( "build log:\n %s\n", pBuffer );
            }
            else
            {
                RIDEHAL_ERROR( "Unable to get build log!" );
            }
            logs.clear();
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

RideHalError_e OpenclSrv::LoadFromBinary( const unsigned char *pBinaryFile,
                                          const char *pKernelName )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    m_program = clCreateProgramWithBinary( m_context, 1, &m_deviceID, NULL, &pBinaryFile, NULL,
                                           &retCL );
    if ( retCL != CL_SUCCESS )
    {
        RIDEHAL_ERROR( "Unable to create program with binary, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
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

    std::vector<void *> ptrs;
    for ( auto &it : m_memMap )
    {
        ptrs.push_back( it.first );
    }
    for ( auto ptr : ptrs )
    {
        ret = DeregBuf( ptr );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Unable to deregister buffer %d", ptr );
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
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        auto it = m_memMap.find( pBufferHost );
        if ( it == m_memMap.end() )
        {
            cl_mem_pmem_host_ptr clBufHostPtr = { 0 };
            clBufHostPtr.pmem_handle = 0;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_PMEM_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_IOCOHERENT_QCOM;
            clBufHostPtr.pmem_hostptr = pBufferHost;
            cl_mem bufferCL =
                    clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM, size,
                                    &clBufHostPtr, &retCL );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to create CL buffer, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                m_memMap[pBufferHost] = { bufferCL };
                *pBufferCL = bufferCL;
            }
        }
        else
        {
            *pBufferCL = it->second.clMem;
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
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        auto it = m_memMap.find( pBufferHost );
        if ( it != m_memMap.end() )
        {
            retCL = clReleaseMemObject( it->second.clMem );
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
