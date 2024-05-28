// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_SAMPLE_DATA_TYPES_HPP_
#define _RIDEHAL_SAMPLE_DATA_TYPES_HPP_

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
    /* Extra DataFrame information for Image and Tensor */
    uint64_t frameId;
    uint64_t timestamp;

    /* Extra DataFrame information for Tensor */
    std::string name;
    float quantScale;
    int32_t quantOffset;

public:
    RideHal_BufferType_e BufferType() { return buffer->sharedBuffer.type; }
    RideHal_SharedBuffer_t &SharedBuffer() { return buffer->sharedBuffer; }
    void *data() { return buffer->sharedBuffer.data(); }
} DataFrame_t;

typedef struct
{
    std::vector<DataFrame_t> frames;

public:
    RideHal_BufferType_e BufferType( int index ) { return frames[index].BufferType(); }
    RideHal_SharedBuffer_t &SharedBuffer( int index ) { return frames[index].SharedBuffer(); }
    void *data( int index ) { return frames[index].data(); }

    uint64_t FrameId( int index ) { return frames[index].frameId; };
    uint64_t Timestamp( int index ) { return frames[index].timestamp; };

    std::string Name( int index ) { return frames[index].name; };
    float QuantScale( int index ) { return frames[index].quantScale; };
    int32_t QuantOffset( int index ) { return frames[index].quantOffset; };

    void Add( DataFrame_t &frame ) { frames.push_back( frame ); }
} DataFrames_t;

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

#endif   // _RIDEHAL_SAMPLE_DATA_TYPES_HPP_
