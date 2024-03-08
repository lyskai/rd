# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( REMAP_SOURCES Remap.cpp )

list( APPEND TARGET_LIBRARIES FadasIface FadasIfaceStub)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/FadasIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/FadasIface/bld/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/FadasIface/externals/include )

add_library( RideHalRemap STATIC ${REMAP_SOURCES} )

set_property( TARGET RideHalRemap PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalRemap PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalRemap PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalRemap PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::Remap ALIAS RideHalRemap)

install(TARGETS RideHalRemap DESTINATION lib)

endif()