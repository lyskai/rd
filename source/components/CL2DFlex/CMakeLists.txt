# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CL2DFlex_SOURCES CL2DFlex.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon OpenclIface)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/components/CL2DFlex )
add_library( RideHalCL2DFlex STATIC ${CL2DFlex_SOURCES} )

set_property( TARGET RideHalCL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCL2DFlex PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalCL2DFlex DESTINATION lib)
