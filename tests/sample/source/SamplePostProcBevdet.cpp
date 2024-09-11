// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


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

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceBboxDet = KernelCode( __kernel void BboxDet(
        __global uchar *hm, __global uchar *reg, __global uchar *height, __global uchar *dim,
        __global uchar *rot, __global uchar *vel, __global int *objClsIds,
        __global float *objDetObjs, uint clsNum, uint maxObjNum, uint inputH, uint inputW,
        float thresh, float hmScale, int hmOffset, float regScale, int regOffset, float heightScale,
        int heightOffset, float dimScale, int dimOffset, float rotScale, int rotOffset,
        float velScale, int velOffset, float outSizeFactor, float voxelSizeX, float voxelSizeY,
        float pCldRangeX, float pCldRangeY ) {
    int global_idx_c = get_global_id( 0 );
    int global_idx_y = get_global_id( 1 );
    int global_idx_x = get_global_id( 2 );

    int idx = ( global_idx_y * inputW + global_idx_x ) * clsNum + global_idx_c;
    float obj_hm = ( hm[idx] + hmOffset ) * hmScale;
    float obj_prob = native_recip( native_exp( -1.0 * obj_hm ) + 1.0 );

    if ( obj_prob > thresh )
    {
        uint h_index = ( global_idx_y * inputW + global_idx_x );
        uint reg_index = ( global_idx_y * inputW + global_idx_x ) * 2;
        uint dim_index = ( global_idx_y * inputW + global_idx_x ) * 3;

        float x = global_idx_x + ( reg[reg_index] + regOffset ) * regScale;
        float y = global_idx_y + ( reg[reg_index + 1] + regOffset ) * regScale;
        float z = ( height[h_index] + heightOffset ) * heightScale;

        float dx = native_exp( ( dim[dim_index + 1] + dimOffset ) * dimScale );
        float dy = native_exp( ( dim[dim_index] + dimOffset ) * dimScale );
        float dz = native_exp( ( dim[dim_index + 2] + dimOffset ) * dimScale );

        float sine = ( rot[reg_index] + rotOffset ) * rotScale;
        float cosine = ( rot[reg_index + 1] + rotOffset ) * rotScale;
        float yaw = atan2( sine, cosine );

        float vel0 = ( vel[reg_index] + velOffset ) * velScale;
        float vel1 = ( vel[reg_index + 1] + velOffset ) * velScale;

        x = x * outSizeFactor * voxelSizeX + pCldRangeX;
        y = y * outSizeFactor * voxelSizeY + pCldRangeY;
        z -= dz / 2.0;

        uint obj_idx = atomic_inc( &objClsIds[maxObjNum] );
        if ( obj_idx < maxObjNum )
        {
            uint item_idx = obj_idx * 10;
            objClsIds[obj_idx] = global_idx_c;
            objDetObjs[item_idx] = obj_prob;
            objDetObjs[item_idx + 1] = x;
            objDetObjs[item_idx + 2] = y;
            objDetObjs[item_idx + 3] = z;
            objDetObjs[item_idx + 4] = dx;
            objDetObjs[item_idx + 5] = dy;
            objDetObjs[item_idx + 6] = dz;
            objDetObjs[item_idx + 7] = yaw;
            objDetObjs[item_idx + 8] = vel0;
            objDetObjs[item_idx + 9] = vel1;
        }
    }
} );

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
    } v = { (uint32_t) ( ( 1 << 23 ) * ( clipp + 121.2740575f + 27.7280233f / ( 4.84252568f - z ) -
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

Point2D_t SamplePostProcBevdet::ProjectToImage( Point2D_t &pt, Point2D_t &center, float yaw )
{
    Point2D_t imgPt;
    float yaw_ = yaw - M_PI / 2;

    imgPt.x = cos( yaw_ ) * pt.x + sin( yaw_ ) * pt.y + center.x;
    imgPt.y = -sin( yaw_ ) * pt.x + cos( yaw_ ) * pt.y + center.y;

    imgPt.x = m_offsetX + m_ratioW * ( imgPt.x - m_minX );
    imgPt.y = m_offsetY + m_ratioH * ( m_maxY - imgPt.y );

    imgPt.x = std::round( imgPt.x );
    imgPt.y = std::round( imgPt.y );

    return imgPt;
}

RideHalError_e SamplePostProcBevdet::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_processor = Get( config, "processor", RIDEHAL_PROCESSOR_CPU );
    m_scoreThreshold = Get( config, "score_threshold", 0.49f );
    m_NMSThreshold = Get( config, "nms_threshold", 0.6f );
    m_outSizeFactor = Get( config, "out_size_factor", 8.0f );

    m_minX = Get( config, "min_x", -10.0f );
    m_maxY = Get( config, "max_y", 40.0f );
    m_offsetX = Get( config, "offset_x", 832.5f );
    m_offsetY = Get( config, "offset_y", 0.0f );
    m_ratioW = Get( config, "ratio_w", 12.75f );
    m_ratioH = Get( config, "ratio_h", 12.75f );

    if ( ( RIDEHAL_PROCESSOR_CPU != m_processor ) && ( RIDEHAL_PROCESSOR_GPU != m_processor ) )
    {
        RIDEHAL_ERROR( "invalid processor type" );
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

    // OpenCL Init
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( RIDEHAL_PROCESSOR_GPU == m_processor )
        {
            ret = m_OpenclSrvObj.Init( name.c_str(), LOGGER_LEVEL_ERROR );
            SetCLParams();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to initialize openclSrvObj" );
            }
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceBboxDet, "BboxDet" );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to load kernel source code" );
                }
            }
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = RegisterOutputBuffers();
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to create output buffer" );
                }
            }
        }
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
            if ( RIDEHAL_PROCESSOR_CPU == m_processor )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( tensors.FrameId( 0 ) );
                PostProcCPU( tensors );
                PROFILER_END();
                TRACE_END( tensors.FrameId( 0 ) );
            }
            else if ( RIDEHAL_PROCESSOR_GPU == m_processor )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( tensors.FrameId( 0 ) );
                ret = RegisterInputBuffers( tensors );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to create input buffer" );
                }
                ret = PostProcCL( tensors );
                PROFILER_END();
                TRACE_END( tensors.FrameId( 0 ) );
            }
            else
            {
                RIDEHAL_ERROR( "invalid processor type" );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }
}

