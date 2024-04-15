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
    C2D_InputConfig_t inputConfigs[RIDE_HAL_MAX_INPUTS];
    C2D_ImageResolution_t outputResolution;
    RideHal_ImageFormat_e outputFormat;
    uint32_t numOfInputs;
    uint32_t numOfOutputs;
    uint32_t batchSize;
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
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutputs the input shared buffers
    /// @param numOutputs the number of the output shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs );

    /// @brief Register shared buffers for each input
    /// @param pBuffer the input shared buffers array
    /// @param numBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                         uint32_t numOfInputBuffers );

    /// @brief Register shared buffers for output
    /// @param pBuffers the output shared buffer
    /// @param numBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                          uint32_t numOfOutputBuffers );

    /// @brief Deregister shared buffers for each input
    /// @param pBuffer the input shared buffers array
    /// @param numBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                           uint32_t numOfInputBuffers );

    /// @brief Deregister shared buffers for output
    /// @param pBuffers the output shared buffer
    /// @param numBuffers the number of shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                            uint32_t numOfOutputBuffers );

private:
    RideHalError_e createSurface( void *surface, uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                  void *bufferAddr, uint32_t width, uint32_t height,
                                  uint32_t *stride, uint32_t *actualHeight, bool isSource );
    RideHalError_e createYUVSurface( C2D_YUV_SURFACE_DEF *surfaceDef, uint32_t *surfaceId,
                                     RideHal_ImageFormat_e format, void *bufferAddr, uint32_t width,
                                     uint32_t height, uint32_t *stride, uint32_t *actualHeight,
                                     bool isSource );
    RideHalError_e createRGBSurface( C2D_RGB_SURFACE_DEF *surfaceDef, uint32_t *surfaceId,
                                     RideHal_ImageFormat_e format, void *bufferAddr, uint32_t width,
                                     uint32_t height, uint32_t *stride, bool isSource );

    uint32_t GetC2DFormatType( RideHal_ImageFormat_e format );

private:
    static const uint32_t MAX_BUFFER_NUM = 16;

    C2D_ImageResolution_t m_inputResolutions[RIDE_HAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_inputFormats[RIDE_HAL_MAX_INPUTS];
    C2D_ROIConfig_t m_rois[RIDE_HAL_MAX_INPUTS];

    RideHal_ImageFormat_e m_outputFormat;
    uint32_t m_batchSize = 1;
    uint32_t m_numOfInputs = 1;
    uint32_t m_numOfOutputs = 1;
    uint32_t m_outputWidth = 0;
    uint32_t m_outputHeight = 0;
    uint32_t m_outputSize = 0;

    uint32_t m_stride[RIDE_HAL_NUM_IMAGE_PLANES];
    uint32_t m_actualHeight[RIDE_HAL_NUM_IMAGE_PLANES];

    std::unordered_map<void *, std::pair<void *, C2D_OBJECT>> m_inputBufferSurfaceMap;
    std::unordered_map<void *, std::pair<void *, uint32_t>> m_outputBufferSurfaceMap;

};   // class C2D

}   // namespace component
}   // namespace ridehal

#endif   // _RIDE_HAL_C2D_HPP_

