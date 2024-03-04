/***************************************************************************/ /**
 @brief
    Program to test QNN

 @internal
    Copyright (c) 2020-2023 Qualcomm Technologies, Inc.
    All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
 *******************************************************************************/

#include <chrono>
#include <cinttypes>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "ride/hal/Buffer.hpp"
#include "ride/hal/QnnRuntime.hpp"
#include "ride/hal/Types.hpp"

static bool lDisableDumpingOutputs = false;
static int lBatchSize = 1; /* special used to test HTP DMA offset feature */
static int lDisableQnnPerf = 0;

using namespace ride::hal;

static void *load( const char *path, size_t *sz )
{
    void *out;
    FILE *fp = fopen( path, "rb" );
    assert( fp );
    fseek( fp, 0, SEEK_END );
    *sz = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    out = malloc( *sz );
    fread( out, 1, *sz, fp );
    fclose( fp );
    printf( "load %s %d\n", path, (int) *sz );
    return out;
}

static void save( char *path, void *out, size_t sz )
{
    FILE *fp = fopen( path, "wb" );
    assert( fp );
    fwrite( out, 1, sz, fp );
    fclose( fp );
    printf( "save %s %d\n", path, (int) sz );
}

void waitkey( void )
{
    printf( "waitkey: type enter to continue\n" );
    int ch = getchar();
    (void) ch;
}


typedef struct
{
    std::string modelPath;
    int iterations;
    int cdsp;
    bool userbuffer;
    std::vector<ride::hal::RideHal_SharedBuffer_t> inputs;
    int id;
    int tid; /* deploy this on which thread */
    int delayMs = 0;
    int periodMs = 0;
} ArgType;

class QnnTestRunner
{
public:
    QnnTestRunner() {}
    ~QnnTestRunner() {}

