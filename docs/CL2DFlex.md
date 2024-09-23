*Menu*:
- [1. CL2DFlex Overview](#1-cl2dflex-overview)
- [2. CL2DFlex Data Structures](#2-cl2dflex-data-structures)
- [3. CL2DFlex APIs](#3-cl2dflex-apis)
- [4. Typical use case](#4-typical-use-case)
  - [4.1 Set configurations](#41-set-configurations)
  - [4.2 API Call flow](#42-api-call-flow)

# 1. CL2DFlex Overview
The RideHal CL2DFlex component is based on OpenCL library, it provides user-friendly APIs and visible CL kernels to do color conversion and resize on single image input. Currently support color conversion and resize of multiple image inputs to single output. The supported color conversion pipelines are NV12 to RGB, UYVY to RGB, UYVY to NV12.

# 2. CL2DFlex Data Structures
- [CL2DFlex_Config_t](../include/ridehal/component/CL2DFlex.hpp#L41)

# 3. CL2DFlex APIs 
- [CL2DFlex::Init](../include/ridehal/component/CL2DFlex.hpp#L64)
- [CL2DFlex::RegisterBuffers](../include/ridehal/component/CL2DFlex.hpp#L75)
- [CL2DFlex::Start](../include/ridehal/component/CL2DFlex.hpp#L81)
- [CL2DFlex::Execute](../include/ridehal/component/CL2DFlex.hpp#L93)
- [CL2DFlex::Stop](../include/ridehal/component/CL2DFlex.hpp#L100)
- [CL2DFlex::DeRegisterBuffers](../include/ridehal/component/CL2DFlex.hpp#L110)
- [CL2DFlex::Deinit](../include/ridehal/component/CL2DFlex.hpp#L119)

# 4. Typical use case

## 4.1 Set configurations

Ridehal CL2DFlex component can do image color conversion and resize. Take a NV12 to RGB resize pipeline as example, the configuration parameters can be set as:
```c++
    CL2DFlex_Config_t CL2DFlexConfig;
    CL2DFlexConfig.numOfInputs = 1;
    CL2DFlexConfig.inputWidths[0] = 1920;
    CL2DFlexConfig.inputHeights[0] = 1024;
    CL2DFlexConfig.inputFormats[0] = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputWidth = 1152;
    CL2DFlexConfig.outputHeight = 800;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
```

## 4.2 API Call flow

The typical call flow of a Ridehal CL2DFlex pipeline is showed as following example:
```c++
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHal_SharedBuffer_t input;
    ret = input.Allocate( CL2DFlexConfig.inputWidths[0], CL2DFlexConfig.inputHeights[0],
                          CL2DFlexConfig.inputFormats[0] );
    RideHal_SharedBuffer_t output;
    ret = output.Allocate( CL2DFlexConfig.outputWidth, CL2DFlexConfig.outputHeight,
                           CL2DFlexConfig.outputFormat );
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ret = CL2DFlexObj.Execute( &input, 1,&output );
    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ret = CL2DFlexObj.Deinit();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.
Calling of RegisterBuffers and DeRegisterBuffers API for input/output buffer is optional, if the register/deregister step is not done by user explicitly, it would be done in execute/deinit step implicitly. 

Reference:
- [gtest_CL2DFlex](../tests/unit_test/components/CL2DFlex/gtest_CL2DFlex.cpp)
- [SampleCL2DFlex](../tests/sample/source/SampleCL2DFlex.cpp)
