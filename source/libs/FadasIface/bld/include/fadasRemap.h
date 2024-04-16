/***************************************************************************//**
@file
    fadasRemap.h

@brief
    Geometric Transformation

@example uyvy_remap/app.cpp
@example vyuy_remap/app.cpp
@example one/one.cpp

@defgroup remap Geometrical Transformation

@details
    Generic geometric and color transformation using the algorithm pipeline
    specified by #FadasRemapPipeline_e along with some initialization
    methods.  This feature is useful for lens distortion correction (LDC) using
    intrinsic calibration constants along geometric corrections using extrinsic
    calibration parameters.

@internal
    Copyright (c) 2020-2023 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASREMAP_H
#define FADASREMAP_H

#ifndef DOXYGEN_SKIP
#include <stdint.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#define DEFAULT_NTHREADS 4

/************************************************************************//**
@brief
    Remap algorithm pipeline specifying what algorithm pipeline is run by
    subsequent calls to FadasRemap_Run() and FadasRemap_RunMT().

@param FADAS_REMAP_PIPELINE_1C8
    Use map on a single-channel 8-bit image and ignore the ROI scaling
    variable.

@param FADAS_REMAP_PIPELINE_1C8_ROISCALE
    Use map on a single-channel 8-bit image and also use the ROI scaling
    variable to scale the given ROI.

@param FADAS_REMAP_PIPELINE_3C888
    Use map on a three-channel 8-bit image and ignore the ROI scaling
    variable.

@param FADAS_REMAP_PIPELINE_3C888_ROISCALE
    Use map on a three-channel 8-bit image and also use the ROI scaling
    variable to scale the given ROI.

@param FADAS_REMAP_PIPELINE_YUV888_TO_RGB888
    Use map on a three-channel 8-bit YUV image and convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB().

@param FADAS_REMAP_PIPELINE_UYVY_TO_RGB888
    Use map on a 8-bit UYVY image and convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB().

@param FADAS_REMAP_PIPELINE_VYUY_TO_RGB888
    Use map on a 8-bit VYUY image and convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB().

@param FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_ROISCALE
    Use map on a 8-bit UYVY image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB(), and
    also use the ROI scaling variable to scale the given ROI.

@param FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_ROISCALE
    Use map on a 8-bit VYUY image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB(), and
    also use the ROI scaling variable to scale the given ROI.

@param FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMI8
    Use map on a 8-bit UYVY image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB(), and
    also normalize output image with int8 output.

@param FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8
    Use map on a 8-bit UYVY image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_UYVYtoRGB(), and
    also normalize output image with uint8 output.

@param FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888
    Use map on a 8-bit Y8UV8 image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_Y8UV8toRGB888().

@param FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888
    Use map on a 8-bit Y8UV8 image, convert the color space
    from YUV to BGR using the matrix defined FadasCvtYUV_Y8UV8toBGR888().

@param FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMI8
    Use map on a 8-bit Y8UV8 image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_Y8UV8toRGB888(), and
    also normalize output image with int8 output.

@param FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8
    Use map on a 8-bit Y8UV8 image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_Y8UV8toRGB888(), and
    also normalize output image with uint8 output.

@param FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_ROISCALE
    Use map on a 8-bit Y8UV8 image, convert the color space
    from YUV to RGB using the matrix defined FadasCvtYUV_Y8UV8toRGB888(), and
    also use the ROI scaling variable to scale the given ROI.

@param FADAS_REMAP_PIPELINE_UYVY_TO_BGR888
    Use map on a 8-bit UYVY image and convert the color space
    from YUV to BGR.

@ingroup remap
****************************************************************************/
typedef enum : int32_t
{
    FADAS_REMAP_PIPELINE_1C8 = 0,                  ///< One 8-bit channel unconverted
    FADAS_REMAP_PIPELINE_1C8_ROISCALE,             ///< 8-bit channel unconverted + ROI scaling
    FADAS_REMAP_PIPELINE_3C888,                    ///< Three 8-bit channels unconverted
    FADAS_REMAP_PIPELINE_3C888_ROISCALE,           ///< Three 8-bit channels unconverted + ROI scaling
    FADAS_REMAP_PIPELINE_YUV888_TO_RGB888,         ///< Convert YUV888 to RGB888 [see FadasCvtYUV_UYVYtoRGB()]
    FADAS_REMAP_PIPELINE_UYVY_TO_RGB888,           ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()]
    FADAS_REMAP_PIPELINE_VYUY_TO_RGB888,           ///< Convert VYUY to RGB
    FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_ROISCALE,  ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + ROI scaling
    FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_ROISCALE,  ///< Convert VYUY to RGB + ROI scaling
    FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMI8,    ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + Renormalization with int8 output
    FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8,    ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + Renormalization with uint8 output
    FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888,
    FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888,
    FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMI8,
    FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8,
    FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_ROISCALE,
    FADAS_REMAP_PIPELINE_UYVY_TO_BGR888,           ///< Convert UYVY to BGR
    FADAS_REMAP_PIPELINE_MAX                       ///<  Do not use.  Must be last.
} FadasRemapPipeline_e;



