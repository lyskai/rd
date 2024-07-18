// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

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
    size_t inputWidth;                  /**<input image width*/
    size_t inputHeight;                 /**<input image height*/
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
     * @cond CL2DFlex::Init @endcond
     * @brief Initialize the CL2DFlex pipeline
     * @param[in] pName the CL2DFlex unique instance name
     * @param[in] pConfig the CL2DFlex configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @cond CL2DFlex::Start @endcond
     * @brief Start the CL2DFlex pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @cond CL2DFlex::Stop @endcond
     * @brief Stop the CL2DFlex pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @cond CL2DFlex::Deinit @endcond
     * @brief Deinitialize the CL2DFlex pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @cond CL2DFlex::RegisterBuffers @endcond
     * @brief Register buffers for CL2DFlex
     * @param[in] pBuffers buffers to be registered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @cond CL2DFlex::DeRegisterBuffers @endcond
     * @brief Deregister buffers for CL2DFlex
     * @param[in] pBuffers buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @cond CL2DFlex::Execute @endcond
     * @brief Execute the CL2DFlex pipeline
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInput,
                            const RideHal_SharedBuffer_t *pOutput );

private:
    CL2DFlex_Config_t m_config;
    OpenclSrv m_OpenclSrvObj;

private:
    RideHalError_e ConvertFromNV12ToRGB( const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToRGB( const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ConvertFromUYVYToNV12( const RideHal_SharedBuffer_t *pInput,
                                          const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromNV12ToRGB( const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToRGB( const RideHal_SharedBuffer_t *pInput,
                                        const RideHal_SharedBuffer_t *pOutput );
    RideHalError_e ResizeFromUYVYToNV12( const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput );

};   // class CL2DFlex

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_CL2DFLEX_HPP
