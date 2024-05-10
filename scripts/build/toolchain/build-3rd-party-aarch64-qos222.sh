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


# build and install SDL2 for TinyViz
if ! [ -f SDL2-2.0.14.tar.gz ]; then
  wget https://www.libsdl.org/release/SDL2-2.0.14.tar.gz --no-check-certificate
fi

if ! [ -d SDL2-2.0.14 ]; then
  tar xf SDL2-2.0.14.tar.gz
fi

if ! [ -d $destdir/include/SDL2 ]; then
  cd SDL2-2.0.14
  sed -i '241,249s/^/    \/\/ /' ./src/audio/qsa/SDL_qsa_audio.c
  ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      --prefix=$destdir --host=aarch64-unknown-nto-qnx \
      --with-sysroot --enable-esd=no --enable-pulseaudio=no
  make -j16
  make install
  cd -
fi


if ! [ -f SDL2_gfx-1.0.4.tar.gz ]; then
  wget https://versaweb.dl.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz --no-check-certificate
fi

if ! [ -d SDL2_gfx-1.0.4 ]; then
  tar xf SDL2_gfx-1.0.4.tar.gz
fi

if ! [ -f $destdir/lib/libSDL2_gfx.so ]; then
  cd SDL2_gfx-1.0.4
  ./configure CXXFLAGS="-O3 -g" CFLAGS="-O3 -g" \
      --prefix=$destdir --host=aarch64-unknown-nto-qnx \
      --with-sysroot --with-sdl-prefix=$destdir --enable-mmx=no
  make -j16
  make install
  cd -
fi

if ! [ -f SDL2_ttf-2.0.15.tar.gz ]; then
  wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz --no-check-certificate
fi

if ! [ -d SDL2_ttf-2.0.15 ]; then
  tar xf SDL2_ttf-2.0.15.tar.gz
fi

if ! [ -f $destdir/lib/libSDL2_ttf.so ]; then
  cd SDL2_ttf-2.0.15
  if ! [ -d $destdir/include/freetype2 ]; then
    cd external/freetype-2.9.1
    ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
        CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
        LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT" \
        --with-png=no \
        --prefix=$destdir --host=aarch64-unknown-nto-qnx --with-sysroot
    sed -i '86s/^/# /' ./builds/unix/unix-cc.mk
    make -j16
    make install
    cd -
  fi
  ./configure CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT" \
      LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT" \
      --prefix=$destdir --with-sdl-prefix=$destdir \
      --with-ft-prefix=$destdir --host=aarch64-unknown-nto-qnx
  sed -i '264d' ./Makefile
  sed -i '264i CFLAGS = -O3 -g -I${destdir}/include/freetype2 -I${destdir}/include/SDL2 -D_REENTRANT' ./Makefile
  sed -i '279d' ./Makefile
  sed -i '279i FT2_CFLAGS = -I${destdir}/include/freetype2' ./Makefile
  sed -i '281d' ./Makefile
  sed -i '281i FT2_LIBS = -L${destdir}/lib -lfreetype' ./Makefile
  sed -i '293d' ./Makefile
  sed -i '293i LIBS =  -L${destdir}/lib -lfreetype -L${destdir}/lib -lSDL2' ./Makefile
  make -j16 destdir=${destdir}
  make install
  cd -
fi