/************************************************************************//**
@brief
    Distortion structure.

@param k1-k8
    Coefficients corresponding to the camera distortion model.

@ingroup remap
****************************************************************************/
typedef struct
{
    float64_t k1;  ///< First coefficient of the camera distortion model
    float64_t k2;  ///< Second coefficient of the camera distortion model
    float64_t k3;  ///< Third coefficient of the camera distortion model
    float64_t k4;  ///< Fourth coefficient of the camera distortion model
    float64_t k5;  ///< Fifth coefficient of the camera distortion model
    float64_t k6;  ///< Sixth coefficient of the camera distortion model
    float64_t k7;  ///< Seventh coefficient of the camera distortion model
    float64_t k8;  ///< Eighth coefficient of the camera distortion model
} FadasDistCoeffs_t;



/************************************************************************//**
@brief
    Generic geometric transformation map.
****************************************************************************/
typedef struct FadasRemapMap FadasRemapMap_t;



/************************************************************************//**
@brief
    Initialize FastADAS remap feature.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    Pointer to the license key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_Init( const char *licenseKey );



/************************************************************************//**
@brief
    Deinitialize FastADAS remap feature.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_DeInit( void );



/************************************************************************//**
@brief
    Creates a multithreaded worker pool specifically for the generic
    geometric transformation feature.

@param nThreads
    Requested number of threads, can be limited to the maximum number
    of threads if requesting more than that limit.  The maximum is the
    maximum number of physical or logical cores.  If requesting 0 threads,
    use the maximum number.

@param pThreadsAffinity
    Array to set threads affinity

@param ePipeline
    Execution pipeline to use with this worker pool.

@return
    Worker pool pointer.

@ingroup remap
****************************************************************************/
FADAS_API
void* FadasRemap_CreateWorkers( uint32_t             nThreads,
                                int32_t              pThreadsAffinity[],
                                FadasRemapPipeline_e ePipeline );



