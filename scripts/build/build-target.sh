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

## Run tests on x86 builds
case $target in
aarch64-qos222)
    source /opt/qos222/env.sh
    ;;
aarch64-hgy)
    source /opt/hgy/env.sh
    ;;
esac

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

# Create run-time package
echo "Generating $pkgname"
tar -C $topdir --xform="s/run/pkg/" --exclude="*.a" --use-compress-program=pigz -cf $pkgname run-$target