RideHalError_e SamplePostProcBevdet::RegisterInputBuffers( DataFrames_t &tensors )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RideHal_SharedBuffer_t hm = tensors.SharedBuffer( m_indexs[0] );
    RideHal_SharedBuffer_t reg = tensors.SharedBuffer( m_indexs[1] );
    RideHal_SharedBuffer_t height = tensors.SharedBuffer( m_indexs[2] );
    RideHal_SharedBuffer_t dim = tensors.SharedBuffer( m_indexs[3] );
    RideHal_SharedBuffer_t rot = tensors.SharedBuffer( m_indexs[4] );
    RideHal_SharedBuffer_t vel = tensors.SharedBuffer( m_indexs[5] );

    m_height = hm.tensorProps.dims[1];
    m_width = hm.tensorProps.dims[2];
    m_classNum = hm.tensorProps.dims[3];
    m_hmScale = tensors.QuantScale( m_indexs[0] );
    m_hmOffset = tensors.QuantOffset( m_indexs[0] );
    m_regScale = tensors.QuantScale( m_indexs[1] );
    m_regOffset = tensors.QuantOffset( m_indexs[1] );
    m_heightScale = tensors.QuantScale( m_indexs[2] );
    m_heightOffset = tensors.QuantOffset( m_indexs[2] );
    m_dimScale = tensors.QuantScale( m_indexs[3] );
    m_dimOffset = tensors.QuantOffset( m_indexs[3] );
    m_rotScale = tensors.QuantScale( m_indexs[4] );
    m_rotOffset = tensors.QuantOffset( m_indexs[4] );
    m_velScale = tensors.QuantScale( m_indexs[5] );
    m_velOffset = tensors.QuantOffset( m_indexs[5] );

    // Create hm data buffer
    ret = m_OpenclSrvObj.RegBuf( hm.data(), hm.size, hm.buffer.dmaHandle, &m_clInputHMBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create hm data buffer" );
    }

    // Create reg data buffer
    ret = m_OpenclSrvObj.RegBuf( reg.data(), reg.size, reg.buffer.dmaHandle, &m_clInputRegBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create reg data buffer" );
    }

    // Create height data buffer
    ret = m_OpenclSrvObj.RegBuf( height.data(), height.size, height.buffer.dmaHandle,
                                 &m_clInputHeightBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create height data buffer" );
    }

    // Create dim data buffer
    ret = m_OpenclSrvObj.RegBuf( dim.data(), dim.size, dim.buffer.dmaHandle, &m_clInputDimBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create dim data buffer" );
    }

    // Create rot data buffer
    ret = m_OpenclSrvObj.RegBuf( rot.data(), rot.size, rot.buffer.dmaHandle, &m_clInputRotBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create rot data buffer" );
    }

    // Create vel data buffer
    ret = m_OpenclSrvObj.RegBuf( vel.data(), vel.size, vel.buffer.dmaHandle, &m_clInputVelBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create vel data buffer" );
    }

    return ret;
}

