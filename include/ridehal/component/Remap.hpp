// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_REMAP_HPP_
#define _RIDEHAL_REMAP_HPP_

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasSrv.hpp"
#include "fadas.h"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::FadasIface;

namespace ridehal
{
namespace component
{

/// @brief ridehal::component
///
/// Remap Interface

typedef struct
{
    float *pMapX; /*pointer for X map*/
    float *pMapY; /*pointer for Y map*/
} Remap_MapTable_t;

typedef struct
{
    RideHal_ImageFormat_e inputFormat; /*input image format*/
    uint32_t inputWidth;               /*input format width*/
    uint32_t inputHeight;              /*input format height*/
    uint32_t mapWidth;                 /*output map width*/
    uint32_t mapHeight;                /*output map height*/
    Remap_MapTable_t remapTable;       /*remap table, used if enable undistortion*/
    FadasROI_t ROI;                    /*region of interest structure of Fadas*/
} Remap_InputConfig_t;

typedef struct
{
    RideHal_ProcessorType_e processor;                     /*pipelie processor type*/
    Remap_InputConfig_t inputConfigs[RIDEHAL_MAX_INPUTS]; /*input images configuration*/
    uint32_t numOfInputs;                                  /*number of input images*/
    uint32_t outputWidth;                                  /*output image width*/
    uint32_t outputHeight;                                 /*output image height*/
    RideHal_ImageFormat_e outputFormat;                    /*output image format*/
    FadasNormlzParams_t normlzR;                           /*normalize parameter for R channel*/
    FadasNormlzParams_t normlzG;                           /*normalize parameter for G channel*/
    FadasNormlzParams_t normlzB;                           /*normalize parameter for B channel*/
    bool bEnableUndistortion;                              /*enable undistortion or not*/
    bool bEnableNormalize;                                 /*enable normalization or not*/
} Remap_Config_t;

class Remap : public ComponentIF
{
public:
    Remap();
    ~Remap();

    /// @brief Initialize the remap pipeline
    /// @param pName the remap unique instance name
    /// @param pConfig the remap configuration paramaters
    /// @param level the logger message level
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const Remap_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /// @brief Register buffer for remap
    /// @param pBuffers a list of buffers to be registeer
    /// @param numBuffers number of buffers
    /// @param bufferType buffer type, could be IN, OUT, INOUT
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e RegBuf( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                           FadasBufType_e bufferType );

    /// @brief Deregister buffer for remap
    /// @param pBuffers a list of buffers to be deregister
    /// @param numBuffers number of buffers
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e DeregBuf( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /// @brief Start the remap pipeline
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the remap pipeline
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the remap pipeline
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

    /// @brief execute
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutput the output shared buffers
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );

private:
    FadasRemap m_fadasRemapObj;
    Remap_Config_t m_config;

};   // class Remap

}   // namespace component
}   // namespace ridehal

#endif   // _RIDEHAL_REMAP_HPP_