    bool init( ArgType &args )
    {
        m_Args = args;

        bool ret;
        size_t index = 0;
        auto TestID = m_Args.id;
        auto modelPath = m_Args.modelPath;
        auto cdsp = m_Args.cdsp;
        auto &inputs = m_Args.inputs;
        auto userbuffer = m_Args.userbuffer;
        printf( "[%d] Test models %s run %d iterations on HTP%d userbuffer=%s with %lu inputs "
                "batch_size %d\n",
                TestID, modelPath.c_str(), m_Args.iterations, cdsp,
                m_Args.userbuffer ? "true" : "false", inputs.size(), lBatchSize );
        auto begin = std::chrono::steady_clock::now();
        m_Config = { modelPath, 0, cdsp };
        if ( cdsp >= ( 2 + 14 + 1 ) )
        {
            m_Config.backendId = ride::hal::QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_CPU;
            m_Config.backendCoreId = 0;
        }
        else if ( cdsp >= ( 2 + 14 ) )
        {
            m_Config.backendId = ride::hal::QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_GPU;
            m_Config.backendCoreId = 0;
        }
        else if ( cdsp >= 2 )
        {
            m_Config.backendId = ride::hal::QnnRuntime_Backend_e::QNNRUNTIME_BACKEND_HTP_MCP;
            m_Config.backendCoreId = cdsp - 2;
        }
        ret = m_QRte.Init( "QnnTest", m_Config );
        if ( true != ret )
        {
            printf( "[%d] Failed to create QNN Runtime, error is %d\n", TestID, ret );
            return false;
        }
        auto end = std::chrono::steady_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        printf( "[%d] init cost %.2f ms\n", TestID, (float) cost / 1000.0 );

        ret = m_QRte.GetInputInfos( m_InputInfos );
        if ( true != ret )
        {
            printf( "[%d] Failed to get input buffer sizes\n", TestID );
            return false;
        }

        ret = m_QRte.GetOutputInfos( m_OutputInfos );
        if ( true != ret )
        {
            printf( "[%d] Failed to get output buffer sizes\n", TestID );
            return false;
        }

        index = 0;
        for ( auto info : m_InputInfos )
        {
            printf( "[%d] input %" PRIu64 " size %" PRIu32 "\n", TestID, index, info.size );
            uint64_t memHandle = 0;
            void *ptr;
            if ( true == userbuffer )
            {
                ptr = malloc( info.size * lBatchSize );
            }
            else
            {
                auto buffer =
                        std::make_shared<Buffer>( info.size * lBatchSize, Buffer::Type::DMABUF );
                m_Buffers.push_back( buffer );
                ptr = buffer->getWritableDataPtr();
                memHandle = reinterpret_cast<uint64_t>( buffer->getDmaHandle() );
            }
            if ( nullptr != ptr )
            {
                ride::hal::RideHal_SharedBuffer_t sharedBuffer;
                sharedBuffer.buffer.pData = (uint8_t *) ptr;
                sharedBuffer.buffer.size = info.size * lBatchSize;
                if ( true == userbuffer )
                {
                    sharedBuffer.buffer.dmaHandle = 0;
                    sharedBuffer.type = QBUFFER_TYPE_HEAP;
                }
                else
                {
                    sharedBuffer.buffer.dmaHandle = (uint64_t) memHandle;
                    sharedBuffer.type = QBUFFER_TYPE_DMABUF;
                }
                sharedBuffer.offset = info.size * ( lBatchSize - 1 );
                sharedBuffer.data() = sharedBuffer.buffer.pData + sharedBuffer.offset;
                sharedBuffer.size = info.size;
                m_InputBuffers.push_back( sharedBuffer );
                if ( index < inputs.size() )
                {
                    auto input_size = info.size;
                    printf( "[%d] load real input for input %" PRIu64 "\n", TestID, index );
                    if ( inputs[index].size == input_size )
                    {
                        memcpy( sharedBuffer.data(), inputs[index].buffer.pData, input_size );
                    }
                    else if ( ( inputs[index].size == ( input_size * sizeof( float ) ) ) &&
                              ( info.quant_scale != 0 ) )
                    {   // guess an FP32 input for DMA UINT8 IO
                        printf( "[%d] quantize it\n", TestID );
                        auto scale = info.quant_scale;
                        auto offset = info.quant_offset;
                        float *fData = (float *) inputs[index].buffer.pData;
                        uint8_t *pData = (uint8_t *) sharedBuffer.data();
                        for ( size_t j = 0; j < input_size; j++ )
                        {
                            pData[j] = (uint8_t) std::min(
                                    std::max( std::round( fData[j] / scale + offset ), 0.0f ),
                                    255.0f );
                        }
                        save( (char *) "quantize.raw", pData, input_size );
                    }
                    else
                    {
                        printf( "input size not correct, abort test\n" );
                        return false;
                    }
                }
                else
                {
                    // std::generate( buffer.buffer.pData, buffer.buffer.pData + buffer.size,
                    // std::rand );
                }
            }
            else
            {
                printf( "[%d] Failed to allocate input buffer with size %" PRIu32 "\n", TestID,
                        info.size );
                return false;
            }
            index++;
        }
        index = 0;
        for ( auto info : m_OutputInfos )
        {
            printf( "[%d] output %" PRIu64 " size %" PRIu32 "\n", TestID, index, info.size );
            uint64_t memHandle = 0;
            void *ptr;
            if ( true == userbuffer )
            {
                ptr = malloc( info.size );
            }
            else
            {
                auto buffer = std::make_shared<Buffer>( info.size, Buffer::Type::DMABUF );
                m_Buffers.push_back( buffer );
                ptr = buffer->getWritableDataPtr();
                memHandle = reinterpret_cast<uint64_t>( buffer->getDmaHandle() );
            }
            if ( nullptr != ptr )
            {
                ride::hal::RideHal_SharedBuffer_t sharedBuffer;
                sharedBuffer.buffer.pData = (uint8_t *) ptr;
                sharedBuffer.buffer.size = info.size;
                if ( true == userbuffer )
                {
                    sharedBuffer.buffer.dmaHandle = 0;
                    sharedBuffer.type = QBUFFER_TYPE_HEAP;
                }
                else
                {
                    sharedBuffer.buffer.dmaHandle = (uint64_t) memHandle;
                    sharedBuffer.type = QBUFFER_TYPE_DMABUF;
                }
                sharedBuffer.offset = 0;
                sharedBuffer.data() = sharedBuffer.buffer.pData + sharedBuffer.offset;
                sharedBuffer.size = info.size;
                m_OutputBuffers.push_back( sharedBuffer );
            }
            else
            {
                printf( "[%d] Failed to allocate output buffer with size %" PRIu32 "\n", TestID,
                        info.size );
                return false;
            }
            index++;
        }

        for ( auto &info : m_InputInfos )
        {
            printf( "#[%d] input %s: quant_scale=%f, quant_offset=%d, size=%u, offset=%u, type=%d, "
                    "dims=%s\n",
                    TestID, info.name.c_str(), info.quant_scale, info.quant_offset, info.size,
                    info.offset, (int) info.dataType, info.shape().c_str() );
        }

        for ( auto &info : m_OutputInfos )
        {
            printf( "#[%d] output %s: quant_scale=%f, quant_offset=%d, size=%u, offset=%u, "
                    "type=%d, "
                    "dims=%s\n",
                    TestID, info.name.c_str(), info.quant_scale, info.quant_offset, info.size,
                    info.offset, (int) info.dataType, info.shape().c_str() );
        }

        return true;
    }

