/***************************************************************************//**
@file
    fadasSVD.h

@brief
    Matrix Decomposition

@defgroup matrix_decomp Matrix Decomposition

@details
    Decomposition of matrices.

@internal
    Copyright (c) 2020-2022 Qualcomm Technologies, Inc. All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASSVD_H
#define FADASSVD_H

#include <stdint.h>
#include "fadas.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************//**
@brief
    Initialize SVD module.
    \n\b WARNING: Must be called once before other FastADAS functions
    except FadasVersion().


@param licenseKey
    License key string.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_Init(const char* licenseKey);



 /************************************************************************//**
@brief
    Deinitialize SVD module.
    \n\b WARNING: Must be called once after all other FastADAS functions
    except FadasVersion().

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_DeInit(void);



/************************************************************************//**
@brief
    Compute a singular value decomposition (SVD) of a matrix of a float type A = U*diag[w]*Vt;
    for solving problems such as least-squares, underdetermined linear systems, and
    matrix inversion. The algorithm does not compute the full U and
    V matrices, it computes a condensed version of U and V that is
    sufficient to solve most problems that use SVD.

@param A
    Pointer to the input matrix of dimension m x n.
    \n\b WARNING: Must be 128-byte aligned.

@param m
    Number of rows of matrix A

@param n
    Number of columns of matrix A

@param w
    Pointer to the buffer that holds n singular values. When m > n, the buffer contains n singular
    values while when m < n, only the first m singular values are significant. However,
    during allocation, it should be allocated as a buffer to hold n floats.
    \n\b WARNING: Must be 128-byte aligned.

@param U
    Pointer to the U matrix whose dimension is m x min(m,n). This is not the full size U matrix obtained
    from the conventional SVD algorithm, but is sufficient for solving problems like least-squares,
    under-determined linear systems, matrix inversion and so forth. While allocating, allocate as
    a matrix of m x n floats.
    \n\b WARNING: Must be 128-byte aligned.

@param Vt
    Pointer to the V matrix whose dimension is n x min(m,n). Not the full size V matrix obtained from
    the conventional SVD algorithm, but is sufficient for solving problems like least-squares,
    under-determined linear systems, matrix inversion and so forth. While allocating, allocate as
    a matrix of n x n floats.
    \n\b WARNING: Must be 128-byte aligned.

@param tmpU
    Temporary buffer used in processing. It must be allocated as an array of size m x n.
    \n\b WARNING: Must be 128-byte aligned.

@param tmpV
    Pointer to the temporary buffer used in processing. It must be allocated as an array of size m x n.
    \n\b WARNING: Must be 128-byte aligned.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_SVDf32( const float32_t *__restrict A,
                              uint32_t        m,
                              uint32_t        n,
                              float32_t       *__restrict w,
                              float32_t       *__restrict U,
                              float32_t       *__restrict Vt,
                              float32_t       *tmpU,
                              float32_t       *tmpV );



/************************************************************************//**
@brief
    Executes Cholesky decomposition algorithm on a symmetric and positive definite
    matrix to solve the linear system A*x = b, where A is an N x N matrix and x and b
    are vectors of size N.

@param A
    Pointer to the matrix A or size N x N.
    \n\b WARNING:
    \li This matrix is modified during computation. Save the
    original matrix  if necessary
    \li Must be 128-byte aligned

@param b
    Pointer to the vector b of size N.
    \n\b WARNING: Must be 128-byte aligned.

@param diag
    Pointer to the buffer for the diagonal of matrix A. This buffer is for
    computation.
    \n\b WARNING: Must be 128-byte aligned.

@param N
    Size of matrix and vectors.

@param x
    Pointer to the output vector x of size N.
    \n\b WARNING: Must be 128-byte aligned.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_SolveCholeskyf32( float32_t       *__restrict A,
                                        const float32_t *__restrict b,
                                        float32_t       *__restrict diag,
                                        uint32_t        N,
                                        float32_t       *__restrict x );



/************************************************************************//**
@brief
    Solves a linear system of equations using LU decomposition. Given a system
    defined by A x = b, the function decomposes A into a lower triangular matrix
    L and an upper triangular matrix U. It then computes x by solving L y = B
    by forward substitution for y, and then solving the system of linear equations
    U x = y by backward substitution for x.
    \n\b NOTE: Because not all matrices have a LU decomposition, pivoting ensures 
    that a nonsingular matrix can be solved.

@param A
    Input coefficient matrix of the linear system of dimension N x N
    \n\b WARNING:
    \li Must be square 
    \li Must be 128-byte aligned

@param b
    Component vector of the linear system of dimension N x 1.

@param N
    Dimension of the input matrix A and component vector b.

@param pivot
    N x 1 pivot array that is populated as follows: for each k = 0, 1, .... , N-1,
    the i-th element of pivot contains the row interchanged with row i when k = i.
    Pivoting ensures numerical stability and LU
    factorization for the input matrix A.

@param x
    The solution of the linear system, an N x 1 vector.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_SolveLUf32( float32_t *__restrict A,
                                  float32_t *__restrict b,
                                  uint32_t  N,
                                  uint8_t   *__restrict pivot,
                                  float32_t *__restrict x );



/************************************************************************//**
@brief
    Executes LDL decomposition algorithm on a symmetric and positive definite
    matrix to solve the linear system A*x = b, where A is an NxN matrix and
    x and b are vectors of size N.

@param A
    Pointer to the matrix A or size N x N.
    \n\b WARNING:
    \li This matrix is modified during computation. Save
    the original matrix if necessary
    \li Must be 128-byte aligned

@param b
    Pointer to the vector b of size N.
    \n\b WARNING: Must be 128-byte aligned.

@param diag
    Pointer to the buffer for the diagonal of matrix A. This buffer is for
    computation.
    \n\b WARNING: Must be 128-byte aligned.

@param N
    Size of matrix and vectors.

@param x
    Pointer to the output vector x of size N.
    \n\b WARNING: Must be 128-byte aligned.

@return
    #FADAS_ERROR_NONE -- Success.

@ingroup matrix_decomp
****************************************************************************/
FADAS_API
FadasError_e FadasSVD_SolveLDLf32( float32_t       *__restrict A,
                                   const float32_t *__restrict b,
                                   float32_t       *__restrict diag,
                                   uint32_t        N,
                                   float32_t       *__restrict x );


#ifdef __cplusplus
}
#endif

#endif /* FADASSVD_H */
