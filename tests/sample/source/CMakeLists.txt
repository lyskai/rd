
set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set( RIDEHAL_SAMPLE_SOURCES
    SampleIF.cpp
    SampleC2D.cpp
    SamplePlrPost.cpp
    SamplePlrPre.cpp
    SamplePostProcCenternet.cpp
    SamplePostProcBevdet.cpp
    SampleCamera.cpp
    SampleDataReader.cpp
    SampleRecorder.cpp
    SampleRemap.cpp
    SampleVideoEncoder.cpp
    SharedBufferPool.cpp
    SampleCL2DFlex.cpp
    SampleDataOnline.cpp
    SampleSharedRing.cpp
    SampleFpsAdapter.cpp
    SysTrace.cpp
)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND RIDEHAL_SAMPLE_SOURCES SampleGL2DFlex.cpp )
endif()

set( RIDEHAL_SAMPLE_DEFINITIONS )

find_package(RideHal)
if( NOT TARGET RideHal )
  message(FATAL_ERROR "RideHal SDK not found")
endif()

set( TARGET_LIBRARIES RideHal SharedRing )

if( NOT TARGET TinyViz )
  message(STATUS "build sample without TinyViz")
else()
  list( APPEND RIDEHAL_SAMPLE_SOURCES SampleTinyViz.cpp )
  list( APPEND TARGET_LIBRARIES TinyViz )
endif()

if( DEFINED ENV{QNN_SDK_ROOT} )
    set( QNN_SDK_ROOT $ENV{QNN_SDK_ROOT} )
endif()

if ( NOT DEFINED QNN_SDK_ROOT )
  message(STATUS "build sample without QnnRuntime")
else()
  list( APPEND RIDEHAL_SAMPLE_SOURCES SampleQnn.cpp )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
  list( APPEND RIDEHAL_SAMPLE_DEFINITIONS WITH_RSM_V2 )
  list( APPEND TARGET_LIBRARIES rsm_client )
  list( APPEND TARGET_LIBRARIES socket )
endif()

add_library( RideHalSample SHARED ${RIDEHAL_SAMPLE_SOURCES} )

set_property( TARGET RideHalSample PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalSample PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalSample PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalSample PUBLIC ${TARGET_LIBRARIES} )

target_compile_definitions( RideHalSample PUBLIC ${RIDEHAL_SAMPLE_DEFINITIONS} )

install(TARGETS RideHalSample DESTINATION lib)
