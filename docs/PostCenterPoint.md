*Menu*:
- [1. PostCenterPoint overview](#1-postcenterpoint-overview)
- [2. PostCenterPoint Data Structures](#2-postcenterpoint-data-structures)
- [3. PostCenterPoint APIs](#3-postcenterpoint-apis)
- [4. PostCenterPoint Examples](#4-postcenterpoint-examples)
  - [4.1 PostCenterPoint initialization](#41-postcenterpoint-initialization)
  - [4.2 PostCenterPoint execution](#42-postcenterpoint-execution)

# 1. PostCenterPoint overview

The Component PostCenterPoint is a postprocessing that extracts and filters bounding boxes from the center point network output according to the definition in paper [PointPillars: Fast Encoders for Object Detection from Point Clouds](https://arxiv.org/pdf/1812.05784).

# 2. PostCenterPoint Data Structures

- [PostCenterPoint_3DBBoxFilterParams_t](../include/ridehal/component/PostCenterPoint.hpp#L34)
- [PostCenterPoint_Config_t](../include/ridehal/component/PostCenterPoint.hpp#L68)
- [PostCenterPoint_Object3D_t](../include/ridehal/component/PostCenterPoint.hpp#L86)

# 3. PostCenterPoint APIs

- [Init](../include/ridehal/component/PostCenterPoint.hpp#L108)
- [Start](../include/ridehal/component/PostCenterPoint.hpp#L137)
- [Stop](../include/ridehal/component/PostCenterPoint.hpp#L143)
- [Deinit](../include/ridehal/component/PostCenterPoint.hpp#L149)
- [Execute](../include/ridehal/component/PostCenterPoint.hpp#L167)
- [RegisterBuffers](../include/ridehal/component/PostCenterPoint.hpp#L122)
- [DeRegisterBuffers](../include/ridehal/component/PostCenterPoint.hpp#L131)

# 4. PostCenterPoint Examples

## 4.1 PostCenterPoint initialization

The [gtest plrPostConfig0](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L51) and [gtest plrPostConfig1](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L71) are 2 typical PostCenterPoint configuration examples according to the pointpilalr model configuration.

And the plrPostConfig0 is for the [OpenPCDet default pointpillar config](https://github.com/open-mmlab/OpenPCDet/blob/master/tools/cfgs/kitti_models/pointpillar.yaml).

```c++
static PostCenterPoint_Config_t plrPostConfig0 = {
        RIDEHAL_PROCESSOR_HTP0,
        0.16,
        0.16, /* pillar size: x, y */
        0.0,
        -39.68, /* min Range, x, y */
        69.12,
        39.68,                                        /* max Range, x, y */
        3,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

PostCenterPoint plrPost;
RideHalError_e ret;

ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_INFO );
ret = plrPost.Start();
```

## 4.2 PostCenterPoint execution

The gtest code [SANITY_PostCenterPoint](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L336) is a good example, it does load the related input and output buffers from raw file by calling API [LoadRaw](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L121) or [LoadPoints](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L103). And this code also demonstrates that how to decode the detection output buffer, below is a copy of it.

```c++
    PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
    for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
    {
        printf( "[%d] class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
                "theta=%.3f, mean=[%.3f %.3f %.3f %.3f]x%u\n",
                i, pObj->label, pObj->score, pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                pObj->height, pObj->theta, pObj->meanPtX, pObj->meanPtY, pObj->meanPtZ,
                pObj->meanIntensity, pObj->numPts );
        pObj++;
    }
```

And the [SamplePlrPost](../tests/sample/source/SamplePlrPost.cpp#L228) is an end to end pipeline demo that how to call Execute API to extract bounding boxes.
