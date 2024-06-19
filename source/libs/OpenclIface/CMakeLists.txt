
# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )

add_library(OpenclIface SHARED OpenclIface.cpp )
target_include_directories(OpenclIface PUBLIC ${HEADERS_DIR})
target_link_libraries(OpenclIface PUBLIC OpenCL)
add_library(Ride::Hal::Libs::OpenclIface ALIAS OpenclIface)
install(TARGETS OpenclIface DESTINATION lib)


