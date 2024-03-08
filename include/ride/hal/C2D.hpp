// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_C2D_HPP_
#define _RIDE_HAL_C2D_HPP_

#include <array>
#include <cinttypes>
#include <memory>
#include <vector>

#include "ride/hal/Buffer.hpp"
#include "ride/hal/ExecutorIF.hpp"
#include "ride/hal/Logger.hpp"
#include "ride/hal/Types.hpp"

using namespace ride::hal;

namespace ride
{
namespace hal
{
namespace component
{

typedef struct
{
    uint32_t width;
    uint32_t height;
} C2D_ImageResolution_t;

typedef struct
{
    uint32_t topX;
    uint32_t topY;
    uint32_t width;
    uint32_t height;
} C2D_ROIConfig_t;

typedef struct
{
    RideHal_ImageFormat_e inputFormat;
    C2D_ImageResolution_t inputResolution;
    C2D_ROIConfig_t ROI;
} C2D_InputConfig_t;

typedef struct
{
    C2D_InputConfig_t inputConfigs[RIDE_HAL_MAX_INPUTS];
    C2D_ImageResolution_t outputResolution;
    RideHal_ImageFormat_e outputFormat;
    uint32_t batchSize = 1;
    uint32_t align = 1;
    uint32_t stride = 0;
} C2D_Config_t;


/// @brief Component C2D
///
/// C2D convert 1 camera frame into RGB and do normalize
class C2D final : public ExecutorIF
{
public:
    C2D();
    ~C2D();

    /// @brief Initialize the component
    /// @param name the component unique instance name
    /// @param pLogger the logger used by the component to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const C2D_Config_t *pConfig, Logger *pLogger );

    /// @brief Start the C2D executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the C2D executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the C2D executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

    /// @brief Execute
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutputs the input shared buffers
    /// @param numOutputs the number of the output shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs );


private:
    std::vector<std::array<uint32_t, 2>> m_InputResolutions;
    std::vector<RideHal_ImageFormat_e> m_InputFormats;
    std::vector<size_t> m_InputSizes;
    std::array<uint32_t, 2> m_OutputResolution;
    RideHal_ImageFormat_e m_OutputFormat;
    size_t m_OutputSize = 0;
    std::vector<std::array<uint32_t, 4>> m_ROIs;
    uint32_t m_BatchSize = 1;
    uint32_t m_Align = 1;
    uint32_t m_Stride = 0;

};   // class C2D

}   // namespace component
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_C2D_HPP_