    bool run()
    {
        bool ret;
        auto TestID = m_Args.id;
        auto cdsp = m_Args.cdsp;
        auto &inputs = m_Args.inputs;
        auto modelPath = m_Args.modelPath;
        ride::hal::QnnRuntime_Perf_t perf = { 0, 0, 0, 0 };
        ride::hal::QnnRuntime_Perf_t *ptrPerf = &perf;

        if ( lDisableQnnPerf )
        {
            ptrPerf = nullptr;
        }

        if ( 0 == m_Iter )
        {
            m_Tstart = std::chrono::steady_clock::now();
        }

        auto begin = std::chrono::steady_clock::now();
        ret = m_QRte.Execute( m_InputBuffers, m_OutputBuffers, ptrPerf );
        if ( true != ret )
        {
            printf( "[%d] Failed to run, error is %d\n", TestID, ret );
            return false;
        }
        auto end = std::chrono::steady_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        m_Total += cost;
        m_TotalQnn += perf.qnn;
        m_TotalRpc += perf.rpc;
        m_TotalQnnAcc += perf.qnnAccelerator;
        m_TotalAcc += perf.accelerator;
        printf( "[%d-%d-%d] QNN %d iterations, cost %.2f ms, perf: qnn="
                "%" PRIu64 " rpc=%" PRIu64 " qnn acc=%" PRIu64 " acc=%" PRIu64 "\n",
                TestID, cdsp, m_Args.tid, m_Iter, (float) cost / 1000.0, perf.qnn, perf.rpc,
                perf.qnnAccelerator, perf.accelerator );
        if ( ( inputs.size() > 0 ) && ( false == lDisableDumpingOutputs ) )
        {
            for ( size_t i = 0; i < m_OutputBuffers.size(); i++ )
            {
                char path[128];
                snprintf( path, sizeof( path ), "output%lu.raw", i );
                save( path, m_OutputBuffers[i].buffer.pData, m_OutputBuffers[i].size );
                auto &info = m_OutputInfos[i];
                // enhanced way to dump the output for accuracy analyze
                uint8_t *pData = m_OutputBuffers[i].buffer.pData;
                float *fData = new float[info.size];
                for ( size_t i = 0; i < info.size; i++ )
                {
                    fData[i] = info.quant_scale * ( pData[i] - info.quant_offset );
                }
                snprintf( path, sizeof( path ), "%s.raw", info.name.c_str() );
                save( path, fData, info.size * sizeof( float ) );
                delete[] fData;
            }
        }

        m_Iter++;

        if ( 0 == ( m_Iter % m_Args.iterations ) )
        {
            auto Tend = std::chrono::steady_clock::now();
            cost = std::chrono::duration_cast<std::chrono::microseconds>( Tend - m_Tstart ).count();
            float FPS = m_Iter / ( cost / 1000000.f );
            printf( "[%d-%d-%d] %s on HTP%d QNN avg cost %.2f ms for %d loops, FPS=%.2f,"
                    " qnn avg %.2f ms FPS=%.2f,"
                    " rpc avg %.2f ms FPS=%.2f,"
                    " qnn accerlator avg %.2f ms FPS=%.2f,"
                    " accerlator avg %.2f ms FPS=%.2f\n",
                    TestID, cdsp, m_Args.tid, modelPath.c_str(), cdsp,
                    (float) m_Total / m_Iter / 1000.0, m_Iter, FPS,
                    (float) m_TotalQnn / m_Iter / 1000.0,
                    (float) 1000000.0 / ( m_TotalQnn / m_Iter ),
                    (float) m_TotalRpc / m_Iter / 1000.0,
                    (float) 1000000.0 / ( m_TotalRpc / m_Iter ),
                    (float) m_TotalQnnAcc / m_Iter / 1000.0,
                    (float) 1000000.0 / ( m_TotalQnnAcc / m_Iter ),
                    (float) m_TotalAcc / m_Iter / 1000.0,
                    (float) 1000000.0 / ( m_TotalAcc / m_Iter ) );
        }

        return true;
    }

