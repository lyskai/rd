# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COLORCONVERTOR_SOURCES ColorConvertor.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon OpenclIface)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/components/ColorConvertorCL )
add_library( RideHalColorConvertor STATIC ${COLORCONVERTOR_SOURCES} )

set_property( TARGET RideHalColorConvertor PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalColorConvertor PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalColorConvertor PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalColorConvertor PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalColorConvertor DESTINATION lib)