RideHalError_e SamplePostProcBevdet::RegisterOutputBuffers()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t outputClsIdBufSize =
            ( MAX_OBJ_NUM + 1 ) * sizeof( int );   // the last element is object num
    size_t outputDetObjBufSize = MAX_OBJ_NUM * 10 * sizeof( float );
    uint64_t bufHandle = 0;

    // create output classId data buffer
    ret = m_outputClsIdBuf.Allocate( outputClsIdBufSize );
    bufHandle = m_outputClsIdBuf.buffer.dmaHandle;
    ret = m_OpenclSrvObj.RegBuf( m_outputClsIdBuf.data(), outputClsIdBufSize, bufHandle,
                                 &m_clOutputClsIdBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create output classId data buffer" );
    }

    // create output detObj data buffer
    ret = m_outputDetObjBuf.Allocate( outputDetObjBufSize );
    bufHandle = m_outputDetObjBuf.buffer.dmaHandle;
    ret = m_OpenclSrvObj.RegBuf( m_outputDetObjBuf.data(), outputDetObjBufSize, bufHandle,
                                 &m_clOutputDetObjBuf );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to create output degObj data buffer" );
    }

    return ret;
}

void SamplePostProcBevdet::PostProcCPU( DataFrames_t &tensors )
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
        Road2DObject_t obj;
        obj.classId = selected[i].classId;
        obj.prob = selected[i].prob;
        Point2D_t center{ selected[i].x, selected[i].y };
        Point2D_t pt0{ -selected[i].dx / 2, selected[i].dy / 2 };
        Point2D_t pt1{ selected[i].dx / 2, selected[i].dy / 2 };
        Point2D_t pt2{ selected[i].dx / 2, -selected[i].dy / 2 };
        Point2D_t pt3{ -selected[i].dx / 2, -selected[i].dy / 2 };

        obj.points[0] = ProjectToImage( pt0, center, selected[i].yaw );
        obj.points[1] = ProjectToImage( pt1, center, selected[i].yaw );
        obj.points[2] = ProjectToImage( pt2, center, selected[i].yaw );
        obj.points[3] = ProjectToImage( pt3, center, selected[i].yaw );
        objs.objs.push_back( obj );
    }

    objs.frameId = tensors.FrameId( 0 );
    objs.timestamp = tensors.Timestamp( 0 );

    m_pub.Publish( objs );
    RIDEHAL_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, objs.objs.size(),
                   tensors.FrameId( 0 ) );
}

