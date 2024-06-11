// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_POINTPILLAR_POSTPROC_HPP
#define RIDEHAL_POINTPILLAR_POSTPROC_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasPlr.hpp"
#include "fadas.h"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::FadasIface;

namespace ridehal
{
namespace component
{
typedef struct
{
    uint32_t maxNumFilter; /**< Maximum number of filtered boxes allowed after enabling BBox
                            * filter. */
    float minCentreX;      /**< Min values of x used for range check for centre of detection. */
    float minCentreY;      /**< Min values of y used for range check for centre of detection. */
    float minCentreZ;      /**< Min values of z used for range check for centre of detection. */
    float maxCentreX;      /**< Max values of x used for range check for centre of detection. */
    float maxCentreY;      /**< Max values of y used for range check for centre of detection. */
    float maxCentreZ;      /**< Max values of z used for range check for centre of detection. */
    bool *labelSelect;
} PointPillarPostProc_3DBBoxFilterParams_t;

/** @brief PointPillarPostProc component configuration */
typedef struct
{
    RideHal_ProcessorType_e processor; /**< processor type */
    float pillarXSize;                 /**< Pillar size in x direction in meters. */
    float pillarYSize;                 /**< Pillar size in y direction in meters. */
    float minXRange;                   /**< Minimum range value in x direction. */
    float minYRange;                   /**< Minimum range value in y direction. */
    float maxXRange;                   /**< Maximum range value in x direction.  */
    float maxYRange;                   /**< Maximum range value in y direction. */
    uint32_t numClass;                 /**< Number of classes detected by the network. */
    uint32_t maxNumInPts;     /**< Maximum number of points in the input point cloud data. */
    uint32_t numInFeatureDim; /**< Number of features for each point in the input point cloud data.
                               * For e.g., if point cloud data contains (x, y, z, r) features for
                               * each point, then numInFeatureDim is 4. */
    uint32_t maxNumDetOut;    /**< Maximum number of 3D bounding boxes expected in the output. */

    uint32_t stride;

    float32_t threshScore; /**< Confidence score threshold. */
    float32_t threshIOU;   /**< Overlap threshold. */

    PointPillarPostProc_3DBBoxFilterParams_t filterParams; /* Filtering parameters */

    /**< Flag to enable/disable mapping of point cloud points to 3D bounding box. */
    bool bMapPtsToBBox;

    /**< Flag to enable/disable filtering of 3D bounding boxes based on min/max
     * ranges provided for centre of box, distance from LiDAR & specified
     * class labels before mapping point cloud points to 3D bounding box.
     * NOTE: This flag will be used only if bMapPtsToBBox is set to true. */
    bool bBBoxFilter;
} PointPillarPostProc_Config_t;

typedef struct
{
    int label;       /**< The object label ID */
    float score;     /**< The object confidence score */
    float x;         /**< The x co-ordinate of centre point */
    float y;         /**< The y co-ordinate of centre point */
    float z;         /**< The z co-ordinate of centre point */
    float length;    /**< The length of the 3D box */
    float width;     /**< The width of the 3D box */
    float height;    /**< The height of the 3D box */
    float theta;     /**< The orientation of the 3D box relative to the viewing angle */
    float meanPtX;   /**< The mean values of x coordinates of all points inside this bounding box */
    float meanPtY;   /**< The mean values of y coordinates of all points inside this bounding box */
    float meanPtZ;   /**< The mean values of z coordinates of all points inside this bounding box */
    uint32_t numPts; /**< Number of 3D points located inside this bounding box */
    float meanIntensity; /**< Mean of intensities of all points inside the bounding box */
} PointPillarPostProc_Object3D_t;

#define POINTPILLAR_OBJECT_3D_DIM ( sizeof( PointPillarPostProc_Object3D_t ) / sizeof( float ) )

/**
 * @brief PointPillarPostProc
 * Component PointPillarPostProc that creates point pillars from point cloud data.
 */
class PointPillarPostProc : public ComponentIF
{

public:
    PointPillarPostProc();
    ~PointPillarPostProc();

    /**
     * @brief Initialize the point pillar pipeline
     * @param[in] pName the point pillar unique instance name
     * @param[in] pConfig the point pillar configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const PointPillarPostProc_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for point pillar
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                                    FadasBufType_e bufferType );

    /**
     * @brief Deregister buffers for point pillar
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @cond PointPillarPostProc::Start @endcond
     * @brief Start the point pillar pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Stop the point pillar pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief deinitialize the point pillar pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @brief execute the point pillar pipeline
     * @param[in] pHeatmap Pointer to heatmap buffer
     * @param[in] pXY Pointer to buffer containing x,y co-ordinates of center point
     * @param[in] pZ Pointer to buffer containing z co-ordinate of center point
     * @param[in] pSize Pointer to buffer containing length, width, height for each detection
     * @param[in] pTheta Pointer to buffer containing orientation of each detection
     * @param[in] pInPts The input point cloud where size in bytes
     *             NOTE: pInPts is nullptr if bMapPtsToBBox is true.
     * @param[out] pDetections Pointer to buffer that represent 3D bounding box
     *        [N, PointPillarPostProc_Object3D_t]
     *                   [N, ( label,   score,   x,      y,       z,      length,
     *                         width,   height,  theta,  meanPtX, meanPtY, meanPtZ,
     *                         numPts,  meanIntensity )]
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pHeatmap,
                            const RideHal_SharedBuffer_t *pXY, const RideHal_SharedBuffer_t *pZ,
                            const RideHal_SharedBuffer_t *pSize,
                            const RideHal_SharedBuffer_t *pTheta,
                            const RideHal_SharedBuffer_t *pInPts,
                            RideHal_SharedBuffer_t *pDetections );

private:
    PointPillarPostProc_Config_t m_config;

    FadasPlrPostProc m_plrPost;

    uint32_t m_height;
    uint32_t m_width;

    RideHal_SharedBuffer_t m_BBoxList;
    RideHal_SharedBuffer_t m_labels;
    RideHal_SharedBuffer_t m_scores;
    RideHal_SharedBuffer_t m_metadata;

};   // class PointPillarPostProc

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_POINTPILLAR_POSTPROC_HPP
