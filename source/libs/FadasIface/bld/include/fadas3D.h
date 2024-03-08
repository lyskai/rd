/***************************************************************************//**
@file
    fadas3D.h

@brief
    3D Transformation

@defgroup three_d 3D Transformation

@details
    3D transformations (for example, RT matrices) and associated data transformations
    (for example, Rodrigues vectors).

@internal
    Copyright (c) 2020-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADAS3D_H
#define FADAS3D_H

#include <stdint.h>



#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    FADAS_RODRIGUES_VECTOR_TO_MATRIX = 0, ///<  Transform vector to matrix.
    FADAS_RODRIGUES_MATRIX_TO_VECTOR,     ///<  Transform matrix to vector.
    FADAS_RODRIGUES_MAX = 0x7FFFFFFF      ///<  Do not use */
} FadasRodriguesType_e;

/************************************************************************//**
@brief
    Initialize the 3D transform module.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().

@param licenseKey
    License key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup three_d
****************************************************************************/
FADAS_API
FadasError_e Fadas3D_Init( const char *licenseKey );



/************************************************************************//**
@brief
    Deinitialize 3D transform module.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup three_d
****************************************************************************/
FADAS_API
FadasError_e Fadas3D_DeInit( void );



/************************************************************************//**
@brief
    Multiplies a 4 x 4 RT matrix with a number of points and outputs the
    result into separate memory.

@param RT
    Pointer to 4 x 4 RT matrix. It is explicitly assumed that the
    matrix is composed of a rotation and translation matrix and the
    bottom row is [0, 0, 0, 1].

@param RTStride
    RT matrix stride (in bytes). Gap between the first
    element of a row and that of the successive row. If the value is equal
    to 0, a value is calculated assuming contiguous memory.

@param nPts
    Number of points to follow in both input and output.

@param src
    Input points represented by an array of 1x4 floating-point vectors.
    \n\b WARNING: Must be 128-byte aligned.

@param srcStride
    Input points stride. The gap (in terms of bytes) between the
    first element of a 1x4 vector and the first element of the successive 1x4
    vector. If the value is equal to 0, a value is calculated assuming
    contiguous memory.
    \n\b WARNING: Must be a multiple of 8.

@param dst
    Output image with the same type and size as the input image. The
    size of buffer is dstStride*n_pts bytes.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Output points stride.  The gap (in terms of bytes) between the
    first element of a 1x4 vector and the first element of the successive 1x4
    vector. If the value is equal to 0, a value is calculated assuming
     contiguous memory.
    \n\b WARNING: Must be a multiple of 8.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup three_d
****************************************************************************/
FADAS_API
FadasError_e Fadas3D_MultiplyPtsWth4x4RT( const float32_t *__restrict RT,
                                          uint32_t        RTStride,
                                          uint32_t        nPts,
                                          const float32_t *__restrict src,
                                          uint32_t        srcStride,
                                          float32_t       *__restrict dst,
                                          uint32_t        dstStride );


/************************************************************************//**
@brief
    Converts a rotation matrix to a rotation vector or vice versa.

@param src
    Pointer containing either the 1 x 3 rotation vector or
    3 x 3 rotation matrix in a flattened fashion.

@param convType
    Might hold values #FADAS_RODRIGUES_VECTOR_TO_MATRIX or
    #FADAS_RODRIGUES_MATRIX_TO_VECTOR.

@param dst
    Pointer to either the 1 x 3 rotation vector or 3 x 3
    rotation matrix in a flattened fashion.

@param jcb
    Pointer to the output Jacobian matrix, 3 x 9 or 9 x 3, which is a matrix of
    partial derivatives of the output array components with respect
    to the input array components.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup three_d
****************************************************************************/
FADAS_API
FadasError_e Fadas3D_Rodrigues( const float32_t        *__restrict src,
                                FadasRodriguesType_e   convType,
                                float32_t              *__restrict dst,
                                float32_t              *__restrict jcb );


#ifdef __cplusplus
}
#endif

#endif /* FADAS3D_H */
