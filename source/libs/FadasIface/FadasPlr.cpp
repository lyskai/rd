//  Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#include "FadasPlr.hpp"
#include <string.h>

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

FadasPlrPreProc::FadasPlrPreProc()
{
    m_plrHandler.hHandle = nullptr;
}

FadasPlrPreProc::~FadasPlrPreProc() {}

RideHalError_e FadasPlrPreProc::SetParams( float pillarXSize, float pillarYSize, float pillarZSize,
                                           float minXRange, float minYRange, float minZRange,
                                           float maxXRange, float maxYRange, float maxZRange,
                                           uint32_t maxNumPtsIn, uint32_t numInFeatureDim,
                                           uint32_t maxNumPlrs, uint32_t maxNumPtsPerPlr,
                                           uint32_t numOutFeatureDim )
{
    m_pillarXSize = pillarXSize;
    m_pillarYSize = pillarYSize;
    m_pillarZSize = pillarZSize;
    m_minXRange = minXRange;
    m_minYRange = minYRange;
    m_minZRange = minZRange;
    m_maxXRange = maxXRange;
    m_maxYRange = maxYRange;
    m_maxZRange = maxZRange;
    m_maxNumPtsIn = maxNumPtsIn;
    m_numInFeatureDim = numInFeatureDim;
    m_maxNumPlrs = maxNumPlrs;
    m_maxNumPtsPerPlr = maxNumPtsPerPlr;
    m_numOutFeatureDim = numOutFeatureDim;

    m_bParamSet = true;

    return RIDEHAL_ERROR_NONE;
}

