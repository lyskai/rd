// Copyright 2024 Qualcomm Technologies, Inc. All rights reserved.
// Confidential & Proprietary.

#include "ride/hal/Remap.hpp"
#include "FadasSrv.hpp"

#include <map>
#include <vector>

namespace ride
{
namespace hal
{
namespace component
{

using namespace ride::hal::libs::FadasIface;

#define ALIGN_128( x ) ( ( ( ( x ) + 127 ) >> 7 ) << 7 )

static const FadasSrv::Core g_CoreIdConversion[] = {
        FadasSrv::Core::DSP0,   // ProcessorType::DSP0
        FadasSrv::Core::DSP1,   // ProcessorType::DSP1
        FadasSrv::Core::CPU     // ProcessorType::CPU
};

class RemapCPU final : public RemapImpl
{
public:
    RemapCPU();
    ~RemapCPU();
    bool init( const Remap_Config_t *pConfig );
    void kill();
    RideHalError_e remap( const RideHal_SharedBuffer_t *inputs,
                          const RideHal_SharedBuffer_t *outputs );

private:
    bool creatRemapTables( const Remap_Config_t *pConfig );
    FadasRemapPipeline_e getPipeline( RideHal_ImageFormat_e inputFormat );

private:
    size_t m_OutputWidth;
    size_t m_OutputHeight;
    RideHal_ImageFormat_e m_InputFormats[RIDE_HAL_MAX_INPUTS];
    uint32_t m_InputWidths[RIDE_HAL_MAX_INPUTS];
    uint32_t m_InputHeights[RIDE_HAL_MAX_INPUTS];
    size_t m_InputSizes[RIDE_HAL_MAX_INPUTS];
    size_t m_OutputSize;
    RideHal_ImageFormat_e m_OutputFormat;
    FadasNormlzParams_t m_Normlz[3];
    FadasROI_t m_ROIs[RIDE_HAL_MAX_INPUTS];
    int m_BatchSize = 1;
    std::map<RideHal_ImageFormat_e, void *> m_WorkersMap;
    std::vector<void *> m_WorkerPtrs;
    std::vector<FadasRemapMap_t *> m_RemapPtrs;
    bool m_bEnableNormalize;
    bool m_bEnableUndistortion;
};

RemapCPU::RemapCPU() {}

RemapCPU::~RemapCPU() {}

bool RemapCPU::creatRemapTables( const Remap_Config_t *pConfig )
{
    bool ret;
    for ( int i = 0; i < pConfig->numOfInputs; i++ )
    {
        auto inputWidth = m_InputWidths[i];
        auto inputHeight = m_InputHeights[i];
        auto mapWidth = pConfig->inputConfigs[i].mapWidth;
        auto mapHeight = pConfig->inputConfigs[i].mapHeight;
        size_t mapSize = mapWidth * mapHeight * sizeof( float );
        float *mapXPtr = pConfig->inputConfigs[i].remapTable.pMapX;
        float *mapYPtr = pConfig->inputConfigs[i].remapTable.pMapY;
        auto inputFormat = m_InputFormats[i];
        auto pipeline = getPipeline( inputFormat );

        auto remapPtr = FadasRemap_CreateMapFromMap( inputWidth, inputHeight, mapWidth, mapHeight,
                                                     mapWidth * sizeof( float ), mapXPtr, mapYPtr,
                                                     pipeline, 0 );
        if ( remapPtr == nullptr )
        {
            // hogl::post( m_HoglArea, m_HoglArea->ERROR, "Failed to create a remap map for CPU" );
            return false;
        }
        m_RemapPtrs.push_back( remapPtr );
    }

    return true;
}

FadasRemapPipeline_e RemapCPU::getPipeline( RideHal_ImageFormat_e inputFormat )
{
    FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX;
    if ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat )
    {
        if ( true == m_bEnableNormalize )
        {
            pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8;
        }
        else
        {
            pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888;
        }
    }
    else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat )
    {
        if ( false == m_bEnableNormalize )
        {
            pipeline = FADAS_REMAP_PIPELINE_3C888;
        }
        else
        {
            // hogl::post( m_HoglArea, m_HoglArea->ERROR,"Remap doesn't support to do normalization
            // on RGB input" );
        }
    }

