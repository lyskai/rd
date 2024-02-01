// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef _RIDE_HAL_COMPONENT_IF_HPP_
#define _RIDE_HAL_COMPONENT_IF_HPP_

#include <string>

#include "ride/hal/Logger.hpp"
#include "ride/hal/Types.hpp"


namespace ride
{
namespace hal
{

/// @brief RideHal Component state
typedef enum
{
    RIDE_HAL_COMPONENT_STATE_INITIAL = 0,   ///< the initial state
    RIDE_HAL_COMPONENT_STATE_READY,         ///< the ready state
    RIDE_HAL_COMPONENT_STATE_RUNNING,       ///< the running state
    RIDE_HAL_COMPONENT_STATE_MAX
} RideHal_ComponentState_t;

/// @brief ride::hal::ComponentIF
///
/// Component Interface
class ComponentIF
{
public:
    ComponentIF() = default;
    ~ComponentIF() = default;

    /// @brief Initialize the component
    /// @param name the component unique instance name
    /// @param pConfig the component opaque configuration paramaters
    /// @param pLogger the logger used by the component to log messages
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    virtual RideHalError_e Init( std::string name, const void *pConfig, Logger *pLogger ) = 0;

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
    /// @brief Log a message
    /// @param level the message log level
    /// @param pFormat the message format
    /// @param ... variable arguments
    /// @return void
    void Log( Logger::Level_e level, const char *pFormat, ... );

    /// @brief check is the component OK to go to the new state
    /// @param newState the new state
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e IsOKToGoToState( RideHal_ComponentState_t newState );

    /// @brief put the component into new state
    /// @param newState the new state
    /// @return RIDE_HAL_ERROR_NONE on success, others on failure
    RideHalError_e GoToState( RideHal_ComponentState_t newState );

private:
    std::string m_Name;
    Logger *m_pLogger = nullptr;
    RideHal_ComponentState_t m_State = RIDE_HAL_COMPONENT_STATE_INITIAL;

};   // class ComponentIF

}   // namespace hal
}   // namespace ride

#endif   // _RIDE_HAL_COMPONENT_IF_HPP_
