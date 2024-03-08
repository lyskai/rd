/***************************************************************************//**
@file
    fadasTensor.h

@brief
   Tensors (including 1D and 2D)

@defgroup tensor Tensors

@details
    Multi-dimensional tensor operations including vectors and matrices.

@internal
    Copyright (c) 2020-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASTENSOR_H
#define FADASTENSOR_H

#include <stdint.h>



#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************//**
@brief
    Matrix structure.

@param ptr
    memory pointer of the data
    \n\b WARNING: shall be 128-byte aligned.

@param numRows
    Number of rows in a matrix

@param numCols
    Number of columns in a matrix

@param Stride
    Number of bytes between the two rows of a matrix
    \n\b WARNING: Must be multiple of 128 bytes.
****************************************************************************/
typedef struct
{
    float32_t *ptr;
    uint32_t  numRows;
    uint32_t  numCols;
    uint32_t  stride;
} FadasMatrix_t;



/************************************************************************//**
@brief
    Tensor format structure

@param n
    Number of batches in the tensor

@param w
    Width of the tensor

@param h
    Height of the tensor

@param c
    Number of channels in the tensor

@param n_stride
    Stride for the batch dimension

@param w_stride
    Stride for the width dimension

@param h_stride
    Stride for the height dimension

@param c_stride
    Stride for the channel dimension

@ingroup tensor
****************************************************************************/
typedef struct FadasTensorFormat
{
    uint32_t n;
    uint32_t w;
    uint32_t h;
    uint32_t c;
    uint32_t n_stride;
    uint32_t w_stride;
    uint32_t h_stride;
    uint32_t c_stride;
} FadasTensorFormat_t;



