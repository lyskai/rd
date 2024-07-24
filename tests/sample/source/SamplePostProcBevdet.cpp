// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SamplePostProcBevdet.hpp"
#include "ridehal/sample/box_iou_rotated_utils.h"

#include <algorithm>
#include <assert.h>
#include <cmath>

namespace ridehal
{
namespace sample
{

#define U2F( dname, index ) ( ( dname[index] + dname##_offset ) * dname##_scale )

static float fastPow( float p )
{
    float offset = ( p < 0 ) ? 1.0f : 0.0f;
    float clipp = ( p < -126 ) ? -126.0f : p;
    int32_t w = (int32_t) clipp;
    float z = clipp - w + offset;
    union
    {
        uint32_t i;
        float f;
    } v = { ( uint32_t )( ( 1 << 23 ) * ( clipp + 121.2740575f + 27.7280233f / ( 4.84252568f - z ) -
                                          1.49012907f * z ) ) };

    return v.f;
}

static float fastExp( float p )
{
    return fastPow( 1.442695040f * p );
}

static float fastSigmoid( float x )
{
    return 0.5f * ( x / ( 1.0f + std::abs( x ) ) + 1.0f );
}

SamplePostProcBevdet::SamplePostProcBevdet() {}
SamplePostProcBevdet::~SamplePostProcBevdet() {}

RideHalError_e SamplePostProcBevdet::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_scoreThreshold = Get( config, "score_threshold", 0.49f );
    m_NMSThreshold = Get( config, "nms_threshold", 0.6f );
    m_outSizeFactor = Get( config, "out_size_factor", 8.0f );

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

