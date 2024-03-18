# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CAMERA_SOURCES
    Camera.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon qcxclient qcxosal )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES qcxplatform )
endif()

add_library( RideHalCamera STATIC ${CAMERA_SOURCES} )

set_property( TARGET RideHalCamera PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCamera PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCamera PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCamera PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS RideHalCamera DESTINATION lib )