*Menu*:
- [1. RideHal ColorConvertor Data Structures](#1-ridehal-ColorConvertor-data-structures)
  - [1.1 The details of ColorConvertor_Config_t](#11-ColorConvertor_config_t)
- [2. RideHal ColorConvertor APIs](#2-ridehal-ColorConvertor-apis)
  - [2.1 The details of ColorConvertor::Init](#21-ColorConvertorinit)
  - [2.2 The details of ColorConvertor::RegisterBuffers](#22-ColorConvertorRegisterBuffers)
  - [2.3 The details of ColorConvertor::DeRegisterBuffers](#23-ColorConvertordeRegisterBuffers)
  - [2.4 The details of ColorConvertor::Start](#24-ColorConvertorstart)
  - [2.5 The details of ColorConvertor::Stop](#25-ColorConvertorstop)
  - [2.6 The details of ColorConvertor::Deinit](#26-ColorConvertordeinit)
  - [2.7 The details of ColorConvertor::Execute](#27-ColorConvertorexecute)
- [3. Typical use case](#3-typical-use-case)
  - [3.1 Set configurations](#31-set-configurations)
  - [3.2 Call flow](#32-call-flow)

# 1. RideHal ColorConvertor Data Structures
## 1.1 The details of ColorConvertor_Config_t
The structure [ColorConvertor_Config_t](../include/ridehal/component/ColorConvertor.hpp#L35) contains all the required configurable parameters for a ColorConvertor pipeline. It contains:
- inputWidth, input image width.
- inputHeight, input image height.
- inputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported input image format is NV12 for now, so it could noly be RIDEHAL_IMAGE_FORMAT_NV12.
- outputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported output image format is RGB for now, so it could noly be RIDEHAL_IMAGE_FORMAT_RGB888.

# 2. RideHal ColorConvertor APIs 
## 2.1 The details of ColorConvertor::Init
[ColorConvertor::Init](../include/ridehal/component/ColorConvertor.hpp#L56) do all the initialization work for a ColorConvertor pipeline, including initialize the ComponentIF and logger, parse configuration parameters, setup OpenCL command queue and context, load OpenCL kernel and build OpenCL program . It should be called at the beginning of pipeline.
## 2.2 The details of ColorConvertor::RegisterBuffers
[ColorConvertor::RegisterBuffers](../include/ridehal/component/ColorConvertor.hpp#L87) register OpenCL device buffers from host buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be registered at execute step.
## 2.3 The details of ColorConvertor::DeRegisterBuffers
[ColorConvertor::DeRegisterBuffers](../include/ridehal/component/ColorConvertor.hpp#L96) deregister OpenCL device buffers from host buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be deregistered at deinit step.
## 2.4 The details of ColorConvertor::Start
[ColorConvertor::Start](../include/ridehal/component/ColorConvertor.hpp#L64) start the ColorConvertor pipeline, empty for now.
## 2.5 The details of ColorConvertor::Stop
[ColorConvertor::Stop](../include/ridehal/component/ColorConvertor.hpp#L71) stop the ColorConvertor pipeline, empty for now.
## 2.6 The details of ColorConvertor::Deinit
[ColorConvertor::Deinit](../include/ridehal/component/ColorConvertor.hpp#L78) do all the deinitialization work for a ColorConvertor pipeline, including deinitialize ComponentIF and logger, release OpenCL kernel and program, deregister all the OpenCL buffers remained. It should be called at the ending of pipeline.
## 2.7 The details of ColorConvertor::Execute
[ColorConvertor::Execute](../include/ridehal/component/ColorConvertor.hpp#L105) execute the ColorConvertor pipeline. Currently only the pipeline of single image input buffer to single output image buffer is supported.

# 3. Typical use case
## 3.1 Set configurations
Ridehal ColorConvertor component support to convert NV12 image input to RGB image output. The configuration parameters could be set as followed example:
```c++
    ColorConvertor ColorConvertorObj;
    ColorConvertor_Config_t ColorConvertorConfig;
    char pName[20] = "ColorConvertor";
    ColorConvertorConfig.inputWidth = 1920;
    ColorConvertorConfig.inputHeight = 1024;
    ColorConvertorConfig.inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    ColorConvertorConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
```
## 3.2 API Call flow
The typical call flow of a Ridehal ColorConvertor pipeline is showed as below codes:
```c++
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHal_SharedBuffer_t input;
    ret = input.Allocate( ColorConvertorConfig.inputWidth, ColorConvertorConfig.inputHeight,
                          ColorConvertorConfig.inputFormat );
    RideHal_SharedBuffer_t output;
    ret = output.Allocate( ColorConvertorConfig.inputWidth, ColorConvertorConfig.inputHeight,
                           ColorConvertorConfig.outputFormat );
    ret = ColorConvertorObj.Init( pName, &ColorConvertorConfig );
    ret = ColorConvertorObj.RegisterBuffers( &input, 1 );
    ret = ColorConvertorObj.RegisterBuffers( &output, 1 );
    ret = ColorConvertorObj.Execute( &input, &output );
    ret = ColorConvertorObj.DeRegisterBuffers( &input, 1 );
    ret = ColorConvertorObj.DeRegisterBuffers( &output, 1 );
    ret = ColorConvertorObj.Deinit();
    ret = input.Free();
    ret = output.Free();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.
Calling of RegisterBuffers and DeRegisterBuffers API for input/output buffer is optional, if the register step is not done by user explicitly, it would be done in Execute API implicitly. 

Reference:
- [gtest_ColorConvertor](../tests/unit_test/components/ColorConvertor/gtest_ColorConvertor.cpp)
