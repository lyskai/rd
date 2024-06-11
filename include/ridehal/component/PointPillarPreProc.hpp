// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_POINTPILLAR_PREPROC_HPP
#define RIDEHAL_POINTPILLAR_PREPROC_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasPlr.hpp"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::FadasIface;

namespace ridehal
{
namespace component
{

/** @brief PointPillarPreProc component configuration */
typedef struct
{
    RideHal_ProcessorType_e processor; /**< processor type */
    float pillarXSize;                 /**< Pillar size in x direction in meters. */
    float pillarYSize;                 /**< Pillar size in y direction in meters. */
    float pillarZSize;                 /**< Pillar size in z direction in meters. */
    float minXRange;                   /**< Minimum range value in x direction. */
    float minYRange;                   /**< Minimum range value in y direction. */
    float minZRange;                   /**< Minimum range value in z direction. */
    float maxXRange;                   /**< Maximum range value in x direction.  */
    float maxYRange;                   /**< Maximum range value in y direction. */
    float maxZRange;                   /**< Maximum range value in z direction. */
    uint32_t maxNumInPts;              /**< Maximum number of points in input point cloud. */
    uint32_t numInFeatureDim;  /**< Number of features for each point in the input point cloud data.
                                * For e.g., if point cloud data contains (x, y, z, r) features for
                                * each point, then numInFeatureDim is 4. */
    uint32_t maxNumPlrs;       /**< Maximum number of point pillars that can be created. */
    uint32_t maxNumPtsPerPlr;  /**< Maximum number of points to map to each pillar. */
    uint32_t numOutFeatureDim; /**< Number of features for each point in point pillars. */
} PointPillarPreProc_Config_t;

/**
 * @brief PointPillarPreProc
 * Component PointPillarPreProc that creates point pillars from point cloud data.
 */
class PointPillarPreProc : public ComponentIF
{

public:
    PointPillarPreProc();
    ~PointPillarPreProc();

    /**
     * @brief Initialize the point pillar pipeline
     * @param[in] pName the point pillar unique instance name
     * @param[in] pConfig the point pillar configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const PointPillarPreProc_Config_t *pConfig,
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
     * @cond PointPillarPreProc::Start @endcond
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
     * @param[in] pInPts The input point cloud where size in bytes
     *                 is maxNumInPts x 4 x sizeof(float32_t).
     * @param[out] pOutPlrs The output point pillars, where memory (in bytes)
     *                 for each pillar is maxNumPlrs x sizeof(float32_t)
     * @param[out] pOutFeature The output point pillar feature points where
     *                 memory size in bytes for all pillars is
     *                 maxNumPlrs * maxNumPtsPerPlr x numOutFeatureDim x sizeof(float32_t)
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInPts,
                            const RideHal_SharedBuffer_t *pOutPlrs,
                            const RideHal_SharedBuffer_t *pOutFeature );

private:
    PointPillarPreProc_Config_t m_config;

    FadasPlrPreProc m_plrPre;

};   // class PointPillarPreProc

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_POINTPILLAR_PREPROC_HPP