RideHalError_e SamplePostProcBevdet::PostProcCL( DataFrames_t &tensors )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numArgs = 30;
    OpenclIface_WorkParams_t openclWorkParams;
    openclWorkParams.workDim = 3;
    size_t globalWorkSize[3] = { m_classNum, m_height, m_width };
    size_t globalWorkOffset[3] = { 0, 0, 0 };
    openclWorkParams.pGlobalWorkSize = globalWorkSize;
    openclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    openclWorkParams.pLocalWorkSize = NULL;

    int *pdetClsIds = (int *) m_outputClsIdBuf.data();
    float *pdetObjs = (float *) m_outputDetObjBuf.data();
    *( pdetClsIds + MAX_OBJ_NUM ) = 0;

    ret = m_OpenclSrvObj.Execute( m_openclArgs, numArgs, &openclWorkParams );

    int objNum = *( pdetClsIds + MAX_OBJ_NUM );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute BboxDet kernel" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    // non-maximum suppression
    Object obj;
    Road2DObject_t det2DObj;
    std::vector<Object> detObjects;
    Road2DObjects_t det2DObjs;
    for ( uint32_t i = 0; i < objNum; i++ )
    {
        uint32_t idx = i * 10;
        obj.classId = *( pdetClsIds + i );
        obj.prob = *( pdetObjs + idx );
        obj.x = *( pdetObjs + idx + 1 );
        obj.y = *( pdetObjs + idx + 2 );
        obj.z = *( pdetObjs + idx + 3 );
        obj.dx = *( pdetObjs + idx + 4 );
        obj.dy = *( pdetObjs + idx + 5 );
        obj.dz = *( pdetObjs + idx + 6 );
        obj.yaw = *( pdetObjs + idx + 7 );
        obj.vel[0] = *( pdetObjs + idx + 8 );
        obj.vel[1] = *( pdetObjs + idx + 9 );

        detObjects.push_back( obj );
    }

    NMS( detObjects, m_NMSThreshold );
    for ( int i = 0; i < objNum; ++i )
    {

        det2DObj.classId = detObjects[i].classId;
        det2DObj.prob = detObjects[i].prob;
        Point2D_t center{ detObjects[i].x, detObjects[i].y };
        Point2D_t pt0{ -detObjects[i].dx / 2, detObjects[i].dy / 2 };
        Point2D_t pt1{ detObjects[i].dx / 2, detObjects[i].dy / 2 };
        Point2D_t pt2{ detObjects[i].dx / 2, -detObjects[i].dy / 2 };
        Point2D_t pt3{ -detObjects[i].dx / 2, -detObjects[i].dy / 2 };

        det2DObj.points[0] = ProjectToImage( pt0, center, detObjects[i].yaw );
        det2DObj.points[1] = ProjectToImage( pt1, center, detObjects[i].yaw );
        det2DObj.points[2] = ProjectToImage( pt2, center, detObjects[i].yaw );
        det2DObj.points[3] = ProjectToImage( pt3, center, detObjects[i].yaw );
        det2DObjs.objs.push_back( det2DObj );
    }

    det2DObjs.frameId = tensors.FrameId( 0 );
    det2DObjs.timestamp = tensors.Timestamp( 0 );

    m_pub.Publish( det2DObjs );
    RIDEHAL_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, det2DObjs.objs.size(),
                   tensors.FrameId( 0 ) );

    return ret;
}

