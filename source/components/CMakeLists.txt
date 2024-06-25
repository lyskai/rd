# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_COMMON_SOURCES
    ComponentIF.cpp
)

set( TARGET_LIBRARIES RideHalCommon )

add_library( RideHalComponentCommon STATIC ${COMPONENT_COMMON_SOURCES} )

set_property( TARGET RideHalComponentCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalComponentCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalComponentCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalComponentCommon PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalComponentCommon DESTINATION lib)

add_subdirectory(C2D)
add_subdirectory(Camera)
add_subdirectory(PostCenterPoint)
add_subdirectory(QnnRuntime)
add_subdirectory(ColorConvertorCL)
add_subdirectory(Remap)
add_subdirectory(VideoEncoder)
add_subdirectory(Voxelization)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    add_subdirectory(GLConverter)
endif()