    return pipeline;
}

bool RemapCPU::init( const Remap_Config_t *pConfig )
{
    m_OutputFormat = pConfig->outputFormat;
    m_BatchSize = pConfig->numOfInputs;
    m_Normlz[0] = pConfig->normlzR;
    m_Normlz[1] = pConfig->normlzG;
    m_Normlz[2] = pConfig->normlzB;
    m_OutputWidth = pConfig->outputWidth;
    m_OutputHeight = pConfig->outputHeight;
    m_OutputSize = m_OutputHeight * m_OutputWidth * 3;
    m_bEnableNormalize = pConfig->bEnableNormalize;
    m_bEnableUndistortion = pConfig->bEnableUndistortion;

    for ( int i = 0; i < m_BatchSize; i++ )
    {
        m_InputFormats[i] = pConfig->inputConfigs[i].inputFormat;
        m_ROIs[i] = pConfig->inputConfigs[i].ROI;
        m_InputWidths[i] = pConfig->inputConfigs[i].inputWidth;
        m_InputHeights[i] = pConfig->inputConfigs[i].inputHeight;
        if ( RIDE_HAL_IMAGE_FORMAT_UYVY == m_InputFormats[i] )
        {
            m_InputSizes[i] = m_InputHeights[i] * ALIGN_128( m_InputWidths[i] * 2 );
        }
        else
        {
            m_InputSizes[i] = m_InputHeights[i] * ALIGN_128( m_InputWidths[i] * 3 );
        }
    }

    bool ret = FadasSrv::Initialize( FadasSrv::Core::CPU );
    if ( false == ret )
    {
        // hogl::post( m_HoglArea, m_HoglArea->ERROR, "FadasIface CPU Init failed" );
        return false;
    }

    int32_t pThreadsAffinity[] = { 0, 1, 2, 3 };
    for ( int i = 0; i < m_BatchSize; i++ )
    {
        auto inputFormat = m_InputFormats[i];
        auto pipeline = getPipeline( inputFormat );
        if ( FADAS_REMAP_PIPELINE_MAX == pipeline )
        {
            return false;
        }
        auto it = m_WorkersMap.find( inputFormat );
        if ( m_WorkersMap.end() == it )
        {
            auto workerPtr = FadasRemap_CreateWorkers( 4, pThreadsAffinity, pipeline );
            if ( workerPtr == nullptr )
            {
                // hogl::post( m_HoglArea, m_HoglArea->ERROR,"Failed to create a remap worker for
                // CPU" );
                return false;
            }
            m_WorkerPtrs.push_back( workerPtr );
            m_WorkersMap[inputFormat] = workerPtr;
        }
        else
        {
            m_WorkerPtrs.push_back( it->second );
        }
    }

    if ( true == m_bEnableUndistortion )
    {
        ret = creatRemapTables( pConfig );
        if ( false == ret )
        {
            return false;
        }
    }
    else
    {
        for ( int i = 0; i < m_BatchSize; i++ )
        {
            auto inputWidth = m_InputWidths[i];
            auto inputHeight = m_InputHeights[i];
            auto mapWidth = pConfig->inputConfigs[i].mapWidth;
            auto mapHeight = pConfig->inputConfigs[i].mapHeight;
            auto inputFormat = m_InputFormats[i];
            auto pipeline = getPipeline( inputFormat );

            auto remapPtr = FadasRemap_CreateMapNoUndistortion( inputWidth, inputHeight, mapWidth,
                                                                mapHeight, pipeline, 0 );
            if ( remapPtr == nullptr )
            {
                // hogl::post( m_HoglArea, m_HoglArea->ERROR,"Failed to create a remap map no
                // undistortion for CPU" );
                return false;
            }
            m_RemapPtrs.push_back( remapPtr );
        }
    }

    return true;
}

void RemapCPU::kill() {}

RideHalError_e RemapCPU::remap( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *outputs )
{
    for ( int i = 0; i < m_BatchSize; i++ )
    {
        uint8_t *pSrc = (uint8_t *) inputs[i].data();
        uint8_t *pDst = (uint8_t *) outputs[0].data() + i * m_OutputSize;
        FadasImage_t srcImg;
        srcImg.props.width = m_InputWidths[i];
        srcImg.props.height = m_InputHeights[i];
        auto inputFormat = m_InputFormats[i];
        srcImg.props.numPlanes = 1;
        srcImg.plane[0] = pSrc;
        if ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat )
        {
            srcImg.props.format = FADAS_IMAGE_FORMAT_UYVY;
            srcImg.props.stride[0] = ALIGN_128( srcImg.props.width * 2 );
        }
        else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat )
        {
            srcImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
            srcImg.props.stride[0] = ALIGN_128( srcImg.props.width * 3 );
        }
        else
        {
        }
        srcImg.bAllocated = false;

