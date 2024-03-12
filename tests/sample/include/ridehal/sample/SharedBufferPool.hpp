//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef _RIDE_HAL_SHARED_BUFFER_POOL_HPP_
#define _RIDE_HAL_SHARED_BUFFER_POOL_HPP_

#include <cinttypes>
#include <cstring>
#include <memory>
#include <vector>

#include "ridehal/common/LoggerIF.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"


using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer;
    uint64_t pubHandle;
} SharedBuffer_t;

class SharedBufferPool : public LoggerIF
{
public:
    SharedBufferPool();
    ~SharedBufferPool();

    std::shared_ptr<SharedBuffer_t> Get();

    RideHalError_e Init( std::string name, Logger *pLogger, uint32_t number, uint32_t width,
                         uint32_t height, RideHal_ImageFormat_e format,
                         RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT );

    RideHalError_e Init( std::string name, Logger *pLogger, uint32_t number, uint32_t batchSize,
                         uint32_t width, uint32_t height, RideHal_ImageFormat_e format,
                         RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT );

    RideHalError_e Init( std::string name, Logger *pLogger, uint32_t number,
                         RideHal_ImageProps_t &imageProps,
                         RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT );

    RideHalError_e Init( std::string name, Logger *pLogger, uint32_t number,
                         RideHal_TensorProps_t &tensorProps,
                         RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_DEFAULT );

private:
    RideHalError_e Init( std::string name, Logger *pLogger, uint32_t number );
    void Deleter( SharedBuffer_t *ptrToDelete );

    struct SharedBufferInfo
    {
        SharedBuffer_t sharedBuffer;
        int dirty;
    };

    std::vector<SharedBufferInfo> m_queue;
    bool m_bIsInited = false;
};

}   // namespace sample
}   // namespace ridehal

#endif   // #ifndef _RIDE_HAL_SHARED_BUFFER_POOL_HPP_
