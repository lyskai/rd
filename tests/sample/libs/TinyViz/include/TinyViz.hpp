//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#ifndef QRIDE_STACK_TINYVIZ_HPP
#define QRIDE_STACK_TINYVIZ_HPP
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "CamInfo.hpp"
#include "HelpWindow.hpp"
#include "TinyVizIF.hpp"

#include "ridehal/common/Logger.hpp"

using namespace ridehal::common;

namespace QRide
{
namespace Stack
{

class TinyViz : public TinyVizIF
{
public:
    TinyViz() = default;
    TinyViz( const TinyViz & ) = delete;
    TinyViz &operator=( const TinyViz & ) = delete;
    ~TinyViz() override = default;

    bool init( PixelFormat format = PixelFormat::YUY2, uint32_t winW = 1920,
               uint32_t winH = 1024 ) override;
    bool start() override;
    bool stop() override;

    bool addCamera( const std::string camName, uint32_t camW, uint32_t camH ) override;
    bool addData( const std::string camName, CamFrame_t & ) override;
#if 0
    bool addData( const std::string camName, DataTypes::LaneBoundary & ) override;
    bool addData( const std::string camName, DataTypes::RoadDelimiter & ) override;
    bool addData( const std::string camName, DataTypes::RoadSurface & ) override;
    bool addData( const std::string camName, DataTypes::RoadObjects & ) override;
    bool addData( const std::string camName, DataTypes::TrafficSign & ) override;
#endif

private:
    bool isCamSelected( size_t id );
    void rendererThread();
    void setAllCamActive();
    void printRendererInfo( SDL_Renderer *ren );
    bool renderBlack( SDL_Renderer *ren, SDL_Texture *tex2M, size_t idx );
    bool renderCam( CamInfo &camInfo, SDL_Renderer *ren, size_t idx = 0 );

    void renderFPS( SDL_Renderer *ren, const SDL_Rect &DestR, uint32_t xShift, float &fps );
    void updateFPS( const std::string &camName, std::deque<uint64_t> &queue, uint64_t timestamp );
    void updateFPS( CamInfo &camInfo );

    template<class DataType>
    void renderBB( const uint64_t targetPTS, const uint64_t historyWindow,
                   std::map<uint64_t, std::list<DataType>> &queue, SDL_Renderer *ren,
                   const SDL_Rect &DestR, const float scaleX, const float scaleY,
                   const SDL_Color &color, const bool closeLoop );

    std::map<std::string, CamInfo> m_CamInfoMap;
    std::vector<std::string> m_CamNameList;
    float m_LastRenderFPS = 20;

    NumTextCollect m_NumTextures;

    SDL_Window *m_Win = nullptr;

    uint32_t m_WindowW = 1920, m_WindowH = 1080;
    int m_WindowCol = 1;
    int m_WindowRow = 1;

    uint32_t m_PixelFormat = SDL_PIXELFORMAT_YUY2;

    size_t m_ActiveCamIDX = 0;
    size_t m_ActiveMultiViewIDX = 0;

    bool m_Stop = false;
    bool m_ShowHelp = false;
    bool m_PauseRenderer = false;
    bool m_EnableCamFPSCap = true;

    std::unique_ptr<std::thread> m_RendererThread;

    RIDEHAL_DECLARE_LOGGER();
};

}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_HPP
