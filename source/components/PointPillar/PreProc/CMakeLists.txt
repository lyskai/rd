#  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( PLRPRE_SOURCES PointPillarPreProc.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface )

add_library( RideHalPointPillarPreProc STATIC ${PLRPRE_SOURCES} )

set_property( TARGET RideHalPointPillarPreProc PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalPointPillarPreProc PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalPointPillarPreProc PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalPointPillarPreProc PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalPointPillarPreProc DESTINATION lib)
