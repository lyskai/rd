# How to build RideHal with TinyViz for HGY Ubuntu

## Set workspace path:
export workspace=$PWD (or other path.)

## Prepare host environment
On Ubuntu 20.04 host environment, install dependent packages:
- Install basic dependency:
```sh
sudo apt install rsync wget curl vim \
    cpio unzip zip lsof bc pixz pigz tree gawk \
    build-essential pkg-config libtool autoconf automake debhelper \
    bison flex gdb gdb-multiarch strace ltrace tzdata locales \
    chrpath texinfo binutils ftp lftp telnet \
    apt-transport-https ca-certificates gnupg software-properties-common \
    openssh-client git git-lfs diffstat openssl libssl-dev
```

- Install SDL2 dependency:
```sh
sudo apt install libwayland-dev libwayland-bin libegl-dev libxkbcommon-dev
```

- Install QNN Linux dependency:
```sh
sudo apt install libncurses5 libgl1 libasound2-dev \
    libnss3 libgbm-dev desktop-file-utils \
    && add-apt-repository 'deb http://cz.archive.ubuntu.com/ubuntu focal main universe' \
    && apt-get update && apt install -y \
    flatbuffers-compiler libflatbuffers-dev rename
```

- Install HGY Ubuntu toolchain:
```sh
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
sudo rm -rf /lib/ld-linux-aarch64.so.1
sudo ln -sf /usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1 /lib/ld-linux-aarch64.so.1
```

- Install cmake:
```sh
wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0.tar.gz -P $workspace --no-check-certificate
cd $workspace && tar -zxf cmake-3.20.0.tar.gz
cd $workspace/cmake-3.20.0
sudo ./bootstrap
sudo make -j8
sudo make install
cd $workspace && rm -rf $workspace/cmake-3.20.0
```

## Setup HGY Ubuntu toolchain env
```sh
mkdir -p $workspace/ubuntu
# copy oecore-x86_64-aarch64-sa8775-ubuntu-toolchain-nodistro.0.sh from LR.AU.0.1.1.r2-16600-gen4meta.1-2\apps_proc\poky\build\tmp-glibc\deploy\sdk to $workspace/ubuntu
cd $workspace/ubuntu
sudo chmod +x ./oecore-x86_64-aarch64-sa8775-ubuntu-toolchain-nodistro.0.sh
sudo ./oecore-x86_64-aarch64-sa8775-ubuntu-toolchain-nodistro.0.sh -y -d .
export UBUNTU_HOST=$workspace/ubuntu/sysroots/x86_64-oesdk-linux
export UBUNTU_TARGET=$workspace/ubuntu/sysroots/aarch64-oe-linux
export TOOLCHAIN_SYSROOT=$UBUNTU_TARGET
```

## Build and install SDL libraries

- Set toolchain environment
```sh
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
export LD=aarch64-linux-gnu-ld
export AR=aarch64-linux-gnu-ar
export AS=aarch64-linux-gnu-as
export NM=aarch64-linux-gnu-nm
export RANLIB=aarch64-linux-gnu-ranlib
export STRIP=aarch64-linux-gnu-strip
```

- Set toolchain flags
```sh
export CFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT -I$TOOLCHAIN_SYSROOT/usr/include/linux-ark"
export CXXFLAGS="-O3 -g --sysroot=$TOOLCHAIN_SYSROOT -I$TOOLCHAIN_SYSROOT/usr/include/linux-ark"
export LDFLAGS="--sysroot=$TOOLCHAIN_SYSROOT"
export LDFLAGS="$LDFLAGS -L$TOOLCHAIN_SYSROOT/usr/lib/aarch64-linux-gnu"
```

- Set and create object file path
```sh
export TARGET=aarch64-ubuntu
export PKG_NAME=ridehal-$TARGET.tar.gz
export INSTALL_PATH=/opt/ridehal
export TARGET_PATH=$workspace/opt/ridehal
export WORK_DIR=$TARGET_PATH/bld-$TARGET
export DST_DIR=$TARGET_PATH/run-$TARGET/opt/ridehal

mkdir -p $TARGET_PATH $WORK_DIR $DST_DIR
```

- Set cmake environment:
Create a file named toolchain-aarch64-ubuntu.cmake under $workspace/ubuntu path.
Fill with these contents:
```
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR aarch64 )

set( arch gcc_hgyaarch64le )

set( CMAKE_C_COMPILER aarch64-linux-gnu-gcc )
set( CMAKE_C_COMPILER_TARGET ${arch} )
set( CMAKE_CXX_COMPILER aarch64-linux-gnu-g++ )
set( CMAKE_CXX_COMPILER_TARGET ${arch} )

set( CMAKE_SYSROOT $ENV{UBUNTU_TARGET} )

add_link_options( "-L$ENV{UBUNTU_TARGET}/usr/lib/aarch64-linux-gnu" )
include_directories( $ENV{UBUNTU_TARGET}/usr/include/linux-ark )
```
Set the cmake toolchain env:
```sh
export CMAKE_TOOLCHAIN_FILE=$workspace/ubuntu/toolchain-aarch64-ubuntu.cmake
```

