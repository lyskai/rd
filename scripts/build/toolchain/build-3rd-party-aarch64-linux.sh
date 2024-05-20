#!/bin/sh
# Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
# Confidential & Proprietary.

set -xe

homedir=$(dirname $0)
homedir=$(realpath $homedir)

# Build directory
workdir=$1/third_party

# Install / runtime directory
destdir=$2/opt/ridehal

mkdir -p $destdir || exit 1
mkdir -p $workdir && cd $workdir || exit 1

export CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT"
export CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT"
export LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT"

# build and install SDL2 for TinyViz
cd $THIRD_PARTY_DIR
if [ ! -d $destdir/include/SDL2 ]; then
    if [ ! -f SDL2-2.0.14.tar.gz ]; then
        echo "Package SDL2-2.0.14.tar.gz not found under $THIRD_PARTY_DIR"
    else
        tar -zxf SDL2-2.0.14.tar.gz -C $workdir
        cd $workdir/SDL2-2.0.14
        ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$destdir --host=aarch64-oe-linux \
          --with-sysroot --enable-esd=no --enable-pulseaudio=no \
          --enable-dbus=no
        make -j16
        make install
    fi
fi

# build and install SDL2_gfx
cd $THIRD_PARTY_DIR
if [ ! -f $destdir/lib/libSDL2_gfx.so ]; then
    if [ ! -f SDL2_gfx-1.0.4.tar.gz ]; then
        echo "Package SDL2_gfx-1.0.4.tar.gz not found under $THIRD_PARTY_DIR"
    else
        tar -zxf SDL2_gfx-1.0.4.tar.gz -C $workdir
        cd $workdir/SDL2_gfx-1.0.4
        cp /usr/share/libtool/build-aux/config.sub .
        cp /usr/share/libtool/build-aux/config.guess .
        ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$destdir --host=aarch64-oe-linux \
          --with-sysroot --with-sdl-prefix=$destdir --enable-mmx=no
        make -j16
        make install
    fi
fi

# build and install SDL2_ttf
cd $THIRD_PARTY_DIR
if [ ! -f $destdir/lib/libSDL2_ttf.so ]; then
    if [ ! -f SDL2_ttf-2.0.15.tar.gz ]; then
        echo "Package SDL2_ttf-2.0.15.tar.gz not found under $THIRD_PARTY_DIR"
    else
        export CC="$CC --sysroot=$TOOLCHAIN_SYSROOT"
        tar -zxf SDL2_ttf-2.0.15.tar.gz -C $workdir
        cd $workdir/SDL2_ttf-2.0.15/external/freetype-2.9.1
        sh ./autogen.sh
        ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$destdir --with-png=no \
          --host=aarch64-oe-linux --with-sysroot
        sed -i '86s/^/# /' ./builds/unix/unix-cc.mk
        make -j16
        make install
        cd $workdir/SDL2_ttf-2.0.15
        ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
          --prefix=$destdir \
          --with-sdl-prefix=$destdir \
          --with-ft-prefix=$destdir \
          --host=aarch64-oe-linux --with-sysroot
        sed -i '264d' ./Makefile
        sed -i '264i CFLAGS = ${C_FLAGS} -I${destdir}/include/freetype2 -I${destdir}/include/SDL2 -D_REENTRANT' ./Makefile
        sed -i '279d' ./Makefile
        sed -i '279i FT2_CFLAGS = -I${destdir}/include/freetype2' ./Makefile
        sed -i '281d' ./Makefile
        sed -i '281i FT2_LIBS = ${LD_FLAGS} -L${destdir}/lib -lfreetype' ./Makefile
        sed -i '293d' ./Makefile
        sed -i '293i LIBS = ${LD_FLAGS} -L${destdir}/lib -lfreetype -lSDL2' ./Makefile
        make -j16 destdir=${destdir} C_FLAGS="${CFLAGS}" LD_FLAGS="${LDFLAGS}"
        make install
    fi
fi
