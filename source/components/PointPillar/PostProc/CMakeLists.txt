#  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( PLRPRE_SOURCES PointPillarPostProc.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface )

add_library( RideHalPointPillarPostProc STATIC ${PLRPRE_SOURCES} )

set_property( TARGET RideHalPointPillarPostProc PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalPointPillarPostProc PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalPointPillarPostProc PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalPointPillarPostProc PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalPointPillarPostProc DESTINATION lib)
