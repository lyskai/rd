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

/** @brief CL2DFlex valid pipelines */
typedef enum
{
    CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB,  /**<color convert only from nv12 to rgb, roi.width
                                               should be equal to output width and roi.height should
                                               be equal to output height*/
    CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB,  /**<color convert only from uyvy to rgb, roi.width
                                                should be equal to output width and roi.height should
                                                be equal to output height*/
    CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12, /**<color convert only from uyvy to nv12, roi.width
                                               should be equal to output width and roi.height should
                                               be equal to output height*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB,    /**<color convert and resize use nearest point
                                                        from nv12 to rgb*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB,    /**<color convert and resize use nearest point
                                                        from uyvy to rgb*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12,   /**<color convert and resize use nearest point
                                                        from uyvy to nv12*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB,     /**<resize use nearest point from rgb to rgb*/
    CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB, /**<color convert and letterbox with fixed
                                                        height/width ratio use nearest point from
                                                        nv12 to rgb, padding 0 to the redundant
                                                        bottom or right edge*/
    CL2DFLEX_PIPELINE_MAX
} CL2DFlex_Pipeline_e;

/** @brief CL2DFlex valid work modes */
typedef enum
{
    CL2DFLEX_WORK_MODE_CONVERT, /**<color convert only, roi.width should be equal to output width
                                  and roi.height should be equal to output height*/
    CL2DFLEX_WORK_MODE_RESIZE_NEAREST,    /**<color convert and resize use nearest point*/
    CL2DFLEX_WORK_MODE_RESIZE_BILINEAR,   /**<color convert and resize use bilinear interpolation*/
    CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST, /**<color convert and letterbox with fixed height/width
                                            ratio use nearest point*/
    CL2DFLEX_WORK_MODE_MAX
} CL2DFlex_Work_Mode_e;

/** @brief CL2DFlex input images ROI configuration */
typedef struct
{
    uint32_t x;      /**<ROI beginnning x coordinate*/
    uint32_t y;      /**<ROI beginnning y coordinate*/
    uint32_t width;  /**<ROI width, x+width must be smaller than
                        input width*/
    uint32_t height; /**<ROI height, y+height must be smaller
                        than input height*/
} CL2DFlex_ROIConfig_t;

/** @brief CL2DFlex component configuration */
typedef struct
{
    uint32_t numOfInputs;                                   /**<number of input images*/
    CL2DFlex_Work_Mode_e workModes[RIDEHAL_MAX_INPUTS];     /**<work mode for each input*/
    RideHal_ImageFormat_e inputFormats[RIDEHAL_MAX_INPUTS]; /**<input image format for each batch*/
    uint32_t inputWidths[RIDEHAL_MAX_INPUTS];      /**<input image width for each batch, must be
                                                    an integer   multiple of 2*/
    uint32_t inputHeights[RIDEHAL_MAX_INPUTS];     /**<input image height for each batch, must be an
                                                    integer  multiple of 2*/
    RideHal_ImageFormat_e outputFormat;            /**<output image format*/
    uint32_t outputWidth;                          /**<output image width*/
    uint32_t outputHeight;                         /**<output image height*/
    CL2DFlex_ROIConfig_t ROIs[RIDEHAL_MAX_INPUTS]; /**<ROI configurations for each batch*/
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
     * @brief Execute the CL2DFlex pipeline normally
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Execute the CL2DFlex pipeline. Currently support color conversion and resize of
     * multiple image inputs to single output. The supported color conversion pipelines are NV12 to
     * RGB, UYVY to RGB, UYVY to NV12.
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, const uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );

    /**
     * @brief Execute the CL2DFlex pipeline with ROI parameters
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @param[in] pROIs the ROI configurations for each execution
     * @param[in] numROIs the number of ROI configuration parameters, also equal to the batch number
     * of output buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Execute the CL2DFlex pipeline with ROI parameters, one input buffer to one output
     * buffer with multiple batches, used for cases such as traffic light detection.
     */
    RideHalError_e ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                   const RideHal_SharedBuffer_t *pOutput,
                                   const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

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
    cl_kernel m_kernel[RIDEHAL_MAX_INPUTS];
    CL2DFlex_Pipeline_e m_pipelines[RIDEHAL_MAX_INPUTS]; /**<input image format for each batch*/

private:
    RideHalError_e ConvertFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                         uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                         uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToNV12( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                          uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                          const RideHal_SharedBuffer_t *pInput,
                                          const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                        uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                        const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                        uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                        const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToNV12( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                         uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                         const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromRGBToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                       uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                       const RideHal_SharedBuffer_t *pInput,
                                       const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e LetterboxFromNV12ToRGB( uint32_t inputId, cl_kernel *pKernel, cl_mem bufferSrc,
                                           uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
                                           const RideHal_SharedBuffer_t *pInput,
                                           const RideHal_SharedBuffer_t *pOutput );

};   // class CL2DFlex

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_CL2DFLEX_HPP