/************************************************************************//**
@brief
    Destroys a generic geometric transformation worker pool.

@param wrkrs
    Pointer to the worker pool created by FadasRemap_CreateWorkers().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_DestroyWorkers( void* wrkrs );



/************************************************************************//**
@brief
    Single thread version of FadasRemap_RunMT().  Applies a generic
    geometric transformation to either an 8-bit grayscale image or an RGB888
    image.  The interpolation method is specified through a parameter.

@details
    The image format is specified as part of map creation and initialization
    and assumed constant thereafter.
    The value of each pixel in the destination image is obtained from a
    location of the source image through a per-element mapping as defined in
    the mapping matrices. The mapping has sub-pixel precision, thus
    interpolations are involved. The interpolation method is bilinear
    interpolation.

    The following image illustrates some of the variable definitions below
    assuming that the map has an implicit downscaling initialized in
    one of the initialization functions.

    @cond \image html Remap_roiScale.png width=1200px @endcond

    The upper-left corner of the ROI and the scaled ROI are the
    same. The upper-left of the ROI is assumed to be the
    upper-left (0,0) of the destination image.  However, that upper-left
    corner of the destination can be set to point elsewhere within the
    physical destination image as long as the resulting ROI did not land
    outside physical memory.

@param map
    Pointer to the map created by FadasRemap_CreateMap.

@param src
    Pointer to the input image.  Because the map value map pixel locations
    are backwards to the source image, the source image dimension should be
    equal to or larger than the range of map values for that dimension.
    \n\b WARNING: Must be 128-byte aligned.

@param dst
    Pointer to the output image. While not required, the destination image
    dimensions are typically the same size as the ROI. The width and stride
    must be equal to or larger than the ROI width.  The height must be equal
    to or larger than the ROI height.
    \n\b WARNING: Must be 128-byte aligned.

@param mapROI
    Pointer to the ROI in output image relative to the virtual destination
    image coordinate system which is also the map output coordinate system.
    If the mapping from the source to destination image includes a scale
    reduction (for example, 2x), the ROI must express coordinates within
    those reduced dimensions.

@param roiScale
    Optional parameter that only applies to pipelines that include ROISCALE
    and is only guaranteed for monotonically increasing maps (lens
    models) from the principal point (for example, avoid folds in map space).
    The supported range for roiSale is a minimum of 1.0 to a maximum of 8.0.

    The scale of the destination ROI is relative to the scale of the output
    image.  The overall scale of the output map relative to the input image
    is determined at map creation [\a e.g., FadasRemap_CreateMapFromCalib()].
    This scale change is in addition to that scale change.  Typically used
    when feeding fixed-size ROIs into a DL network that requires fixed-sized
    input dimensions.  The upper-left corner values of the ROI remain the
    same.  Only the width and height scale. The caller must adjust the
    upper-left corner coordinates.\n

    For example, a map might have a scale reduction in each dimension of 4.0
    built into it. A value of 2.2 for this parameter indicates that the ROI
    dimensions are 2.2 times the output map size. Therefore, an ROI width of
    10 corresponds to a width of 22 in destination image coordinates, which
    then corresponds to a width of 88 in input image coordinates.
    \n\b WARNING: The corresponding destination image coordinates must fit
    within the destination image boundaries and the corresponding input image
    coordinates must fit within the input image boundaries.

@param normlz
    Pointer to array of structure holding normalization parameters for all
    channel.  This is an optional parameter, required only when output image
    is to be normalized.
    \n\b WARNING:
    \li normlz[i].sub \f$(s_r)\f$ has limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz[i].mul \f$(m_r)\f$ has limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz[i].add \f$(a_r)\f$ has limited valid range of \f$-255 \le a_r \le 255\f$.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_Run( FadasRemapMap_t          *map_,
                             FadasImage_t             *__restrict src,
                             FadasImage_t             *__restrict dst,
                             FadasROI_t               *mapROI,
                             float32_t                roiScale = 1.0,
                             FadasNormlzParams_t      *normlz  = nullptr);



/************************************************************************//**
@brief
    Multithreaded version of FadasRemap_Run().  Applies a generic geometric
    transformation to either an 8-bit grayscale image or an RGB888 image.
    The interpolation method is specified through a parameter.

@details
    The image format is specified as part of map creation and initialization
    and assumed constant thereafter.
    The value of each pixel in the destination image is obtained from a
    location of the source image through a per-element mapping as defined in
    the mapping matrices. The mapping has sub-pixel precision, thus
    interpolations are involved. The interpolation method is bilinear
    interpolation.

@param wrkrs
    Pointer to the worker pool created by FadasRemap_CreateWorkers.

@param map
    Pointer to the map created by FadasRemap_CreateMap().

@param src
    Pointer to the input image.  Because map value map pixel locations
    backwards to the source image, the source image dimension should be equal
    to or larger than the range of map values for that dimension.
    \n\b WARNING: Must be 128-byte aligned.

@param dst
    Pointer to the output image. While not required, the destination image
    dimensions are typically the same size as the ROI. The width and stride
    must be equal to or larger than the ROI width. The height must be equal
    to or larger than the ROI height.
    \n\b WARNING: Must be 128-byte aligned.

@param mapROI
    Pointer to the ROI in the output image relative to the virtual
    destination image coordinate system, which is also the map output
    coordinate system.  If the mapping from the source to destination image
    includes a scale reduction (for example, 2x), the ROI must express
    coordinates within those reduced dimensions.

@param roiScale
    Optional parameter that only applies to pipelines that include roiScale
    and is only guaranteed for monotonically increasing maps (lens
    models) from the principal point (for example, avoid folds in map space).
    Supported range for roiSale is a minimum of 1.0 to a maximum of 8.0.

    The scale of the destination ROI is relative to the scale of the output
    image. The overall scale of the output map relative to the input image is
    determined at map creation [\a e.g., FadasRemap_CreateMapFromCalib()].
    This scale change is in addition to that scale change. Typically
    used when feeding fixed-size ROIs into a DL network requiring fixed-sized
    input dimensions. The upper-left corner of the ROI remains the same
    values. Only the width and height scale. The caller must adjust
    the UL corner coordinates.\n

    For example, a map might have a built-in scale reduction of 4.0 in each
    dimension.  A value of 2.2 for this parameter means that the ROI
    dimensions are 2.2 times the output map size. Therefore, an ROI width of
    10 corresponds to a width of 22 in destination image coordinates, which
    then corresponds to a width of 88 in input image coordinates.
    \n\b WARNING: The corresponding destination image coordinates must fit
    within the destination image boundaries and the corresponding input image
    coordinates must fit within the input image boundaries.

@param normlz
    Pointer to array of structure holding normalization parameters for all
    channel.  This is an optional parameter, required only when output image
    is to be normalized.
    \n\b WARNING:
    \li normlz[i].sub \f$(s_r)\f$ has limited valid range of \f$-255 \le s_r \le 255\f$.
    \li normlz[i].mul \f$(m_r)\f$ has limited valid range of \f$-4 \le m_r \le 4\f$.
    \li normlz[i].add \f$(a_r)\f$ has limited valid range of \f$-255 \le a_r \le 255\f$.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_RunMT( void                *wrkrs,
                               FadasRemapMap_t     *map_,
                               FadasImage_t        *src,
                               FadasImage_t        *dst,
                               FadasROI_t          *mapROI,
                               float32_t           roiScale = 1.0,
                               FadasNormlzParams_t *normlz  = nullptr);



/************************************************************************//**
@brief
    Creates a generic geometric transformation map without distortion
    but can still include downscaling or color conversion.

@param srcWidth
    Input image width.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.
    \n\b WARNING: Must be a multiple of 8.

@param srcHeight
    Input image height.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.

@param dstWidth
    Output image width.
    \n\b WARNING: Must be a multiple of 8.

@param dstHeight
    Output image height.

@param ePipeline
    Descriptor of execution pipeline to use.

@param borderConst
    Value to put in locations where the result is not driven by the
    mapped inputs.

@return
    Map pointer.

@ingroup remap
****************************************************************************/
FADAS_API
FadasRemapMap_t* FadasRemap_CreateMapNoUndistortion(
                                       uint32_t               srcWidth,
                                       uint32_t               srcHeight,
                                       uint32_t               dstWidth,
                                       uint32_t               dstHeight,
                                       FadasRemapPipeline_e   ePipeline,
                                       uint8_t                borderConst );



