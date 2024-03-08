# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEOENCODER_SOURCES
    VideoEncoder.cpp
)

set( TARGET_LIBRARIES )

find_package(vidc)
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND TARGET_LIBRARIES vidc )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
endif()

add_library( RideHalComponentVideoEncoder STATIC ${COMPONENT_VIDEOENCODER_SOURCES} )

set_property( TARGET RideHalComponentVideoEncoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalComponentVideoEncoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalComponentVideoEncoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalComponentVideoEncoder PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::Component::VideoEncoder ALIAS RideHalComponentVideoEncoder)
