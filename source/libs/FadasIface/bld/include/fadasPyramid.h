/***************************************************************************//**
@file
    fadasPyramid.h

@brief
    Image Pyramid

@defgroup pyramid Image Pyramid

@details
    Create image pyramids [See
    <a href="https://en.wikipedia.org/wiki/Pyramid_(image_processing)">here</a>].
    Currently only supports downscaling by 2 with bi-linear interpolation.

@internal
    Copyright (c) 2020-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASPYRAMID_H
#define FADASPYRAMID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define FADAS_MAX_PYRAMID_LEVELS 6


/************************************************************************//**
@brief
    Pyramid image structure.

@param data
    Image data of this image level.

@param bAllocated
    Status of the memory allocation by pyramid.

@ingroup pyramid
****************************************************************************/
typedef struct
{
    FadasImage_t data;
    bool         bAllocated;
} FadasPyramid_Level_t;


/************************************************************************//**
@brief
    Algorithm pipeline.

@param FADAS_PYRAMID_PIPELINE_DownscaleUYVYAndRGB888
    Scale UYUV image down by 1x to 8x in each dimension, the ratios in
    height and width need not be the same, and convert to RGB888.

@param FADAS_PYRAMID_PIPELINE_DownscaleUYVYBy2AndRGB888
    Scale UYUV image down by two in each dimension, the ratios in height
    and width need not be the same, and convert to RGB888.

@param FADAS_PYRAMID_PIPELINE_DownscaleYUV888AndRGB888
    Scale YUV888 image down by 1x to 8x in each dimension, the ratios in
    height and width need not be the same, and convert to RGB888.

@param FADAS_PYRAMID_PIPELINE_DownscaleYUV888By2AndRGB888
    Scale YUV888 image down by two in each dimension, the ratios in height
    and width need not be the same, and convert to RGB888.

@param FADAS_PYRAMID_PIPELINE_DownscaleY8UV8By2
    Scale Y8UV8 image down by 2 in each dimension and the ratios in height
    and width need not be the same and convert to RGB888.

@param FADAS_PYRAMID_PIPELINE_DownscaleY10UV10By2
    Scale Y10UV10 image down by 2 in each dimension and the ratios in height
    and width need not be the same and convert to RGB888.

@ingroup cvtyuv
****************************************************************************/
typedef enum
{
    FADAS_PYRAMID_PIPELINE_DownscaleBy2 = 0,            ///< Single planar luma image (Y) with 8 bit data in 8 bit carriers
    FADAS_PYRAMID_PIPELINE_DownscaleRGB888By2,          ///< Single planar image (RGB) with 8 bit data in 8 bit carriers
    FADAS_PYRAMID_PIPELINE_DownscaleY8UV8By2,           ///< Multiplanar image (Y,UV) with 8 bit data in 8 bit carriers
    FADAS_PYRAMID_PIPELINE_DownscaleY10UV10By2,         ///< Multiplanar image (Y,UV) with 10 bit data in 16 bit carriers
    FADAS_PYRAMID_PIPELINE_MAX = 0x7FFFFFFF             ///< \b WARNING: Do not use
} FadasPyramidPipeline_e;



/************************************************************************//**
@brief
    Initialize the pyramid feature.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    License key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_Init( const char* licenseKey );

/************************************************************************//**
@brief
    Deinitialize the FastADAS pyramid feature.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_DeInit( void );



/************************************************************************//**
@brief
    Allocates memory for a pyramid.

@param srcProps
    Image properties of the source image.
    \n\b WARNING: Image sizes must be appropriate for the specified numLevels so
    that the dimensions are properly divisible by 2 for each next level of
    the pyramid.

@param pyramid
    Pointer to pyramid.

@param numLevels
    Number of levels to the pyramid.
    \n\b WARNING: Must not exceed #FADAS_MAX_PYRAMID_LEVELS.

@param externalLevel0
    If true, memory is only allocated for levels 1 through (numLevels-1)
    so that level 0 can point to externally allocated memory.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_Allocate( FadasImgProps_t      srcProps,
                                    FadasPyramid_Level_t *pyramid,
                                    uint32_t             numLevels,
                                    bool                 externalLevel0 = false );

/************************************************************************//**
@brief
    Frees memory for a pyramid.

@param pyramid
    Pointer to pyramid.

@param numLevels
    Number of levels to the pyramid.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_Free( FadasPyramid_Level_t *pyramid,
                                uint32_t             numLevels );



/************************************************************************//**
@brief
    Runs the pyramid creation feature to populate the different levels of a
    previously allocated pyramid.

@param src
    Pointer to the source image to build the pyramid.
    \n\b WARNING: Image sizes must be appropriate for the given numLevels so
    that the dimensions are properly divisible by 2 for each next level of
    the pyramid.

@param dst
    Pointer to a previously allocated pyramid.

@param numLevels
    Number of levels to the pyramid.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_Run( FadasImage_t         *src,
                               FadasPyramid_Level_t *dst,
                               uint32_t             numLevels);



/************************************************************************//**
@brief
    Creates a multithreaded worker pool for the pyramidal down
    scalar feature.

@param nThreads
    Requested number of threads, can limit to the maximum number
    of threads if requesting more than that limit. The maximum is the
    maximum number of physical or logical cores. If requesting 0 threads,
    use the maximum number.

@param pThreadsAffinity
    Array for setting threads affinity

@param eType
    Pipeline type.

@return
    Worker pool pointer.

@ingroup pyramid
****************************************************************************/
FADAS_API
void* FadasPyramid_CreateWorkers(   uint32_t                nThreads,
                                    int32_t                 pThreadsAffinity[],
                                    FadasPyramidPipeline_e  eType );


/************************************************************************//**
@brief
    Destroys a pyramidal down-scalar worker pool.

@param wrkrs
    Pointer to the worker pool created by FadasCvtYUV_CreateWorkers.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup pyramid
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_DestroyWorkers( void* wrkrs );


/************************************************************************//**
@brief
    Runs a multithreaded pipeline for this pyramid module.

@details
    Runs a multithreaded pipeline using a worker pool created by
    FadasPyramid_CreateWorkers(). At worker pool creation, the
    pipeline of one or more chained algorithms is defined.

@param wrkrs
    Pointer to the worker pool created by FadasPyramid_CreateWorkers.

@param src
    Pointer to the source image to build the pyramid.

@param dst
    Pointer to pyramid.

@param numLevels
    Number of levels to the pyramid.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup cvtyuv
****************************************************************************/
FADAS_API
FadasError_e FadasPyramid_RunMT(void                    *wrkrs,
                                FadasImage_t            *src,
                                FadasPyramid_Level_t    *dst,
                                uint32_t                numLevels);

#ifdef __cplusplus
}
#endif

#endif /* FADASPYRAMID_H */
