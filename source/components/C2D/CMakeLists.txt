# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( BUFFER_MANAGER_DIR ${PROJECT_SOURCE_DIR}/source/buffer_manager )
set( BUFFER_MANAGER_SOURCES
    ${BUFFER_MANAGER_DIR}/Buffer.cpp
    ${BUFFER_MANAGER_DIR}/BufferManager.cpp
    ${BUFFER_MANAGER_DIR}/Image.cpp
    ${BUFFER_MANAGER_DIR}/Tensor.cpp
)

set( COMPONENT_COMMON_DIR ${PROJECT_SOURCE_DIR}/source/components/common )
set( COMPONENT_COMMON_SOURCES
    ${COMPONENT_COMMON_DIR}/ComponentIF.cpp
    ${COMPONENT_COMMON_DIR}/Logger.cpp
)

set( TARGET_LIBRARIES Ride::Hal::Component::C2D)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
    find_package(pmem)
    list( APPEND BUFFER_MANAGER_SOURCES ${BUFFER_MANAGER_DIR}/Buffer_QNX.cpp )
    list( APPEND COMPONENT_COMMON_SOURCES ${COMPONENT_COMMON_DIR}/Logger_QNX.cpp )
    list( APPEND TARGET_LIBRARIES pmem fastrpc_pmem apdf slog2 planedef c2d30 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
    
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND BUFFER_MANAGER_SOURCES ${BUFFER_MANAGER_DIR}/Buffer_Linux.cpp )
    list( APPEND COMPONENT_COMMON_SOURCES ${COMPONENT_COMMON_DIR}/Logger_Linux.cpp )
    list( APPEND TARGET_LIBRARIES kiumd apdf aosal planedef C2D2 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

set( COMPONENT_C2D_SOURCES
    C2D.cpp
    C2D_Impl.cpp
)

add_library( RideHalComponentC2D STATIC ${COMPONENT_C2D_SOURCES} ${BUFFER_MANAGER_SOURCES} ${COMPONENT_COMMON_SOURCES} )

set_property( TARGET RideHalComponentC2D PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalComponentC2D PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalComponentC2D PUBLIC ${HEADERS_DIR} )
target_include_directories( RideHalComponentC2D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( RideHalComponentC2D PUBLIC ${TARGET_LIBRARIES} )

add_library(Ride::Hal::Component::C2D ALIAS RideHalComponentC2D)
install( TARGETS RideHalComponentC2D DESTINATION lib )

