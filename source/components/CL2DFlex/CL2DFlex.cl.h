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

            int yOffset =
                    srcOffset + mad24( ( y + roiY ) << 1, inputStride0, ( ( x + roiX ) << 1 ) );
            int uOffset = srcOffset + inputPlane0Size +
                          mad24( ( y + roiY ), inputStride1, ( ( x + roiX ) << 1 ) );
            int dstOffset1 = dstOffset + mad24( y << 1, outputStride, x * 6 );
            int dstOffset2 = dstOffset1 + outputStride;

            float4 YSrcVals, Yvec4;
            float4 U4, V4, UV4, YU, YV, YUV;
            uchar4 dst1Val4, dst2Val4, uYU, uYV, uYUV;
            uchar2 dst1Val2, dst2Val2;

            YSrcVals.s01 = convert_float2( vload2( 0, srcPtr + yOffset ) );
            YSrcVals.s23 = convert_float2( vload2( 0, srcPtr + yOffset + inputStride0 ) );
            YSrcVals -= 16.0f;
            Yvec4 = max( 0, YSrcVals );
            Yvec4 *= 1.163999557f;

            float2 UV = convert_float2( vload2( 0, srcPtr + uOffset ) );
            UV -= 128.0f;

            U4 = (float4) ( UV.s0, UV.s0, UV.s0, UV.s0 );
            V4 = (float4) ( UV.s1, UV.s1, UV.s1, UV.s1 );
            UV4 = -0.390999794f * U4 - 0.812999725f * V4 + 0.5f;
            U4 = 2.017999649f * U4 + 0.5f;
            V4 = 1.5959997177f * V4 + 0.5f;

            YUV = Yvec4 + UV4;
            YU = Yvec4 + U4;
            YV = Yvec4 + V4;

            uYU = convert_uchar4_sat( YU );
            uYV = convert_uchar4_sat( YV );
            uYUV = convert_uchar4_sat( YUV );
            dst1Val4 = (uchar4) ( uYV.s0, uYUV.s0, uYU.s0, uYV.s1 );
            dst1Val2 = (uchar2) ( uYUV.s1, uYU.s1 );
            dst2Val4 = (uchar4) ( uYV.s2, uYUV.s2, uYU.s2, uYV.s3 );
            dst2Val2 = (uchar2) ( YUV.s3, YU.s3 );

            vstore4( dst1Val4, 0, dstPtr + dstOffset1 );
            vstore2( dst1Val2, 2, dstPtr + dstOffset1 );
            vstore4( dst2Val4, 0, dstPtr + dstOffset2 );
            vstore2( dst2Val2, 2, dstPtr + dstOffset2 );
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
