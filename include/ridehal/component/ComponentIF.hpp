// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDEHAL_COMPONENT_IF_HPP_
#define _RIDEHAL_COMPONENT_IF_HPP_

#include <string>

#include "ridehal/common/Logger.hpp"
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
class ComponentIF
{
public:
    ComponentIF() = default;
    ~ComponentIF() = default;

    /// @brief Start the component
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Start() = 0;

    /// @brief Stop the component
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Stop() = 0;

    /// @brief deinitialize the component
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Deinit();

    /// @brief get the current state of the component
    /// @return the current state of the component
    RideHal_ComponentState_t GetState();

    /// @brief get the name of the component
    /// @return the name of the component
    const char *GetName();

protected:
    /// @brief Initialize the component
    /// @param pName the component unique instance name
    /// @param level the logger message level
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

protected:
    std::string m_name;
    RIDEHAL_DECLARE_LOGGER();
    RideHal_ComponentState_t m_state = RIDEHAL_COMPONENT_STATE_INITIAL;

};   // class ComponentIF

}   // namespace component
}   // namespace ridehal

#endif   // _RIDEHAL_COMPONENT_IF_HPP_
