#  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( PLRPOST_SOURCES PostCenterPoint.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface )

add_library( RideHalPostCenterPoint STATIC ${PLRPOST_SOURCES} )

set_property( TARGET RideHalPostCenterPoint PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalPostCenterPoint PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalPostCenterPoint PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalPostCenterPoint PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalPostCenterPoint DESTINATION lib)
