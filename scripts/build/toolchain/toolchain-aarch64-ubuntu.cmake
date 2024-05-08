#  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
#  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_hgyaarch64le )

set( CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{UBUNTU_TARGET} )

# workaround to fix link issue of standard libs
add_link_options( "-L$ENV{UBUNTU_TARGET}/usr/lib/aarch64-linux-gnu" )

# workaround to fix include issues
include_directories( $ENV{UBUNTU_TARGET}/usr/include/linux-ark )