RideHalError_e FadasPlrPreProc::CreatePreProcCPU()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    FadasPt_3Df32_t plrSize = { m_pillarXSize, m_pillarYSize, m_pillarZSize };
    FadasPt_3Df32_t minRange = { m_minXRange, m_minYRange, m_minZRange };
    FadasPt_3Df32_t maxRange = { m_maxXRange, m_maxYRange, m_maxZRange };

    m_plrHandler.hHandle = FadasVM_PointPillar_Create( plrSize, minRange, maxRange, m_maxNumPtsIn,
                                                       m_numInFeatureDim, m_maxNumPlrs,
                                                       m_maxNumPtsPerPlr, m_numOutFeatureDim );

    if ( nullptr == m_plrHandler.hHandle )
    {
        RIDEHAL_ERROR( "CPU Create PointPillar Fail!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::DestroyPreProcCPU()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == m_plrHandler.hHandle )
    {
        RIDEHAL_ERROR( "hHandle is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        FadasError_e error = FadasVM_PointPillar_Destroy( m_plrHandler.hHandle );
        if ( FADAS_ERROR_NONE != error )
        {
            RIDEHAL_ERROR( "CPU destroy pointpiller handle %p fail: %d!", m_plrHandler.hHandle,
                           error );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::CreatePreProcDSP()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    AEEResult result;

    FadasIface_Pt3D_t plrSize = { m_pillarXSize, m_pillarYSize, m_pillarZSize };
    FadasIface_Pt3D_t minRange = { m_minXRange, m_minYRange, m_minZRange };
    FadasIface_Pt3D_t maxRange = { m_maxXRange, m_maxYRange, m_maxZRange };

    result = FadasIface_PointPillarCreate(
            m_handle64, &plrSize, &minRange, &maxRange, m_maxNumPtsIn, m_numInFeatureDim,
            m_maxNumPlrs, m_maxNumPtsPerPlr, m_numOutFeatureDim, &m_plrHandler.handle64 );
    if ( AEE_SUCCESS != result )
    {
        RIDEHAL_ERROR( "DSP create pointpiller fail: 0x%x!", result );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::DestroyPreProcDSP()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    AEEResult result;

    if ( 0 == m_plrHandler.handle64 )
    {
        RIDEHAL_ERROR( "handle64 is 0!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        result = FadasIface_PointPillarDestroy( m_handle64, m_plrHandler.handle64 );
        if ( AEE_SUCCESS != result )
        {
            RIDEHAL_ERROR( "DSP destroy pointpiller fail: 0x%x!", result );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::CreatePreProc()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( false == m_bParamSet )
    {
        RIDEHAL_ERROR( "Parameter not set!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( ( RIDEHAL_PROCESSOR_HTP0 == m_processor ) ||
              ( RIDEHAL_PROCESSOR_HTP1 == m_processor ) )
    {
        m_handle64 = GetRemoteHandle64();
        ret = CreatePreProcDSP();
    }
    else
    {
        ret = CreatePreProcCPU();
    }


    return ret;
}

RideHalError_e FadasPlrPreProc::PointPillarRunCPU( const RideHal_SharedBuffer_t *pInPts,
                                                   const RideHal_SharedBuffer_t *pOutPlrs,
                                                   const RideHal_SharedBuffer_t *pOutFeature )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    FadasError_e error;
    uint32_t numOutPlrs = 0;
    uint32_t numPts = pInPts->tensorProps.dims[0];
    const float32_t *pInPtsData = (const float32_t *) pInPts->data();
    FadasVM_PointPillar_t *pOutPlrsData = (FadasVM_PointPillar_t *) pOutPlrs->data();
    float32_t *pOutFeatureData = (float32_t *) pOutFeature->data();
    int fdPts = -1;
    int fdOutPlrs = -1;
    int fdOutFeature = -1;

    fdPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        RIDEHAL_ERROR( "register pInPts buffer fail!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fdOutPlrs = RegBuf( pOutPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            RIDEHAL_ERROR( "register pOutPlrs buffer fail!" );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fdOutFeature = RegBuf( pOutFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            RIDEHAL_ERROR( "register pOutFeature buffer fail!" );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        error = FadasVM_PointPillar_Run( m_plrHandler.hHandle, numPts, pInPtsData, pOutPlrsData,
                                         pOutFeatureData, &numOutPlrs );
        if ( FADAS_ERROR_NONE != error )
        {
            RIDEHAL_ERROR( "CPU PointPillar Run fail: %d!", error );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::PointPillarRunDSP( const RideHal_SharedBuffer_t *pInPts,
                                                   const RideHal_SharedBuffer_t *pOutPlrs,
                                                   const RideHal_SharedBuffer_t *pOutFeature )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t numOutPlrs = 0;
    uint32_t numPts = pInPts->tensorProps.dims[0];
    int fdPts = -1;
    int fdOutPlrs = -1;
    int fdOutFeature = -1;

    fdPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        RIDEHAL_ERROR( "register pInPts buffer fail!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fdOutPlrs = RegBuf( pOutPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            RIDEHAL_ERROR( "register pOutPlrs buffer fail!" );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fdOutFeature = RegBuf( pOutFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            RIDEHAL_ERROR( "register pOutFeature buffer fail!" );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        AEEResult result = FadasIface_PointPillarRun(
                m_handle64, m_plrHandler.handle64, numPts, fdPts, pInPts->offset,
                numPts * m_numInFeatureDim * sizeof( float ), fdOutPlrs, pOutPlrs->offset,
                pOutPlrs->size, fdOutFeature, pOutFeature->offset, pOutFeature->size, &numOutPlrs );
        if ( AEE_SUCCESS != result )
        {
            RIDEHAL_ERROR( "DSP PointPillar Run fail: 0x%x!", result );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e FadasPlrPreProc::PointPillarRun( const RideHal_SharedBuffer_t *pInPts,
                                                const RideHal_SharedBuffer_t *pOutPlrs,
                                                const RideHal_SharedBuffer_t *pOutFeature )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_PROCESSOR_HTP0 == m_processor ) || ( RIDEHAL_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = PointPillarRunDSP( pInPts, pOutPlrs, pOutFeature );
    }
    else
    {
        ret = PointPillarRunCPU( pInPts, pOutPlrs, pOutFeature );
    }


    return ret;
}

RideHalError_e FadasPlrPreProc::DestroyPreProc()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_PROCESSOR_HTP0 == m_processor ) || ( RIDEHAL_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = DestroyPreProcDSP();
    }
    else
    {
        ret = DestroyPreProcCPU();
    }


    return ret;
}

}   // namespace FadasIface
}   // namespace libs
}   // namespace ridehal