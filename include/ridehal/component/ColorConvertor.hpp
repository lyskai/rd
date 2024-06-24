// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_COLORCONVERTOR_HPP
#define RIDEHAL_COLORCONVERTOR_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "ColorConvertor.cl.h"
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

/** @brief ColorConvertor component configuration */
typedef struct
{
    uint32_t inputWidth;                /**<input image width*/
    uint32_t inputHeight;               /**<input image height*/
    RideHal_ImageFormat_e inputFormat;  /**<input image format*/
    RideHal_ImageFormat_e outputFormat; /**<output image format*/
} ColorConvertor_Config_t;

class ColorConvertor : public ComponentIF
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    ColorConvertor();
    ~ColorConvertor();

    /**
     * @cond ColorConvertor::Init @endcond
     * @brief Initialize the ColorConvertor pipeline
     * @param[in] pName the ColorConvertor unique instance name
     * @param[in] pConfig the ColorConvertor configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const ColorConvertor_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @cond ColorConvertor::Start @endcond
     * @brief Start the ColorConvertor pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @cond ColorConvertor::Stop @endcond
     * @brief Stop the ColorConvertor pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @cond ColorConvertor::Deinit @endcond
     * @brief Deinitialize the ColorConvertor pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @cond ColorConvertor::RegisterBuffers @endcond
     * @brief Register buffers for ColorConvertor
     * @param[in] pBuffers buffers to be registered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @cond ColorConvertor::DeRegisterBuffers @endcond
     * @brief Deregister buffers for ColorConvertor
     * @param[in] pBuffers buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @cond ColorConvertor::Execute @endcond
     * @brief Execute the ColorConvertor pipeline
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInput,
                            const RideHal_SharedBuffer_t *pOutput );

private:
    ColorConvertor_Config_t m_config;
    OpenclSrv m_OpenclSrvObj;

private:
    RideHalError_e FromNV12ToRGB( const RideHal_SharedBuffer_t *pInput,
                                  const RideHal_SharedBuffer_t *pOutput );

};   // class ColorConvertor

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_COLORCONVERTOR_HPP
