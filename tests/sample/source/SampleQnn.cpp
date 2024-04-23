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
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_modelPath = Get( config, "model_path", "" );
    m_config.modelPath = m_modelPath.c_str();
    if ( "" == m_config.modelPath )
    {
        RIDEHAL_ERROR( "invalid modelPath\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.backendType = Get( config, "processor", RIDEHAL_PROCESSOR_HTP0 );
    if ( RIDEHAL_PROCESSOR_MAX == m_config.backendType )
    {
        RIDEHAL_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
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

RideHalError_e SampleQnn::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SampleIF::Init( m_config.backendType );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_qnn.Init( name.c_str(), &m_config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_qnn.GetInputInfo( &m_inputInfoList );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_qnn.GetOutputInfo( &m_outputInfoList );
    }

    const size_t outputNum = m_outputInfoList.num;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_tensorPools.resize( outputNum );

        size_t index = 0;
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = m_tensorPools[index].Init(
                    "Qnn." + name + "." + std::to_string( index ), LOGGER_LEVEL_INFO, m_poolSize,
                    m_outputInfoList.pInfo[i].properties, RIDEHAL_BUFFER_USAGE_HTP );
            index += 1;
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
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

RideHalError_e SampleQnn::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_qnn.Start();
    if ( RIDEHAL_ERROR_NONE == ret )
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
        if ( RIDEHAL_ERROR_NONE == ret )
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
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "QNN failed to do image to tensor convert for frameId %" PRIu64
                                   ": ret = %d",
                                   frames.frames[0].frameId, ret );
                    break;
                }
                inputs.push_back( sharedBuffer );
            }

            for ( size_t i = 0; ( i < m_outputInfoList.num ) && ( RIDEHAL_ERROR_NONE == ret );
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
                    ret = RIDEHAL_ERROR_NOMEM;
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                bool locked = false;
                ret = SampleIF::Lock();
                locked = ( RIDEHAL_ERROR_NONE == ret );
                PROFILER_BEGIN();
                ret = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(), outputs.size() );
                if ( true == locked )
                {
                    PROFILER_END();
                    SampleIF::Unlock();
                }
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                Tensors_t outTensors;
                size_t index = 0;
                for ( auto &buffer : outputBuffers )
                {
                    Tensor_t tensor;
                    tensor.buffer = buffer;
                    tensor.name = m_outputInfoList.pInfo[index].pName;
                    tensor.quantScale = m_outputInfoList.pInfo[index].quantScale;
                    tensor.quantOffset = m_outputInfoList.pInfo[index].quantOffset;
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
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    ret = m_qnn.Stop();

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleQnn::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_qnn.Deinit();

    return ret;
}

REGISTER_SAMPLE( Qnn, SampleQnn );

}   // namespace sample
}   // namespace ridehal
