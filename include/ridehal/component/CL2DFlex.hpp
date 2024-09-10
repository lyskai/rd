// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2DFLEX_HPP
#define RIDEHAL_CL2DFLEX_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "OpenclIface.hpp"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::OpenclIface;

namespace ridehal
{
namespace component
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief CL2DFlex component configuration */
typedef struct
{
    size_t inputWidth;                  /**<input image width, an integer multiple of 2*/
    size_t inputHeight;                 /**<input image height, an integer multiple of 2*/
    size_t outputWidth;                 /**<output image width*/
    size_t outputHeight;                /**<output image height*/
    RideHal_ImageFormat_e inputFormat;  /**<input image format*/
    RideHal_ImageFormat_e outputFormat; /**<output image format*/
} CL2DFlex_Config_t;

class CL2DFlex : public ComponentIF
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    CL2DFlex();
    ~CL2DFlex();

    /**
     * @brief Initialize the CL2DFlex pipeline
     * @param[in] pName the CL2DFlex unique instance name
     * @param[in] pConfig the CL2DFlex configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the initialization work for a CL2DFlex pipeline: parse configuration parameters,
     * setup OpenCL command queue and context, load OpenCL kernel and build OpenCL program. Must be
     * called at the beginning of pipeline.
     */
    RideHalError_e Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for CL2DFlex
     * @param[in] pBuffers buffers to be registered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Register device buffers from host buffers for input and output data. This step can be
     * done by user or skipped. If skipped, all the buffers will be registered at execute step.
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the CL2DFlex pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Execute the CL2DFlex pipeline
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Execute the CL2DFlex pipeline. Currently support color conversion and resize of single
     * image input to single output image. The supported color conversion pipelines are NV12 to RGB,
     * UYVY to RGB, UYVY to NV12.
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInput,
                            const RideHal_SharedBuffer_t *pOutput );

    /**
     * @brief Stop the CL2DFlex pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deregister buffers for CL2DFlex
     * @param[in] pBuffers buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Deregister device buffers from host buffers for input and output data. This step can
     * be done by user or skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the CL2DFlex pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the deinitialization work for a CL2DFlex pipeline: release OpenCL context,
     * command queue, kernel and program, deregister all the OpenCL buffers remained. Must be
     * called at the ending of pipeline.
     */
    RideHalError_e Deinit();

private:
    CL2DFlex_Config_t m_config;
    OpenclSrv m_OpenclSrvObj;

private:
    RideHalError_e ConvertFromNV12ToRGB( cl_mem bufferSrc, cl_mem bufferDst,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToRGB( cl_mem bufferSrc, cl_mem bufferDst,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToNV12( cl_mem bufferSrc, cl_mem bufferDst,
                                          const RideHal_SharedBuffer_t *pInput,
                                          const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromNV12ToRGB( cl_mem bufferSrc, cl_mem bufferDst,
                                        const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToRGB( cl_mem bufferSrc, cl_mem bufferDst,
                                        const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToNV12( cl_mem bufferSrc, cl_mem bufferDst,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );

};   // class CL2DFlex

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_CL2DFLEX_HPP
