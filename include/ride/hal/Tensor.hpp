// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_TENSOR_HPP_
#define _RIDE_HAL_TENSOR_HPP_

#include "ride/hal/Buffer.hpp"

namespace ride
{
namespace hal
{
namespace memory
{

class Tensor : public Buffer
{
public:
    Tensor();
    ~Tensor();

    /// @brief Allocate the DMA memory for tensor with specified tensor properties
    /// @param pTensorProps the specified tensor properties
    /// @param flags the DMA buffer flags
    /// @param usage the DMA buffer usage
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Allocate( const RideHal_TensorProps_t *pTensorProps,
                             RideHal_BufferFlags_t flags = 0,
                             RideHal_BufferUsage_e usage = RIDE_HAL_BUFFER_USAGE_HTP );
};

}   // namespace memory
}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_TENSOR_HPP_
