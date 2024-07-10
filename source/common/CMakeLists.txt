# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMMON_HEADERS
    ${HEADERS_DIR}/ridehal/common/BufferManager.hpp
    ${HEADERS_DIR}/ridehal/common/Logger.hpp
    ${HEADERS_DIR}/ridehal/common/SharedBuffer.hpp
    ${HEADERS_DIR}/ridehal/common/Types.hpp
)

set( COMMON_SOURCES
    Buffer.cpp
    BufferManager.cpp
    Image.cpp
    Tensor.cpp
    Logger.cpp
)

set( TARGET_LIBRARIES )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    list( APPEND COMMON_SOURCES Buffer_QNX.cpp Logger_QNX.cpp )
    list( APPEND TARGET_LIBRARIES pmem_client pmemext fastrpc_pmem apdf slog2 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND COMMON_SOURCES Buffer_Linux.cpp Logger_Linux.cpp )    
    list( APPEND TARGET_LIBRARIES kiumd apdf aosal pthread dl )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

add_library( RideHalCommon OBJECT ${COMMON_SOURCES} )

set_property( TARGET RideHalCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCommon PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMMON_HEADERS} DESTINATION include/ridehal/common )
