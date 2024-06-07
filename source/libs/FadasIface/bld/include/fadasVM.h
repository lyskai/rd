/***************************************************************************//**
@file
    fadasVM.h

@brief
    Voxel Map

@defgroup vm Voxel Map

@defgroup vm_h Voxel Map Helpers

@details
    Voxel map feature.

@internal
    Copyright (c) 2022-2023 Qualcomm Technologies, Inc.
    All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
*******************************************************************************/
#ifndef FADASVM_H
#define FADASVM_H

#include <stdint.h>
#include <fadas.h>

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************//**
@brief
    Structure to represent point pillars

@param centre
    Pillar co-ordinates in x-y plane

@param numPts
    Number of points in the pillar

****************************************************************************/
typedef struct
{
    FadasPt_3Df32_t centre;
    float32_t       numPts;
} FadasVM_PointPillar_t;

/************************************************************************//**
@brief
    RPN network output buffers.
    \n\b WARNING: All buffers must be 128-byte aligned.

@param pHeatmap
    Pointer to heatmap buffer.

@param pXY
    Pointer to buffer containing x,y co-ordinates of center point.

@param pZ
    Pointer to buffer containing z co-ordinate of center point.

@param pSize
    Pointer to buffer containing length, width, height for each detection.

@param pTheta
    Pointer to buffer containing orientation of each detection.

****************************************************************************/
typedef struct {
    float32_t     *pHeatmap;
    float32_t     *pXY;
    float32_t     *pZ;
    float32_t     *pSize;
    float32_t     *pTheta;
} Fadas3DRPNBufs_t;

/************************************************************************//**
@brief
    Structure to represent metadata corresponding to 3D bounding box.

@param numPts
    Number of 3D points located inside the bounding box.

@param meanPt
    3D point formed by computing the mean values of x, y and z coordinates of
    all points inside the bounding box.

@param meanIntensity
    Mean of intensities of all points inside the bounding box.

****************************************************************************/
typedef struct {
    FadasPt_3Df32_t meanPt;
    uint32_t        numPts;
    float32_t       meanIntensity;
} Fadas3DBBoxMetadata_t;

/************************************************************************//**
@brief
    Buffers to represent 3D bounding box.
    \n\b WARNING: All buffers must be 128-byte aligned.

@param pBBoxList
    Pointer to buffer containing 3D bounding box co-ordinates and dimensions.

@param pLabels
    Pointer to buffer containing labels corresponding to each bounding box.

@param pScores
    Pointer to buffer containing scores corresponding to each bounding box.

@param pMetadata
    Pointer to buffer containing metadata corresponding to each bounding box.

****************************************************************************/

typedef struct {
    FadasCuboidf32_t      *pBBoxList;
    uint32_t              *pLabels;
    float32_t             *pScores;
    Fadas3DBBoxMetadata_t *pMetadata;
} Fadas3DBBoxBufs_t;

/************************************************************************//**
@brief
    Initialization paramaters for BBox Filter.

@param maxNumFilter
    Maximum number of filtered boxes allowed after enabling BBox filter.

@param minCentre
    Min values of x, y, and z used for range check for centre of detection.

@param maxCentre
    Max values of x, y and z used for range check for centre of detection.

@param labelSelect
    Pointer to buffer providing selection/exclusion status of labels.
****************************************************************************/
typedef struct {
    uint32_t            maxNumFilter;
    FadasPt_3Df32_t     minCentre;
    FadasPt_3Df32_t     maxCentre;
    bool                *labelSelect;
} Fadas3DBBoxFilterParams_t;

/************************************************************************//**
@brief
    Initialization paramaters for 3D bounding box extraction.

@param strideInPts
    Input point cloud stride.

@param numClass
    Number of classes detected by the network.

@param grid
    Grid co-ordinates.

@param maxNumInPts
    Maximum number of points in the input point cloud data.

@param maxNumDetOut
    Maximum number of 3D bounding boxes expected in the output.

@param threshScore
    Confidence score threshold.

@param threshIOU
    Overlap threshold.

@param filterParams
    Filtering parameters

****************************************************************************/
typedef struct {
    uint32_t                    strideInPts;
    uint32_t                    numClass;
    Fadas2DGrid_t               grid;
    uint32_t                    maxNumInPts;
    uint32_t                    maxNumDetOut;
    float32_t                   threshScore;
    float32_t                   threshIOU;
    Fadas3DBBoxFilterParams_t   filterParams;
} Fadas3DBBoxInitParams_t;


