# 1. RideHal C2D Data Structures

- [C2D_Config_t](../include/ridehal/component/C2D.hpp#L53)
- [C2D_InputConfig_t](../include/ridehal/component/C2D.hpp#L46)
- [C2D_ROIConfig_t](../include/ridehal/component/C2D.hpp#L38)
- [C2D_ImageResolution_t](../include/ridehal/component/C2D.hpp#L29)

## 1.1 The details of C2D_Config_t
C2D_Config_t is the data structure that used to initialize C2D component. It contains C2D input configurations, all the parameters needs to be set by user.
The parameter numOfInputs must be an positive integer in the range of [1, 32]. The inputConfigs is an array which contains 32 elements, each element contains image format, image resolution and ROI as input parameters of each input stream. This version of C2D supports NV12 as input image format. The input image width and height must be within the range of [128, 3000]. For input ROI, the topX and topY must be less than image width and height, and ROI width less than image width - topX, ROI height less than image height - topY.

# 2. RideHal C2D APIs

- [Initialize the C2D component](../include/ridehal/component/C2D.hpp#L78)
- [Start the C2D pipeline](../include/ridehal/component/C2D.hpp#L86)
- [Stop the C2D pipeline](../include/ridehal/component/C2D.hpp#L93)
- [Deinitialize the C2D component](../include/ridehal/component/C2D.hpp#L100)
- [Execute the C2D pipeline](../include/ridehal/component/C2D.hpp#L110)
- [Register shared buffers for each input](../include/ridehal/component/C2D.hpp#L120)
- [Register shared buffers for output](../include/ridehal/component/C2D.hpp#L130)
- [Deregister shared buffers for each input](../include/ridehal/component/C2D.hpp#L140)
- [Deregister shared buffers for output](../include/ridehal/component/C2D.hpp#L150)

# 3. Typical Use Case

It needs just several steps to configure and launch C2D component. User needs to set input and output parameters, the input format supports NV12, and output format supports UYVY. 

- Step 1: Configure input/output parameters
```c
C2D C2DObj;
C2D_Config_t C2DConfig;
C2D_Config_t *pC2DConfig = &C2DConfig;
char pName[5] = "C2D";
C2DConfig.numOfInputs = 1;

for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
{
    C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    C2DConfig.inputConfigs[i].inputResolution.width = 600;
    C2DConfig.inputConfigs[i].inputResolution.height = 600;
    C2DConfig.inputConfigs[i].ROI.topX = 100;
    C2DConfig.inputConfigs[i].ROI.topY = 100;
    C2DConfig.inputConfigs[i].ROI.width = 100;
    C2DConfig.inputConfigs[i].ROI.height = 100;
}

RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
uint32_t C2DOutputWidth = 600;
uint32_t C2DOutputHeight = 600;
```

- Step 2: Allocate input/output buffers
```c
RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                            C2DConfig.inputConfigs[0].inputResolution.height,
                            C2DConfig.inputConfigs[0].inputFormat );

RideHal_SharedBuffer_t output;
ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );
```

- Step 3: Initialize and start the pipeline
```c
ret = C2DObj.Init( pName, pC2DConfig );
ret = C2DObj.Start();
ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );
ret = C2DObj.Stop();
ret = C2DObj.Deinit();
```

Reference: 
- [gtest_ComponentC2D](../tests/unit_test/components/C2D/gtest_ComponentC2D.cpp)
- [SampleC2D](../tests/sample/source/SampleC2D.cpp)
