# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( TARGET_LIBRARIES RideHalComponentCommon )

list( APPEND TARGET_LIBRARIES EGL GLESv2_adreno drm gbm  )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/drm )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/libdrm )

set( COMPONENT_GL2DFLEX_SOURCES
     GL2DFlex.cpp
)

add_library( RideHalGL2DFlex STATIC ${COMPONENT_GL2DFLEX_SOURCES} )

set_property( TARGET RideHalGL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalGL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalGL2DFlex PUBLIC ${HEADERS_DIR} )
target_include_directories( RideHalGL2DFlex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( RideHalGL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS RideHalGL2DFlex DESTINATION lib )

