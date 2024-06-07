//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#ifndef QRIDE_FADAS_PLR_HPP
#define QRIDE_FADAS_PLR_HPP

#include "FadasSrv.hpp"
#include <vector>

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

class FadasPlrPreProc : public FadasSrv
{
public:
    FadasPlrPreProc();
    ~FadasPlrPreProc();

    RideHalError_e SetParams( float pillarXSize, float pillarYSize, float pillarZSize,
                              float minXRange, float minYRange, float minZRange, float maxXRange,
                              float maxYRange, float maxZRange, uint32_t maxNumPtsIn,
                              uint32_t numInFeatureDim, uint32_t maxNumPlrs,
                              uint32_t maxNumPtsPerPlr, uint32_t numOutFeatureDim );

    RideHalError_e CreatePreProc();
    RideHalError_e PointPillarRun( const RideHal_SharedBuffer_t *pInPts,
                                   const RideHal_SharedBuffer_t *pOutPlrs,
                                   const RideHal_SharedBuffer_t *pOutFeature );
    RideHalError_e DestroyPreProc();

private:
    RideHalError_e CreatePreProcCPU();
    RideHalError_e CreatePreProcDSP();

    RideHalError_e PointPillarRunCPU( const RideHal_SharedBuffer_t *pInPts,
                                      const RideHal_SharedBuffer_t *pOutPlrs,
                                      const RideHal_SharedBuffer_t *pOutFeature );
    RideHalError_e PointPillarRunDSP( const RideHal_SharedBuffer_t *pInPts,
                                      const RideHal_SharedBuffer_t *pOutPlrs,
                                      const RideHal_SharedBuffer_t *pOutFeature );

    RideHalError_e DestroyPreProcCPU();
    RideHalError_e DestroyPreProcDSP();

private:
    typedef union
    {
        void *hHandle;
        uint64_t handle64;
    } FadasPlrPreProc_PillarHandler_t;


private:
    remote_handle64 m_handle64;
    FadasPlrPreProc_PillarHandler_t m_plrHandler;

    /*  Parameters */
    float m_pillarXSize;
    float m_pillarYSize;
    float m_pillarZSize;
    float m_minXRange;
    float m_minYRange;
    float m_minZRange;
    float m_maxXRange;
    float m_maxYRange;
    float m_maxZRange;
    uint32_t m_maxNumPtsIn;
    uint32_t m_numInFeatureDim;
    uint32_t m_maxNumPlrs;
    uint32_t m_maxNumPtsPerPlr;
    uint32_t m_numOutFeatureDim;

    bool m_bParamSet = false;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace ridehal

#endif   // QRIDE_FADAS_PLR_HPP
