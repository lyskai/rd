// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_OPENCL_IFACE_HPP
#define RIDEHAL_OPENCL_IFACE_HPP

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_qcom.h>
#include <map>
#include <vector>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"
using namespace ridehal::common;

namespace ridehal
{
namespace libs
{
namespace OpenclIface
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief OpenCL execute arguments structure */
typedef struct
{
    void *pArg;     /**argument pointer*/
    size_t argSize; /**argument size*/
} OpenclIfcae_Arg_t;

/** @brief OpenCL execute work parameters structure */
typedef struct
{
    size_t *pGlobalWorkOffset; /**global work offset*/
    size_t *pGlobalWorkSize;   /**global work size*/
    size_t *pLocalWorkSize;    /**local work size*/
    size_t workDim;            /**work dimensions*/
} OpenclIface_WorkParams_t;

/** @brief OpenCL memory info structure */
typedef struct
{
    cl_mem clMem; /**OpenCL memory buffer*/
} OpenclIface_MemInfo_t;

class OpenclSrv
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    /**
     * @brief Initialize the OpenclIface object
     * @param[in] pName the OpenclIface unique instance name
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the initialization work for a OpenclIface object: get platform ID, get device
     * ID, create context, create command queue, get device info. Must be called at the beginning of
     * pipeline.
     */
    RideHalError_e Init( const char *pName, Logger_Level_e level );

    /**
     * @brief Load OpenCL program from source file
     * @param[in] pSourceFile the OpenCL program source file string pointer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Create and build OpenCL program from source file, print build log if compilation
     * fail. In this way the kernel code is easy to read and modify. Must be called after Init.
     */
    RideHalError_e LoadFromSource( const char *pSourceFile );

    /**
     * @brief Load OpenCL program from binary file
     * @param[in] pBinaryFile the OpenCL program binary file string pointer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Create and build OpenCL program from prebuilt binary file. In this way the compilation
     * time is saved. Must be called after Init.
     */
    RideHalError_e LoadFromBinary( const unsigned char *pBinaryFile );

    /**
     * @brief Create OpenCL kernel from OpenCL program
     * @param[in] pKernel the OpenCL kernel pointer
     * @param[in] pKernelName the OpenCL kernel name string pointer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Create OpenCL kernel from OpenCL program and store to kernel map. Can be called
     * multiple times to create multiple kernels from a single program.
     */
    RideHalError_e CreateKernel( cl_kernel *pKernel, const char *pKernelName );

    /**
     * @brief Deinitialize the OpenclIface object
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the deinitialization work for a OpenclIface object: release kernel, release
     * program, release context, release command queue, release memory map. Must be called at the
     * ending of pipeline.
     */
    RideHalError_e Deinit();

    /**
     * @brief Register the OpenclIface buffer
     * @param[in] pBuffer the RideHal buffer pointer to register
     * @param[in] pBufferCL the OpenCL memory buffer pointer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Create an OpenCL memory buffer and register the host RideHal buffer to it in zero
     * memory copy method. Then store the OpenCL memory buffer to buffer map, so the same host
     * buffer would not be registered twice.
     */
    RideHalError_e RegBuf( const RideHal_Buffer_t *pBuffer, cl_mem *pBufferCL );

    /**
     * @brief Deregister the OpenclIface buffer
     * @param[in] pBuffer the RideHal buffer pointer to deregister
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Release the OpenCL memory buffer corresponding to the host RideHal buffer and erase it
     * in the buffer map.
     */
    RideHalError_e DeregBuf( const RideHal_Buffer_t *pBuffer );

    /**
     * @brief Execute the OpenclIface kernel
     * @param[in] pKernel the OpenCL kernel pointer
     * @param[in] pArgs the arguments structure pointer for certain kernel
     * @param[in] numOfArgs the arguments number
     * @param[in] pWorkParam the work parameters structure pointer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Set arguments for an OpenCL kernel and execute the kernel in GPU device. The number of
     * arguments must correspond to kernel codes.
     */
    RideHalError_e Execute( const cl_kernel *pKernel, const OpenclIfcae_Arg_t *pArgs,
                            size_t numOfArgs, const OpenclIface_WorkParams_t *pWorkParam );


private:
    cl_platform_id m_platformID;                      /**OpenCL platform ID*/
    cl_device_id m_deviceID;                          /**OpenCL device ID*/
    cl_command_queue m_commandQueue;                  /**OpenCL command queue*/
    cl_context m_context;                             /**OpenCL context*/
    cl_program m_program;                             /**OpenCL program*/
    std::map<void *, OpenclIface_MemInfo_t> m_memMap; /**OpenCL memory map*/
    std::map<std::string, cl_kernel> m_kernelMap;     /**OpenCL kernel map*/

protected:
    RIDEHAL_DECLARE_LOGGER();
};

}   // namespace OpenclIface
}   // namespace libs
}   // namespace ridehal

#endif   // RIDEHAL_OPENCL_IFACE_HPP
