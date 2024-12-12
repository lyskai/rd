// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2DFLEX_CLH
#define RIDEHAL_CL2DFLEX_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceCL2DFlex = KernelCode(

        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };
        __constant float coeffY = 1.163999557f;
        __constant float4 coeffUV4 =
                ( float4 )( 2.017999649f, -0.812999725f, -0.390999794f, 1.5959997177f );

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

            uchar4 dst1Val4, dst2Val4;
            uchar2 dst1Val2, dst2Val2;

            float2 Y12 = convert_float2( vload2( 0, srcPtr + yOffset ) ) - 16.0f;
            float2 Y34 = convert_float2( vload2( 0, srcPtr + yOffset + inputStride0 ) ) - 16.0f;
            Y12 = max( 0, Y12 ) * coeffY;
            Y34 = max( 0, Y34 ) * coeffY;

            float2 UV = convert_float2( vload2( 0, srcPtr + uOffset ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;

            float4 Y1UV = UV4 + Y12.s0;
            float4 Y2UV = UV4 + Y12.s1;
            float4 Y3UV = UV4 + Y34.s0;
            float4 Y4UV = UV4 + Y34.s1;

            dst1Val4 = convert_uchar4_sat( ( float4 )( Y1UV.s3, Y1UV.s1, Y1UV.s0, Y2UV.s3 ) );
            dst1Val2 = convert_uchar2_sat( ( float2 )( Y2UV.s1, Y2UV.s0 ) );
            dst2Val4 = convert_uchar4_sat( ( float4 )( Y3UV.s3, Y3UV.s1, Y3UV.s0, Y4UV.s3 ) );
            dst2Val2 = convert_uchar2_sat( ( float2 )( Y4UV.s1, Y4UV.s0 ) );

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

            float4 UYVY = convert_float4( vload4( 0, uSrc ) );
            float2 UV = ( float2 )( UYVY.s0, UYVY.s2 ) - 128.0f;
            float2 Y1Y2 = ( float2 )( UYVY.s1, UYVY.s3 ) - 16.0f;
            Y1Y2 = max( 0, Y1Y2 ) * coeffY;
            float4 Y1UV = ( float4 )( Y1Y2.s0 );
            float4 Y2UV = ( float4 )( Y1Y2.s1 );
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;

            Y1UV += UV4;
            Y2UV += UV4;
            uchar4 udst1 = convert_uchar4_sat( ( float4 )( Y1UV.s3, Y1UV.s1, Y1UV.s0, Y2UV.s3 ) );
            uchar2 udst2 = convert_uchar2_sat( ( float2 )( Y2UV.s1, Y2UV.s0 ) );
            vstore4( udst1, 0, dst );
            vstore2( udst2, 2, dst );
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

            int xIn = round( (float) ( x + roiX ) * native_recip( (float) resizeWidth ) *
                             (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) * native_recip( (float) resizeHeight ) *
                             (float) inputHeight );
            int yPtr = mad24( yIn, inputStride0, xIn );
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
            float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            uchar3 RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
        }

        __kernel void ResizeUYVYToRGB( __global const uchar *srcPtr, int srcOffset,
                                       __global uchar *dstPtr, int dstOffset, int inputHeight,
                                       int inputWidth, int resizeHeight, int resizeWidth,
                                       int inputStride, int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *src = srcPtr + srcOffset;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );

            int xIn = round( (float) ( x + roiX ) * native_recip( (float) resizeWidth ) *
                             (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) * native_recip( (float) resizeHeight ) *
                             (float) inputHeight );
            int yPtr = mad24( yIn, inputStride, xIn << 1 ) + 1;
            int uPtr = mad24( yIn, inputStride, ( xIn / 2 ) * 4 );
            float Y = max( 0, src[yPtr] - 16 ) * coeffY;
            float3 UV3 = convert_float3( vload3( 0, src + uPtr ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV3.s0, UV3.s2, UV3.s0, UV3.s2 );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            uchar3 RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
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

        __kernel void ResizeRGBToRGB( __global const uchar *srcPtr, int srcOffset,
                                      __global uchar *dstPtr, int dstOffset, int inputHeight,
                                      int inputWidth, int resizeHeight, int resizeWidth,
                                      int inputStride, int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            int xIn = round( (float) ( x + roiX ) / (float) resizeWidth * (float) inputWidth );
            int yIn = round( (float) ( y + roiY ) / (float) resizeHeight * (float) inputHeight );
            int ptr = mad24( yIn, inputStride, xIn * 3 );
            dst[0] = srcPtr[srcOffset + ptr + 0];
            dst[1] = srcPtr[srcOffset + ptr + 1];
            dst[2] = srcPtr[srcOffset + ptr + 2];
        }

        __kernel void LetterboxNV12ToRGB(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                int inputHeight, int inputWidth, int resizeHeight, int resizeWidth,
                int inputStride0, int inputPlane0Size, int inputStride1, int outputStride, int roiX,
                int roiY, float inputRatio, float outputRatio, int paddingValue ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            uchar3 RGB;
            if ( inputRatio < outputRatio )
            {
                if ( y < ( resizeWidth * inputRatio ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeWidth ) *
                                     (float) inputWidth ) +
                              roiX;
                    int yIn = round( (float) y * native_recip( (float) resizeWidth ) *
                                     (float) inputWidth ) +
                              roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
            else
            {
                if ( x < ( resizeHeight * native_recip( inputRatio ) ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeHeight ) *
                                     (float) inputHeight ) +
                              roiX;
                    int yIn = round( (float) y * native_recip( (float) resizeHeight ) *
                                     (float) inputHeight ) +
                              roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
        }

        __kernel void LetterboxNV12ToRGBMultiple(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                __global const int *roiPtr, int resizeHeight, int resizeWidth, int inputStride0,
                int inputPlane0Size, int inputStride1, int outputStride, int paddingValue ) {
            int i = get_global_id( 0 );
            int x = get_global_id( 1 );
            int y = get_global_id( 2 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + i * resizeHeight * outputStride +
                                  mad24( y, outputStride, x * 3 );
            int4 XYWH = vload4( 0, roiPtr + i * 4 );
            float inputRatio = (float) XYWH.s3 * native_recip( (float) XYWH.s2 );
            float outputRatio = (float) resizeHeight * native_recip( (float) resizeWidth );
            uchar3 RGB;

            if ( inputRatio < outputRatio )
            {
                if ( y < ( resizeWidth * inputRatio ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeWidth ) *
                                     (float) XYWH.s2 ) +
                              XYWH.s0;
                    int yIn = round( (float) y * native_recip( (float) resizeWidth ) *
                                     (float) XYWH.s2 ) +
                              XYWH.s1;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
            else
            {
                if ( x < ( resizeHeight * native_recip( inputRatio ) ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeHeight ) *
                                     (float) XYWH.s3 ) +
                              XYWH.s0;
                    int yIn = round( (float) y * native_recip( (float) resizeHeight ) *
                                     (float) XYWH.s3 ) +
                              XYWH.s1;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
        }

        __kernel void ResizeNV12ToRGBMultiple(
                __global const uchar *srcPtr, int srcOffset, __global uchar *dstPtr, int dstOffset,
                __global const int *roiPtr, int resizeHeight, int resizeWidth, int inputStride0,
                int inputPlane0Size, int inputStride1, int outputStride ) {
            int i = get_global_id( 0 );
            int x = get_global_id( 1 );
            int y = get_global_id( 2 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + i * resizeHeight * outputStride +
                                  mad24( y, outputStride, x * 3 );
            int4 XYWH = vload4( 0, roiPtr + i * 4 );
            uchar3 RGB;
            int xIn = round( (float) x * native_recip( (float) resizeWidth ) * (float) XYWH.s2 ) +
                      XYWH.s0;
            int yIn = round( (float) y * native_recip( (float) resizeHeight ) * (float) XYWH.s3 ) +
                      XYWH.s1;
            int yPtr = mad24( yIn, inputStride0, xIn );
            float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
            int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
            float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
            UV4 += Y;
            RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
            vstore3( RGB, 0, dst );
        }

        __kernel void Compress( __read_only image2d_t srcPlane, __write_only image2d_t dstPlane,
                                sampler_t sampler ) {
            const int x = get_global_id( 0 );
            const int y = get_global_id( 1 );
            const int2 coord = ( int2 )( x, y );
            const float4 pixel = read_imagef( srcPlane, sampler, coord );
            write_imagef( dstPlane, coord, pixel );
        } );

#endif   // RIDEHAL_CL2DFLEX_CLH