        FadasImage_t rgbImg;
        rgbImg.props.width = m_OutputWidth;
        rgbImg.props.height = m_OutputHeight;
        rgbImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
        rgbImg.props.stride[0] = m_OutputWidth * 3;
        rgbImg.props.numPlanes = 1;
        rgbImg.plane[0] = pDst;
        rgbImg.bAllocated = false;

        FadasROI_t roi = m_ROIs[i];
        FadasRemapMap_t *remapPtr = m_RemapPtrs[0];
        if ( i < (int) m_RemapPtrs.size() )
        {
            remapPtr = m_RemapPtrs[i];
        }

        (void) FadasRegBuf( FADAS_BUF_TYPE_IN, pSrc, m_InputSizes[i] );
        (void) FadasRegBuf( FADAS_BUF_TYPE_OUT, pDst, m_OutputSize );
        FadasError_e ret;
        if ( false == m_bEnableUndistortion )
        {
            ret = FadasRemap_RunMT( m_WorkerPtrs[i], remapPtr, &srcImg, &rgbImg, &roi );
        }
        else
        {
            ret = FadasRemap_RunMT( m_WorkerPtrs[i], remapPtr, &srcImg, &rgbImg, &roi, 1.0,
                                    m_Normlz );
        }

        (void) FadasDeregBuf( pSrc );
        (void) FadasDeregBuf( pDst );
        if ( FADAS_ERROR_NONE != ret )
        {
            // hogl::post( m_HoglArea, m_HoglArea->ERROR, "Remap888 failed for batch %d: ret =
            // 0x%x", i, ret );
            return RIDE_HAL_ERROR_FAIL;
        }
        pSrc = pSrc + m_InputSizes[i];
        pDst = pDst + m_OutputSize * i;
    }

    return RIDE_HAL_ERROR_NONE;
}

class RemapDSP final : public RemapImpl
{
public:
    RemapDSP();
    ~RemapDSP();
    bool init( const Remap_Config_t *pConfig );
    void kill();
    RideHalError_e remap( const RideHal_SharedBuffer_t *inputs,
                          const RideHal_SharedBuffer_t *outputs );

private:
    bool creatRemapTables( const Remap_Config_t *pConfig );
    FadasIface_FadasRemapPipeline_e getPipeline( RideHal_ImageFormat_e inputFormat );

private:
    Remap_ProcessorType_t m_Processor;
    size_t m_OutputWidth;
    size_t m_OutputHeight;
    RideHal_ImageFormat_e m_InputFormats[RIDE_HAL_MAX_INPUTS];
    uint32_t m_InputWidths[RIDE_HAL_MAX_INPUTS];
    uint32_t m_InputHeights[RIDE_HAL_MAX_INPUTS];
    size_t m_InputSizes[RIDE_HAL_MAX_INPUTS];
    size_t m_OutputSize;
    RideHal_ImageFormat_e m_OutputFormat;
    FadasIface_FadasNormlzParams_t m_Normlz[3];
    FadasIface_FadasROI_t m_ROIs[RIDE_HAL_MAX_INPUTS];
    std::vector<FadasIface_FadasImgProps_t> m_SrcImgProps;
    std::vector<uint32_t> m_Offsets;
    int m_BatchSize = 1;
    std::map<RideHal_ImageFormat_e, uint64> m_WorkersMap;
    std::vector<uint64> m_WorkerPtrs;
    std::vector<uint64> m_RemapPtrs;
    bool m_bEnableNormalize;
    bool m_bEnableUndistortion;
    remote_handle64 m_Handle64;
};

RemapDSP::RemapDSP() {}

