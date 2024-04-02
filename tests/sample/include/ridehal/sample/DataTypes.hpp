// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_SAMPLE_DATA_TYPES_HPP_
#define _RIDE_HAL_SAMPLE_DATA_TYPES_HPP_

#include <vector>

#include "ridehal/sample/SharedBufferPool.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

typedef struct
{
    std::shared_ptr<SharedBuffer_t> buffer;
    uint64_t frameId;
    uint64_t timestamp;
} CamFrame_t;

typedef struct
{
    std::vector<CamFrame_t> frames;
} CamFrames_t;

typedef struct
{
    std::shared_ptr<SharedBuffer_t> buffer;
    std::string name;
    float quantScale;
    int32_t quantOffset;
} Tensor_t;

typedef struct
{
    std::vector<Tensor_t> tensors;
    uint64_t frameId;
    uint64_t timestamp;
} Tensors_t;

typedef struct
{
    int classId;
    float prob;
    float topX;
    float topY;
    float bottomX;
    float bottomY;
} Road2DObject_t;

typedef struct
{
    std::vector<Road2DObject_t> objs;
    uint64_t frameId;
    uint64_t timestamp;
} Road2DObjects_t;

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDE_HAL_SAMPLE_DATA_TYPES_HPP_
