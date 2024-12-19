// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_CLH
#define RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_CLH

KernelCode(

        __kernel void ConvertUBWC( __read_only image2d_t srcPlane, __write_only image2d_t dstPlane,
                                   sampler_t sampler ) {
            const int x = get_global_id( 0 );
            const int y = get_global_id( 1 );
            const int2 coord = ( int2 )( x, y );
            const float4 pixel = read_imagef( srcPlane, sampler, coord );
            write_imagef( dstPlane, coord, pixel );
        }

)

#endif   // RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_CLH
