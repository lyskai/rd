/***************************************************************************//**
@file
    fadasFPE.h

@brief
    Feature Point Extraction

@example fpe/app.cpp

@defgroup fpe Feature Point Extraction

@details
    Feature Point Extraction.

@internal
    Copyright 2021-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASFPE_H
#define FADASFPE_H

#include <stdint.h>
#include <fadas.h>


#ifdef __cplusplus
extern "C"
{
#endif


/************************************************************************//**
@brief
    Algorithm pipeline.

@param FADAS_FPE_PIPELINE_FAST10
    Extract feature points from images using the FAST10 corners algorithm.
    A border of 4 pixels is ignored.

@param FADAS_FPE_PIPELINE_FAST10Scores
    Same as FADAS_FPE_PIPELINE_FAST10 but adds scores.  Extract feature
    points from images using the FAST10 corners algorithm.  Include score
    estimates as well.  A border of 4 pixels is ignored.

@param FADAS_FPE_PIPELINE_FAST10ScoresNMS
    Same as FADAS_FPE_PIPELINE_FAST10Scores but adds non-maximum suppression.
    \n\b WARNING:  A border of 4 pixels is ignored.

@param FADAS_FPE_PIPELINE_FAST10ScoresNMSu12
    Same as FADAS_FPE_PIPELINE_FAST10Scores but adds non-maximum suppression.
    \n\b WARNING:  A border of 4 pixels is ignored.

@ingroup fpe
****************************************************************************/
typedef enum : int32_t
{
    FADAS_FPE_PIPELINE_FAST10 = 0,          ///< FAST10 feature points
    FADAS_FPE_PIPELINE_FAST10Scores,        ///< FAST10 feature points with scores
    FADAS_FPE_PIPELINE_FAST10ScoresNMS,     ///< FADAS_FPE_PIPELINE_FAST10Scores + non-maximum suppression
    FADAS_FPE_PIPELINE_FAST10Scores12b,     ///< FADAS_FPE_PIPELINE_FAST10Scores for 12-bit images
    FADAS_FPE_PIPELINE_FAST10ScoresNMS12b,  ///< FADAS_FPE_PIPELINE_FAST10ScoresNMS for 12-bit images
    FADAS_FPE_PIPELINE_MAX                  ///< \b WARNING: must be last
} FadasFPEPipeline_e;



/************************************************************************//**
@brief
    Feature point extraction object
****************************************************************************/
typedef struct FadasFPE FadasFPE_t;


/************************************************************************//**
@brief
    Initialize feature point extraction.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    Pointer to the license key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_Init( const char *licenseKey );



/************************************************************************//**
@brief
    Deinitialize feature point extraction.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_DeInit( void );



/************************************************************************//**
@brief
    Create a feature point extraction object.

@param imgProps
    Image properties that must match the properties of the srcImg parameter in FadasFPE_Run().

@return
    Pointer to object.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasFPE_t* FadasFPE_Create( FadasImgProps_t imgProps );



/************************************************************************//**
@brief
    Destroy feature point extraction object.
@param obj
    Pointer to the FPE object returned by FadasFPE_Create().
@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_Destroy( FadasFPE_t *obj );



/************************************************************************//**
@brief
    Extracts feature points from image and computes an optional score for
    each point.

@param type
    Descriptor of the execution pipeline to use.

@param obj
    Pointer to the FPE object returned by FadasFPE_Create().

@param srcImg
    Pointer to the input image frame data.

@param roi
    Pointer to the specification of the region of interest to extract feature points
    from.
    \n\b WARNING: See #FadasFPEPipeline_e for potential untouched border
    pixels for algorithm pipelines that supplant the specified ROI
    where the ROI crosses those border pixels.

@param barrier
    Threshold value for how strong associated score of a feature point must be
    before inclusion in the list of points. When there are FAST corners,
    the threshold value \f$(b)\f$ is added to or subtracted from the
    center pixel \f$(I_c)\f$ to test that each pixel \f$(I_i)\f$ in the segment
    passes the test: \f$I_i > I_c + b\f$ or \f$I_i < I_c - b\f$.


@param nCornersMax
    Maximum number of features that can be added to the following arrays.
    This number usually represents the amount of memory allocated.

@param nCorners
    Pointer to the number of features found.

@param x
    Pointer to the x values for the locations of features found.

@param y
    Pointer to the y values for the locations of features found.

@param score
    Pointer to the score values for the locations of features found if
    requested by the chosen pipeline value.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_Run( FadasFPEPipeline_e eType,
                           FadasFPE_t         *obj,
                           FadasImage_t       *src,
                           FadasROI_t         *roi,
                           int16_t            barrier,
                           uint32_t           nCornersMax,
                           uint32_t           *nCorners,
                           float32_t          *ptX,
                           float32_t          *ptY,
                           uint32_t           *ptScore );



/************************************************************************//**
@brief
    Extracts feature points from an image and computes an optional score for
    each point.

@param wrkrs
    Pointer to the worker pool created by FadasFPE_CreateWorkers().

@param obj
    Pointer to the FPE object returned by FadasFPE_Create().

@param src
    Pointer to the input image frame data.

@param roi
    Pointer to the specification of the region of interest to extract feature
    points from.
    \n\b WARNING:  See #FadasFPEPipeline_e for potential untouched border
    pixels for different algorithm pipelines that supplant the specified ROI
    where the ROI crosses those border pixels.

@param barrier
    Threshold value for how strong the associated score feature point must be
    before inclusion in the list of points. When there are FAST corners,
    this threshold value \f$(b)\f$ is  added to or subtracted from
    the center pixel \f$(I_c)\f$ to test that each pixel \f$(I_i)\f$ in the
    segment passes the test: \f$I_i > I_c + b\f$ or \f$I_i < I_c - b\f$.


@param nCornersMax
    Maximum number of features that can be added to the following arrays.
    This number usually represents the amount of memory allocated.

@param nCorners
    Pointer to the number of features found.

@param x
    Pointer to the x values for the locations of features found.

@param y
    Pointer to the y values for the locations of features found.

@param score
    Pointer to the score values for the locations of features found if
    requested by the chosen pipeline value.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_RunMT( void         *wrkrs,
                             FadasFPE_t   *obj,
                             FadasImage_t *src,
                             FadasROI_t   *roi,
                             int16_t      barrier,
                             uint32_t     nCornersMax,
                             uint32_t     *nCorners,
                             float32_t    *ptX,
                             float32_t    *ptY,
                             uint32_t     *ptScore );

/************************************************************************//**
@brief
    Creates a multithreaded worker pool specifically for the feature point
    extraction feature.

@param nThreads
    Requested number of threads. Can be limited to the maximum number
    of threads if requesting more than that the limit. The maximum is the
    maximum number of physical or logical cores. If requesting 0 threads,
    use the maximum number.

@param pThreadsAffinity
    Array for setting threads affinity

@param eType
    Descriptor of execution pipeline to use.

@return
    Worker pool pointer.

@ingroup fpe
****************************************************************************/
FADAS_API
void* FadasFPE_CreateWorkers( uint32_t           nThreads,
                              int32_t            pThreadsAffinity[],
                              FadasFPEPipeline_e eType );



/************************************************************************//**
@brief
    Destroys a feature point extraction worker pool.

@param wrkrs
    Pointer to the worker pool created by FadasFPE_CreateWorkers().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpe
****************************************************************************/
FADAS_API
FadasError_e FadasFPE_DestroyWorkers( void* wrkrs );



#ifdef __cplusplus
}
#endif

#endif /* FADASFPE_H */
