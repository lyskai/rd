// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_EXECUTOR_IF_HPP_
#define _RIDE_HAL_EXECUTOR_IF_HPP_

#include "ride/hal/ComponentIF.hpp"

namespace ride
{
namespace hal
{

/// @brief ride::hal::ExecutorIF
///
/// Executor Interface
class ExecutorIF : public ComponentIF
{
public:
    ExecutorIF() = default;
    ~ExecutorIF() = default;

    /// @brief Start the executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the executor
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;

    /// @brief Execute
    /// @param pInputs the input shared buffers
    /// @param numInputs the number of the input shared buffers
    /// @param pOutputs the input shared buffers
    /// @param numOutputs the number of the output shared buffers
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                    const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs ) = 0;
};   // class ExecutorIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_EXECUTOR_IF_HPP_
