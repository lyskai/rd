# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CAMERA_SOURCES
    Component_QcarCam.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon RideHalBufferManager qcxclient qcxosal pmem_client)

add_library( RideHalCamera SHARED ${CAMERA_SOURCES} )

set_property( TARGET RideHalCamera PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCamera PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCamera PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCamera PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::Component::Camera ALIAS RideHalCamera)
