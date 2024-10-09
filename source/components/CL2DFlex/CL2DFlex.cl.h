// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2DFLEX_CLH
#define RIDEHAL_CL2DFLEX_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceCL2DFlex = KernelCode(

        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };

        __kernel void ConvertNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputStride0,
                                        int inputPlane0Size, int inputStride1, int outputStride,
                                        int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc =
                    srcPtr + srcOffset +
                    mad24( ( y + roiY ) << 1, inputStride0, ( ( x + roiX ) << 1 ) );
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size +
                                         mad24( ( y + roiY ), inputStride1, ( ( x + roiX ) << 1 ) );
            __global uchar *dst1 = dstPtr + dstOffset + mad24( y << 1, outputStride, x * 6 );
            __global uchar *dst2 = dst1 + outputStride;
            float Y1 = max( 0, ySrc[0] - 16 );
            float Y2 = max( 0, ySrc[1] - 16 );
            float Y3 = max( 0, ySrc[inputStride0] - 16 );
            float Y4 = max( 0, ySrc[inputStride0 + 1] - 16 );
            float U = uSrc[0] - 128;
            float V = uSrc[1] - 128;
            dst1[0] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[4] * V + 0.5f );
            dst1[1] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst1[2] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[1] * U + 0.5f );
            dst1[3] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[4] * V + 0.5f );
            dst1[4] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst1[5] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[1] * U + 0.5f );
            dst2[0] = convert_uchar_sat( coeffs[0] * Y3 + coeffs[4] * V + 0.5f );
            dst2[1] = convert_uchar_sat( coeffs[0] * Y3 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst2[2] = convert_uchar_sat( coeffs[0] * Y3 + coeffs[1] * U + 0.5f );
            dst2[3] = convert_uchar_sat( coeffs[0] * Y4 + coeffs[4] * V + 0.5f );
            dst2[4] = convert_uchar_sat( coeffs[0] * Y4 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst2[5] = convert_uchar_sat( coeffs[0] * Y4 + coeffs[1] * U + 0.5f );
        }

        __kernel void ConvertUYVYToRGB( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputStride,
                                        int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *uSrc =
                    srcPtr + srcOffset + mad24( ( y + roiY ), inputStride, ( x + roiX ) * 4 );
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 6 );
            float U = uSrc[0] - 128;
            float Y1 = max( 0, uSrc[1] - 16 );
            float V = uSrc[2] - 128;
            float Y2 = max( 0, uSrc[3] - 16 );
            dst[0] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[4] * V + 0.5f );
            dst[1] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst[2] = convert_uchar_sat( coeffs[0] * Y1 + coeffs[1] * U + 0.5f );
            dst[3] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[4] * V + 0.5f );
            dst[4] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst[5] = convert_uchar_sat( coeffs[0] * Y2 + coeffs[1] * U + 0.5f );
        }

        __kernel void ConvertUYVYToNV12( __global const uchar *srcPtr, int srcOffset,
                                         __global uchar *dstPtr, int dstOffset, int inputStride,
                                         int outputStride0, int outputPlane0Size, int outputStride1,
                                         int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *src =
                    srcPtr + srcOffset + mad24( ( y + roiY ) << 1, inputStride, ( x + roiX ) * 4 );
            __global uchar *dst1 = dstPtr + dstOffset + mad24( y << 1, outputStride0, x << 1 );
            __global uchar *dst2 =
                    dstPtr + dstOffset + outputPlane0Size + mad24( y, outputStride1, x << 1 );
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

        __kernel void ResizeNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                       __global uchar *dstPtr, int dstOffset, int inputHeight,
                                       int inputWidth, int resizeHeight, int resizeWidth,
                                       int inputStride0, int inputPlane0Size, int inputStride1,
                                       int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            int xIn = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int yPtr = mad24( yIn, inputStride0, xIn );
            float Y = max( 0, ySrc[yPtr] - 16 );
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float U = uSrc[uPtr] - 128;
            float V = uSrc[uPtr + 1] - 128;
            dst[0] = convert_uchar_sat( coeffs[0] * Y + coeffs[4] * V + 0.5f );
            dst[1] = convert_uchar_sat( coeffs[0] * Y + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst[2] = convert_uchar_sat( coeffs[0] * Y + coeffs[1] * U + 0.5f );
        }

        __kernel void ResizeUYVYToRGB( __global const uchar *srcPtr, int srcOffset,
                                       __global uchar *dstPtr, int dstOffset, int inputHeight,
                                       int inputWidth, int resizeHeight, int resizeWidth,
                                       int inputStride, int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *src = srcPtr + srcOffset;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            int xIn = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int yPtr = mad24( yIn, inputStride, xIn << 1 ) + 1;
            float Y = max( 0, src[yPtr] - 16 );
            int uPtr = mad24( yIn, inputStride, ( xIn / 2 ) * 4 );
            float U = src[uPtr] - 128;
            float V = src[uPtr + 2] - 128;
            dst[0] = convert_uchar_sat( coeffs[0] * Y + coeffs[4] * V + 0.5f );
            dst[1] = convert_uchar_sat( coeffs[0] * Y + coeffs[2] * U + coeffs[3] * V + 0.5f );
            dst[2] = convert_uchar_sat( coeffs[0] * Y + coeffs[1] * U + 0.5f );
        }

        __kernel void ResizeUYVYToNV12(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                int inputHeight, int inputWidth, int resizeHeight, int resizeWidth, int inputStride,
                int outputStride0, int outputPlane0Size, int outputStride1, int roiX, int roiY,
                int outputHeight, int outputWidth ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global uchar *ydst = dstPtr + dstOffset + mad24( y, outputStride0, x );
            int xIn1 = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn1 = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int yPtr = mad24( yIn1, inputStride, xIn1 << 1 ) + 1;
            ydst[0] = srcPtr[yPtr + srcOffset];
            if ( x < outputWidth / 2 )
            {
                if ( y < outputHeight / 2 )
                {
                    __global uchar *udst = dstPtr + dstOffset + outputPlane0Size +
                                           mad24( y, outputStride1, x << 1 );
                    int xIn2 = round( (float) ( ( x + roiX / 2 ) << 1 ) / (float) resizeWidth *
                                      (float) inputWidth );
                    int yIn2 = round( (float) ( ( y + roiY / 2 ) << 1 ) / (float) resizeHeight *
                                      (float) inputHeight );
                    int uPtr = mad24( yIn2, inputStride, ( xIn2 / 2 ) * 4 );
                    udst[0] = srcPtr[uPtr + srcOffset];
                    udst[1] = srcPtr[uPtr + srcOffset + 2];
                }
            }
        }

        __kernel void LetterboxNV12ToRGB(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                int inputHeight, int inputWidth, int resizeHeight, int resizeWidth,
                int inputStride0, int inputPlane0Size, int inputStride1, int outputStride, int roiX,
                int roiY, float inputRatio, float outputRatio ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            if ( inputRatio < outputRatio )
            {
                if ( y < ( resizeWidth * inputRatio ) )
                {
                    int xIn = round( (float) x / (float) resizeWidth * (float) inputWidth ) + roiX;
                    int yIn = round( (float) y / (float) resizeWidth * (float) inputWidth ) + roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float U = uSrc[uPtr] - 128;
                    float V = uSrc[uPtr + 1] - 128;
                    dst[0] = convert_uchar_sat( coeffs[0] * Y + coeffs[4] * V + 0.5f );
                    dst[1] = convert_uchar_sat( coeffs[0] * Y + coeffs[2] * U + coeffs[3] * V +
                                                0.5f );
                    dst[2] = convert_uchar_sat( coeffs[0] * Y + coeffs[1] * U + 0.5f );
                }
                else
                {
                    dst[0] = 0;
                    dst[1] = 0;
                    dst[2] = 0;
                }
            }
            else
            {
                if ( x < ( resizeHeight / inputRatio ) )
                {
                    int xIn =
                            round( (float) x / (float) resizeHeight * (float) inputHeight ) + roiX;
                    int yIn =
                            round( (float) y / (float) resizeHeight * (float) inputHeight ) + roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float U = uSrc[uPtr] - 128;
                    float V = uSrc[uPtr + 1] - 128;
                    dst[0] = convert_uchar_sat( coeffs[0] * Y + coeffs[4] * V + 0.5f );
                    dst[1] = convert_uchar_sat( coeffs[0] * Y + coeffs[2] * U + coeffs[3] * V +
                                                0.5f );
                    dst[2] = convert_uchar_sat( coeffs[0] * Y + coeffs[1] * U + 0.5f );
                }
                else
                {
                    dst[0] = 0;
                    dst[1] = 0;
                    dst[2] = 0;
                }
            }
        } );

#endif   // RIDEHAL_CL2DFLEX_CLH