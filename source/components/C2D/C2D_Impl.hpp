// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_C2D_IMPL_HPP_
#define _RIDE_HAL_C2D_IMPL_HPP_

#include <array>
#include <c2d2.h>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

using namespace ridehal::common;

RideHal_ImageFormat_e ConvertToTensorDataType( const std::string &formatStr );
float GetFormatDepthSize( RideHal_ImageFormat_e format );
uint32_t ConvertToC2DFormatType( RideHal_ImageFormat_e format );

class C2DImpl
{
public:
    C2DImpl();
    ~C2DImpl();
    RideHalError_e init( std::array<uint32_t, 2> &inputResolution,
                         RideHal_ImageFormat_e inputFormat,
                         std::array<uint32_t, 2> &outputResolution,
                         RideHal_ImageFormat_e outputFormat, std::array<uint32_t, 4> &roi,
                         uint32_t align );

    RideHalError_e draw( void *input, void *output );

private:
    RideHalError_e createYUVSurface( void *surface, uint32_t *surfaceId,
                                     RideHal_ImageFormat_e format,
                                     std::array<uint32_t, 2> &resolution, bool isSource );
    RideHalError_e createRGBSurface( void *surface, uint32_t *surfaceId,
                                     RideHal_ImageFormat_e format,
                                     std::array<uint32_t, 2> &resolution, bool isSource );
    RideHalError_e createSurface( void *surface, uint32_t *surfaceId, RideHal_ImageFormat_e format,
                                  std::array<uint32_t, 2> &resolution, bool isSource );

    RideHalError_e updateYUVSurface( C2D_YUV_SURFACE_DEF *surfaceDef, uint32_t surfaceId, void *ptr,
                                     RideHal_ImageFormat_e format,
                                     std::array<uint32_t, 2> &resolution, bool isSource );
    RideHalError_e updateRGBSurface( C2D_RGB_SURFACE_DEF *surfaceDef, uint32_t surfaceId, void *ptr,
                                     RideHal_ImageFormat_e format,
                                     std::array<uint32_t, 2> &resolution, bool isSource );
    RideHalError_e updateSurface( void *surfaceDef, uint32_t surfaceId, void *ptr,
                                  RideHal_ImageFormat_e format, std::array<uint32_t, 2> &resolution,
                                  bool isSource );

private:
    void *m_SourceDef = nullptr;
    void *m_TargetDef = nullptr;
    uint32_t m_SourceSurface = 0;
    uint32_t m_TargetSurface = 0;
    C2D_OBJECT m_C2dObject;

    std::array<uint32_t, 2> m_InputResolution;
    std::array<uint32_t, 2> m_OutputResolution;
    RideHal_ImageFormat_e m_InputFormat;
    RideHal_ImageFormat_e m_OutputFormat;
    std::array<uint32_t, 4> m_ROI;
    uint32_t m_Align = 1;

    RideHal_SharedBuffer_t m_SharedBuffer;
};

#endif
