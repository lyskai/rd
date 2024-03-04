#  Copyright 2022 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
if( CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" )

if(NOT DEFINED ENV{QNN_SDK_ROOT})
message(FATAL_ERROR "env QNN_SDK_ROOT is not defined")
endif()
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include/ride/hal )

set( QNN_HEADERS
    ${HEADERS_DIR}/QnnRuntime.hpp
   )

set( QNN_SOURCES
    src/Log/LogUtils.cpp
    src/Log/Logger.cpp
    src/PAL/src/linux/Directory.cpp
    src/PAL/src/linux/Path.cpp
    src/PAL/src/linux/DynamicLoading.cpp
    src/PAL/src/linux/FileOp.cpp
    src/PAL/src/common/StringOp.cpp
    src/PAL/src/common/GetOpt.cpp
    src/Utils/DataUtil.cpp
    src/Utils/QnnSampleAppUtils.cpp
    src/Utils/DynamicLoadUtil.cpp
    src/Utils/IOTensor.cpp
    src/WrapperUtils/QnnWrapperUtils.cpp
    src/QnnRuntime.cpp
   )

add_library(QnnRuntime STATIC ${QNN_HEADERS} ${QNN_SOURCES})
# setting LINKER_LANGUAGE is necessary if QNN_SOURCES is empty
set_property(TARGET QnnRuntime PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET QnnRuntime PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(QnnRuntime PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Log
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PAL/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/src/WrapperUtils
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
	$ENV{QNN_SDK_ROOT}/include/QNN
        )
target_compile_definitions(QnnRuntime PUBLIC QNN_API=)
set( TARGET_LIBRARIES stdc++ cdsprpc fastrpc fastrpc_pmem)

target_link_libraries(QnnRuntime PUBLIC ${TARGET_LIBRARIES})

add_executable(qnn_test src/qnn_test.cpp)
target_link_libraries(qnn_test PRIVATE QnnRuntime cdsprpc)
install(TARGETS qnn_test DESTINATION bin)
endif() # aarch64-QNX
