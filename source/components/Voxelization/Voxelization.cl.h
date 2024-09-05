// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_VOXELIZATION_CLH
#define RIDEHAL_VOXELIZATION_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceClusterPoints = KernelCode(

        __kernel void ClusterPointsFromXYZR(
                __global const float *pInPts, __global float *pOutPlrs, __global float *pOutFeature,
                __global int *coorToPlrIdx, __global int *numOfPts, const float minXRange,
                const float minYRange, const float minZRange, const float maxXRange,
                const float maxYRange, const float maxZRange, const float pillarXSize,
                const float pillarYSize, const float pillarZSize, const int gridXSize,
                const int gridYSize, const int maxNumPlrs, const int maxNumPtsPerPlr,
                const int numOutFeatureDim ) {
            int x = get_global_id( 0 );
            float xPts = pInPts[x * 4 + 0];
            float yPts = pInPts[x * 4 + 1];
            float zPts = pInPts[x * 4 + 2];
            float rPts = pInPts[x * 4 + 3];
            if ( ( xPts > minXRange ) && ( xPts < maxXRange ) && ( yPts > minYRange ) &&
                 ( yPts < maxYRange ) && ( zPts > minZRange ) && ( zPts < maxZRange ) )
            {
                int xCoor = floor( ( xPts - minXRange ) / pillarXSize );
                int yCoor = floor( ( yPts - minYRange ) / pillarYSize );
                int id = yCoor * gridXSize + xCoor;
                int plrIdx = 0;
                if ( atomic_load( (atomic_int *) &numOfPts[maxNumPlrs] ) < maxNumPlrs )
                {
                    if ( atomic_cmpxchg( &coorToPlrIdx[id], -1, 0 ) == -1 )
                    {
                        coorToPlrIdx[id] = atomic_inc( &numOfPts[maxNumPlrs] );
                        plrIdx = coorToPlrIdx[id];
                        if ( plrIdx < maxNumPlrs )
                        {
                            pOutPlrs[plrIdx * 4 + 0] = (float) xCoor;
                            pOutPlrs[plrIdx * 4 + 1] = (float) yCoor;
                            pOutPlrs[plrIdx * 4 + 2] = 0.0;
                        }
                    }
                    else
                    {
                        plrIdx = coorToPlrIdx[id];
                    }

                    if ( plrIdx < maxNumPlrs )
                    {
                        if ( atomic_load( (atomic_int *) &numOfPts[plrIdx] ) < maxNumPtsPerPlr )
                        {
                            int numPts = atomic_inc( &numOfPts[plrIdx] );
                            pOutPlrs[plrIdx * 4 + 3] = numPts + 1;
                            if ( numPts < maxNumPtsPerPlr )
                            {
                                int featureID = plrIdx * maxNumPtsPerPlr * numOutFeatureDim +
                                                numPts * numOutFeatureDim;
                                pOutFeature[featureID + 0] = xPts;
                                pOutFeature[featureID + 1] = yPts;
                                pOutFeature[featureID + 2] = zPts;
                                pOutFeature[featureID + 3] = rPts;
                            }
                        }
                    }
                }
            }
        }

        __kernel void ClusterPointsFromXYZRT(
                __global const float *pInPts, __global int *pOutPlrs, __global float *pOutFeature,
                __global int *coorToPlrIdx, __global int *numOfPts, const float minXRange,
                const float minYRange, const float minZRange, const float maxXRange,
                const float maxYRange, const float maxZRange, const float pillarXSize,
                const float pillarYSize, const float pillarZSize, const int gridXSize,
                const int gridYSize, const int maxNumPlrs, const int maxNumPtsPerPlr,
                const int numOutFeatureDim ) {
            int x = get_global_id( 0 );
            float xPts = pInPts[x * 5 + 0];
            float yPts = pInPts[x * 5 + 1];
            float zPts = pInPts[x * 5 + 2];
            float rPts = pInPts[x * 5 + 3];
            float tPts = pInPts[x * 5 + 4];
            if ( ( xPts > minXRange ) && ( xPts < maxXRange ) && ( yPts > minYRange ) &&
                 ( yPts < maxYRange ) && ( zPts > minZRange ) && ( zPts < maxZRange ) )
            {
                int xCoor = floor( ( xPts - minXRange ) / pillarXSize );
                int yCoor = floor( ( yPts - minYRange ) / pillarYSize );
                int id = yCoor * gridXSize + xCoor;
                int plrIdx = 0;
                if ( atomic_load( (atomic_int *) &numOfPts[maxNumPlrs] ) < maxNumPlrs )
                {
                    if ( atomic_cmpxchg( &coorToPlrIdx[id], -1, 0 ) == -1 )
                    {
                        coorToPlrIdx[id] = atomic_inc( &numOfPts[maxNumPlrs] );
                        plrIdx = coorToPlrIdx[id];
                        if ( plrIdx < maxNumPlrs )
                        {
                            pOutPlrs[plrIdx * 2 + 0] = xCoor;
                            pOutPlrs[plrIdx * 2 + 1] = yCoor;
                        }
                    }
                    else
                    {
                        plrIdx = coorToPlrIdx[id];
                    }

                    if ( plrIdx < maxNumPlrs )
                    {
                        if ( atomic_load( (atomic_int *) &numOfPts[plrIdx] ) < maxNumPtsPerPlr )
                        {
                            int numPts = atomic_inc( &numOfPts[plrIdx] );
                            if ( numPts < maxNumPtsPerPlr )
                            {
                                int featureID = plrIdx * maxNumPtsPerPlr * numOutFeatureDim +
                                                numPts * numOutFeatureDim;
                                pOutFeature[featureID + 0] = xPts;
                                pOutFeature[featureID + 1] = yPts;
                                pOutFeature[featureID + 2] = zPts;
                                pOutFeature[featureID + 3] = rPts;
                                pOutFeature[featureID + 4] = tPts;
                            }
                        }
                    }
                }
            }
        }

);