    void deinit()
    {
        auto TestID = m_Args.id;
        auto begin = std::chrono::steady_clock::now();
        m_QRte.Deinit();
        auto end = std::chrono::steady_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        printf( "[%d] deinit cost %.2f ms\n", TestID, (float) cost / 1000.0 );
    }

    int get_iterations() { return m_Args.iterations; }
    int get_tid() { return m_Args.tid; }

    int get_delay() { return m_Args.delayMs; }
    int get_period() { return m_Args.periodMs; }

private:
    ArgType m_Args;
    ride::hal::QnnRuntime_Config_t m_Config;
    ride::hal::QnnRuntime m_QRte;
    std::vector<ride::hal::QnnRuntime_TensorInfo_t> m_InputInfos;
    std::vector<ride::hal::QnnRuntime_TensorInfo_t> m_OutputInfos;
    std::vector<ride::hal::RideHal_SharedBuffer_t> m_InputBuffers;
    std::vector<ride::hal::RideHal_SharedBuffer_t> m_OutputBuffers;
    uint64_t m_Total = 0;
    uint64_t m_TotalQnn = 0;
    uint64_t m_TotalRpc = 0;
    uint64_t m_TotalQnnAcc = 0;
    uint64_t m_TotalAcc = 0;
    int m_Iter = 0;

    std::vector<std::shared_ptr<Buffer>> m_Buffers;

    std::chrono::time_point<std::chrono::steady_clock> m_Tstart;
};

bool thread_main( int interations, std::vector<std::shared_ptr<QnnTestRunner>> runners )
{
    int tid = runners[0]->get_tid();
    int delayMs = runners[0]->get_delay();
    uint64_t periodUs = (uint64_t) runners[0]->get_period() * 1000;
    uint64_t total = 0;

    std::this_thread::sleep_for( std::chrono::milliseconds( delayMs ) );
    for ( int i = 0; i < interations; i++ )
    {
        auto begin = std::chrono::steady_clock::now();
        for ( auto &runner : runners )
        {
            runner->run();
        }
        auto end = std::chrono::steady_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        total += cost;
        if ( (uint64_t) cost < periodUs )
        {
            std::this_thread::sleep_for( std::chrono::microseconds( periodUs - cost ) );
        }

        if ( runners.size() > 1 )
        {
            printf( "[%d] %d iterations cost %.2f ms\n", tid, i, (float) cost / 1000.0 );
        }
    }
    if ( runners.size() > 1 )
    {
        printf( "[%d] avg cost %.2f ms\n", tid, (float) total / interations / 1000.0 );
    }

    return true;
}

void usage( char *prog )
{
    printf( "usage: %s [-d] \"modelPath iterations [cdsp(0|1)] [userbuffer(0|1)] [tid] [input "
            "list]\" [-s delayMs] [-p periodMs]"
            " [ \"modelPath iterations [cdsp(0|1)] [userbuffer(0|1)] [tid] [input list]\" [-s "
            "delayMs] [-p periodMs] ] ...\n"
            "for example:\n"
            "  [xxx]: [] optional parameter, but must be required if its following optional "
            "parameter is given\n"
            "  -d: disable dump outputs to file if input_list is given\n",
            prog );
    printf( "  %s \". 10000\"\n", prog );
    printf( "  %s \". 10000\" \". 10000\"\n", prog );
}

std::vector<std::string> split( const std::string &s, char seperator )
{
    std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while ( ( pos = s.find( seperator, pos ) ) != std::string::npos )
    {
        std::string substring( s.substr( prev_pos, pos - prev_pos ) );

        output.push_back( substring );

        prev_pos = ++pos;
    }

    output.push_back( s.substr( prev_pos, pos - prev_pos ) );   // Last word

    return output;
}

// #include <hogl/engine.hpp>
// #include <hogl/format-basic.hpp>
// #include <hogl/format-raw.hpp>
// #include <hogl/mask.hpp>
// #include <hogl/output-file.hpp>
// #include <hogl/output-null.hpp>
// #include <hogl/output-ride::hale.hpp>
// #include <hogl/output-stderr.hpp>
// #include <hogl/output-stdout.hpp>
// #include <hogl/platform.hpp>
// #include <hogl/timesource.hpp>
// #include <hogl/tls.hpp>
// void init_hogl()
// {
//     static hogl::format *format;
//     static hogl::output *output;
//     format = new hogl::format_basic( "fast1" );
//     output = new hogl::output_stdout( *format );
//     hogl::engine::options opts = hogl::engine::default_options;
//     opts.default_mask = hogl::mask( ".*:.*(INFO|WARN|ERROR|FATAL|DROPMARK|TSOFULLMARK)", 0 );
//     hogl::activate( *output, opts );
// }

