// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_CL2DFLEX_CLH
#define RIDEHAL_CL2DFLEX_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceConvertNV12ToRGB = KernelCode(
        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };

        __kernel void ConvertNV12ToRGB( __global const uchar *srcPtr, __global uchar *dstPtr,
                                        int rows, int cols, int inputStride0, int inputHeight0,
                                        int inputStride1, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < cols / 2 )
            {
                if ( y < rows / 2 )
                {
                    __global const uchar *ySrc = srcPtr + mad24( y << 1, inputStride0, ( x << 1 ) );
                    __global const uchar *uSrc = srcPtr + inputStride0 * inputHeight0 +
                                                 mad24( y, inputStride1, ( x << 1 ) );
                    __global uchar *dst1 =
                            dstPtr + mad24( y << 1, cols * 3, mad24( x << 1, 3, 0 ) );
                    __global uchar *dst2 = dst1 + outputStride * 3;
                    float Y1 = ySrc[0];
                    float Y2 = ySrc[1];
                    float Y3 = ySrc[inputStride0];
                    float Y4 = ySrc[inputStride0 + 1];
                    float U = ( (float) uSrc[0] ) - 128;
                    float V = ( (float) uSrc[1] ) - 128;
                    float ruv = fma( coeffs[4], V, 0.5f );
                    float guv = fma( coeffs[3], V, fma( coeffs[2], U, 0.5f ) );
                    float buv = fma( coeffs[1], U, 0.5f );
                    Y1 = max( 0.f, Y1 - 16.f ) * coeffs[0];
                    dst1[0] = convert_uchar_sat( Y1 + ruv );
                    dst1[1] = convert_uchar_sat( Y1 + guv );
                    dst1[2] = convert_uchar_sat( Y1 + buv );
                    Y2 = max( 0.f, Y2 - 16.f ) * coeffs[0];
                    dst1[3] = convert_uchar_sat( Y2 + ruv );
                    dst1[4] = convert_uchar_sat( Y2 + guv );
                    dst1[5] = convert_uchar_sat( Y2 + buv );
                    Y3 = max( 0.f, Y3 - 16.f ) * coeffs[0];
                    dst2[0] = convert_uchar_sat( Y3 + ruv );
                    dst2[1] = convert_uchar_sat( Y3 + guv );
                    dst2[2] = convert_uchar_sat( Y3 + buv );
                    Y4 = max( 0.f, Y4 - 16.f ) * coeffs[0];
                    dst2[3] = convert_uchar_sat( Y4 + ruv );
                    dst2[4] = convert_uchar_sat( Y4 + guv );
                    dst2[5] = convert_uchar_sat( Y4 + buv );
                }
            }
        } );

static const char *s_pSourceConvertUYVYToRGB = KernelCode(
        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };

        __kernel void ConvertUYVYToRGB( __global const uchar *srcPtr, __global uchar *dstPtr,
                                        int rows, int cols, int inputStride, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < cols / 2 )
            {
                if ( y < rows )
                {
                    __global const uchar *uSrc = srcPtr + mad24( y, inputStride, x * 4 );
                    __global uchar *dst = dstPtr + mad24( y, cols * 3, mad24( x << 1, 3, 0 ) );
                    float U = ( (float) uSrc[0] ) - 128;
                    float Y1 = uSrc[1];
                    float V = ( (float) uSrc[2] ) - 128;
                    float Y2 = uSrc[3];
                    float ruv = fma( coeffs[4], V, 0.5f );
                    float guv = fma( coeffs[3], V, fma( coeffs[2], U, 0.5f ) );
                    float buv = fma( coeffs[1], U, 0.5f );
                    Y1 = max( 0.f, Y1 - 16.f ) * coeffs[0];
                    dst[0] = convert_uchar_sat( Y1 + ruv );
                    dst[1] = convert_uchar_sat( Y1 + guv );
                    dst[2] = convert_uchar_sat( Y1 + buv );
                    Y2 = max( 0.f, Y2 - 16.f ) * coeffs[0];
                    dst[3] = convert_uchar_sat( Y2 + ruv );
                    dst[4] = convert_uchar_sat( Y2 + guv );
                    dst[5] = convert_uchar_sat( Y2 + buv );
                }
            }
        } );

static const char *s_pSourceConvertUYVYToNV12 = KernelCode( __kernel void ConvertUYVYToNV12(
        __global const uchar *srcPtr, __global uchar *dstPtr, int rows, int cols, int inputStride,
        int outputStride0, int outputHeight0, int outputStride1 ) {
    int x = get_global_id( 0 );
    int y = get_global_id( 1 );
    if ( x < cols / 2 )
    {
        if ( y < rows / 2 )
        {
            __global const uchar *src = srcPtr + mad24( y << 1, inputStride, x * 4 );
            __global uchar *dst1 = dstPtr + mad24( y << 1, outputStride0, x << 1 );
            __global uchar *dst2 =
                    dstPtr + outputStride0 * outputHeight0 + mad24( y, outputStride1, x << 1 );
            float U1 = src[0];
            float V1 = src[2];
            float U2 = src[0 + inputStride];
            float V2 = src[2 + inputStride];
            dst1[0] = src[1];
            dst1[1] = src[3];
            dst1[0 + outputStride0] = src[1 + inputStride];
            dst1[1 + outputStride0] = src[3 + inputStride];
            dst2[0] = convert_uchar_sat( ( U1 + U2 ) / 2.0f );
            dst2[1] = convert_uchar_sat( ( V1 + V2 ) / 2.0f );
        }
    }
} );