static const char *s_pSourceFeatureGather = KernelCode(

        __kernel void FeatureGatherFromXYZR(
                __global float *pOutPlrs, __global float *pOutFeature, __global int *numOfPts,
                const float minXRange, const float minYRange, const float minZRange,
                const float pillarXSize, const float pillarYSize, const float pillarZSize,
                const int maxNumPlrs, const int maxNumPtsPerPlr, const int numOutFeatureDim,
                const int numOfPillar ) {
            int x = get_global_id( 0 );
            if ( x < numOfPillar )
            {
                pOutPlrs[x * 4 + 3] = min( pOutPlrs[x * 4 + 3], (float) maxNumPtsPerPlr );
                int numPts = (int) pOutPlrs[x * 4 + 3];
                float meanX = 0.0;
                float meanY = 0.0;
                float meanZ = 0.0;
                for ( int i = 0; i < numPts; i++ )
                {
                    int id1 = x * maxNumPtsPerPlr * numOutFeatureDim + i * numOutFeatureDim;
                    meanX += pOutFeature[id1 + 0];
                    meanY += pOutFeature[id1 + 1];
                    meanZ += pOutFeature[id1 + 2];
                }
                meanX = meanX / numPts;
                meanY = meanY / numPts;
                meanZ = meanZ / numPts;
                float pillarX = minXRange + pOutPlrs[x * 4 + 0] * pillarXSize + 0.5 * pillarXSize;
                float pillarY = minYRange + pOutPlrs[x * 4 + 1] * pillarYSize + 0.5 * pillarYSize;
                float pillarZ = minZRange + pOutPlrs[x * 4 + 2] * pillarZSize + 0.5 * pillarZSize;
                for ( int j = 0; j < maxNumPtsPerPlr; j++ )
                {
                    int id2 = x * maxNumPtsPerPlr * numOutFeatureDim + j * numOutFeatureDim;
                    if ( j < numPts )
                    {
                        pOutFeature[id2 + 4] = pOutFeature[id2 + 0] - meanX;
                        pOutFeature[id2 + 5] = pOutFeature[id2 + 1] - meanY;
                        pOutFeature[id2 + 6] = pOutFeature[id2 + 2] - meanZ;
                        pOutFeature[id2 + 7] = pOutFeature[id2 + 0] - pillarX;
                        pOutFeature[id2 + 8] = pOutFeature[id2 + 1] - pillarY;
                        pOutFeature[id2 + 9] = pOutFeature[id2 + 2] - pillarZ;
                    }
                    else
                    {
                        pOutFeature[id2 + 0] = 0.0;
                        pOutFeature[id2 + 1] = 0.0;
                        pOutFeature[id2 + 2] = 0.0;
                        pOutFeature[id2 + 3] = 0.0;
                        pOutFeature[id2 + 4] = 0.0;
                        pOutFeature[id2 + 5] = 0.0;
                        pOutFeature[id2 + 6] = 0.0;
                        pOutFeature[id2 + 7] = 0.0;
                        pOutFeature[id2 + 8] = 0.0;
                        pOutFeature[id2 + 9] = 0.0;
                    }
                }
            }
            else
            {
                pOutPlrs[x * 4 + 0] = 0.0;
                pOutPlrs[x * 4 + 1] = 0.0;
                pOutPlrs[x * 4 + 2] = 0.0;
                pOutPlrs[x * 4 + 3] = 0.0;
            }
        }

        __kernel void FeatureGatherFromXYZRT(
                __global int *pOutPlrs, __global float *pOutFeature, __global int *numOfPts,
                const float minXRange, const float minYRange, const float minZRange,
                const float pillarXSize, const float pillarYSize, const float pillarZSize,
                const int maxNumPlrs, const int maxNumPtsPerPlr, const int numOutFeatureDim,
                const int numOfPillar ) {
            int x = get_global_id( 0 );
            if ( x < numOfPillar )
            {
                numOfPts[x] = min( numOfPts[x], maxNumPtsPerPlr );
                int numPts = numOfPts[x];
                float meanX = 0.0;
                float meanY = 0.0;
                float meanZ = 0.0;
                for ( int i = 0; i < numPts; i++ )
                {
                    int id1 = x * maxNumPtsPerPlr * numOutFeatureDim + i * numOutFeatureDim;
                    meanX += pOutFeature[id1 + 0];
                    meanY += pOutFeature[id1 + 1];
                    meanZ += pOutFeature[id1 + 2];
                }
                meanX = meanX / numPts;
                meanY = meanY / numPts;
                meanZ = meanZ / numPts;
                float pillarX = minXRange + pOutPlrs[x * 2 + 0] * pillarXSize + 0.5 * pillarXSize;
                float pillarY = minYRange + pOutPlrs[x * 2 + 1] * pillarYSize + 0.5 * pillarYSize;
                for ( int j = 0; j < maxNumPtsPerPlr; j++ )
                {
                    int id2 = x * maxNumPtsPerPlr * numOutFeatureDim + j * numOutFeatureDim;
                    if ( j < numPts )
                    {
                        pOutFeature[id2 + 5] = pOutFeature[id2 + 0] - meanX;
                        pOutFeature[id2 + 6] = pOutFeature[id2 + 1] - meanY;
                        pOutFeature[id2 + 7] = pOutFeature[id2 + 2] - meanZ;
                        pOutFeature[id2 + 8] = pOutFeature[id2 + 0] - pillarX;
                        pOutFeature[id2 + 9] = pOutFeature[id2 + 1] - pillarY;
                    }
                    else
                    {
                        pOutFeature[id2 + 0] = 0.0;
                        pOutFeature[id2 + 1] = 0.0;
                        pOutFeature[id2 + 2] = 0.0;
                        pOutFeature[id2 + 3] = 0.0;
                        pOutFeature[id2 + 4] = 0.0;
                        pOutFeature[id2 + 5] = 0.0;
                        pOutFeature[id2 + 6] = 0.0;
                        pOutFeature[id2 + 7] = 0.0;
                        pOutFeature[id2 + 8] = 0.0;
                        pOutFeature[id2 + 9] = 0.0;
                    }
                }
            }
            else
            {
                pOutPlrs[x * 2 + 0] = 0;
                pOutPlrs[x * 2 + 1] = 0;
            }
        }

);

#endif   // RIDEHAL_VOXELIZATION_CLH