/************************************************************************//**
@brief
    Creates a generic geometric transformation map from one compatible with
    OpenCV floating-point maps.

@param camWidth
    Input image width.
    \n\b WARNING:
    \li Must equal the image sizes input to Run() operations, which
    also must match the size of the image used for the calibration model.
    \li Must be a multiple of 8.

@param camHeight
    Input image height.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.

@param mapWidth
    Input map width.
    \n\b WARNING: Must be a multiple of 8.

@param mapHeight
    Input map height.

@param mapX
    Floating point matrix. Each element is the column coordinate of the
    mapped location in the source image. If dst(i,j) maps to src(ii,jj),
    mapX(i,j)=jj. The matrix has the same width and height as the destination
    image. The size of buffer is mapStride*dstHeight bytes.

@param mapY
    Floating point matrix. Each element is the row coordinate of the mapped
    location in the source image. If dst(i,j) maps to src(ii,jj), mapY(i,j)=ii.
    The matrix has the same width and height as the destination image.
    The size of buffer is mapStride*dstHeight bytes.

@param mapStride
    Stride of the mapX and mapY is typically set to
    dstWidth*sizeof(float32_t).

@param ePipeline
    Descriptor of execution pipeline to use.

@param borderConst
    Value to put in locations where the result is not driven by the
    mapped inputs.

@param nThreads
    Number of Threads as input, Default value = 4

@return
    Map pointer.

@ingroup remap
****************************************************************************/
FADAS_API
FadasRemapMap_t* FadasRemap_CreateMapFromMap(
                              uint32_t                    camWidth,
                              uint32_t                    camHeight,
                              uint32_t                    mapWidth,
                              uint32_t                    mapHeight,
                              uint32_t                    mapStride,
                              const float32_t* __restrict mapX,
                              const float32_t* __restrict mapY,
                              FadasRemapPipeline_e        ePipeline,
                              uint8_t                     borderConst,
                              uint32_t                    nThreads = DEFAULT_NTHREADS );


