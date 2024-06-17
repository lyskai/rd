// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SamplePlrPost.hpp"


namespace ridehal
{
namespace sample
{

SamplePlrPost::SamplePlrPost() {}
SamplePlrPost::~SamplePlrPost() {}

RideHalError_e SamplePlrPost::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_config.processor = Get( config, "processor", RIDEHAL_PROCESSOR_HTP0 );
    if ( RIDEHAL_PROCESSOR_MAX == m_config.processor )
    {
        RIDEHAL_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.pillarXSize = Get( config, "pillar_size_x", 0.16f );
    m_config.pillarYSize = Get( config, "pillar_size_y", 0.16f );
    m_config.minXRange = Get( config, "min_x", 0.0f );
    m_config.minYRange = Get( config, "min_y", -39.68f );
    m_config.maxXRange = Get( config, "max_x", 69.12f );
    m_config.maxYRange = Get( config, "max_y", 39.68f );
    m_config.numClass = Get( config, "num_class", 3 );
    m_config.maxNumInPts = Get( config, "max_points", 300000 );
    m_config.numInFeatureDim = Get( config, "in_feature_dim", 4 );
    m_config.maxNumDetOut = Get( config, "max_det_out", 500 );
    m_config.stride = Get( config, "stride", 2 );
    m_config.threshScore = Get( config, "thresh_score", 0.4f );
    m_config.threshIOU = Get( config, "thresh_iou", 0.4f );
    m_config.bMapPtsToBBox = Get( config, "map_points_to_bbox", false );
    m_config.bBBoxFilter = false;

    m_offsetX = Get( config, "offset_x", 514 );
    m_offsetY = Get( config, "offset_y", 0 );
    m_ratioW = Get( config, "ratio_w", 12.903225806451614f );
    m_ratioH = Get( config, "ratio_h", 12.903225806451614f );

    m_bDebug = Get( config, "debug", false );

    m_Indexs = Get( config, "output_indexs", m_Indexs );
    RIDEHAL_INFO( "output indexs = [%u %u %u %u %u]", m_Indexs[0], m_Indexs[1], m_Indexs[2],
                  m_Indexs[3], m_Indexs[4] );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputLidarTopicName = Get( config, "input_lidar_topic", "" );
    if ( "" == m_inputLidarTopicName )
    {
        RIDEHAL_ERROR( "no input lidar topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SamplePlrPost::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t detTsProp = {
                RIDEHAL_TENSOR_TYPE_FLOAT_32,
                { m_config.maxNumDetOut, POINTPILLAR_OBJECT_3D_DIM },
                2,
        };

        ret = m_objsPool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, detTsProp,
                               RIDEHAL_BUFFER_USAGE_HTP );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SampleIF::Init( m_config.processor );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_plrPost.Init( name.c_str(), &m_config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_lidarSub.Init( name, m_inputLidarTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_infSub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SamplePlrPost::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_plrPost.Start();
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SamplePlrPost::ThreadMain, this );
    }

    return ret;
}

void SamplePlrPost::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t lidarFrames;
        ret = m_lidarSub.Receive( lidarFrames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive lidar frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           lidarFrames.FrameId( 0 ), lidarFrames.Timestamp( 0 ) );
            DataFrames_t infFrames;
            /* lidar frame inference generally in 10 fps */
            ret = m_infSub.Receive( infFrames, 500 );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                RIDEHAL_DEBUG( "receive inference frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                               infFrames.FrameId( 0 ), infFrames.Timestamp( 0 ) );
                std::shared_ptr<SharedBuffer_t> detOut = m_objsPool.Get();
                if ( nullptr != detOut )
                {
                    RideHal_SharedBuffer_t &inPts = lidarFrames.SharedBuffer( 0 );
                    RideHal_SharedBuffer_t &heatmap = infFrames.SharedBuffer( m_Indexs[0] );
                    RideHal_SharedBuffer_t &xy = infFrames.SharedBuffer( m_Indexs[1] );
                    RideHal_SharedBuffer_t &z = infFrames.SharedBuffer( m_Indexs[2] );
                    RideHal_SharedBuffer_t &size = infFrames.SharedBuffer( m_Indexs[3] );
                    RideHal_SharedBuffer_t &theta = infFrames.SharedBuffer( m_Indexs[4] );

                    ret = SampleIF::Lock();
                    if ( RIDEHAL_ERROR_NONE == ret )
                    {
                        PROFILER_BEGIN();
                        detOut->sharedBuffer.tensorProps.dims[0] = m_config.maxNumDetOut;
                        ret = m_plrPost.Execute( &heatmap, &xy, &z, &size, &theta, &inPts,
                                                 &detOut->sharedBuffer );
                        if ( RIDEHAL_ERROR_NONE == ret )
                        {
                            PROFILER_END();
                            PointPillarPostProc_Object3D_t *pObj =
                                    (PointPillarPostProc_Object3D_t *) detOut->sharedBuffer.data();
                            if ( m_bDebug )
                            {
                                printf( "lidar frameId %" PRIu64 ", number of detections %" PRIu32
                                        "\n",
                                        lidarFrames.FrameId( 0 ),
                                        detOut->sharedBuffer.tensorProps.dims[0] );
                            }
                            for ( uint32_t i = 0; i < detOut->sharedBuffer.tensorProps.dims[0];
                                  i++ )
                            {
                                // TODO: generate 2D bbox and publist to TinyViz
                                if ( m_bDebug )
                                {
                                    printf( "  [%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, "
                                            "%d],\n",
                                            pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                                            pObj->height, pObj->theta, pObj->score, pObj->label );
                                }
                                pObj++;
                            }
                        }
                        else
                        {
                            RIDEHAL_ERROR( "Extract BBox failed for %" PRIu64 " : %d",
                                           infFrames.FrameId( 0 ), ret );
                        }
                        (void) SampleIF::Unlock();
                    }
                }
            }
        }
    }
}

RideHalError_e SamplePlrPost::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    ret = m_plrPost.Stop();

    return ret;
}

RideHalError_e SamplePlrPost::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_plrPost.Deinit();

    return ret;
}

REGISTER_SAMPLE( PlrPost, SamplePlrPost );

}   // namespace sample
}   // namespace ridehal
