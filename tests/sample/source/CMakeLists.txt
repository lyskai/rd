# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set( RIDEHAL_SAMPLE_SOURCES
    SampleIF.cpp
    SamplePostProcCenternet.cpp
    SampleCamera.cpp
    SampleDataReader.cpp
    SampleRecorder.cpp
    SampleRemap.cpp
    SampleVideoEncoder.cpp
    SharedBufferPool.cpp
)

set( RIDEHAL_SAMPLE_DEFINITIONS )

set( TARGET_LIBRARIES RideHal )

if( NOT TARGET TinyViz )
  message(INFO "build sample without TinyViz")
else()
  list( APPEND RIDEHAL_SAMPLE_SOURCES SampleTinyViz.cpp )
  list( APPEND TARGET_LIBRARIES TinyViz )
endif()

if( NOT TARGET RideHalQnnRuntime )
  message(INFO "build sample without QnnRuntime")
else()
  list( APPEND RIDEHAL_SAMPLE_SOURCES SampleQnn.cpp )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
  #list( APPEND RIDEHAL_SAMPLE_DEFINITIONS WITH_RSM_V2 )
  #list( APPEND TARGET_LIBRARIES rsm_client )
endif()

add_library( RideHalSample SHARED ${RIDEHAL_SAMPLE_SOURCES} )

set_property( TARGET RideHalSample PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalSample PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalSample PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalSample PUBLIC ${TARGET_LIBRARIES} )

target_compile_definitions( RideHalSample PUBLIC ${RIDEHAL_SAMPLE_DEFINITIONS} )

install(TARGETS RideHalSample DESTINATION lib)
