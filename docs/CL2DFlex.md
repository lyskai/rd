*Menu*:
- [1. RideHal CL2DFlex Data Structures](#1-ridehal-CL2DFlex-data-structures)
  - [1.1 The details of CL2DFlex_Config_t](#11-CL2DFlex_config_t)
- [2. RideHal CL2DFlex APIs](#2-ridehal-CL2DFlex-apis)
  - [2.1 The details of CL2DFlex::Init](#21-CL2DFlexinit)
  - [2.2 The details of CL2DFlex::RegisterBuffers](#22-CL2DFlexRegisterBuffers)
  - [2.3 The details of CL2DFlex::DeRegisterBuffers](#23-CL2DFlexdeRegisterBuffers)
  - [2.4 The details of CL2DFlex::Start](#24-CL2DFlexstart)
  - [2.5 The details of CL2DFlex::Stop](#25-CL2DFlexstop)
  - [2.6 The details of CL2DFlex::Deinit](#26-CL2DFlexdeinit)
  - [2.7 The details of CL2DFlex::Execute](#27-CL2DFlexexecute)
- [3. Typical use case](#3-typical-use-case)
  - [3.1 Set configurations](#31-set-configurations)
  - [3.2 Call flow](#32-call-flow)

# 1. RideHal CL2DFlex Data Structures
## 1.1 The details of CL2DFlex_Config_t
The structure [CL2DFlex_Config_t](../include/ridehal/component/CL2DFlex.hpp#L35) contains all the required configurable parameters for a CL2DFlex pipeline. It contains:
- inputWidth, input image width.
- inputHeight, input image height.
- inputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported input image format is NV12 for now, so it could noly be RIDEHAL_IMAGE_FORMAT_NV12.
- outputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported output image format is RGB for now, so it could noly be RIDEHAL_IMAGE_FORMAT_RGB888.

# 2. RideHal CL2DFlex APIs 
## 2.1 The details of CL2DFlex::Init
[CL2DFlex::Init](../include/ridehal/component/CL2DFlex.hpp#L56) do all the initialization work for a CL2DFlex pipeline, including initialize the ComponentIF and logger, parse configuration parameters, setup OpenCL command queue and context, load OpenCL kernel and build OpenCL program . It should be called at the beginning of pipeline.
## 2.2 The details of CL2DFlex::RegisterBuffers
[CL2DFlex::RegisterBuffers](../include/ridehal/component/CL2DFlex.hpp#L87) register OpenCL device buffers from host buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be registered at execute step.
## 2.3 The details of CL2DFlex::DeRegisterBuffers
[CL2DFlex::DeRegisterBuffers](../include/ridehal/component/CL2DFlex.hpp#L96) deregister OpenCL device buffers from host buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be deregistered at deinit step.
## 2.4 The details of CL2DFlex::Start
[CL2DFlex::Start](../include/ridehal/component/CL2DFlex.hpp#L64) start the CL2DFlex pipeline, empty for now.
## 2.5 The details of CL2DFlex::Stop
[CL2DFlex::Stop](../include/ridehal/component/CL2DFlex.hpp#L71) stop the CL2DFlex pipeline, empty for now.
## 2.6 The details of CL2DFlex::Deinit
[CL2DFlex::Deinit](../include/ridehal/component/CL2DFlex.hpp#L78) do all the deinitialization work for a CL2DFlex pipeline, including deinitialize ComponentIF and logger, release OpenCL kernel and program, deregister all the OpenCL buffers remained. It should be called at the ending of pipeline.
## 2.7 The details of CL2DFlex::Execute
[CL2DFlex::Execute](../include/ridehal/component/CL2DFlex.hpp#L105) execute the CL2DFlex pipeline. Currently only the pipeline of single image input buffer to single output image buffer is supported.

# 3. Typical use case
## 3.1 Set configurations
Ridehal CL2DFlex component support to convert NV12 image input to RGB image output. The configuration parameters could be set as followed example:
```c++
    CL2DFlex CL2DFlexObj;
    CL2DFlex_Config_t CL2DFlexConfig;
    char pName[20] = "CL2DFlex";
    CL2DFlexConfig.inputWidth = 1920;
    CL2DFlexConfig.inputHeight = 1024;
    CL2DFlexConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    CL2DFlexConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
```
## 3.2 API Call flow
The typical call flow of a Ridehal CL2DFlex pipeline is showed as below codes:
```c++
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHal_SharedBuffer_t input;
    ret = input.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                          CL2DFlexConfig.inputFormat );
    RideHal_SharedBuffer_t output;
    ret = output.Allocate( CL2DFlexConfig.inputWidth, CL2DFlexConfig.inputHeight,
                           CL2DFlexConfig.outputFormat );
    ret = CL2DFlexObj.Init( pName, &CL2DFlexConfig );
    ret = CL2DFlexObj.RegisterBuffers( &input, 1 );
    ret = CL2DFlexObj.RegisterBuffers( &output, 1 );
    ret = CL2DFlexObj.Execute( &input, &output );
    ret = CL2DFlexObj.DeRegisterBuffers( &input, 1 );
    ret = CL2DFlexObj.DeRegisterBuffers( &output, 1 );
    ret = CL2DFlexObj.Deinit();
    ret = input.Free();
    ret = output.Free();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.
Calling of RegisterBuffers and DeRegisterBuffers API for input/output buffer is optional, if the register step is not done by user explicitly, it would be done in Execute API implicitly. 

Reference:
- [gtest_CL2DFlex](../tests/unit_test/components/CL2DFlex/gtest_CL2DFlex.cpp)
