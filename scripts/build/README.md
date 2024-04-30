# Build scripts for RideHal

## How to build the RideHal package with RideHal build docker

```sh
docker images # make sure having the RideWare docker loaded
rideware-toolchain-aarch64-fusion   latest    6f94fb6f5150   2 days ago      9.8GB
rideware-toolchain-aarch64-fusion   v1.3      6f94fb6f5150   2 days ago      9.8GB

# if you don't have latest, use docker tag create it
docker tag rideware-toolchain-aarch64-fusion:v1.3 rideware-toolchain-aarch64-fusion:latest

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

cd /path/to/ridehal
./scripts/build/build-target.sh aarch64-qos222 .
# the ridehal-aarch64-qos222.tar.gz is the build out package for QNX
```

## How to run
For how to run the RideHal package, check this [README](../launch/README.md).

