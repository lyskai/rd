# Overview

The RideHal is Qualcomm ADAS hardware abstraction layer that provides user friendly APIs to access the Qualcomm ADAS hardware accelerator. It is ADAS service oriented to provide user friendly component and utils to cover the perception and compute vision related task.

# Documents

Check this [INDEX](./docs/index.md).

Below is a summary information of the current implemented components.

| Component |  QNX HOST  | HGY Ubuntu  | Library  |   Processor     |  Feaures |
|-----------|------------|-------------|----------|-----------------|---------------------------------------|
| [Camera](./docs/camera.md) | YES| YES | qcarcam | Camera | camera streaming |
| [Remap](./docs/remap.md)| YES | YES | FastADAS | CPU, HTP0, HTP1, GPU | Color Conversion, ROI Crop, Downscaling, Undistortion |
| [C2D](./docs/C2D.md) | YES| NO | C2D | GPU | Color Conversion, ROI Crop, Resize |
| [QnnRuntime](./docs/qnnruntime.md) | YES| YES | QNN | CPU, GPU, HTP0, HTP1 | AI model inference |
| [VideoEncoder](./docs/videoencoder.md) | YES| YES | vidc | VPU | H264, H265 |
| VideoDecoder | YES| YES | vidc | VPU | H264, H265 |
| [Voxelization](./docs/Voxelization.md) | YES| YES | FastADAS, OpenCL| CPU, HTP0, HTP1, GPU | create pilliar |
| [PostCenterPoint](./docs/PostCenterPoint.md) | YES| YES | FastADAS | CPU, HTP0, HTP1 | extract boundbing boxes |
| [CL2DFlex](./docs/CL2DFlex.md) | YES| Yes| OpenCL | GPU | Color Conversion, Resize |
| [GL2DFlex](./docs/GL2DFlex.md) | NO | YES| OpenGL | GPU | Color Conversion, ROI Crop, Resize |

# How to build

For how to build the RideHal package, check this [README](./scripts/build/README.md).

# How to run

For how to run the RideHal package, check this [README](./scripts/launch/README.md).

For how to run the RideHal E2E perception pipeline sample, check this [README](./tests/sample/README.md).
