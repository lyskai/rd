#!/bin/sh
# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary.

set -xe

homedir=$(dirname $0)
homedir=$(realpath $homedir)

# Target to build
target=$1

# Toplevel build dir
topdir=$(realpath $2)

# Build directory
workdir=$topdir/bld-$target

# Install / runtime directory
destdir=$topdir/run-$target

# Package name
pkgname=$topdir/ridehal-$target.tar.gz

setup_env_qos222() {
    if [[ -v BSP_ROOT ]]; then
        echo build with QNX CRM
        echo BSP_ROOT: $BSP_ROOT
        echo QNX_ROOT: $QNX_ROOT
        echo QNX_TARGET: $QNX_TARGET
        echo QNX_HOST: $QNX_HOST
        export PATH=$QNX_HOST/usr/bin:$PATH
        export CC=aarch64-unknown-nto-qnx7.1.0-gcc
        export CXX=aarch64-unknown-nto-qnx7.1.0-g++
        export LD=aarch64-unknown-nto-qnx7.1.0-ld
        export AR=aarch64-unknown-nto-qnx7.1.0-ar
        export AS=aarch64-unknown-nto-qnx7.1.0-as
        export NM=aarch64-unknown-nto-qnx7.1.0-nm
        export RANLIB=aarch64-unknown-nto-qnx7.1.0-ranlib
        export STRIP=aarch64-unknown-nto-qnx7.1.0-strip

        export CMAKE_TOOLCHAIN_FILE=$homedir/toolchain/toolchain-aarch64-qos222.cmake
        export TOOLCHAIN_SYSROOT=$BSP_ROOT/install/aarch64le

        sh $homedir/toolchain/build-3rd-party-aarch64-qos222.sh $workdir $destdir || \
            echo "WARNING: build 3rd party libraries failed"
    else
        source /opt/qos222/env.sh
    fi
}

## Run tests on x86 builds
case $target in
aarch64-qos222)
    setup_env_qos222
    ;;
aarch64-hgy)
    source /opt/hgy/env.sh
    ;;
esac

# build FastADAS interface library
if [[ -v HEXAGON_SDK_ROOT && -v BSP_ROOT ]] ; then
  if ! [ -f $topdir/source/libs/FadasIface/prebuilt/dsp/libFadasIface_skel.so ];  then
    cd $topdir/source/libs/FadasIface/bld
    rm hexagon_Release_toolv*_v68 -fr
    make tree V=hexagon_Release_dynamic_toolv86_v68 VERBOSE=1 V_dynamic=1 || make tree V=hexagon_Release_dynamic_toolv84_v68 VERBOSE=1 V_dynamic=1
    cp -fv hexagon_Release_toolv*_v68/ship/libFadasIface_skel.so ../prebuilt/dsp
    cp -fv hexagon_Release_toolv*_v68/FadasIface.h ../
    cp -fv hexagon_Release_toolv*_v68/FadasIface_stub.c ../FadasIface.c
    $HEXAGON_SDK_ROOT/tools/HEXAGON_Tools/*/Tools/bin/hexagon-strip ../prebuilt/dsp/libFadasIface_skel.so
  fi
fi

mkdir -p $workdir && cd $workdir || exit 1
cmake \
    -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=/opt/ridehal \
    -DCMAKE_PREFIX_PATH=$destdir/opt/ridehal \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    .. || exit 1
make -j 16 || exit 1

# Install
make DESTDIR=$destdir install || exit 1

# Bundle runtime libraries
$homedir/bundle-runtime.py --sysroot "$TOOLCHAIN_SYSROOT" \
    --exclude-paths $destdir \
    --outdir $destdir/opt/ridehal/lib/runtime \
    $(find $destdir -wholename \*/bin/\* -o -name \*.so\*)

if [ -f $QNN_SDK_ROOT/lib/aarch64-qnx/libQnnHtp.so ];  then
# Install QNN Runtime dependencies
case $target in
aarch64-qos222)
    cp -vf $QNN_SDK_ROOT/lib/aarch64-qnx/libQnn* $destdir/opt/ridehal/lib
    cp -vf $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/libQnn* $destdir/opt/ridehal/lib/dsp
    cp -vf $QNN_SDK_ROOT/lib/hexagon-v75/unsigned/libQnn* $destdir/opt/ridehal/lib/dsp
    ;;
aarch64-hgy)
    cp -vf $QNN_SDK_ROOT/lib/aarch64-rh-linux-gcc9.3/libQnn* $destdir/opt/ridehal/lib
    cp -vf $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/libQnn* $destdir/opt/ridehal/lib/dsp
    cp -vf $QNN_SDK_ROOT/lib/hexagon-v75/unsigned/libQnn* $destdir/opt/ridehal/lib/dsp
    ;;
esac
fi

# Create run-time package
echo "Generating $pkgname"
tar -C $topdir --xform="s/run/pkg/" --exclude="*.a" \
    --exclude="*.la" --exclude="include" --exclude="share" \
    --exclude="cmake" \
    --use-compress-program=pigz -cf $pkgname run-$target