RemapDSP::~RemapDSP() {}

FadasIface_FadasRemapPipeline_e RemapDSP::getPipeline( RideHal_ImageFormat_e inputFormat )
{
    FadasIface_FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX_NSP;
    if ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat )
    {
        if ( true == m_bEnableNormalize )
        {
            pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8_NSP;
        }
        else
        {
            pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NSP;
        }
    }
    else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat )
    {
        if ( false == m_bEnableNormalize )
        {
            pipeline = FADAS_REMAP_PIPELINE_3C888_NSP;
        }
        else
        {
            // hogl::post( m_HoglArea, m_HoglArea->ERROR, "Remap doesn't support to do normalization
            // on RGB input" );
        }
    }

    return pipeline;
}

bool RemapDSP::creatRemapTables( const Remap_Config_t *pConfig )
{
    bool ret;
    for ( int i = 0; i < pConfig->numOfInputs; i++ )
    {
        auto inputWidth = m_InputWidths[i];
        auto inputHeight = m_InputHeights[i];
        auto mapWidth = pConfig->inputConfigs[i].mapWidth;
        auto mapHeight = pConfig->inputConfigs[i].mapHeight;
        size_t mapSize = mapWidth * mapHeight * sizeof( float );

        float *mapXPtr = pConfig->inputConfigs[i].remapTable.pMapX;
        float *mapYPtr = pConfig->inputConfigs[i].remapTable.pMapY;

        auto inputFormat = m_InputFormats[i];
        auto pipeline = getPipeline( inputFormat );

        uint64 remapPtr = 0;
        auto retval = FadasIface_FadasRemap_CreateMapFromMap(
                m_Handle64, &remapPtr, inputWidth, inputHeight, mapWidth, mapHeight, mapXPtr,
                mapHeight * mapWidth, mapYPtr, mapHeight * mapWidth, mapWidth * sizeof( float ),
                pipeline, 0 );

        if ( AEE_SUCCESS != retval )
        {
            // hogl::post( m_HoglArea, m_HoglArea->ERROR, "Failed to create a remap map for DSP" );
            return false;
        }
        m_RemapPtrs.push_back( remapPtr );
    }

    return true;
}

