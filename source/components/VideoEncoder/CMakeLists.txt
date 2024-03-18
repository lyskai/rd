# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEOENCODER_SOURCES
    VideoEncoder.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    find_package(vidc)
    list( APPEND TARGET_LIBRARIES vidc )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES vidc vidc_client mmosal )
endif()

add_library( RideHalVideoEncoder STATIC ${COMPONENT_VIDEOENCODER_SOURCES} )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_include_directories( RideHalVideoEncoder PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
target_compile_definitions( RideHalVideoEncoder PUBLIC _VIDC_LRH_LINUX_ )
endif()

set_property( TARGET RideHalVideoEncoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVideoEncoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVideoEncoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVideoEncoder PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS RideHalVideoEncoder DESTINATION lib )
