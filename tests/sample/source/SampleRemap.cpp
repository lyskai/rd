// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleRemap.hpp"


namespace ridehal
{
namespace sample
{

static uint32_t s_rideHalFormatToBytesPerPixel[RIDE_HAL_IMAGE_FORMAT_MAX] = {
        3, /* RIDE_HAL_IMAGE_FORMAT_RGB888 */
        3, /* RIDE_HAL_IMAGE_FORMAT_BGR888 */
        2, /* RIDE_HAL_IMAGE_FORMAT_UYVY */
        1, /* RIDE_HAL_IMAGE_FORMAT_NV12 */
        1  /* RIDE_HAL_IMAGE_FORMAT_P010 */
};

SampleRemap::SampleRemap() {}
SampleRemap::~SampleRemap() {}

RideHalError_e SampleRemap::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    std::string processor = Get( config, "processor", "dsp0" );
    if ( "dsp0" == processor )
    {
        m_config.processor = REMAP_PROCESSOR_DSP0;
    }
    else if ( "dsp1" == processor )
    {
        m_config.processor = REMAP_PROCESSOR_DSP1;
    }
    else if ( "cpu" == processor )
    {
        m_config.processor = REMAP_PROCESSOR_CPU;
    }
    else
    {
        RIDEHAL_ERROR( "invalid processor %s\n", processor.c_str() );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputWidth = Get( config, "output_width", 0 );
    if ( 0 == m_config.outputWidth )
    {
        RIDEHAL_ERROR( "invalid output_width\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputHeight = Get( config, "output_height", 0 );
    if ( 0 == m_config.outputHeight )
    {
        RIDEHAL_ERROR( "invalid output_height\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputFormat = Get( config, "output_format", RIDE_HAL_IMAGE_FORMAT_MAX );
    if ( RIDE_HAL_IMAGE_FORMAT_MAX == m_config.outputFormat )
    {
        RIDEHAL_ERROR( "invalid output_format\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numOfInputs = Get( config, "batch_size", 0 );
    if ( 0 == m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "invalid batch_size\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputConfigs[i].inputWidth = Get( config, "input_width" + std::to_string( i ), 0 );
        if ( 0 == m_config.inputConfigs[i].inputWidth )
        {
            RIDEHAL_ERROR( "invalid input_width%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputHeight =
                Get( config, "input_height" + std::to_string( i ), 0 );
        if ( 0 == m_config.inputConfigs[i].inputHeight )
        {
            RIDEHAL_ERROR( "invalid input_height%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputFormat =
                Get( config, "input_format" + std::to_string( i ), RIDE_HAL_IMAGE_FORMAT_MAX );
        if ( RIDE_HAL_IMAGE_FORMAT_MAX == m_config.inputConfigs[i].inputFormat )
        {
            RIDEHAL_ERROR( "invalid input_format%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].mapWidth =
                Get( config, "map_width" + std::to_string( i ), m_config.outputWidth );
        if ( 0 == m_config.inputConfigs[i].mapWidth )
        {
            RIDEHAL_ERROR( "invalid map_width%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].mapHeight =
                Get( config, "map_height" + std::to_string( i ), m_config.outputHeight );
        if ( 0 == m_config.inputConfigs[i].mapHeight )
        {
            RIDEHAL_ERROR( "invalid map_height%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.x = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( 0 == m_config.inputConfigs[i].ROI.width )
        {
            RIDEHAL_ERROR( "invalid roi_x%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.y = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( 0 == m_config.inputConfigs[i].ROI.height )
        {
            RIDEHAL_ERROR( "invalid roi_y%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.width =
                Get( config, "roi_width" + std::to_string( i ), m_config.outputWidth );
        if ( 0 == m_config.inputConfigs[i].ROI.width )
        {
            RIDEHAL_ERROR( "invalid roi_width%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.height =
                Get( config, "roi_height" + std::to_string( i ), m_config.outputHeight );
        if ( 0 == m_config.inputConfigs[i].ROI.height )
        {
            RIDEHAL_ERROR( "invalid roi_height%u\n", i );
            ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
        }
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleRemap::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        RideHal_ImageProps_t imgProp;
        imgProp.format = m_config.outputFormat;
        imgProp.batchSize = m_config.numOfInputs;
        imgProp.width = m_config.outputWidth;
        imgProp.height = m_config.outputHeight;
        imgProp.stride[0] =
                m_config.outputWidth * s_rideHalFormatToBytesPerPixel[m_config.outputFormat];
        imgProp.actualHeight[0] = m_config.outputHeight;
        imgProp.numPlanes = 1;
        imgProp.extraPadding = 0;

        ret = m_imagePool.Init( name, GetLogger(), m_poolSize, imgProp, RIDE_HAL_BUFFER_USAGE_HTP );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        // TODO: call remap Init
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleRemap::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    m_stop = false;
    m_thread = std::thread( &SampleRemap::ThreadMain, this );
    return ret;
}

void SampleRemap::ThreadMain()
{
    int ret;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %llu, timestamp %llu\n", frames.frames[0].frameId,
                           frames.frames[0].timestamp );
        }
    }
}

RideHalError_e SampleRemap::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    return ret;
}

RideHalError_e SampleRemap::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    return ret;
}

}   // namespace sample
}   // namespace ridehal
