// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#ifndef RIDEHAL_VOXELIZATION_HPP
#define RIDEHAL_VOXELIZATION_HPP

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

#define VOXELIZATION_PILLAR_COORDS_DIM ( sizeof( FadasVM_PointPillar_t ) / sizeof( float ) )

/** @brief Voxelization component configuration */
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
} Voxelization_Config_t;

/**
 * @brief Voxelization
 * Component Voxelization that creates point pillars from point cloud data.
 */
class Voxelization : public ComponentIF
{

public:
    Voxelization();
    ~Voxelization();

    /**
     * @brief Initialize the voxelization pipeline
     * @param[in] pName the voxelization unique instance name
     * @param[in] pConfig the voxelization configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const Voxelization_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for voxelization
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @note It is recommended to call this API to register all the input/output buffers to
     * the voxelization during the initialization phase. But for some reasons, the input buffers
     * maybe not known during the initialization, so it's also OK to not do this, the Execute API
     * will help to do the register only once in case the buffer is not registered before.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                                    FadasBufType_e bufferType );

    /**
     * @brief Deregister buffers for voxelization
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Stop the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deinitialize the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @brief Execute the voxelization pipeline
     * @param[in] pInPts The input point cloud where size in bytes
     *                 is maxNumInPts x 4 x sizeof(float32_t).
     * @param[out] pOutPlrs The output pillar index tensor where memory (in bytes)
     *                 for each pillar is maxNumPlrs x 4 x sizeof(float32_t)
     * @param[out] pOutFeature The output stacked pillar tensor where
     *                 memory size in bytes for all pillars is
     *                 maxNumPlrs * maxNumPtsPerPlr x numOutFeatureDim x sizeof(float32_t)
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInPts,
                            const RideHal_SharedBuffer_t *pOutPlrs,
                            const RideHal_SharedBuffer_t *pOutFeature );

private:
    Voxelization_Config_t m_config;

    FadasPlrPreProc m_plrPre;

};   // class Voxelization

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_VOXELIZATION_HPP
