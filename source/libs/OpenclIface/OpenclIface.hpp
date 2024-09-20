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

typedef struct
{
    void *pArg;
    size_t argSize;
} OpenclIfcae_Arg_t;

typedef struct
{
    size_t *pGlobalWorkOffset;
    size_t *pGlobalWorkSize;
    size_t *pLocalWorkSize;
    size_t workDim;
} OpenclIface_WorkParams_t;

typedef struct
{
    cl_mem clMem;
} OpenclIface_MemInfo_t;

class OpenclSrv
{
public:
    RideHalError_e Init( const char *pName, Logger_Level_e level );
    RideHalError_e LoadFromSource( const char *pSourceFile );
    RideHalError_e LoadFromBinary( const unsigned char *pBinaryFile );
    RideHalError_e CreateKernel( cl_kernel *pKernel, const char *pKernelName );
    RideHalError_e Deinit();
    RideHalError_e RegBuf( const RideHal_Buffer_t *pBuffer, cl_mem *pBufferCL );
    RideHalError_e DeregBuf( const RideHal_Buffer_t *pBuffer );
    RideHalError_e Execute( cl_kernel *pKernel, const OpenclIfcae_Arg_t *pArgs, size_t numOfArgs,
                            const OpenclIface_WorkParams_t *pWorkParam );


private:
    cl_platform_id m_platformID;
    cl_device_id m_deviceID;
    cl_command_queue m_commandQueue;
    cl_context m_context;
    cl_program m_program;
    std::map<void *, OpenclIface_MemInfo_t> m_memMap;
    std::map<std::string, cl_kernel> m_kernelMap;

protected:
    RIDEHAL_DECLARE_LOGGER();
};

}   // namespace OpenclIface
}   // namespace libs
}   // namespace ridehal

#endif   // RIDEHAL_OPENCL_IFACE_HPP
