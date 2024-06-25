
# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )

set( TARGET_LIBRARIES OpenCL )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "QNX" )
list( APPEND TARGET_LIBRARIES OSUser GSLUser )
endif()

add_library(OpenclIface SHARED OpenclIface.cpp )
target_include_directories(OpenclIface PUBLIC ${HEADERS_DIR})
target_link_libraries(OpenclIface PUBLIC ${TARGET_LIBRARIES})
add_library(Ride::Hal::Libs::OpenclIface ALIAS OpenclIface)
install(TARGETS OpenclIface DESTINATION lib)


