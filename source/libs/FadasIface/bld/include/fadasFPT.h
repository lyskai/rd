/***************************************************************************//**
@file
    fadasFPT.h

@brief
    Feature Point Tracking

@example fpt/app.cpp
@example fpt_trcks/app.cpp

@defgroup fpt Feature Point Tracking

@details
    Feature Point Tracking.

@internal
    Copyright (c) 2021-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASFPT_H
#define FADASFPT_H

#include <stdint.h>
#include <fadas.h>


#ifdef __cplusplus
extern "C"
{
#endif


/************************************************************************//**
@brief
    Algorithm pipeline.

@param FADAS_FPT_PIPELINE_LK
    Track feature points from images using the Lucas-Kanade tracking.

@ingroup fpt
****************************************************************************/
typedef enum : int32_t
{
    FADAS_FPT_PIPELINE_LK = 0,          ///< LK tracking
    FADAS_FPT_PIPELINE_MAX              ///< Do not use
} FadasFPTPipeline_e;



/************************************************************************//**
@brief
    Feature point tracking object
****************************************************************************/
typedef struct FadasFPT FadasFPT_t;


/************************************************************************//**
@brief
    Initialize feature point tracking.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    Pointer to license key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_Init( const char *licenseKey );



/************************************************************************//**
@brief
    Deinitialize feature point tracking.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_DeInit( void );



/************************************************************************//**
@brief
    Create feature point tracking object.

@param imgProps
    Image properties, must match properties of the srcImg parameter in FadasFPT_Run().

@param srchSize
    Search size, equates to a srchSize x srchSize patch in algorithms
    using a square patch.

@return
    Pointer to object.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasFPT_t* FadasFPT_Create( const FadasImgProps_t& imgProps,
                             int32_t                srchSize,
                             int32_t                pyrmd_lvls );



/************************************************************************//**
@brief
    Destroy feature point tracking (FPT) object.

@param obj
    Pointer to FPT object returned by FadasFPT_Create().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_Destroy( FadasFPT_t    *obj );



/************************************************************************//**
@brief
    Tracks feature points between images.  Although time-based tracking is a
    typical use case, there is no notion of time built into the algorithm.
    Therefore, the second image can be from the same time but a different
    location to track changes in relative locations.

@param obj
    Pointer to the FPT object returned by FadasFPT_Create().

@param img0
    Pointer to the input image frame data from time t = 0.

@param img1
    Pointer to the input image frame data from time t = 1.

@param roi
    Pointer to the specification of the ROI in which feature
    points should be tracked.

@param n_pts
    Filled with the number of features to track.

@param x
    Pointer to the x values for the feature locations to track and
    update after the call with the new feature location if tracked
    successfully.

@param y
    Pointer to the y values for the feature locations to track and
     after the call with the new feature location if tracked
    successfully.

@param status
    Pointer to the status values for the locations of features found. A
    value of zero signifies that the feature was not successfully
    tracked.  A value of one indicates that the feature was successfully
    tracked.

@param max_iter
    Maximum iterations.

@param max_eps
    Maximum eps

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_Run( FadasFPT_t       *obj,
                           FadasImage_t     *img0,
                           FadasImage_t     *img1,
                           FadasROI_t       *roi,
                           int32_t          n_pts,
                           float32_t        *x,
                           float32_t        *y,
                           int32_t          *status,
                           int32_t          max_iter,
                           float32_t        max_eps );



/************************************************************************//**
@brief
    Multithreaded version of FadasFPT_Run().

@param wrkrs
    Worker pool created by FadasFPT_CreateWorkers().

@param obj
    Pointer to the FPT object returned by FadasFPT_Create().

@param img0
    Pointer to the input image frame data from time t = 0.

@param img1
    Pointer to the input image frame data from time t = 1.

@param roi
    Pointer to specification of the ROI in which feature
    points should be tracked.

@param n_pts
    Filled with the number of features to track.

@param x
    Pointer to the x values for the feature locations to track and
    update after the call with the new feature location if tracked
    successfully.

@param y
    Pointer to the y values for the feature locations to track and
    update after the call with the new feature location if tracked
    successfully.

@param status
    Pointer to the status values for the locations of features found. A
    value of zero signifies that the feature was not successfully
    tracked. A value of one indicates that the feature was successfully
    tracked.

@param max_iter Maximum iterations.

@param max_eps  Maximum eps

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_RunMT( void               *wrkrs,
                             FadasFPT_t         *obj,
                             FadasImage_t       *img0,
                             FadasImage_t       *img1,
                             FadasROI_t         *roi,
                             int32_t            n_pts,
                             float32_t          *x,
                             float32_t          *y,
                             int32_t            *status,
                             int32_t            max_iter,
                             float32_t          max_eps );



/************************************************************************//**
@brief
    Creates a multithreaded worker pool for the feature point
    tracking feature.

@param nThreads
    Requested number of threads;  can limit to the maximum number
    of threads if requesting more than that limit. The maximum is the
    maximum number of physical or logical cores. If requesting 0 threads,
    use the maximum number.

@param pThreadsAffinity
    Array for setting threads affinity

@param eType
    Descriptor of execution pipeline to use.

@return
    Worker pool pointer.

@ingroup fpt
****************************************************************************/
FADAS_API
void* FadasFPT_CreateWorkers( uint32_t           nThreads,
                              int32_t            pThreadsAffinity[],
                              FadasFPTPipeline_e eType );



/************************************************************************//**
@brief
    Destroys a feature point tracking worker pool.

@param wrkrs
    Pointer to the worker pool created by FadasFPT_CreateWorkers().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup fpt
****************************************************************************/
FADAS_API
FadasError_e FadasFPT_DestroyWorkers( void* wrkrs );



#ifdef __cplusplus
}
#endif

#endif
