# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( BUFFER_MANAGER_SOURCES
    Buffer.cpp
    BufferManager.cpp
    Image.cpp
    Tensor.cpp
)

set( TARGET_LIBRARIES )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    find_package(pmem)
    list( APPEND BUFFER_MANAGER_SOURCES Buffer_QNX.cpp )
    list( APPEND TARGET_LIBRARIES pmem fastrpc_pmem apdf )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND BUFFER_MANAGER_SOURCES Buffer_Linux.cpp )    
    list( APPEND TARGET_LIBRARIES kiumd apdf )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

add_library( RideHalBufferManager SHARED ${BUFFER_MANAGER_SOURCES} )

set_property( TARGET RideHalBufferManager PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalBufferManager PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalBufferManager PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalBufferManager PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::BufferManager ALIAS RideHalBufferManager)
