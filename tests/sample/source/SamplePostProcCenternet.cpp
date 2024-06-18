// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SamplePostProcCenternet.hpp"
#include <algorithm>
#include <assert.h>

namespace ridehal
{
namespace sample
{

#define U2F( dname, index ) ( ( dname[index] + dname##Offset ) * dname##Scale )

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

static float sigmoid( float data )
{
    return 1. / ( 1. + fastExp( -data ) );
}

SamplePostProcCenternet::SamplePostProcCenternet() {}
SamplePostProcCenternet::~SamplePostProcCenternet() {}

RideHalError_e SamplePostProcCenternet::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_roiX = Get( config, "roi_x", 0 );
    m_roiY = Get( config, "roi_y", 0 );
    m_camWidth = Get( config, "width", 1920 );
    m_camHeight = Get( config, "height", 1024 );
    m_scoreThreshold = Get( config, "score_threshold", 0.6f );
    m_NMSThreshold = Get( config, "nms_threshold", 0.6f );

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

RideHalError_e SamplePostProcCenternet::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
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

RideHalError_e SamplePostProcCenternet::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = false;
    m_thread = std::thread( &SamplePostProcCenternet::ThreadMain, this );

    return ret;
}

void SamplePostProcCenternet::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t tensors;
        ret = m_sub.Receive( tensors );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            PROFILER_BEGIN();
            ProcessUint8( tensors );
            PROFILER_END();
        }
    }
}

void SamplePostProcCenternet::ProcessUint8( DataFrames_t &tensors )
{
    RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", tensors.FrameId( 0 ),
                   tensors.Timestamp( 0 ) );

    Road2DObjects_t objs;

    auto &hm_ = tensors.SharedBuffer( 0 );
    auto &wh_ = tensors.SharedBuffer( 1 );
    auto &offset_ = tensors.SharedBuffer( 2 );

    int H = (int) hm_.tensorProps.dims[1];
    int W = (int) hm_.tensorProps.dims[2];
    int classNum = (int) hm_.tensorProps.dims[3];

    uint8_t *hm = (uint8_t *) hm_.data();
    float hmScale = tensors.QuantScale( 0 );
    int32_t hmOffset = tensors.QuantOffset( 0 );
    uint8_t *wh = (uint8_t *) wh_.data();
    float whScale = tensors.QuantScale( 1 );
    int32_t whOffset = tensors.QuantOffset( 1 );
    uint8_t *reg = (uint8_t *) offset_.data();
    float regScale = tensors.QuantScale( 2 );
    int32_t regOffset = tensors.QuantOffset( 2 );
    const int kernel_size = 7;

    std::vector<int> class_ids;
    if ( 80 == classNum )
    {
        // coco centernet, only care road object
        // https://github.com/amikelive/coco-labels/blob/master/coco-labels-paper.txt
        class_ids = std::vector<int>{ 0, 2, 5, 7 };
    }
    else
    {
        class_ids.resize( classNum );
        for ( int cls = 0; cls < classNum; cls++ )
        {
            class_ids[cls] = classNum;
        }
    }

    for ( int y = 0; y < H; y++ )
    {
        for ( int x = 0; x < W; x++ )
        {
            for ( int cls : class_ids )
            {
                int idx = ( y * W + x ) * classNum + cls;
                float objProb = sigmoid( U2F( hm, idx ) );
                if ( objProb > m_scoreThreshold )
                {
                    int padding = ( kernel_size - 1 ) / 2;
                    int offset = -padding;
                    int l, m;
                    float max = -1;
                    int max_index = 0;
                    for ( l = 0; l < kernel_size; ++l )
                    {
                        for ( m = 0; m < kernel_size; ++m )
                        {
                            int cur_x = offset + l + x;
                            int cur_y = offset + m + y;
                            int cur_index = ( y * W + x ) * classNum + cls;
                            int valid = ( ( cur_x >= 0 ) && ( cur_x < W ) && ( cur_y >= 0 ) &&
                                          ( cur_y < H ) );
                            float val = ( valid != 0 ) ? sigmoid( U2F( hm, cur_index ) ) : -1;
                            max_index = ( val > max ) ? cur_index : max_index;
                            max = ( val > max ) ? val : max;
                        }
                    }

                    if ( idx == max_index )
                    {
                        int reg_index = ( y * W + x ) * 2;
                        float c_x, c_y;
                        Road2DObject_t det;
                        c_x = x + U2F( reg, reg_index );
                        c_y = y + U2F( reg, reg_index + 1 );
                        float topX = ( c_x - U2F( wh, reg_index ) / 2 ) / W;
                        float topY = ( c_y - U2F( wh, reg_index + 1 ) / 2 ) / H;
                        float bottomX = ( c_x + U2F( wh, reg_index ) / 2 ) / W;
                        float bottomY = ( c_y + U2F( wh, reg_index + 1 ) / 2 ) / H;
                        topX = ( ( topX > 0 ) ? topX * m_camWidth : 0 ) + m_roiX;
                        topY = ( ( topY > 0 ) ? topY * m_camHeight : 0 ) + m_roiY;
                        bottomX = ( ( bottomX < 1 ) ? bottomX : 0.99 ) * m_camWidth + m_roiX;
                        bottomY = ( ( bottomY < 1 ) ? bottomY : 0.99 ) * m_camHeight + m_roiY;
                        det.classId = cls;
                        det.prob = objProb;
                        if ( ( topX < bottomX ) && ( topY < bottomY ) )
                        {
                            det.points[0] = Point2D_t{ topX, topY };
                            det.points[1] = Point2D_t{ bottomX, topY };
                            det.points[2] = Point2D_t{ bottomX, bottomY };
                            det.points[3] = Point2D_t{ topX, bottomY };
                            objs.objs.push_back( det );
                            RIDEHAL_DEBUG( "[frame %" PRIu64 "- %" PRIu64
                                           "] class=%d score=%.3f points=[%.3f %.3f %.3f %.3f]",
                                           tensors.FrameId( 0 ), objs.objs.size() - 1, det.classId,
                                           det.prob, topX, topY, bottomX, bottomY );
                        }
                    }
                }
            }
        }
    }

    NMS( objs.objs, m_NMSThreshold );

    objs.frameId = tensors.FrameId( 0 );
    objs.timestamp = tensors.Timestamp( 0 );

    m_pub.Publish( objs );

    RIDEHAL_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, objs.objs.size(),
                   tensors.FrameId( 0 ) );
}