void SamplePostProcBevdet::SetCLParams()
{
    m_openclArgs[0].pArg = (void *) &m_clInputHMBuf;
    m_openclArgs[0].argSize = sizeof( cl_mem );
    m_openclArgs[1].pArg = (void *) &m_clInputRegBuf;
    m_openclArgs[1].argSize = sizeof( cl_mem );
    m_openclArgs[2].pArg = (void *) &m_clInputHeightBuf;
    m_openclArgs[2].argSize = sizeof( cl_mem );
    m_openclArgs[3].pArg = (void *) &m_clInputDimBuf;
    m_openclArgs[3].argSize = sizeof( cl_mem );
    m_openclArgs[4].pArg = (void *) &m_clInputRotBuf;
    m_openclArgs[4].argSize = sizeof( cl_mem );
    m_openclArgs[5].pArg = (void *) &m_clInputVelBuf;
    m_openclArgs[5].argSize = sizeof( cl_mem );
    m_openclArgs[6].pArg = (void *) &m_clOutputClsIdBuf;
    m_openclArgs[6].argSize = sizeof( cl_mem );
    m_openclArgs[7].pArg = (void *) &m_clOutputDetObjBuf;
    m_openclArgs[7].argSize = sizeof( cl_mem );
    m_openclArgs[8].pArg = (void *) &m_classNum;
    m_openclArgs[8].argSize = sizeof( cl_uint );
    m_openclArgs[9].pArg = (void *) &m_maxObjNum;
    m_openclArgs[9].argSize = sizeof( cl_uint );
    m_openclArgs[10].pArg = (void *) &m_height;
    m_openclArgs[10].argSize = sizeof( cl_uint );
    m_openclArgs[11].pArg = (void *) &m_width;
    m_openclArgs[11].argSize = sizeof( cl_uint );
    m_openclArgs[12].pArg = (void *) &m_scoreThreshold;
    m_openclArgs[12].argSize = sizeof( cl_float );
    m_openclArgs[13].pArg = (void *) &m_hmScale;
    m_openclArgs[13].argSize = sizeof( cl_float );
    m_openclArgs[14].pArg = (void *) &m_hmOffset;
    m_openclArgs[14].argSize = sizeof( cl_int );
    m_openclArgs[15].pArg = (void *) &m_regScale;
    m_openclArgs[15].argSize = sizeof( cl_float );
    m_openclArgs[16].pArg = (void *) &m_regOffset;
    m_openclArgs[16].argSize = sizeof( cl_int );
    m_openclArgs[17].pArg = (void *) &m_heightScale;
    m_openclArgs[17].argSize = sizeof( cl_float );
    m_openclArgs[18].pArg = (void *) &m_heightOffset;
    m_openclArgs[18].argSize = sizeof( cl_int );
    m_openclArgs[19].pArg = (void *) &m_dimScale;
    m_openclArgs[19].argSize = sizeof( cl_float );
    m_openclArgs[20].pArg = (void *) &m_dimOffset;
    m_openclArgs[20].argSize = sizeof( cl_int );
    m_openclArgs[21].pArg = (void *) &m_rotScale;
    m_openclArgs[21].argSize = sizeof( cl_float );
    m_openclArgs[22].pArg = (void *) &m_rotOffset;
    m_openclArgs[22].argSize = sizeof( cl_int );
    m_openclArgs[23].pArg = (void *) &m_velScale;
    m_openclArgs[23].argSize = sizeof( cl_float );
    m_openclArgs[24].pArg = (void *) &m_velOffset;
    m_openclArgs[24].argSize = sizeof( cl_int );
    m_openclArgs[25].pArg = (void *) &m_outSizeFactor;
    m_openclArgs[25].argSize = sizeof( cl_float );
    m_openclArgs[26].pArg = (void *) &m_voxelSize[0];
    m_openclArgs[26].argSize = sizeof( cl_float );
    m_openclArgs[27].pArg = (void *) &m_voxelSize[1];
    m_openclArgs[27].argSize = sizeof( cl_float );
    m_openclArgs[28].pArg = (void *) &m_pointCloudRange[0];
    m_openclArgs[28].argSize = sizeof( cl_float );
    m_openclArgs[29].pArg = (void *) &m_pointCloudRange[1];
    m_openclArgs[29].argSize = sizeof( cl_float );
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

    if ( RIDEHAL_PROCESSOR_GPU == m_processor )
    {
        ret = m_OpenclSrvObj.Deinit();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Release CL resources failed!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

REGISTER_SAMPLE( PostProcBevdet, SamplePostProcBevdet );

}   // namespace sample
}   // namespace ridehal