/************************************************************************//**
@brief
    Creates a generic geometric transformation map from buffer holding map data.

@param mapWidth
    Input map width.
    \n\b WARNING: Must be a multiple of 8.

@param mapHeight
    Input map height.

@param ePipeline
    Descriptor of execution pipeline to use.

@param borderConst
    Value to put in locations where the result is not driven by the
    mapped inputs.

@param pMapBuf
    Buffer holding map data filled by application by reading from a
    map data file.

@return
    Map pointer.

@ingroup remap
****************************************************************************/
FADAS_API
FadasRemapMap_t* FadasRemap_CreateMapFromBuffer(
                              uint32_t                    mapWidth,
                              uint32_t                    mapHeight,
                              FadasRemapPipeline_e        ePipeline,
                              uint8_t                     borderConst,
                              uint8_t* __restrict         pMapBuf);



/************************************************************************//**
@brief
    Creates a generic geometric transformation map from calibration model
    parameters.

@param camWidth
    Input image width.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.
    \n\b WARNING: Must be a multiple of 8.

@param camHeight
    Input image height.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.

@param camProps
    Camera intrinsic calibration parameters.

@param camDistModel
    Camera lens distortion model.

@param camDistCoeffs
    Camera lens distortion parameters.

@param ePipeline
    Input and output image format.

@param mapWidth
    Desired map width. The output size when no ROI is specified for scale
    changes between input and output images.
    \n\b WARNING: Must equal the image sizes output by Run()
    operations.

@param mapHeight
    Desired map height. The output size when no ROI is specified for
    scale changes between input and output images.
    \n\b WARNING: Must equal the image sizes output by Run()
    operations.

@param borderConst
    Value to put in locations where the result is not driven by the
    mapped inputs.

@param extrnscT
    Optional (x,y) translation in pixel coordinates (u,v) to include in
    the map.

@param extrnscR
    Optional rotation (3 x 3 matrix) to include in the map.

@param camPropsNew
    New camera calibration parameters to be used to generate output.
    If this parameter is not set, it will be internally calculated.

@return
    Map pointer.

@ingroup remap
****************************************************************************/
FADAS_API
FadasRemapMap_t* FadasRemap_CreateMapFromCalib(
                             uint32_t                   camWidth,
                             uint32_t                   camHeight,
                             FadasCameraProps_t         camProps,
                             FadasLensDistortionModel_e camDistModel,
                             FadasDistCoeffs_t          camDistCoeffs,
                             FadasRemapPipeline_e       ePipeline,
                             uint32_t                   mapWidth,
                             uint32_t                   mapHeight,
                             uint8_t                    borderConst,
                             float32_t*                 extrnscT = nullptr,
                             float32_t*                 extrnscR = nullptr,
                             FadasCameraProps_t         camPropsNew = { 0.0, 0.0, 0.0, 0.0 });

