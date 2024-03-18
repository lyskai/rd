# Build scripts for RideHal

## How to build the RideHal package

```sh
docker images # make sure having the RideWare docker loaded
rideware-toolchain-aarch64-fusion   latest    ccbc373a84d8   5 days ago      9.1GB
rideware-toolchain-aarch64-fusion   v1.1      ccbc373a84d8   6 days ago      9.1GB
rideware-toolchain-aarch64-fusion   v1.0      cc7fa519ec7d   6 weeks ago     9.01GB

# if you don't have latest, use docker tag create it
docker tag rideware-toolchain-aarch64-fusion:v1.1 rideware-toolchain-aarch64-fusion:latest

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

For how to run the RideHal package, check this [README](../launch/README.md).
