
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEODECODER_HEADERS
    ${HEADERS_DIR}/ridehal/component/VideoDecoder.hpp
)

set( COMPONENT_VIDEODECODER_SOURCES
    VideoDecoder.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND TARGET_LIBRARIES ioctlClient OSAbstraction )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES vidc vidc_client mmosal )
endif()

add_library( RideHalVideoDecoder OBJECT ${COMPONENT_VIDEODECODER_SOURCES} )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_include_directories( RideHalVideoDecoder PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
target_compile_definitions( RideHalVideoDecoder PUBLIC _VIDC_LRH_LINUX_ )
endif()

set_property( TARGET RideHalVideoDecoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVideoDecoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVideoDecoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVideoDecoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_VIDEODECODER_HEADERS} DESTINATION include/ridehal/component )