int main( int argc, char *argv[] )
{
    std::vector<ArgType> args;
    if ( argc <= 1 )
    {
        usage( argv[0] );
        return -1;
    }

    // init_hogl();

    for ( int i = 1; i < argc; i++ )
    {
        std::string argStr = argv[i];
        if ( argStr == "-b" )
        {
            if ( ( ( i + 1 ) < argc ) && ( args.size() > 0 ) )
            {
                lBatchSize = atoi( argv[i + 1] );
                i += 1;
            }
            else
            {
                usage( argv[0] );
                return -1;
            }
            continue;
        }
        if ( argStr == "-D" )
        {
            lDisableQnnPerf = 1;
            continue;
        }
        if ( argStr == "-d" )
        {
            lDisableDumpingOutputs = true;
            continue;
        }
        if ( argStr == "-s" )
        {   // specify the start execution delay time in ms
            if ( ( ( i + 1 ) < argc ) && ( args.size() > 0 ) )
            {
                auto &arg = args.back();
                arg.delayMs = atoi( argv[i + 1] );
                i += 1;
            }
            else
            {
                usage( argv[0] );
                return -1;
            }
            continue;
        }
        if ( argStr == "-p" )
        {   // specify the execution period time in ms
            if ( ( ( i + 1 ) < argc ) && ( args.size() > 0 ) )
            {
                auto &arg = args.back();
                arg.periodMs = atoi( argv[i + 1] );
                i += 1;
            }
            else
            {
                usage( argv[0] );
                return -1;
            }
            continue;
        }
        std::vector<std::string> strings = split( argStr, ' ' );
        if ( strings.size() >= 2 )
        {
            ArgType arg;
            arg.id = (int) args.size();
            arg.modelPath = strings[0];
            arg.iterations = atoi( strings[1].c_str() );
            if ( strings.size() >= 3 )
            {
                arg.cdsp = atoi( strings[2].c_str() );
            }
            else
            {
                arg.cdsp = 0;
            }

            if ( strings.size() >= 4 )
            {
                arg.userbuffer = ( atoi( strings[3].c_str() ) == 1 );
            }
            else
            {
                arg.userbuffer = false;
            }

            if ( strings.size() >= 5 )
            {
                arg.tid = atoi( strings[4].c_str() );
            }
            else
            {
                arg.tid = 10000 + arg.id; /* defaut: each has a own thread */
            }

            for ( int i = 0; i < ( (int) strings.size() - 5 ); i++ )
            {
                ride::hal::RideHal_SharedBuffer_t sharedBuffer;
                sharedBuffer.buffer.pData = (uint8_t *) load( (char *) strings[5 + i].c_str(),
                                                              &sharedBuffer.buffer.size );
                if ( nullptr == sharedBuffer.buffer.pData )
                {
                    return -1;
                }
                arg.inputs.push_back( sharedBuffer );
            }
            args.push_back( arg );
        }
        else
        {
            usage( argv[0] );
            return -1;
        }
    }

    std::map<int, std::vector<std::shared_ptr<QnnTestRunner>>> runnersMap;
    for ( auto &arg : args )
    {
        printf( "Test case %s %d cdsp=%d userbuffer=%s tid=%d with %d inputs, delay %d ms, period "
                "%d ms\n",
                arg.modelPath.c_str(), arg.iterations, arg.cdsp, arg.userbuffer ? "true" : "false",
                arg.tid, (int) arg.inputs.size(), arg.delayMs, arg.periodMs );
        auto runner = std::make_shared<QnnTestRunner>();
        auto ret = runner->init( arg );
        if ( false == ret )
        {
            return -1;
        }
        auto tid = arg.tid;
        auto it = runnersMap.find( tid );
        if ( it == runnersMap.end() )
        {
            runnersMap[tid] = { runner };
        }
        else
        {
            runnersMap[tid].push_back( runner );
        }
    }

    std::vector<std::shared_ptr<std::thread>> threads;
    for ( auto &kv : runnersMap )
    {
        auto runners = kv.second;
        auto iterations = runners[0]->get_iterations();
        auto th = std::make_shared<std::thread>( thread_main, iterations, runners );
        threads.push_back( th );
    }

    for ( auto &th : threads )
    {
        th->join();
    }

    for ( auto &kv : runnersMap )
    {
        auto runners = kv.second;
        for ( auto runner : runners )
        {
            runner->deinit();
        }
    }

    return 0;
}