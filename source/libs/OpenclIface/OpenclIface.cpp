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

    if ( CL_SUCCESS == retCL )
    {
        size_t versionSize = 0;
        (void) clGetDeviceInfo( m_deviceID, CL_DEVICE_VERSION, 0, NULL, &versionSize );
        std::vector<char> version( versionSize );
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_VERSION, versionSize, version.data(), NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL version is %s", version.data() );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device version info, retCL = %d", retCL );
        }

        size_t extensionSize = 0;
        (void) clGetDeviceInfo( m_deviceID, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );
        std::vector<char> extensions( extensionSize );
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_EXTENSIONS, extensionSize, extensions.data(),
                                 NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL extension is %s", extensions.data() );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device extensions info, retCL = %d", retCL );
        }

        cl_uint unit;
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( cl_uint ), &unit,
                                 NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL max compute unit is %d\n", unit );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device max compute units info, retCL = %d", retCL );
        }

        size_t workSizes[3];
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof( size_t ) * 3,
                                 workSizes, NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL max work item sizes is {%d, %d, %d}", workSizes[0], workSizes[1],
                          workSizes[2] );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device max work item sizes info, retCL = %d", retCL );
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
                RIDEHAL_ERROR( "error build log:\n %s\n", pBuffer );
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

    retCL = clReleaseCommandQueue( m_commandQueue );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release command queue, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    retCL = clReleaseContext( m_context );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release context, retCL = %d", retCL );
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

RideHalError_e OpenclSrv::RegBuf( void *pBufferHost, size_t size, uint64_t handle,
                                  cl_mem *pBufferCL )
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
#if defined( __QNXNTO__ )
            cl_mem_pmem_host_ptr clBufHostPtr = { 0 };
            clBufHostPtr.pmem_handle = (uintptr_t) handle;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_PMEM_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_IOCOHERENT_QCOM;
            clBufHostPtr.pmem_hostptr = pBufferHost;
            cl_mem bufferCL =
                    clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM, size,
                                    &clBufHostPtr, &retCL );
#else
            cl_mem_dmabuf_host_ptr clBufHostPtr = { 0 };
            clBufHostPtr.dmabuf_filedesc = (int) handle;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_DMABUF_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
            clBufHostPtr.dmabuf_hostptr = pBufferHost;
            cl_mem bufferCL =
                    clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM, size,
                                    &clBufHostPtr, &retCL );
#endif

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
