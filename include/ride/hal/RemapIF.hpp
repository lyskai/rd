// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_REMAP_IF_HPP_
#define _RIDE_HAL_REMAP_IF_HPP_

#include <cinttypes>
#include <memory>
#include <queue>
#include <thread>
#include <vector>
#include <vdds/pub.hpp>
#include <vdds/sub.hpp>
#include <hogl/area.hpp>
#include <hogl/post.hpp>
#include <inttypes.h>
#include <unistd.h>

#include "ride/hal/ComponentIF.hpp"
#include "ride/hal/BufferManager.hpp"

#include <fadas.h>

namespace ride
{
namespace hal
{
namespace remap
{

/// @brief ride::hal::RemapIF
///
/// Remap Interface
class RemapIF
{
public:
    enum ProcessorType
    {
        DSP0,
        DSP1,
        CPU
    };
 
    struct RemapTable
    {
        float* mapX;
        size_t sizeX;
        float* mapY;
        size_t sizeY;
    };
 
    struct ConfigRemap
    {
        ProcessorType processor;
        RideHal_ImageFormat_e inputFormats[RIDE_HAL_REMAP_MAX_INPUTS];
        // Resoultion: W, H
        uint32_t inputResolutions[RIDE_HAL_REMAP_MAX_INPUTS][2];
        uint32_t outputResolution[RIDE_HAL_REMAP_MAX_INPUTS][2];
        uint32_t mapResolutions[RIDE_HAL_REMAP_MAX_INPUTS][2];
        RemapTable remapTables[RIDE_HAL_REMAP_MAX_INPUTS];
        FadasROI_t ROIs[RIDE_HAL_REMAP_MAX_INPUTS];
        RideHal_ImageFormat_e outputFormat;
        RideHal_TensorType_e outputTensorType;
        FadasNormlzParams_t normlzR;
        FadasNormlzParams_t normlzG;
        FadasNormlzParams_t normlzB;
        uint32_t batchSize = 1;
        bool enable_undistortion = false;
    };
 
public:
    RemapIF() = default;
    ~RemapIF() = default;
 
    /// @brief Initialize the remap pipeline
    /// @param pName the remap unique instance name
    /// @param pConfig the remap configuration paramaters
    /// @param pLogger the logger used by the remap to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Init( const char* pName, Config *pConfig, Logger *pLogger ) = 0;
 
    /// @brief Start the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;
 
    /// @brief Stop the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;
 
    /// @brief deinitialize the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;
 
    /// @brief execute
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutputs the input shared buffers
    /// @param numOutputs the number of the output shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                    const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs=1 ) = 0;
 
};   // class RemapIF

}   // namespace remap
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_REMAP_IF_HPP_
