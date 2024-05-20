# Build scripts for RideHal

*Menu*:
- [Build scripts for RideHal](#build-scripts-for-ridehal)
  - [How to build the RideHal package with RideHal base docker](#how-to-build-the-ridehal-package-with-ridehal-base-docker)
  - [How to build the RideHal package with QNX BSP Snapdragon\_Auto.QX.4.4.0](#how-to-build-the-ridehal-package-with-qnx-bsp-snapdragon_autoqx440)
  - [How to build the RideHal package with Snapdragon\_Auto.HGY.4.1.6.0.r1 Linux SDK](#how-to-build-the-ridehal-package-with-snapdragon_autohgy4160r1-linux-sdk)
  - [How to build the RideHal package with Snapdragon\_Auto.HGY.4.1.6.0.r1 Ubuntu SDK](#how-to-build-the-ridehal-package-with-snapdragon_autohgy4160r1-ubuntu-sdk)
  - [How to run](#how-to-run)

## How to build the RideHal package with RideHal base docker
- Step 1: build the ridehal-toolchain-base docker image.
  Reference: [Guide to build ridehal-toolchain-base docker](docker/README.md)

- Step 2: create the ridehal-toolchain-base docker container
  - Create a script named create-ridehal-base-docker.sh, fill the below shell commands:
    ```
    #!/bin/bash

    export CONTAINER_NAME="ridehal-toolchain-base-env"
    echo $CONTAINER_NAME
    sudo xhost +
    if [ ! -d $PWD/toolchain ]; then
        mkdir $PWD/toolchain
    fi
    sudo docker run --net=host --pid=host -it --privileged --name $CONTAINER_NAME -v $PWD/ridehal:/opt/sdk/ridehal -v $PWD/toolchain:/opt/toolchain ridehal-toolchain-base:v1.0 /bin/bash
    ```
  - Create a script named run-ridehal-base-docker.sh, fill the below shell commands:
    ```
    #!/bin/bash

    sudo docker start ridehal-toolchain-base-env
    sudo docker exec -ti ridehal-toolchain-base-env /bin/bash
    ```
  Run the create-ridehal-base-docker.sh, a docker container named ridehal-toolchain-base-env would be created. If you want to rerun this container, just simply run the run-ridehal-base-docker.sh.

- Step 3: Build ridehal package
  The first time you created the ridehal-toolchain-base-env docker container, a directory named "data" will be created under this path. You need to copy platform CRM toolchain SDK and QNN SDK to "data" path, which would be shown in "/data" path in docker container. Then you need to untar the SDK packages, and rename QNN SDK folder to "qnn_sdk", rename hgy sdk folder to "linux", rename ubuntu sdk folder to "ubuntu".
  Rerun this container, switch to "/opt/sdk/ridehal" path, then use the scripts to build ridehal package:
    - QOS222:
      ```
      ./scripts/build/build-target.sh aarch64-qos222 .
      ```
    - HGY Linux:
      ```
      ./scripts/build/build-target.sh aarch64-linux .
      ```
    - HGY Ubuntu:
      ```
      ./scripts/build/build-target.sh aarch64-ubuntu .
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

## How to build the RideHal package with Snapdragon_Auto.HGY.4.1.6.0.r1 Linux SDK

Below is the list of commands about how to build ridehal:

```sh
export LINUX_SDK_ROOT=/path/to/linux/oe/sdk
# LINUX_SDK_ROOT is a directory contains the sysroots, that means there is directory:
#       ${LINUX_SDK_ROOT}/sysroots/aarch64-oe-linux
# for examples: export LINUX_SDK_ROOT=/local/mnt/workspace/sdk/ubuntu
# if want to build with QNN SDK
source /path/to/QNN_SDK/bin/envsetup.sh
# for example:  source ~/qnn-release/qaisw-v2.16.0.231027072756_64280-auto/bin/envsetup.sh

cd /path/to/ridehal
./scripts/build/build-target.sh aarch64-linux .
# the ridehal-aarch64-linux.tar.gz is the build out package for aarch64 linux
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


