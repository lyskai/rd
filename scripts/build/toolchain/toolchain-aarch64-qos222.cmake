#  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( CMAKE_SYSTEM_NAME QNX )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_ntoaarch64le )

set( CMAKE_C_COMPILER aarch64-unknown-nto-qnx7.1.0-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-unknown-nto-qnx7.1.0-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{BSP_ROOT}/install/aarch64le/ )

# pmem
include_directories( $ENV{BSP_ROOT}/AMSS/inc )
include_directories( $ENV{BSP_ROOT}/install/usr/include )

# apdf
include_directories( $ENV{BSP_ROOT}/install/usr/include/amss/multimedia/apdf/ )
include_directories( $ENV{QNX_ROOT}_patches/target/qnx7/usr/include )
include_directories( $ENV{QNX_ROOT}_patches/target/qnx7/usr/include/WF )

# c2d
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/graphics/include/private/C2D/ )
add_link_options( "-L$ENV{BSP_ROOT}/install/aarch64le/usr/lib/graphics/qc/" )

# vidc
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/inc/ )
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/video/source/common/drivers/inc/ )

# qcarcam
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/cdk_qcx/api/qcarcam/ )
add_link_options( "-L$ENV{BSP_ROOT}//install/aarch64le/lib/camera_qcx/" )

# fadas
include_directories( $ENV{BSP_ROOT}/AMSS/multimedia/fadas/fadas/inc/ )
include_directories( $ENV{BSP_ROOT}/AMSS/platform/qal/clients/fastrpc_lib/inc )