float SamplePostProcCenternet::ComputeIou( const Road2DObject_t &box1, const Road2DObject_t &box2 )
{
    float box1_xmin = box1.points[0].x, box2_xmin = box2.points[0].x;
    float box1_ymin = box1.points[0].y, box2_ymin = box2.points[0].y;
    float box1_xmax = box1.points[2].x, box2_xmax = box2.points[2].x;
    float box1_ymax = box1.points[2].y, box2_ymax = box2.points[2].y;
    float ixmin = std::max( box1_xmin, box2_xmin );
    float iymin = std::max( box1_ymin, box2_ymin );
    float ixmax = std::min( box1_xmax, box2_xmax );
    float iymax = std::min( box1_ymax, box2_ymax );

    assert( ( box1_xmin <= box1_xmax ) && ( box1_ymin <= box1_ymax ) &&
            ( box2_xmin <= box2_xmax ) && ( box2_ymin <= box2_ymax ) );

    float iou = 0.0f;
    if ( ( ixmin < ixmax ) && ( iymin < iymax ) )
    {
        float intersection_area = ( ixmax - ixmin ) * ( iymax - iymin );
        // union = area1 + area2 - intersection
        float union_area = ( box1_xmax - box1_xmin ) * ( box1_ymax - box1_ymin ) +
                           ( box2_xmax - box2_xmin ) * ( box2_ymax - box2_ymin ) -
                           intersection_area;
        iou = ( union_area > 0.0f ) ? intersection_area / union_area : 0.0f;
    }
    return iou;
}

void SamplePostProcCenternet::NMS( std::vector<Road2DObject_t> &boxes, float thres )
{
    std::sort( boxes.begin(), boxes.end(),
               []( const Road2DObject_t &a, const Road2DObject_t &b ) { return a.prob > b.prob; } );

    for ( auto it = boxes.begin(); it != boxes.end(); it++ )
    {
        Road2DObject_t &cand1 = *it;

        for ( auto jt = it + 1; jt != boxes.end(); )
        {
            Road2DObject_t &cand2 = *jt;
            if ( ComputeIou( cand1, cand2 ) >= thres )
                jt = boxes.erase( jt );   // Possible candidate for optimization
            else
                jt++;
        }
    }
}

RideHalError_e SamplePostProcCenternet::Stop()
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

RideHalError_e SamplePostProcCenternet::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    return ret;
}

REGISTER_SAMPLE( PostProcCenternet, SamplePostProcCenternet );

}   // namespace sample
}   // namespace ridehal
