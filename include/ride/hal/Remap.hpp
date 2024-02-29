// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_REMAP_HPP_
#define _RIDE_HAL_REMAP_HPP_

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "ride/hal/BufferManager.hpp"
#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{
namespace component
{

/// @brief ride::hal::component
///
/// Remap Interface

typedef enum
{
    REMAP_PROCESSOR_DSP0,
    REMAP_PROCESSOR_DSP1,
    REMAP_PROCESSOR_CPU
} Remap_ProcessorType_t;

typedef struct
{
    float *pMapX;
    size_t sizeX;
    float *pMapY;
    size_t sizeY;
} Remap_MapTable_t;

typedef struct
{
    uint32_t topX;
    uint32_t topY;
    uint32_t width;
    uint32_t height;
} Remap_ROI_t;

typedef struct
{
    RideHal_ImageFormat_e inputFormat;
    uint32_t inputWidth;
    uint32_t inputHeight;
    uint32_t mapWidth;
    uint32_t mapHeight;
    Remap_MapTable_t remapTable;
    Remap_ROI_t ROI;
} Remap_InputConfig_t;

typedef struct
{
    Remap_ProcessorType_t processor;
    Remap_InputConfig_t inputConfigs[RIDE_HAL_MAX_INPUTS];
    uint32_t numOfInputs;
    uint32_t outputWidth;
    uint32_t outputHeight;
    RideHal_ImageFormat_e outputFormat;
    FadasNormlzParams_t normlzR;
    FadasNormlzParams_t normlzG;
    FadasNormlzParams_t normlzB;
    bool bEnableUndistortion;
    bool bEnableNormalize;
} Remap_Config_t;

class Remap
{
public:
public:
    Remap() = default;
    ~Remap() = default;

    /// @brief Initialize the remap pipeline
    /// @param pName the remap unique instance name
    /// @param pConfig the remap configuration paramaters
    /// @param pLogger the logger used by the remap to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const Remap_Config_t *pConfig, Logger *pLogger );

    /// @brief Start the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the remap pipeline
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

    /// @brief execute
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutputs the input shared buffers
    /// @param numOutputs the number of the output shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs = 1 );

private:
    Remap_Config_t m_Config;
    std::vector<size_t> m_InputSizes;
    std::unique_ptr<RemapImpl> m_Impl = nullptr;

};   // class Remap

}   // namespace component
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_REMAP_HPP_