/************************************************************************//**
@brief
    Initialize the tensor feature.
    \n\b WARNING: Must be called once before other FastADAS functions.
    except FadasVersion().

@param licenseKey
    License key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_Init( const char* licenseKey );



/************************************************************************//**
@brief
    Deinitialize the tensor feature.
    \n\b WARNING: This function must be called once after all other FastADAS functions.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_DeInit( void );



/************************************************************************//**
@brief
    Dot product of N 2D matrices (rank 2 tensors) into N 1D column
    vectors (rank 1 tensors) and output the result into separate memory.
    Equivalent to multiple calls in MATLAB of dot(A, B, 2).

@param n
    Number of A and B matrices, as well as output vectors.

@param nY
    Number of rows in both A and B.

@param nX
    Number of columns in both A and B.

@param A
    Pointer to nX by nY matrix.
    \n\b WARNING: Must be 128-byte aligned.

@param Astride
    Input matrix stride. The gap (in bytes) between the first
    element of a matrix and the gap of the successive matrix.
    \n\b WARNING: Must be a multiple of 4.

@param B
    Pointer to nX by nY matrix.
    \n\b WARNING: Must be 128-byte aligned.

@param Bstride
    Input matrix stride. The gap (in bytes) between the first
    element of a matrix and the gap of the successive matrix.
    \n\b WARNING: Must be a multiple of 4.

@param c
    Output vectors of size nY.
    \n\b WARNING: Must be 128-byte aligned.

@param cStride
    Output vector stride. The gap (in bytes) between the first
    element of a vector and the gap of the successive vector.
    \n\b WARNING: Must be a multiple of 4.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_Dot2Dx( uint32_t        n,
                    uint32_t        nY,
                    uint32_t        nX,
                    const float32_t *__restrict A,
                    uint32_t        Astride,
                    const float32_t *__restrict B,
                    uint32_t        Bstride,
                    float32_t       *__restrict c,
                    uint32_t        cStride );


/************************************************************************//**
@brief
    Dot product of two 2D matrices (rank 2 tensors) with a multiple 1D column
    vectors (rank 1 tensors) and outputs the result into separate memory.
    Equivalent to multiple calls in MATLAB of dot(A, B, 1).

@param n
    Number of A and B matrices, as well as output vectors.

@param nY
    Number of rows in both A and B.

@param nX
    Number of columns in both A and B.

@param A
    Pointer to nX by nY matrix.
    \n\b WARNING: Must be 128-byte aligned.

@param Astride
    Input matrix stride. The gap (in bytes) between the first
    element of a matrix and that of the next successive matrix.
    \n\b WARNING: Must be a multiple of 4.

@param B
    Pointer to nX by nY matrix.
    \n\b WARNING: Must be 128-byte aligned.

@param Bstride
    Input matrix stride.  The gap (in bytes) between the first
    element of a matrix and that of the next successive matrix.
    \n\b WARNING: Must be a multiple of 4.

@param c
    Output vectors of size nX.
    \n\b WARNING: Must be 128-byte aligned.

@param cStride
    Output vector stride.  The gap (in bytes) between the first
    element of a vector and that of the next successive vector.
    \n\b WARNING: Must be a multiple of 4.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_Dot2Dy( uint32_t        n,
                    uint32_t        nY,
                    uint32_t        nX,
                    const float32_t *__restrict A,
                    uint32_t        Astride,
                    const float32_t *__restrict B,
                    uint32_t        Bstride,
                    float32_t       *__restrict c,
                    uint32_t        cStride );



/************************************************************************//**
@brief
    Multiplies a 2D 3 x 3 matrix (rank 2 tensor) with multiple 1D column
    vectors (rank 1 tensors) and outputs the result into separate memory.

@param M
    Pointer to a 3 x 3 matrix.

@param Mstride
    Input matrix stride (in bytes), the gap between the first
    element of a row and that of the successive row. If the value is equal
    to 0, a value is calculated assuming contiguous memory.

@param nPts
    Number of points to follow in both input and output.

@param src
    Pointer to the input points represented by an array of 1 x 3 floating-point vectors.
    \n\b WARNING: Must be 128-byte aligned.

@param srcStride
    Input points stride, gap (in terms of bytes) between the
    first element of a 1 x 3 vector and the first element of the successive 1 x 3
    vector. If the value is equal to 0, a value is calculated assuming
    contiguous memory.
    \n\b WARNING:
    \li Must be a multiple of 8.
    \li Must be 128-byte aligned.

@param dst
    Pointer to the output image that has the same type, and size as the input image. The
    size of buffer is dstStride*n_pts bytes.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Output points stride, the gap (in terms of bytes) between the
    first element of a 1 x 3 vector and the first element of the successive 1 x 3
    vector. If the value is equal to 0, a value is calculated assuming
    contiguous memory.
    \n\b WARNING:
    \li Must be a multiple of 8. 
    \li Must be 128-byte aligned.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_Multiply3x3byNx3( const float32_t *__restrict M,
                              uint32_t        Mstride,
                              uint32_t        nPts,
                              const float32_t *__restrict src,
                              uint32_t        srcStride,
                              float32_t       *__restrict dst,
                              uint32_t        dstStride );



/************************************************************************//**
@brief
    Multiplies a 2D 4x4 matrix (rank 2 tensor) with a multiple 1D column
    vectors (rank 1 tensors) and outputs the result into separate memory.

@param M
    Pointer to 4 x 4 matrix.

@param Mstride
    Input matrix stride (in bytes), the gap between the first
    element of a row and that of the successive row. If the value is equal
    to 0, a value is calculated assuming contiguous memory.

@param nPts
    Number of points to follow in both input and output.

@param src
    Input points represented by an array of 1 x 4 floating-point vectors.
    \n\b WARNING: Must be 128-byte aligned.

@param srcStride
    Input points stride, the gap (in terms of bytes) between the
    first element of a 1 x 4 vector and the first element of the successive 1 x 4
    vector. If the value is equal to 0, a value is calculated assuming
    contiguous memory.
    \n\b WARNING:
    \li Must be a multiple of 8
    \li Must be 128-byte aligned.

@param dst
    Output image which has the same type, and size as the input image. The
    size of buffer is dstStride*nPts bytes.
    \n\b WARNING: Must be 128-byte aligned.

@param dstStride
    Output points stride, the gap (in terms of bytes) between the
    first element of a 1 x 4 vector and the first element of the successive 1 x 4
    vector. If the value is equal to 0, a value is calculated assuming
    contiguous memory.
    \n\b WARNING:
    \li Must be a multiple of 8 
    \li Must be 128-byte aligned.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_Multiply4x4byNx4( const float32_t  *__restrict M,
                              uint32_t         Mstride,
                              uint32_t         nPts,
                              const float32_t  *__restrict src,
                              uint32_t         srcStride,
                              float32_t        *__restrict dst,
                              uint32_t         dstStride );


/************************************************************************//**
@brief
    Multiplies multiple 2D matrices and stores the output matrices in
    separate memory.

@param n
    Number of matrices to multiply.

@param src1
    Pointer to the input matrix structure that describes dimensions and stride between
    two rows of the matrix in bytes. The structure also stores the
    memory where source matrix or matrices are stored.

@param src2
    Pointer to the input matrix structure that describes dimensions and stride between
    two rows of the matrix in bytes. The structure also stores the
    memory where source matrix or matrices are stored.

@param dst
    Pointer to the destination matrix structure that stores the result matrix or
    matrices dimensions, stride in bytes between matrix rows. The structure
    also stores the memory address where the result is stored.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API FadasError_e
FadasTensor_MatrixMultiply( uint32_t       n,
                            FadasMatrix_t  *src1,
                            FadasMatrix_t  *src2,
                            FadasMatrix_t  *dst );



/************************************************************************//**
@brief
    Converts a tensor from NCHW to NHWC format.

@param A
    Pointer to the source tensor in float32 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A

@param B
    Pointer to the destination tensor in float32 format
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NCHW2NHWCf32( const float32_t     *__restrict srcA,
                                       FadasTensorFormat_t *AFormat,
                                       float32_t           *__restrict dstB,
                                       FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Converts a tensor from NHWC to NCHW format.

@param A
    Pointer to the source tensor in float32 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A.

@param B
    Pointer to the destination tensor in float32 format.
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NHWC2NCHWf32( const float32_t     *__restrict srcA,
                                       FadasTensorFormat_t *AFormat,
                                       float32_t           *__restrict dstB,
                                       FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Converts Tensor from NCHW to NHWC format

@param A
    Pointer to the source tensor in int8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A.

@param B
    Pointer to the destination tensor in int8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NCHW2NHWCi8( const int8_t        *__restrict srcA,
                                      FadasTensorFormat_t *AFormat,
                                      int8_t              *__restrict dstB,
                                      FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Converts a tensor from NHWC to NCHW format.

@param A
    Pointer to the source tensor in int8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A.

@param B
    Pointer to the destination tensor in int8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NHWC2NCHWi8( const int8_t        *__restrict srcA,
                                      FadasTensorFormat_t *AFormat,
                                      int8_t              *__restrict dstB,
                                      FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Converts a tensor from NCHW to NHWC format.

@param A
    Pointer to the source tensor in uint8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A.

@param B
    Pointer to the destination tensor in uint8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NCHW2NHWCu8( const uint8_t       *__restrict srcA,
                                      FadasTensorFormat_t *AFormat,
                                      uint8_t             *__restrict dstB,
                                      FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Converts a tensor from NHWC to NCHW format.

@param A
    Pointer to the dource tensor in uint8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param AFormat
    Pointer to the format descriptor for source tensor A.

@param B
    Pointer to the destination tensor in uint8 format.
    \n\b WARNING: Must be 128-byte aligned.

@param BFormat
    Pointer to the format descriptor for destination tensor B.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_NHWC2NCHWu8( const uint8_t       *__restrict srcA,
                                      FadasTensorFormat_t *AFormat,
                                      uint8_t             *__restrict dstB,
                                      FadasTensorFormat_t *BFormat );



/************************************************************************//**
@brief
    Four-dimensional version of FadasCvtYUV_Renormalize().
    Scales a tensor \f$(A)\f$ of 8-bit NHWC values \f$(A_{ijkl})\f$ to create
    a 32-bit floating point tensor \f$(B)\f$ by subtracting a constant
    \f$(s)\f$, multiplying by another constant \f$(m)\f$, and then
    adding by another constant \f$(a)\f$.
    \f[
    B_{ijkl} = m (A_{ijkl} - s) + a
    \f]

    When dealing with the output of a quantized network running on
    fixed-point hardware or software, this routine can reverse that
    quantization by using a zero as the addition value \f$(a=0)\f$.

    Global batch normalization using a global mean \f$(s = \mu)\f$ and
    standard deviation \f$(m = 1/\sigma)\f$ along with no addition \f$(a=0)\f$.
    \f[
    B_{ijkl} = (A_{ijkl} - \mu)/\sigma
    \f]
    Scaling to a new mean \f$(\mu')\f$ and width \f$(\sigma')\f$ can be
    combined into single constants \f$m = \sigma'/\sigma\f$, \f$s = \mu\f$,
    and \f$a = \mu'\f$.
    \f{eqnarray*}{
    B_{ijkl} &=& m (A_{ijkl} - s) + a \\
             &=& (\sigma'/\sigma)(A_{ijkl} - \mu) + \mu'
    \f}

@param A
    Pointer to the source tensor.
    \n\b WARNING: Must be 128-byte aligned.

@param Aformat
    Pointer to the format descriptor for source tensor.

@param B
    Pointer to the destination tensor.
    \n\b WARNING: Must be 128-byte aligned.

@param Bformat
    Pointer to the format descriptor for the destination tensor.
    \n\b WARNING: Last dimension (C) must stride of 4 [sizeof(float32_t)].

@param normlz
    Normalization parameters apply to all color channels equally.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_RenormalizeNHWCu8f32( const uint8_t       *__restrict srcA,
                                               FadasTensorFormat_t *Aformat,
                                               float32_t           *__restrict dstB,
                                               FadasTensorFormat_t *Bformat,
                                               FadasNormlzParams_t normlz );


/************************************************************************//**
@brief
    Similar to a four-dimensional version of FadasCvtYUV_Renormalize().
    Scales a tensor \f$(A)\f$ of 8-bit NCHW values \f$(A_{ijkl})\f$ to create
    a 32-bit floating point tensor \f$(B)\f$ by subtracting a constant
    \f$(s)\f$, then multiplying by another constant \f$(m)\f$, and then
    adding by another constant \f$(a)\f$ thereafter.
    \f[
    B_{ijkl} = m (A_{ijkl} - s) + a
    \f]

    When dealing with the output of a quantized network running on
    fixed-point hardware or software, this routine can reverse that
    quantization by using a zero as the addition value \f$(a=0)\f$.

    Global batch normalization using a global mean \f$(s = \mu)\f$ and
    standard deviation \f$(m = 1/\sigma)\f$ along with no addition \f$(a=0)\f$.
    \f[
    B_{ijkl} = (A_{ijkl} - \mu)/\sigma
    \f]
    Scaling to a new mean \f$(\mu')\f$ and width \f$(\sigma')\f$ can be
    combined into single constants \f$m = \sigma'/\sigma\f$, \f$s = \mu\f$,
    and \f$a = \mu'\f$.
    \f{eqnarray*}{
    B_{ijkl} &=& m (A_{ijkl} - s) + a \\
             &=& (\sigma'/\sigma)(A_{ijkl} - \mu) + \mu'
    \f}

@param A
    Pointer to the source tensor
    \n\b WARNING: Must be 128-byte aligned.

@param Aformat
    Pointer to the format descriptor for source tensor.

@param B
    Pointer to the destination tensor.
    \n\b WARNING: Must be 128-byte aligned.

@param Bformat
    Pointer to the format descriptor for destination tensor.
    \n\b WARNING: Last dimension (W) must stride of 4 [sizeof(float32_t)].

@param normlz
    Normalization parameters apply to all color channels equally.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup tensor
****************************************************************************/
FADAS_API
FadasError_e FadasTensor_RenormalizeNCHWu8f32( const uint8_t       *__restrict srcA,
                                               FadasTensorFormat_t *Aformat,
                                               float32_t           *__restrict dstB,
                                               FadasTensorFormat_t *Bformat,
                                               FadasNormlzParams_t normlz );


#ifdef __cplusplus
}
#endif

#endif /* FADASTENSOR_H */
