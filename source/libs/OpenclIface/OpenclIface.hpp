//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#ifndef QRIDE_OPENCL_SRV_HPP
#define QRIDE_OPENCL_SRV_HPP

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_qcom.h>
#include <map>

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

class OpenclSrv
{
public:
    RideHalError_e Init( const char *pName, Logger_Level_e level );
    RideHalError_e LoadFromSource( const char *pSourceFile, const char *pKernelName );
    RideHalError_e LoadFromBinary( const char *pBinaryFile, const char *pKernelName );
    RideHalError_e Deinit();
    RideHalError_e RegBuf( void *pBufferHost, size_t size, cl_mem *pBufferCL );
    RideHalError_e DeregBuf( void *pBufferHost );
    RideHalError_e Execute( const OpenclIfcae_Arg_t *pArgs, size_t numOfArgs,
                            const OpenclIface_WorkParams_t *pWorkParam );


private:
    cl_platform_id m_platformID;
    cl_device_id m_deviceID;
    cl_command_queue m_commandQueue;
    cl_context m_context;
    cl_kernel m_kernel;
    cl_program m_program;
    std::string m_kernelName;
    std::string m_sourceFile;
    std::string m_binaryFile;
    std::map<void *, cl_mem> m_memMap;

protected:
    RIDEHAL_DECLARE_LOGGER();
};

}   // namespace OpenclIface
}   // namespace libs
}   // namespace ridehal

#endif   // QRIDE_OPENCL_SRV_HPP
