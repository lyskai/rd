// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_C2D_HPP_
#define _RIDE_HAL_C2D_HPP_

#include <array>
#include <c2d2.h>
#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ridehal/component/ComponentIF.hpp"

namespace ridehal
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
    uint32_t numOfInputs;
    C2D_InputConfig_t inputConfigs[RIDE_HAL_MAX_INPUTS];
} C2D_Config_t;


/// @brief Component C2D
///
/// C2D convert 1 camera frame into RGB and do normalize
class C2D final : public ComponentIF
{
public:
    C2D();
    ~C2D();

    /// @brief Initialize the component
    /// @param name the component unique instance name
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, const C2D_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

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
    /// @param[in] pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param[out] pOutput the output shared buffer
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );

    /// @brief Register shared buffers for each input
    /// @param[in] pInputBuffer the input shared buffers array
    /// @param numOfInputBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                         uint32_t numOfInputBuffers );

    /// @brief Register shared buffers for output
    /// @param[out] pOutputBuffer the output shared buffer
    /// @param numOfOutputBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                          uint32_t numOfOutputBuffers );

    /// @brief Deregister shared buffers for each input
    /// @param[in] pInputBuffer the input shared buffers array
    /// @param numOfInputBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                           uint32_t numOfInputBuffers );

    /// @brief Deregister shared buffers for output
    /// @param[out] pOutputBuffer the output shared buffer
    /// @param numOfOutputBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                            uint32_t numOfOutputBuffers );

private:
    RideHalError_e createSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                  void *bufferAddr, uint32_t width, uint32_t height,
                                  uint32_t *stride, uint32_t *actualHeight, bool isSource );
    RideHalError_e createYUVSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                     void *bufferAddr, uint32_t width, uint32_t height,
                                     uint32_t *stride, uint32_t *actualHeight, bool isSource );
    RideHalError_e createRGBSurface( uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                     void *bufferAddr, uint32_t width, uint32_t height,
                                     uint32_t *stride, bool isSource );

    uint32_t GetC2DFormatType( RideHal_ImageFormat_e format );

private:
    uint32_t m_numOfInputs = 1;
    C2D_ImageResolution_t m_inputResolutions[RIDE_HAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_inputFormats[RIDE_HAL_MAX_INPUTS];
    C2D_ROIConfig_t m_rois[RIDE_HAL_MAX_INPUTS];

    std::unordered_map<void *, C2D_OBJECT> m_inputBufferSurfaceMap;
    std::unordered_map<void *, uint32_t> m_outputBufferSurfaceMap;

};   // class C2D

}   // namespace component
}   // namespace ridehal

#endif   // _RIDE_HAL_C2D_HPP_

