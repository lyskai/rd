*Menu*:
- [1. RideHal Remap Data Structures](#1-ridehal-remap-data-structures)
  - [1.1 The details of Remap_Config_t](#11-remap_config_t)
  - [1.2 The details of Remap_InputConfig_t](#12-remap_inputconfig_t)
  - [1.3 The details of Remap_MapTable_t](#13-remap_maptable_t)
- [2. RideHal remap APIs](#2-ridehal-remap-apis)
  - [2.1 The details of Remap::Init](#21-remapinit)
  - [2.2 The details of Remap::RegBuf](#22-remapregbuf)
  - [2.3 The details of Remap::DeregBuf](#23-remapderegbuf)
  - [2.4 The details of Remap::Start](#24-remapstart)
  - [2.5 The details of Remap::Stop](#25-remapstop)
  - [2.6 The details of Remap::Deinit](#26-remapdeinit)
  - [2.7 The details of Remap::Execute](#27-remapexecute)
- [3. Typical use case](#3-typical-use-case)
  - [3.1 Set configurations](#31-set-configurations)
  - [3.2 Call flow](#32-call-flow)

# 1. RideHal Remap Data Structures
## 1.1 The details of Remap_Config_t
The structure [Remap_Config_t](../include/ridehal/component/Remap.hpp#L64) contains all the required configurable parameters for a remap pipeline. It contains:
- processor, [RideHal_ProcessorType_e](../include/ridehal/common/Types.hpp#L83) type parameter. Now CPU and DSP processors are supported to execute remap calculation, so it could be RIDEHAL_PROCESSOR_CPU, RIDEHAL_PROCESSOR_HTP0, RIDEHAL_PROCESSOR_HTP1.
- inputConfigs, [Remap_InputConfig_t](../include/ridehal/component/Remap.hpp#L48) type array, Contains input images information, the array length is [RIDEHAL_MAX_INPUTS](../include/ridehal/common/Types.hpp#L23).
- numOfInputs, number of input images.
- outputWidth, output image width.
- outputHeight, output image height.
- outputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported output image format is RGB for now, so it could noly be RIDEHAL_IMAGE_FORMAT_RGB888.
- normlzR, normalize parameter for R channel.
- normlzG, normalize parameter for G channel.
- normlzB, normalize parameter for B channel.
- bEnableUndistortion, enable undistortion or not.
- bEnableNormalize, enable normalization or not.
## 1.2 The details of Remap_InputConfig_t
The structure [Remap_InputConfig_t](../include/ridehal/component/Remap.hpp#L48) contains all the required configurable parameters for a input image. It contains:
- inputFormat, [RideHal_ImageFormat_e](../include/ridehal/common/Types.hpp#L112) type parameter. The supported input image format is RGB and UYVY for now, so it could be RIDEHAL_IMAGE_FORMAT_RGB888 or RIDEHAL_IMAGE_FORMAT_UYVY.
- inputWidth, input format width.
- inputHeight, input format height.
- mapWidth, output map width.
- mapHeight, output map height.
- remapTable, [Remap_MapTable_t](../include/ridehal/component/Remap.hpp#L36) type parameter. The remap table used if enable undistortion.
- ROI, region of interest structure of Fadas.
## 1.3 The details of Remap_MapTable_t
The structure [Remap_MapTable_t](../include/ridehal/component/Remap.hpp#L36) contains two float pointers which indicate the remap table.
- pMapX, floating point matrix. Each element is the column coordinate of the mapped location in the source image. Data size is mapWidth*mapHeight.
- pMapY, floating point matrix. Each element is the row coordinate of the mapped location in the source image. Data size is mapWidth*mapHeight.

# 2. RideHal remap APIs 
## 2.1 The details of Remap::Init
[Remap::Init](../include/ridehal/component/Remap.hpp#L90) do all the initialization work for a remap pipeline, including initialize the CPU&DSP processor and logger, create remap worker, create remap map. It should be called at the beginning of pipeline.
## 2.2 The details of Remap::RegBuf
[Remap::RegBuf](../include/ridehal/component/Remap.hpp#L101) register buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be registered at execute step.
## 2.3 The details of Remap::DeregBuf
[Remap::DeregBuf](../include/ridehal/component/Remap.hpp#L111) deregister buffers for input and output data. This step could be done by user or skipped. If skipped, all the buffers will be registered at deinit step.
## 2.4 The details of Remap::Start
[Remap::Start](../include/ridehal/component/Remap.hpp#L118) start the remap pipeline, empty for now.
## 2.5 The details of Remap::Stop
[Remap::Stop](../include/ridehal/component/Remap.hpp#L125) stop the remap pipeline, empty for now.
## 2.6 The details of Remap::Deinit
[Remap::Deinit](../include/ridehal/component/Remap.hpp#L132) do all the deinitialization work for a remap pipeline, including deinitialize the CPU&DSP processor and logger, destroy remap worker, destroy remap map. It should be called at the ending of pipeline.
## 2.7 The details of Remap::Execute
[Remap::Execute](../include/ridehal/component/Remap.hpp#L142) execute the remap pipeline. Currently  the pipeline of multiple images input buffers remap to single output image buffer is supported.

# 3. Typical use case
## 3.1 Set configurations
Ridehal Remap component support to do downscaling, color conversion, ROI crop, normalization and undistortion for multiple batches input images. The configuration parameters could be set as followed example:
```c++
    Remap_Config_t RemapConfig;
    char pName[10] = "Remap";
    RemapConfig.processor = RIDEHAL_PROCESSOR_HTP0;
    RemapConfig.numOfInputs = 2;
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        RemapConfig.inputConfigs[inputId].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        RemapConfig.inputConfigs[inputId].inputWidth = 512;
        RemapConfig.inputConfigs[inputId].inputHeight = 512;
        RemapConfig.inputConfigs[inputId].mapWidth = 256;
        RemapConfig.inputConfigs[inputId].mapHeight = 256;
        RemapConfig.inputConfigs[inputId].ROI.x = 0;
        RemapConfig.inputConfigs[inputId].ROI.y = 0;
        RemapConfig.inputConfigs[inputId].ROI.width = 256;
        RemapConfig.inputConfigs[inputId].ROI.height = 256;
    }
    RemapConfig.outputFormat = RIDEHAL_IMAGE_FORMAT_RGB888;
    RemapConfig.outputWidth = 256;
    RemapConfig.outputHeight = 256;
    RemapConfig.bEnableUndistortion = true;
    RemapConfig.bEnableNormalize = true;
    RemapConfig.normlzR.sub = 0.0;
    RemapConfig.normlzR.mul = 1.0;
    RemapConfig.normlzR.add = 0.0;
    RemapConfig.normlzG.sub = 0.0;
    RemapConfig.normlzG.mul = 1.0;
    RemapConfig.normlzG.add = 0.0;
    RemapConfig.normlzB.sub = 0.0;
    RemapConfig.normlzB.mul = 1.0;
    RemapConfig.normlzB.add = 0.0;
```
Note that normalization is invalid for RGB input format, the ROI.width+ROI.x should not be larger than mapWidth and the ROI.height+ROI.y should not be larger than mapHeight.
If bEnableUndistortion is set to true, user could do undistortion or lens distortion correction for fisheye type camera by using the calibrated mapping table mapX and mapY. The mapping table mapX and mapY are floating point matrixs, each element is the column/row coordinate of the mapped location in the source image.
```c++
    float *mapX = (float *) mapXBuffer.data();
    float *mapY = (float *) mapYBuffer.data();
    for ( int i = 0; i < mapHeight; i++ )
    {
        for ( int j = 0; j < mapWidth; j++ )
        {
            mapX[i * mapWidth + j] = j / mapWidth * inputWidth;
            mapY[i * mapWidth + j] = i / mapHeight * inputHeight;
        }
    }
    RemapConfig.inputConfigs[inputId].remapTable.pMapX = mapX;
    RemapConfig.inputConfigs[inputId].remapTable.pMapY = mapY;
```
## 3.2 API Call flow
The typical call flow of a Ridehal Remap pipeline is showed as below codes:
```c++
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    ret = RemapObj.Init( pName, pRemapConfig );
    RideHal_SharedBuffer_t inputs[RemapConfig.numOfInputs];
    for ( uint32_t inputId = 0; inputId < RemapConfig.numOfInputs; inputId++ )
    {
        ret = inputs[inputId].Allocate( RemapConfig.inputConfigs[inputId].inputWidth,
                                        RemapConfig.inputConfigs[inputId].inputHeight,
                                        RemapConfig.inputConfigs[inputId].inputFormat );
    }
    RideHal_SharedBuffer_t output;
    ret = output.Allocate( RemapConfig.numOfInputs, RemapConfig.outputWidth,
                           RemapConfig.outputHeight, RemapConfig.outputFormat );
    ret = RemapObj.RegBuf( inputs, RemapConfig.numOfInputs, FADAS_BUF_TYPE_IN );
    ret = RemapObj.RegBuf( &output, 1, FADAS_BUF_TYPE_OUT );
    ret = RemapObj.Execute( inputs, RemapConfig.numOfInputs, &output );
    ret = RemapObj.DeregBuf( inputs, RemapConfig.numOfInputs );
    ret = RemapObj.DeregBuf( &output, 1 );
    ret = RemapObj.Deinit();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.
Calling of RegBuf and DeregBuf API for input/output buffer is optional, if the register step is not done by user explicitly, it would be done in Execute API implicitly. 

Reference:
- [gtest_Remap](../tests/unit_test/components/Remap/gtest_Remap.cpp)
- [SampleRemap](../tests/sample/source/SampleRemap.cpp)