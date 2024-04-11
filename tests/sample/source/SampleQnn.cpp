// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.


#include "ridehal/sample/SampleQnn.hpp"


namespace ridehal
{
namespace sample
{

SampleQnn::SampleQnn() {}
SampleQnn::~SampleQnn() {}

RideHalError_e SampleQnn::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_config.modelPath = Get( config, "model_path", "" );
    if ( "" == m_config.modelPath )
    {
        RIDEHAL_ERROR( "invalid modelPath\n" );
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.backendId = QNNRUNTIME_BACKEND_HTP;
    m_config.backendCoreId = Get( config, "core", 0 );

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

RideHalError_e SampleQnn::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = SampleIF::Init( (RideHal_ProcessorType_e) m_config.backendCoreId );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_qnn.Init( name.c_str(), &m_config );
    }

    uint32_t inputNum = 0;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_qnn.GetInputInfos( nullptr, &inputNum );
    }
    printf( "inputNum: %d\n", inputNum );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_inputInfos.resize( inputNum );
        ret = m_qnn.GetInputInfos( &m_inputInfos[0], &inputNum );
    }


    uint32_t outputNum = 0;
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_qnn.GetOutputInfos( nullptr, &outputNum );
    }
    printf( "outputNum: %d\n", outputNum );

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_outputInfos.resize( outputNum );
        ret = m_qnn.GetOutputInfos( &m_outputInfos[0], &outputNum );
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_tensorPools.resize( outputNum );

        size_t index = 0;
        for ( int i = 0; i < outputNum; ++i )
        {
            printf( "output i= %d\n", i );
            printf( "numDims: %d\n", m_outputInfos[i].properties.numDims );
            printf( "type: %d\n", m_outputInfos[i].properties.type );
            for ( int ii = 0; ii < m_outputInfos[i].properties.numDims; ++ii )
            {
                printf( "dims[ii=: %d], dims= %d\n", ii, m_outputInfos[i].properties.dims[ii] );
            }

            ret = m_tensorPools[index].Init(
                    "Qnn." + name + "." + std::to_string( index ), LOGGER_LEVEL_INFO, m_poolSize,
                    m_outputInfos[i].properties, RIDE_HAL_BUFFER_USAGE_HTP );
            index += 1;
            if ( RIDE_HAL_ERROR_NONE != ret )
            {
                break;
            }
        }
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

RideHalError_e SampleQnn::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_qnn.Start();
    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleQnn::ThreadMain, this );
    }

    return ret;
}

void SampleQnn::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        CamFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDE_HAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.frames[0].frameId, frames.frames[0].timestamp );
            std::vector<RideHal_SharedBuffer_t> inputs;
            std::vector<RideHal_SharedBuffer_t> outputs;
            std::vector<std::shared_ptr<SharedBuffer_t>> outputBuffers;
            for ( auto &frame : frames.frames )
            {
                RideHal_SharedBuffer_t sharedBuffer;
                ret = frame.buffer->sharedBuffer.ImageToTensor( &sharedBuffer );
                if ( RIDE_HAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "QNN failed to do image to tensor convert for frameId %" PRIu64
                                   ": ret = %d",
                                   frames.frames[0].frameId, ret );
                    break;
                }
                inputs.push_back( sharedBuffer );
            }

            for ( size_t i = 0; ( i < m_outputInfos.size() ) && ( RIDE_HAL_ERROR_NONE == ret );
                  i++ )
            {
                std::shared_ptr<SharedBuffer_t> buffer = m_tensorPools[i].Get();
                if ( nullptr != buffer )
                {
                    outputs.push_back( buffer->sharedBuffer );
                    outputBuffers.push_back( buffer );
                }
                else
                {
                    ret = RIDE_HAL_ERROR_NORES;
                }
            }

            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                bool locked = false;
                ret = SampleIF::Lock();
                locked = ( RIDE_HAL_ERROR_NONE == ret );
                ret = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(), outputs.size() );
                if ( true == locked )
                {
                    SampleIF::Unlock();
                }
            }

            if ( RIDE_HAL_ERROR_NONE == ret )
            {
                Tensors_t outTensors;
                size_t index = 0;
                for ( auto &buffer : outputBuffers )
                {
                    Tensor_t tensor;
                    tensor.buffer = buffer;
                    tensor.name = m_outputInfos[index].pName;
                    tensor.quantScale = m_outputInfos[index].quantScale;
                    tensor.quantOffset = m_outputInfos[index].quantOffset;
                    outTensors.tensors.push_back( tensor );
                    index++;
                }
                outTensors.frameId = frames.frames[0].frameId;
                outTensors.timestamp = frames.frames[0].timestamp;
                m_pub.Publish( outTensors );
            }
            else
            {
                RIDEHAL_ERROR( "QNN Execute failed for frameId %" PRIu64 ": ret = %d",
                               frames.frames[0].frameId, ret );
            }
        }
    }
}

RideHalError_e SampleQnn::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    ret = m_qnn.Stop();

    return ret;
}

RideHalError_e SampleQnn::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    ret = m_qnn.Deinit();

    return ret;
}

REGISTER_SAMPLE( Qnn, SampleQnn );

}   // namespace sample
}   // namespace ridehal
