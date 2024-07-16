// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_CL2DFLEX_CLH
#define RIDEHAL_CL2DFLEX_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceConvertNV12ToRGB = KernelCode(
        __constant float c_YUV2RGBCoeffs_420[5] = { 1.163999557f, 2.017999649f, -0.390999794f,
                                                    -0.812999725f, 1.5959997177f };

        __kernel void ConvertNV12ToRGB( __global const uchar *srcptr, __global uchar *dstptr,
                                        int rows, int cols, int inputStride0, int inputHeight0,
                                        int inputStride1, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < cols / 2 )
            {
                if ( y < rows / 2 )
                {
                    __global const uchar *ysrc = srcptr + mad24( y << 1, inputStride0, ( x << 1 ) );
                    __global const uchar *usrc = srcptr + inputStride0 * inputHeight0 +
                                                 mad24( y, inputStride1, ( x << 1 ) );
                    __global uchar *dst1 =
                            dstptr + mad24( y << 1, cols * 3, mad24( x << 1, 3, 0 ) );
                    __global uchar *dst2 = dst1 + outputStride * 3;
                    float Y1 = ysrc[0];
                    float Y2 = ysrc[1];
                    float Y3 = ysrc[inputStride0];
                    float Y4 = ysrc[inputStride0 + 1];
                    float U = ( (float) usrc[0] ) - 128;
                    float V = ( (float) usrc[1] ) - 128;
                    __constant float *coeffs = c_YUV2RGBCoeffs_420;
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
        __constant float c_YUV2RGBCoeffs_420[5] = { 1.163999557f, 2.017999649f, -0.390999794f,
                                                    -0.812999725f, 1.5959997177f };

        __kernel void ConvertUYVYToRGB( __global const uchar *srcptr, __global uchar *dstptr,
                                        int rows, int cols, int inputStride, int outputStride ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            if ( x < cols / 2 )
            {
                if ( y < rows )
                {
                    __global const uchar *usrc = srcptr + mad24( y, inputStride, x * 4 );
                    __global uchar *dst = dstptr + mad24( y, cols * 3, mad24( x << 1, 3, 0 ) );
                    float U = ( (float) usrc[0] ) - 128;
                    float Y1 = usrc[1];
                    float V = ( (float) usrc[2] ) - 128;
                    float Y2 = usrc[3];
                    __constant float *coeffs = c_YUV2RGBCoeffs_420;
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
        __global const uchar *srcptr, __global uchar *dstptr, int rows, int cols, int inputStride,
        int outputStride0, int outputHeight0, int outputStride1 ) {
    int x = get_global_id( 0 );
    int y = get_global_id( 1 );
    if ( x < cols / 2 )
    {
        if ( y < rows / 2 )
        {
            __global const uchar *src = srcptr + mad24( y << 1, inputStride, x * 4 );
            __global uchar *dst1 = dstptr + mad24( y << 1, outputStride0, x << 1 );
            __global uchar *dst2 =
                    dstptr + outputStride0 * outputHeight0 + mad24( y, outputStride1, x << 1 );
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

#endif   // RIDEHAL_CL2DFLEX_CLH