/************************************************************************//**
@brief
    Creates a generic geometric transformation map from calibration model
    parameters.
    This API is obsolete, use FadasRemap_CreateMapFromCalib() instead.

@param camWidth
    Input image width.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.
    \n\b WARNING: Must be a multiple of 8.

@param camHeight
    Input image height.
    \n\b WARNING: Must equal the image sizes input to Run()
    operations, which also must match the size of the image used for the
    calibration model.

@param camProps
    Camera calibration parameters.

@param camDistModel
    Camera lens distortion model.

@param camDistCoeffs
    Fisheye distortion parameters.

@param ePipeline
    Descriptor of execution pipeline to use.

@param mapWidth
    Desired map width. For scale changes between input and
    output images, this is the output width when no ROI is specified.
    \n\b WARNING: Must equal the image sizes output by Run()
    operations.

@param mapHeight
    Desired map height. For scale changes between input and
    output images, this is the output height when no ROI is specified.
    \n\b WARNING: Must equal the image sizes output by Run()
    operations.

@param borderConst
    Value to put in locations where the result is not driven by the
    mapped inputs.

@return
    Map pointer.

@ingroup remap
****************************************************************************/
FADAS_API
FadasRemapMap_t* FadasRemap_CreateMapFromFisheyeCalib(
                                        uint32_t             camWidth,
                                        uint32_t             camHeight,
                                        FadasCameraProps_t   camProps,
                                        FadasDistCoeffs_t    camDistCoeffs,
                                        FadasRemapPipeline_e ePipeline,
                                        uint32_t             mapWidth,
                                        uint32_t             mapHeight,
                                        uint8_t              borderConst );

/************************************************************************//**
@brief
    Creates a camera intrinsic matrix based on the free scaling parameter.

@param cameraProps
    Pointer to the camera intrinsic calibration parameters.

@param distCoeffs
    Camera lens distortion parameters.

@param width
    Width dimension of the input image.

@param height
    Height dimension of the input image.

@param newCameraProps
    Pointer to the output optimal camera matrix.

@param newWidth
    New image width.

@param newHeight
    New image height.

@param alpha
    Parameter for scaling. Value ranges form 0.0 to 1.0.

@param validPixROI
    Pointer to the ROI that defines the validPixels with the new cameraProps parameter.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_GetOptimalNewCameraMatrix (
                            const FadasCameraProps_t* cameraProps,
                            const FadasDistCoeffs_t   distCoeffs,
                            uint32_t                  width,
                            uint32_t                  height,
                            FadasCameraProps_t*       newCameraProps,
                            uint32_t                  newWidth,
                            uint32_t                  newHeight,
                            float32_t                 alpha = 0.0,
                            FadasROI_t*               validPixROI = nullptr );

/************************************************************************//**
@brief
    Destroys a generic geometric transformation map.

@param map
    Pointer to the map created by FadasRemap_CreateMap().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup remap
****************************************************************************/
FADAS_API
FadasError_e FadasRemap_DestroyMap( FadasRemapMap_t *map_ );


#ifdef __cplusplus
}
#endif

#endif /* FADASREMAP_H */
