# Build scripts for RideHal

*Menu*:
- [How to build the RideHal package with RideHal build docker](#how-to-build-the-ridehal-package-with-ridehal-build-docker)
- [How to build the RideHal package with QNX BSP Snapdragon_Auto.QX.4.4.0](#how-to-build-the-ridehal-package-with-qnx-bsp-snapdragon_autoqx440)
- [How to build the RideHal package with Snapdragon_Auto.HGY.4.1.6.0.r1 Ubuntu SDK](#how-to-build-the-ridehal-package-with-snapdragon_autohgy4160r1-ubuntu-sdk)
- [How to run](#how-to-run)

## How to build the RideHal package with RideHal build docker

```sh
docker images # make sure having the RideWare docker loaded
rideware-toolchain-aarch64-fusion   latest    6f94fb6f5150   2 days ago      9.8GB
rideware-toolchain-aarch64-fusion   v1.3      6f94fb6f5150   2 days ago      9.8GB

# if you don't have latest, use docker tag create it
docker tag rideware-toolchain-aarch64-fusion:v1.3 rideware-toolchain-aarch64-fusion:latest

# if want to build with QNN SDK
source /path/to/QNN_SDK/bin/envsetup.sh
# for example:  source ~/qnn-release/qaisw-v2.16.0.231027072756_64280-auto/bin/envsetup.sh
docker run -it \
        -v $PWD/ridehal:/opt/sdk \
        -v $QNN_SDK_ROOT/:/opt/qnn_sdk \
        --net=host --privileged -v /dev/bus/usb:/dev/bus/usb \
        --rm rideware-toolchain-aarch64-fusion:latest bash

# if has QNN SDK run the below command, else skip the below command
source /opt/qnn_sdk/bin/envsetup.sh

cd /opt/sdk
./scripts/build/build-target.sh aarch64-qos222 .
# the ridehal-aarch64-qos222.tar.gz is the build out package for QNX

./scripts/build/build-target.sh aarch64-hgy .
# the ridehal-aarch64-hgy.tar.gz is the build out package for Linux HGY
```

## How to build the RideHal package with QNX BSP Snapdragon_Auto.QX.4.4.0

```sh
# ref the Release Note for Snapdragon_Auto.QX.4.4.0 to download QNX BSP and toolchain and build it

cd /path/to/qnx_ap
source setenv_qos222.sh
# if want to build with QNN SDK
source /path/to/QNN_SDK/bin/envsetup.sh
# for example:  source ~/qnn-release/qaisw-v2.16.0.231027072756_64280-auto/bin/envsetup.sh

cd /path/to/ridehal
./scripts/build/build-target.sh aarch64-qos222 .
# the ridehal-aarch64-qos222.tar.gz is the build out package for QNX
```

## How to build the RideHal package with Snapdragon_Auto.HGY.4.1.6.0.r1 Ubuntu SDK

Need ensure to use host ubuntu 20.04, and with below commands to install the ubuntu aarch64 compiler once.

```sh
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
sudo rm -rf /lib/ld-linux-aarch64.so.1
sudo ln -sf /usr/aarch64-linux-gnu/lib/ld-2.31.so /lib/ld-linux-aarch64.so.1
sudo ln -sf /bin/bash /bin/sh
```

Below is the list of commands about how to build ridehal:

```sh
export UBUNTU_SDK_ROOT=/path/to/ubuntu/sdk
# UBUNTU_SDK_ROOT is a directory contains the sysroots, that means there is directory:
#       ${UBUNTU_SDK_ROOT}/sysroots/aarch64-oe-linux
# for examples: export UBUNTU_SDK_ROOT=/local/mnt/workspace/sdk/ubuntu
# if want to build with QNN SDK
source /path/to/QNN_SDK/bin/envsetup.sh
# for example:  source ~/qnn-release/qaisw-v2.16.0.231027072756_64280-auto/bin/envsetup.sh

cd /path/to/ridehal
./scripts/build/build-target.sh aarch64-ubuntu .
# the ridehal-aarch64-ubuntu.tar.gz is the build out package for aarch64 ubuntu
```

## How to run
For how to run the RideHal package, check this [README](../launch/README.md).