- Build and install SDL2 for TinyViz:
```sh
cd $workspace
wget https://www.libsdl.org/release/SDL2-2.0.14.tar.gz -P $workspace --no-check-certificate
tar -zxf SDL2-2.0.14.tar.gz
cd SDL2-2.0.14
sudo ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
        --prefix=$DST_DIR --host=aarch64-gnu-linux \
        --with-sysroot --enable-esd=no --enable-pulseaudio=no \
        --enable-dbus=no
sudo make -j16
sudo make install
```

- Build and install SDL2_gfx:
```sh
cd $workspace
wget https://versaweb.dl.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz -P $workspace --no-check-certificate
tar -zxf SDL2_gfx-1.0.4.tar.gz
cd SDL2_gfx-1.0.4
cp /usr/share/libtool/build-aux/config.sub .
cp /usr/share/libtool/build-aux/config.guess .
sudo ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
        --prefix=$DST_DIR --host=aarch64-gnu-linux \
        --with-sysroot --with-sdl-prefix=$DST_DIR --enable-mmx=no
sudo make -j16
sudo make install
```

- Build and install SDL2_ttf:
```sh
cd $workspace
wget https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz -P $workspace --no-check-certificate
tar -zxf SDL2_ttf-2.0.15.tar.gz
cd SDL2_ttf-2.0.15/external/freetype-2.9.1
sudo ./autogen.sh
sudo ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
        --prefix=$DST_DIR --with-png=no \
        --host=aarch64-gnu-linux --with-sysroot
sed -i '86s/^/# /' ./builds/unix/unix-cc.mk
sudo make -j16
sudo make install
cd $workspace/SDL2_ttf-2.0.15
sudo ./configure CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
        --prefix=$DST_DIR \
        --with-sdl-prefix=$DST_DIR \
        --with-ft-prefix=$DST_DIR \
        --host=aarch64-gnu-linux
sed -i '264d' ./Makefile
sed -i '264i CFLAGS = ${C_FLAGS} -I${DST_DIR}/include/freetype2 -I${DST_DIR}/include/SDL2 -D_REENTRANT' ./Makefile
sed -i '279d' ./Makefile
sed -i '279i FT2_CFLAGS = -I${DST_DIR}/include/freetype2' ./Makefile
sed -i '281d' ./Makefile
sed -i '281i FT2_LIBS = ${LD_FLAGS} -L${DST_DIR}/lib -lfreetype' ./Makefile
sed -i '293d' ./Makefile
sed -i '293i LIBS = ${LD_FLAGS} -L${DST_DIR}/lib -lfreetype -lSDL2' ./Makefile
sudo make -j16 destdir=${DST_DIR} C_FLAGS="${CFLAGS}" LD_FLAGS="${LDFLAGS}"
sudo make install
```

## Build RideHal package
- Setup QNN SDK env:
Unzip QNN SDK package to $workspace and rename as qnn_sdk.
```sh
source $workspace/qnn_sdk/bin/envsetup.sh
```

- build RideHal SDK:
Get source code and put at $workspace path, then use the following commands to build:
```sh
cd $workspace/ridehal
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    -DCMAKE_INCLUDE_PATH=$TOOLCHAIN_SYSROOT/usr/include \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
    -DCMAKE_PREFIX_PATH=$DST_DIR \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_GCOV=OFF ..
sudo make -j16
sudo make DESTDIR=$DST_DIR install
```

- copy QNN library to RideHal
```sh
mkdir $DST_DIR/lib/dsp
cp $QNN_SDK_ROOT/bin/aarch64-rh-linux-gcc9.3/* $DST_DIR/bin
cp $QNN_SDK_ROOT/lib/aarch64-rh-linux-gcc9.3/* $DST_DIR/lib
cp $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/libQnn* $DST_DIR/lib/dsp
```

- copy font file to RideHal
```sh
cd $workspace
mkdir font
cd font
wget https://dl.dafont.com/dl/?f=liberation_sans -O liberation_sans.zip --no-check-certificate
unzip liberation_sans.zip
mkdir $DST_DIR/lib/runtime
cp LiberationSans-Regular.ttf $DST_DIR/lib/runtime
```

- generate RideHal package
```sh
cd $workspace
mv $DST_DIR/opt/ridehal/lib/lib* $DST_DIR/lib
mv $DST_DIR/opt/ridehal/lib/dsp/* $DST_DIR/lib/dsp
mv $DST_DIR/opt/ridehal/include/* $DST_DIR/include
mv $DST_DIR/opt/ridehal/bin/* $DST_DIR/bin
rm -rf $DST_DIR/opt
tar -C $TARGET_PATH --xform="s/run/pkg/" --exclude="*.a" \
    --exclude="*.la" --exclude="include" --exclude="share" \
    --exclude="cmake" \
    --use-compress-program=pigz -cf $PKG_NAME run-$TARGET
```