bool RemapDSP::init( const Remap_Config_t *pConfig )
{
    m_Processor = pConfig->processor;
    m_OutputFormat = pConfig->outputFormat;
    m_BatchSize = pConfig->numOfInputs;
    m_Normlz[0].sub = pConfig->normlzR.sub;
    m_Normlz[0].mul = pConfig->normlzR.mul;
    m_Normlz[0].add = pConfig->normlzR.add;
    m_Normlz[1].sub = pConfig->normlzG.sub;
    m_Normlz[1].mul = pConfig->normlzG.mul;
    m_Normlz[1].add = pConfig->normlzG.add;
    m_Normlz[2].sub = pConfig->normlzB.sub;
    m_Normlz[2].mul = pConfig->normlzB.mul;
    m_Normlz[2].add = pConfig->normlzB.add;
    m_OutputWidth = pConfig->outputWidth;
    m_OutputHeight = pConfig->outputHeight;
    m_OutputSize = m_OutputHeight * m_OutputWidth * 3;
    m_bEnableNormalize = pConfig->bEnableNormalize;
    m_bEnableUndistortion = pConfig->bEnableUndistortion;

    for ( int i = 0; i < m_BatchSize; i++ )
    {
        m_InputFormats[i] = pConfig->inputConfigs[i].inputFormat;
        m_ROIs[i].x = pConfig->inputConfigs[i].ROI.x;
        m_ROIs[i].y = pConfig->inputConfigs[i].ROI.y;
        m_ROIs[i].width = pConfig->inputConfigs[i].ROI.width;
        m_ROIs[i].height = pConfig->inputConfigs[i].ROI.height;
        m_InputWidths[i] = pConfig->inputConfigs[i].inputWidth;
        m_InputHeights[i] = pConfig->inputConfigs[i].inputHeight;
        m_InputFormats[i] = pConfig->inputConfigs[i].inputFormat;
        if ( RIDE_HAL_IMAGE_FORMAT_UYVY == m_InputFormats[i] )
        {
            m_InputSizes[i] = m_InputHeights[i] * ALIGN_128( m_InputWidths[i] * 2 );
        }
        else
        {
            m_InputSizes[i] = m_InputHeights[i] * ALIGN_128( m_InputWidths[i] * 3 );
        }

        FadasIface_FadasImgProps_t srcImgProp;
        srcImgProp.width = m_InputWidths[i];
        srcImgProp.height = m_InputHeights[i];
        srcImgProp.numPlanes = 1;
        if ( RIDE_HAL_IMAGE_FORMAT_UYVY == m_InputFormats[i] )
        {
            srcImgProp.format = FADAS_IMAGE_FORMAT_UYVY_NSP;
            srcImgProp.stride[0] = ALIGN_128( m_InputWidths[i] * 2 );
        }
        else
        {
            srcImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
            srcImgProp.stride[0] = ALIGN_128( m_InputWidths[i] * 3 );
        }
        m_SrcImgProps.push_back( srcImgProp );
        m_Offsets.push_back( 0 );
    }

    bool ret = FadasSrv::Initialize( g_CoreIdConversion[m_Processor] );
    if ( false == ret )
    {
        // hogl::post( m_HoglArea, m_HoglArea->ERROR, "FadasIface DSP Init failed" );
        return false;
    }

    m_Handle64 = FadasSrv::GetRemoteHandle64( g_CoreIdConversion[m_Processor] );

    for ( int i = 0; i < m_BatchSize; i++ )
    {
        auto inputFormat = m_InputFormats[i];
        auto pipeline = getPipeline( inputFormat );
        if ( FADAS_REMAP_PIPELINE_MAX_NSP == pipeline )
        {
            return false;
        }
        auto it = m_WorkersMap.find( inputFormat );
        if ( m_WorkersMap.end() == it )
        {
            uint64 workerPtr;
            auto retval =
                    FadasIface_FadasRemap_CreateWorkers( m_Handle64, &workerPtr, 4, pipeline );
            if ( AEE_SUCCESS != retval )
            {
                // hogl::post( m_HoglArea, m_HoglArea->ERROR,"Failed to create a remap worker for
                // DSP" );
                return false;
            }
            m_WorkerPtrs.push_back( workerPtr );
            m_WorkersMap[inputFormat] = workerPtr;
        }
        else
        {
            m_WorkerPtrs.push_back( it->second );
        }
    }

    if ( true == m_bEnableUndistortion )
    {
        ret = creatRemapTables( pConfig );
        if ( false == ret )
        {
            return false;
        }
    }
    else
    {
        for ( int i = 0; i < m_BatchSize; i++ )
        {
            auto inputWidth = m_InputWidths[i];
            auto inputHeight = m_InputHeights[i];
            auto mapWidth = pConfig->inputConfigs[i].mapWidth;
            auto mapHeight = pConfig->inputConfigs[i].mapHeight;
            auto inputFormat = m_InputFormats[i];
            auto pipeline = getPipeline( inputFormat );
            uint64 remapPtr = 0;
            auto retVal = FadasIface_FadasRemap_CreateMapNoUndistortion(
                    m_Handle64, &remapPtr, inputWidth, inputHeight, mapWidth, mapHeight, pipeline,
                    0 );
            if ( AEE_SUCCESS != retVal )
            {
                // hogl::post( m_HoglArea, m_HoglArea->ERROR,"Failed to create a remap map no
                // undistortion for DSP" );
                return false;
            }
            m_RemapPtrs.push_back( remapPtr );
        }
    }

    return true;
}

void RemapDSP::kill()
{
    FadasSrv::DeInitialize( g_CoreIdConversion[m_Processor] );
}

