/***************************************************************************//**
@file
    fadasHOG.h

@brief
    Histogram Of Gradients

@defgroup hog Histogram Of Gradients

@details
    Histogram Of Gradients (HOG) module.

@internal
    Copyright (c) 2020-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASHOG_H
#define FADASHOG_H


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************//**
@brief
   Defines the normalization method in the HOG extraction process.

@ingroup hog
****************************************************************************/
typedef enum
{
    FADAS_HOG_NORM_REGULAR = 0,          ///< Regular normalization method
    FADAS_HOG_NORM_RENORMALIZATION = 1,  ///< Re-normalization
    FADAS_HOG_NORM_FHOG = 2,             ///< F-HOG method
    FADAS_HOG_NORM_END,                  ///< Check invalid enum values
    FADAS_HOG_NORM_MAX = 0x7FFFFFFF      ///< \b WARNING: Do not use
} fadasHOGNormMethod;



/************************************************************************//**
@brief
    Initialize the HOG module
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    Pointer to the license key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup hog
****************************************************************************/
FADAS_API
FadasError_e FadasHOG_Init( const char *licenseKey );


 /************************************************************************//**
@brief
    Deinitialize the HOG module.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup hog
****************************************************************************/
FADAS_API
FadasError_e FadasHOG_DeInit( void );


/************************************************************************//**
 @brief
    Create HOG vector and calculate the length of the output vector for HOG
    extraction.

 @param width
    Input window width.

 @param height
    Input window height.

 @param cellSize
    The size of one cell in pixels. The typical cell size is 4.

 @param blockSize
    Block size in pixels; must be a multiple of cellSize.
    \n\b WARNING: When the normMethod parameter is #FADAS_HOG_NORM_FHOG, 
    the blockStep parameter is by default equal to the cellSize parameter.

 @param blockStep
    Block step in pixels when sliding the block over the image. If 
    blockStep is a multiple of the cellSize parameter, take the faster approach when
    the normMethod parameter is #FADAS_HOG_NORM_REGULAR or #FADAS_HOG_NORM_RENORMALIZATION.
    \n\b WARNING: When the normMethod parameter is #FADAS_HOG_NORM_FHOG, blockStep is by
    default equal to the cellSize parameter.

 @param binSize
    Number of bins in the gradient histogram. Typical binSize is 9.

 @param normMethod
    Enum parameter to specify the normalization method for the HOG descriptor construction. See
    fadasHOGNormMethod() for details.

 @param vecLength
    Pointer to the length of the HOG vector in uint32_t.

 @param hogHandle
    Handle to the HOG object.

@ingroup hog
****************************************************************************/
FADAS_API
FadasError_e FadasHOG_Create( uint32_t           width,
                              uint32_t           height,
                              uint32_t           cellSize,
                              uint32_t           blockSize,
                              uint32_t           blockStep,
                              uint32_t           binSize,
                              fadasHOGNormMethod normMethod,
                              uint32_t           *vecLength,
                              void               **hogHandle );



/************************************************************************//**
@brief
    Function to release HOG resources.

@param hogHandle
    Handle to the HOG object.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup hog
****************************************************************************/

FADAS_API
FadasError_e FadasHOG_Destroy( void* hoghandle );


/************************************************************************//**
@brief
    Extract a histogram of oriented gradients (HOG) descriptor given a grayscale image.

@param src
    Pointer to the input grayscale image object to extract HOG descriptors.

@param hogVector
    Pointer to the output descriptor vector in uint16_t.

@param flen
    Length of the output descriptor vector.

@param hogHandle    Handle to the HOG object.
@return
    #FADAS_ERROR_NONE -- Success.

@ingroup hog
****************************************************************************/
FADAS_API
FadasError_e FadasHOG_Run( FadasImage_t       *src,
                           uint16_t           *__restrict hogVector,
                           uint32_t           flen,
                           void               *hogHandle );

#ifdef __cplusplus
}
#endif

#endif /* FADASHOG_H */