/************************************************************************//**
@brief
    Initialize voxel map feature.
    \n\b WARNING: Must be called once before any other FastADAS functions
    except fadasVersion.

@param licenseKey
    License key string.

@return
    FADAS_ERROR_NONE if successful.

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_Init( const char *licenseKey );



/************************************************************************//**
@brief
    De-initialize voxel map feature.
    \n\b WARNING: Must be called once after all other FastADAS functions.

@return
    FADAS_ERROR_NONE if successful.

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_DeInit( void );


/************************************************************************//**
@brief
    Create and initialize point pillar creator object

@param plrSize
    Pillar size in x, y, z direction in meters.

@param minRange
    Minimum range value in x, y and z direction.

@param maxRange
    Maximum range value in x, y and z direction.

@param maxNumPtsIn
    Maximum number of points in input point cloud.

@param numInFeatureDim
    Number of features for each point in the input point cloud data.
    For e.g., if point cloud data contains (x, y, z, r) features for
    each point, then numInFeatureDim is 4.

@param maxNumPlrs
    Maximum number of point pillars that can be created.

@param maxNumPtsPerPlr
    Maximum number of points to map to each pillar.

@param numOutFeatureDim
    Number of features for each point in point pillars.

@return
    Handle to point pillar creator object

@ingroup vm
****************************************************************************/
FADAS_API
void* FadasVM_PointPillar_Create( FadasPt_3Df32_t   plrSize,
                                  FadasPt_3Df32_t   minRange,
                                  FadasPt_3Df32_t   maxRange,
                                  uint32_t          maxNumPtsIn,
                                  uint32_t          numInFeatureDim,
                                  uint32_t          maxNumPlrs,
                                  uint32_t          maxNumPtsPerPlr,
                                  uint32_t          numOutFeatureDim );


/************************************************************************//**
@brief
    Creates point pillars from point cloud data.

@param hPtPlr
    Handle to point pillar creator object.

@param numPts
    Number of points in point cloud data.

@param pInPts
    Pointer to input point cloud where size in bytes
    is maxNumPtsIn x 4 x sizeof(float32_t).
    \n\b WARNING: Must be 128-byte aligned.

@param pOutPlrs
    Pointer to output point pillars, where memory (in bytes)
    for each pillar is maxNumPlrs x sizeof(float32_t)
    \n\b WARNING: Must be 128-byte aligned.

@param pOutFeature
    Pointer to output point pillar feature points where
    memory size in bytes for all pillars is
    maxNumPlrs * maxNumPtsPerPlr x numOutFeatureDim x sizeof(float32_t)
    \n\b WARNING: Must be 128-byte aligned.

@param pNumOutPlrs
    Number of pillars created.

@return
    FADAS_ERROR_NONE -- Success

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_PointPillar_Run( void*                     hPtPlr,
                                      uint32_t                  numPts,
                                      const float32_t*          pInPts,
                                      FadasVM_PointPillar_t*    pOutPlrs,
                                      float32_t*                pOutFeature,
                                      uint32_t*                 pNumOutPlrs );


/************************************************************************//**
@brief
    Frees resources and releases point pillar creator object.

@param hPtPlr
    Handle to point pillar creator object

@return
    FADAS_ERROR_NONE -- Success

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_PointPillar_Destroy(void* hPtPlr);

/************************************************************************//**
@brief
    Create and initialize 3D bounding box extractor object.

@param pBBoxInitParams
    3D bounding box initialiazation parameters.

@return
    Handle to 3D bounding box extractor object.

@ingroup vm
****************************************************************************/
FADAS_API
void* FadasVM_ExtractBBox_Create( const Fadas3DBBoxInitParams_t *pBBoxInitParams );

/************************************************************************//**
@brief
    Extracts and filters bounding boxes from network output.

@param hExtractBBox
    Handle to 3D bounding box extractor object.

@param numPtsIn
    Number of points in input point cloud.

@param pInPtsBuf
    Input point cloud buffer.
    \n\b WARNING: Must be 128-byte aligned.

@param rpnBuf
    Network buffers from which 3D bounding boxes are to be extracted

@param outBuf
    Output buffers containing the 3D bounding boxes and associated metadata

@param pNumDetOut
    Number of 3D bounding boxes in the output.

@param bMapPtsToBBox
    Flag to enable/disable mapping of point cloud points to 3D bounding box.
    \n\b NOTE: The metadata buffer (pBBoxInitParams->outBuf.pMetadata) will
    be updated only if this flag is set to 'true'.

@param bBBoxFilter
    Flag to enable/disable filtering of 3D bounding boxes based on min/max ranges
    provided for centre of box, distance from LiDAR & specified class labels
    before mapping point cloud points to 3D bounding box.
    \n\b NOTE: This flag will be used only if bMapPtsToBBox is set to true.

@return
    FADAS_ERROR_NONE -- Success

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_ExtractBBox_Run( void              *hExtractBBox,
                                      uint32_t          numPtsIn,
                                      const float32_t   *pInPtsBuf,
                                      Fadas3DRPNBufs_t  rpnBuf,
                                      Fadas3DBBoxBufs_t outBuf,
                                      uint32_t          *pNumDetOut,
                                      bool              bMapPtsToBBox = false,
                                      bool              bBBoxFilter = false );

/************************************************************************//**
@brief
    Frees resources and releases 3D bounding box extractor object.

@param hExtractBBox
    Handle to 3D bounding box extractor object.

@return
    FADAS_ERROR_NONE -- Success

@ingroup vm
****************************************************************************/
FADAS_API
FadasError_e FadasVM_ExtractBBox_Destroy( void* hExtractBBox );

#ifdef __cplusplus
}
#endif

#endif /* FADASVM_H */