    m_voxelSize = Get( config, "voxel_size", std::vector<float>{ 0.1f, 0.1f, 0.2f } );
    if ( 3 != m_voxelSize.size() )
    {
        RIDEHAL_ERROR( "Voxel size must be 3!\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_pointCloudRange = Get( config, "pointcloud_range",
                             std::vector<float>{ -51.2f, -51.2f, -5.0f, 51.2f, 51.2f, 3.0f } );
    if ( 6 != m_pointCloudRange.size() )
    {
        RIDEHAL_ERROR( "Pointcloud range must be 6!\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_indexs = Get( config, "output_indexs", m_indexs );
    if ( 6 != m_pointCloudRange.size() )
    {
        RIDEHAL_ERROR( "Output index must be 6!\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SamplePostProcBevdet::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( CPU );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SamplePostProcBevdet::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = false;
    m_thread = std::thread( &SamplePostProcBevdet::ThreadMain, this );

    return ret;
}

void SamplePostProcBevdet::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t tensors;
        ret = m_sub.Receive( tensors );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            PROFILER_BEGIN();
            TRACE_BEGIN( tensors.FrameId( 0 ) );
            ProcessUint8( tensors );
            PROFILER_END();
            TRACE_END( tensors.FrameId( 0 ) );
        }
    }
}

void SamplePostProcBevdet::ProcessUint8( DataFrames_t &tensors )
{
    RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", tensors.FrameId( 0 ),
                   tensors.Timestamp( 0 ) );

    std::vector<Object> selected;
    Road2DObjects_t objs;

    const auto &hm_ = tensors.SharedBuffer( m_indexs[0] );
    const auto &reg_ = tensors.SharedBuffer( m_indexs[1] );
    const auto &height_ = tensors.SharedBuffer( m_indexs[2] );
    const auto &dim_ = tensors.SharedBuffer( m_indexs[3] );
    const auto &rot_ = tensors.SharedBuffer( m_indexs[4] );
    const auto &vel_ = tensors.SharedBuffer( m_indexs[5] );

    int H = (int) hm_.tensorProps.dims[1];
    int W = (int) hm_.tensorProps.dims[2];
    int class_num = (int) hm_.tensorProps.dims[3];

    uint8_t *hm = (uint8_t *) hm_.data();
    float hm_scale = tensors.QuantScale( m_indexs[0] );
    int32_t hm_offset = tensors.QuantOffset( m_indexs[0] );
    uint8_t *reg = (uint8_t *) reg_.data();
    float reg_scale = tensors.QuantScale( m_indexs[1] );
    int32_t reg_offset = tensors.QuantOffset( m_indexs[1] );
    uint8_t *height = (uint8_t *) height_.data();
    float height_scale = tensors.QuantScale( m_indexs[2] );
    int32_t height_offset = tensors.QuantOffset( m_indexs[2] );
    uint8_t *dim = (uint8_t *) dim_.data();
    float dim_scale = tensors.QuantScale( m_indexs[3] );
    int32_t dim_offset = tensors.QuantOffset( m_indexs[3] );
    uint8_t *rot = (uint8_t *) rot_.data();
    float rot_scale = tensors.QuantScale( m_indexs[4] );
    int32_t rot_offset = tensors.QuantOffset( m_indexs[4] );
    uint8_t *vel = (uint8_t *) vel_.data();
    float vel_scale = tensors.QuantScale( m_indexs[5] );
    int32_t vel_offset = tensors.QuantOffset( m_indexs[5] );

    for ( int y = 0; y < H; y++ )
    {
        for ( int x = 0; x < W; x++ )
        {
            for ( int cls = 0; cls < class_num; cls++ )
            {
                int idx = ( y * W + x ) * class_num + cls;
                float objProb = fastSigmoid( U2F( hm, idx ) );
                if ( objProb > m_scoreThreshold )
                {
                    int reg_index = ( y * W + x ) * 2;
                    int dim_index = ( y * W + x ) * 3;
                    int hei_index = ( y * W + x ) * 1;

                    Object det;
                    det.dz = fastExp( U2F( dim, dim_index + 2 ) );
                    det.z = U2F( height, hei_index );
                    float sine = U2F( rot, reg_index );
                    float consine = U2F( rot, reg_index + 1 );

                    det.dy = fastExp( U2F( dim, dim_index ) );
                    det.dx = fastExp( U2F( dim, dim_index + 1 ) );
                    det.x = x + U2F( reg, reg_index );
                    det.y = y + U2F( reg, reg_index + 1 );
                    det.z = det.z - det.dz / 2;
                    det.yaw = std::atan2( sine, consine );

                    det.x = det.x * m_outSizeFactor * m_voxelSize[0] + m_pointCloudRange[0];
                    det.y = det.y * m_outSizeFactor * m_voxelSize[1] + m_pointCloudRange[1];
                    det.classId = cls;
                    det.prob = objProb;
                    det.vel[0] = U2F( vel, reg_index );
                    det.vel[1] = U2F( vel, reg_index + 1 );
                    selected.push_back( det );
                }
            }
        }
    }
    NMS( selected, m_NMSThreshold );
    for ( int i = 0; i < selected.size(); ++i )
    {
        RIDEHAL_DEBUG( "obj %d, class: %d, prob: %f, x: %f, y: %f, yaw: %f, z: %f, dx: %f, dy: %f, "
                       "dz: %f,",
                       i, selected[i].classId, selected[i].prob, selected[i].x, selected[i].y,
                       selected[i].yaw, selected[i].dx, selected[i].dy, selected[i].dz );
    }

    // objs.frameId = tensors.FrameId( 0 );
    // objs.timestamp = tensors.Timestamp( 0 );

    // m_pub.Publish( objs );
    RIDEHAL_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, objs.objs.size(),
                   tensors.FrameId( 0 ) );
}

float SamplePostProcBevdet::ComputeIou( const Object &box1, const Object &box2 )
{
    float box1_raw[5] = { box1.x, box1.y, box1.dx, box1.dy, box1.yaw };
    float box2_raw[5] = { box2.x, box2.y, box2.dx, box2.dy, box2.yaw };

    return detectron2::single_box_iou_rotated( box1_raw, box2_raw );
}

void SamplePostProcBevdet::NMS( std::vector<Object> &boxes, float thres )
{
    std::sort( boxes.begin(), boxes.end(),
               []( const Object &a, const Object &b ) { return a.prob > b.prob; } );

    for ( auto it = boxes.begin(); it != boxes.end(); it++ )
    {
        Object &cand1 = *it;

        for ( auto jt = it + 1; jt != boxes.end(); )
        {
            Object &cand2 = *jt;
            if ( ComputeIou( cand1, cand2 ) >= thres )
                jt = boxes.erase( jt );   // Possible candidate for optimization
            else
                jt++;
        }
    }
}

RideHalError_e SamplePostProcBevdet::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SamplePostProcBevdet::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    return ret;
}

REGISTER_SAMPLE( PostProcBevdet, SamplePostProcBevdet );

}   // namespace sample
}   // namespace ridehal