RideHalError_e RemapDSP::remap( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *outputs )
{
    FadasIface_FadasImgProps_t dstImgProp;
    dstImgProp.width = m_OutputWidth;
    dstImgProp.height = m_OutputHeight;
    dstImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
    dstImgProp.stride[0] = 3 * m_OutputWidth;
    dstImgProp.numPlanes = 1;

    std::vector<int32_t> srcFds;
    for ( int i = 0; i < m_BatchSize; i++ )
    {
        void *input = inputs[i].data();
        int32_t srcFd = FadasSrv::RegBuf( g_CoreIdConversion[m_Processor], input, m_InputSizes[i] );
        if ( srcFd < 0 )
        {
            return RIDE_HAL_ERROR_INVALID_BUF;
        }
        srcFds.push_back( srcFd );
    }

    void *output = outputs[0].data();
    int32_t dstFd = FadasSrv::RegBuf( g_CoreIdConversion[m_Processor], output, m_OutputSize, 0,
                                      m_BatchSize );
    if ( dstFd < 0 )
    {
        return RIDE_HAL_ERROR_INVALID_BUF;
    }

    AEEResult retV;
    if ( false == m_bEnableNormalize )
    {
        retV = FadasIface_FadasRemap_RunMT(
                m_Handle64, m_WorkerPtrs.data(), m_WorkerPtrs.size(), m_RemapPtrs.data(),
                m_RemapPtrs.size(), srcFds.data(), srcFds.size(), m_Offsets.data(),
                m_Offsets.size(), m_SrcImgProps.data(), m_SrcImgProps.size(), dstFd, m_OutputSize,
                &dstImgProp, m_ROIs, m_BatchSize, nullptr, 0 );
    }
    else
    {
        retV = FadasIface_FadasRemap_RunMT(
                m_Handle64, m_WorkerPtrs.data(), m_WorkerPtrs.size(), m_RemapPtrs.data(),
                m_RemapPtrs.size(), srcFds.data(), srcFds.size(), m_Offsets.data(),
                m_Offsets.size(), m_SrcImgProps.data(), m_SrcImgProps.size(), dstFd, m_OutputSize,
                &dstImgProp, m_ROIs, m_BatchSize, m_Normlz, 3 );
    }

    if ( retV != AEE_SUCCESS )
    {
        // hogl::post( m_HoglArea, m_HoglArea->ERROR, "Remap888 failed: ret = 0x%x", retV );
        return RIDE_HAL_ERROR_FAIL;
    }

    return RIDE_HAL_ERROR_NONE;
}

Remap::Remap() {}

Remap::~Remap() {}

RideHalError_e Remap::Start()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    return ret;
}

RideHalError_e Remap::Stop()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    return ret;
}

RideHalError_e Remap::Init( const char *pName, const Remap_Config_t *pConfig, Logger *pLogger )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( nullptr != pConfig )
    {
        m_Config = *pConfig;
    }
    else
    {
        return RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( REMAP_PROCESSOR_CPU == m_Config.processor )
    {
        m_Impl = std::make_unique<RemapCPU>();
    }
    else if ( ( REMAP_PROCESSOR_DSP0 == m_Config.processor ) ||
              ( REMAP_PROCESSOR_DSP1 == m_Config.processor ) )
    {
        m_Impl = std::make_unique<RemapDSP>();
    }
    else
    {
        return RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( !m_Impl->init( &m_Config ) )
    {
        return RIDE_HAL_ERROR_UNKNOWN;
    }

    size_t outputSize = m_Config.numOfInputs * m_Config.outputWidth * m_Config.outputHeight * 3;

    for ( int i = 0; i < m_Config.numOfInputs; i++ )
    {
        auto inputFormat = m_Config.inputConfigs[i].inputFormat;
        size_t inputSize;
        auto width = m_Config.inputConfigs[i].inputWidth;
        auto height = m_Config.inputConfigs[i].inputHeight;
        if ( RIDE_HAL_IMAGE_FORMAT_UYVY == inputFormat )
        {
            inputSize = height * ALIGN_128( width * 2 );
        }
        else if ( RIDE_HAL_IMAGE_FORMAT_RGB888 == inputFormat )
        {
            inputSize = height * ALIGN_128( width * 3 );
        }
        m_InputSizes[i] = inputSize;
    }

    return ret;
}

RideHalError_e Remap::Deinit()
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;
    return ret;
}

RideHalError_e Remap::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                               const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs )
{
    RideHalError_e ret = RIDE_HAL_ERROR_NONE;

    if ( numInputs != m_Config.numOfInputs )
    {
        ret = RIDE_HAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDE_HAL_ERROR_NONE == ret )
    {
        ret = m_Impl->remap( pInputs, pOutputs );
    }

    return ret;
}

}   // namespace component
}   // namespace hal
}   // namespace ride