static const char *s_pSourceResizeNV12ToRGB = KernelCode(
        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };

        __kernel void ResizeNV12ToRGB( __global const uchar *srcPtr, __global uchar *dstPtr,
                                       int inputHeight, int inputWidth, int outputHeight,
                                       int outputWidth, int inputStride0, int inputHeight0,
                                       int inputStride1, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < outputWidth )
            {
                if ( y < outputHeight )
                {
                    __global const uchar *ySrc = srcPtr;
                    __global const uchar *uSrc = srcPtr + inputStride0 * inputHeight0;
                    __global uchar *dst = dstPtr + mad24( y, outputStride, x * 3 );
                    int xIn = round( (float) x / (float) outputWidth * (float) inputWidth );
                    int yIn = round( (float) y / (float) outputHeight * (float) inputHeight );
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = ySrc[yPtr];
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float U = uSrc[uPtr] - 128.0f;
                    float V = uSrc[uPtr + 1] - 128.0f;
                    float ruv = fma( coeffs[4], V, 0.5f );
                    float guv = fma( coeffs[3], V, fma( coeffs[2], U, 0.5f ) );
                    float buv = fma( coeffs[1], U, 0.5f );
                    dst[0] = convert_uchar_sat( Y + ruv );
                    dst[1] = convert_uchar_sat( Y + guv );
                    dst[2] = convert_uchar_sat( Y + buv );
                }
            }
        } );

static const char *s_pSourceResizeUYVYToRGB = KernelCode(
        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };

        __kernel void ResizeUYVYToRGB( __global const uchar *srcPtr, __global uchar *dstPtr,
                                       int inputHeight, int inputWidth, int outputHeight,
                                       int outputWidth, int inputStride, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < outputWidth )
            {
                if ( y < outputHeight )
                {
                    __global const uchar *src = srcPtr;
                    __global uchar *dst = dstPtr + mad24( y, outputStride, x * 3 );
                    int xIn = round( (float) x / (float) outputWidth * (float) inputWidth );
                    int yIn = round( (float) y / (float) outputHeight * (float) inputHeight );
                    int yPtr = mad24( yIn, inputStride, xIn << 1 ) + 1;
                    float Y = src[yPtr];
                    int uPtr = mad24( yIn, inputStride, ( xIn / 2 ) * 4 );
                    float U = src[uPtr] - 128.0f;
                    float V = src[uPtr + 2] - 128.0f;
                    float ruv = fma( coeffs[4], V, 0.5f );
                    float guv = fma( coeffs[3], V, fma( coeffs[2], U, 0.5f ) );
                    float buv = fma( coeffs[1], U, 0.5f );
                    dst[0] = convert_uchar_sat( Y + ruv );
                    dst[1] = convert_uchar_sat( Y + guv );
                    dst[2] = convert_uchar_sat( Y + buv );
                }
            }
        } );

static const char *s_pSourceResizeUYVYToNV12 = KernelCode( __kernel void ResizeUYVYToNV12(
        __global const uchar *srcPtr, __global uchar *dstPtr, int inputHeight, int inputWidth,
        int outputHeight, int outputWidth, int inputStride, int outputStride0, int outputHeight0,
        int outputStride1 ) {
    int x = get_global_id( 0 );
    int y = get_global_id( 1 );
    if ( x < outputWidth )
    {
        if ( y < outputHeight )
        {
            __global uchar *ydst = dstPtr + mad24( y, outputStride0, x );
            int xIn = round( (float) x / (float) outputWidth * (float) inputWidth );
            int yIn = round( (float) y / (float) outputHeight * (float) inputHeight );
            int yPtr = mad24( yIn, inputStride, xIn << 1 ) + 1;
            ydst[0] = srcPtr[yPtr];
        }
    }
    if ( x < outputWidth / 2 )
    {
        if ( y < outputHeight / 2 )
        {
            __global uchar *udst =
                    dstPtr + outputStride0 * outputHeight0 + mad24( y, outputStride1, x << 1 );
            int xIn = round( (float) ( x << 1 ) / (float) outputWidth * (float) inputWidth );
            int yIn = round( (float) ( y << 1 ) / (float) outputHeight * (float) inputHeight );
            int uPtr = mad24( yIn, inputStride, ( xIn / 2 ) * 4 );
            udst[0] = srcPtr[uPtr];
            udst[1] = srcPtr[uPtr + 2];
        }
    }
} );

#endif   // RIDEHAL_CL2DFLEX_CLH