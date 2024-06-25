# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( TARGET_LIBRARIES RideHalComponentCommon )

list( APPEND TARGET_LIBRARIES EGL drm gbm )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/drm )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/libdrm )

set( COMPONENT_GLCONVERTER_SOURCES
    GLConverter.cpp
)

add_library( RideHalGLConverter STATIC ${COMPONENT_GLCONVERTER_SOURCES} )

set_property( TARGET RideHalGLConverter PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalGLConverter PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalGLConverter PUBLIC ${HEADERS_DIR} )
target_include_directories( RideHalGLConverter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( RideHalGLConverter PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS RideHalGLConverter DESTINATION lib )
