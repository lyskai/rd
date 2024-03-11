# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_COMMON_SOURCES
    ComponentIF.cpp
    Logger.cpp
    LoggerIF.cpp
)

set( TARGET_LIBRARIES )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND COMPONENT_COMMON_SOURCES Logger_QNX.cpp )
    list( APPEND TARGET_LIBRARIES slog2 )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND COMPONENT_COMMON_SOURCES Logger_Linux.cpp )
endif()

add_library( RideHalComponentCommon STATIC ${COMPONENT_COMMON_SOURCES} )

set_property( TARGET RideHalComponentCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalComponentCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalComponentCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalComponentCommon PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::Component::Common ALIAS RideHalComponentCommon)
