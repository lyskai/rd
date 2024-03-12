// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_COMPONENT_IF_HPP_
#define _RIDE_HAL_COMPONENT_IF_HPP_

#include <string>

#include "ridehal/common/LoggerIF.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/// @brief ridehal::ComponentIF
///
/// Component Interface
class ComponentIF : public LoggerIF
{
public:
    ComponentIF() = default;
    ~ComponentIF() = default;

    /// @brief Start the component
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the component
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the component
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit() = 0;

    /// @brief get the current state of the component
    /// @return the current state of the component
    RideHal_ComponentState_t GetState();

protected:
    /// @brief Initialize the component
    /// @param pName the component unique instance name
    /// @param pLogger the logger used by the component to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, Logger *pLogger = nullptr );

protected:
    RideHal_ComponentState_t m_state = RIDE_HAL_COMPONENT_STATE_INITIAL;

};   // class ComponentIF

}   // namespace component
}   // namespace ridehal

#endif   // _RIDE_HAL_COMPONENT_IF_HPP_
