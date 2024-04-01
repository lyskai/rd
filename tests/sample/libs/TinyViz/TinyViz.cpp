//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include <cmath>
#include <iostream>
#include <unistd.h>

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_thread.h>

#include "CamInfo.hpp"
#include "HelpWindow.hpp"
#include "TinyViz.hpp"
#include <array>
#include <chrono>
#include <unistd.h>

namespace QRide
{
namespace Stack
{

using namespace std::chrono_literals;
constexpr std::chrono::nanoseconds NSEC = 1s;
constexpr uint64_t NSEC_PER_SEC = NSEC.count();

std::unique_ptr<TinyVizIF> CreateTinyVizInstance()
{
    return std::make_unique<TinyViz>();
}

bool TinyViz::init( PixelFormat pixelFormat, uint32_t winW, uint32_t winH )
{
    RIDEHAL_LOGGER_INIT( "TINYVIZ", LOGGER_LEVEL_INFO );

    m_WindowW = winW;
    m_WindowH = winH;

    switch ( pixelFormat )
    {
        case PixelFormat::YUY2:
            m_PixelFormat = SDL_PIXELFORMAT_YUY2;
            break;
        case PixelFormat::UYVY:
            m_PixelFormat = SDL_PIXELFORMAT_UYVY;
            break;
        case PixelFormat::NV12:
            m_PixelFormat = SDL_PIXELFORMAT_NV12;
            break;
        case PixelFormat::YV12:
            m_PixelFormat = SDL_PIXELFORMAT_YV12;
            break;
        case PixelFormat::RGB:
            m_PixelFormat = SDL_PIXELFORMAT_RGB888;
            break;
        default:
            RIDEHAL_ERROR( "Found unsupported pixel format %d", static_cast<int>( pixelFormat ) );
            return false;
    }

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
    {
        RIDEHAL_ERROR( "SDL_Init Error: %s", SDL_GetError() );
        return false;
    }

    TTF_Init();

    // SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0" );

    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl" );

    return true;
}

bool TinyViz::start()
{
    m_Win = SDL_CreateWindow( "QRide TinyViz", 0, 0, m_WindowW, m_WindowH,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED );
    if ( !m_Win )
    {
        RIDEHAL_ERROR( "SDL_CreateWindow Error: %s", SDL_GetError() );
        return false;
    }

    m_RendererThread = std::make_unique<std::thread>( &TinyViz::rendererThread, this );

    // workaround: to wait for renderer initialization ( segfault otherwise )
    sleep( 2 );
    return true;
}

bool TinyViz::stop()
{
    m_Stop = true;
    if ( m_RendererThread ) m_RendererThread->join();

    m_CamInfoMap.clear();

    SDL_DestroyWindow( m_Win );
    TTF_Quit();
    SDL_Quit();

    RIDEHAL_INFO( "TinyViz stopped" );
    RIDEHAL_LOGGER_DEINIT();

    return true;
}

bool TinyViz::addCamera( const std::string camName, uint32_t width, uint32_t height )
{
    m_CamInfoMap.emplace( camName, CamInfo( camName, width, height ) );
    m_CamNameList.push_back( camName );

    // Give it an initial black background frame
    auto &camInfo = m_CamInfoMap[camName];

    RIDEHAL_INFO( "Added camera %s. resolution %ux%u", camName.c_str(), width, height );

    return true;
}

bool TinyViz::addData( const std::string camName, CamFrame_t &data )
{
    RIDEHAL_DEBUG( "Adding camFrame for %s. pubHandle %" PRIu64 ", timestamp %" PRIu64, camName,
                   data.buffer->pubHandle, data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> camInfoGuard( *camInfo.mutex );
    updateFPS( camName, camInfo.camPTSs, data.timestamp );

    camInfo.camFrame = std::move( data );
    camInfo.setActive();
    return true;
}
#if 0
bool TinyViz::addData( const std::string camName, DataTypes::LaneBoundary &data )
{
    RIDEHAL_DEBUG( "Adding lane boundary for %s. timestamp %" PRIu64,
                camName.c_str(), data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> guard( *camInfo.mutex );
    updateFPS( camName, camInfo.lanPTSs, data.timestamp );

    auto &entry = camInfo.laneBoundaryQueue[data.timestamp];
    entry.emplace_back( std::move( data ) );
    return true;
}

bool TinyViz::addData( const std::string camName, DataTypes::RoadDelimiter & )
{
    // TODO
    return true;
}

bool TinyViz::addData( const std::string camName, DataTypes::RoadSurface & )
{
    // TODO
    return true;
}

bool TinyViz::addData( const std::string camName, DataTypes::RoadObjects &data )
{
    RIDEHAL_DEBUG( "Adding road obj for %s. timestamp %" PRIu64,
                camName.c_str(), data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> guard( *camInfo.mutex );
    updateFPS( camName, camInfo.rosPTSs, data.timestamp );

    auto &entry = camInfo.roadObjectQueue[data.timestamp];
    entry.emplace_back( std::move( data ) );
    return true;
}

bool TinyViz::addData( const std::string camName, DataTypes::TrafficSign &data )
{
    RIDEHAL_DEBUG( "Adding traffic sign for %s. timestamp %" PRIu64,
                camName.c_str(), data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> guard( *camInfo.mutex );
    updateFPS( camName, camInfo.tfsPTSs, data.timestamp );

    auto &entry = camInfo.trafficSignQueue[data.timestamp];
    entry.push_back( data );
    return true;
}
#endif

bool TinyViz::isCamSelected( size_t id )
{
    // single-view
    if ( m_WindowCol == 1 )
    {
        if ( id != m_ActiveCamIDX ) return false;
        return true;
    }

    // multi-view
    return ( m_ActiveMultiViewIDX + 1 ) * 4 > id && id >= m_ActiveMultiViewIDX * 4;
}

void TinyViz::rendererThread()
{
    SDL_Renderer *ren =
            SDL_CreateRenderer( m_Win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( !ren )
    {
        RIDEHAL_ERROR( "SDL_CreateRenderer Error %s", SDL_GetError() );
        return;
    }

    printRendererInfo( ren );

    // Other textureacceess option: SDL_TEXTUREACCESS_STREAMING ( doesn't work quite as good based
    // on the test )
    constexpr size_t TexWidth = 1920;
    constexpr size_t TexHeight = 1020;
    SDL_Texture *tex2M =
            SDL_CreateTexture( ren, m_PixelFormat, SDL_TEXTUREACCESS_TARGET, TexWidth, TexHeight );
    if ( !tex2M )
    {
        RIDEHAL_ERROR( "SDL_CreateTextureFromSurface Error: %s", SDL_GetError() );
        return;
    }

    // init texts
    TTF_Font *font =
            TTF_OpenFont( "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 64 );
    if ( !font ) font = TTF_OpenFont( "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 64 );
    if ( !font ) font = TTF_OpenFont( "/data/LiberationSans-Regular.ttf", 64 );
    for ( auto &cInfo : m_CamInfoMap )
    {
        cInfo.second.initText( ren, font );
    }

    HelpWindowInfo helpWindow;
    helpWindow.init( ren, font );

    m_NumTextures.init( 400, *ren, *font );

    while ( !m_Stop )
    {
        SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xFF );
        // SDL_RenderClear( ren );

        bool isActive = false;

        if ( m_CamInfoMap.empty() )
        {
            SDL_Delay( 10000 );
            continue;
        }
        auto start = std::chrono::steady_clock::now();
        // TODO: mutex lock
        if ( m_WindowCol == 1 )
        {
            isActive = renderCam( m_CamInfoMap[m_CamNameList[m_ActiveCamIDX]], ren );
        }
        else
        {
            size_t multiViewIdxShift = m_ActiveMultiViewIDX * m_WindowCol * m_WindowRow;
            for ( size_t idx = 0; idx < (size_t) m_WindowCol * m_WindowRow; idx++ )
            {
                if ( ( idx + multiViewIdxShift ) >= m_CamNameList.size() )
                {
                    // no camera, render black
                    renderBlack( ren, tex2M, idx );
                    continue;
                }

                // update display buffer
                auto targetCamName = m_CamNameList[idx + multiViewIdxShift];
                auto &camInfo = m_CamInfoMap[targetCamName];
                isActive |= renderCam( camInfo, ren, idx );
            }
        }   // if( m_WindowCol == 1 )

        m_ShowHelp ? helpWindow.render( ren ) : helpWindow.renderF1( ren );

        if ( isActive && !m_PauseRenderer )
        {
            SDL_RenderPresent( ren );
        }
        auto end = std::chrono::steady_clock::now();
        float cost = (float) std::chrono::duration_cast<std::chrono::microseconds>( end - start )
                             .count() /
                     1000.0;
        RIDEHAL_DEBUG( "render cost %.2f ms", cost );
        m_LastRenderFPS = m_LastRenderFPS * 7 / 8 + 1000 / ( cost + 33 ) / 8;
        // TODO: mutex unlock

        // TODO: switch to usleep
        SDL_Delay( 33 );
    }

    m_NumTextures.release();
    helpWindow.close();
    for ( auto &cInfo : m_CamInfoMap )
    {
        cInfo.second.closeText();
    }
    TTF_CloseFont( font );

    SDL_DestroyTexture( tex2M );
    SDL_DestroyRenderer( ren );

    RIDEHAL_DEBUG( "TinyViz thread exited" );
}

void TinyViz::setAllCamActive()
{
    for ( auto &cInfo : m_CamInfoMap ) cInfo.second.setActive();
}

void TinyViz::printRendererInfo( SDL_Renderer *ren )
{
    SDL_RendererInfo rendererInfo;
    if ( SDL_GetRendererInfo( ren, &rendererInfo ) )
    {
        RIDEHAL_ERROR( "SDL_GetRenererInfo Error %s", SDL_GetError() );
        return;
    }

    bool printRendererInfo = false;
    if ( printRendererInfo )
    {
        RIDEHAL_INFO( "SDK_RendererInfo: name: %s, "
                      "renderer driver: %s"
                      "software: %d"
                      "accelerated: %d"
                      "presentvsync: %d",
                      rendererInfo.name, SDL_GetCurrentVideoDriver(),
                      ( rendererInfo.flags & SDL_RENDERER_SOFTWARE ),
                      ( rendererInfo.flags & SDL_RENDERER_ACCELERATED ),
                      ( rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC ) );
    }

    RIDEHAL_INFO( "Available video driver: " );
    for ( int idx = 0; idx < SDL_GetNumVideoDrivers(); idx++ )
    {
        std::string isSelected =
                std::string( SDL_GetVideoDriver( idx ) ) == SDL_GetCurrentVideoDriver()
                        ? " (selected)"
                        : "";
        RIDEHAL_INFO( "%d%s: %s", idx, isSelected, SDL_GetVideoDriver( idx ) );
    }

    RIDEHAL_INFO( "Available renderer driver: " );
    for ( int idx = 0; idx < SDL_GetNumRenderDrivers(); idx++ )
    {
        SDL_RendererInfo rendererInfoByIdx;
        SDL_GetRenderDriverInfo( idx, &rendererInfoByIdx );
        std::string isSelected =
                std::string( rendererInfoByIdx.name ) == rendererInfo.name ? " (selected)" : "";
        RIDEHAL_INFO( "%d%s: %s", idx, isSelected, rendererInfoByIdx.name );
    }
}

bool TinyViz::renderBlack( SDL_Renderer *ren, SDL_Texture *tex2M, size_t idx )
{
    SDL_Rect DestR;
    DestR.w = m_WindowW / m_WindowRow;
    DestR.h = m_WindowH / m_WindowCol;
    DestR.x = ( idx % m_WindowRow ) * DestR.w;
    DestR.y = ( idx / m_WindowRow ) * DestR.h;
    SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xFF );
    SDL_RenderFillRect( ren, &DestR );

    return true;
}

bool TinyViz::renderCam( CamInfo &camInfo, SDL_Renderer *ren, size_t idx )
{
    SDL_Rect DestR;
    DestR.w = m_WindowW / m_WindowRow;
    DestR.h = m_WindowH / m_WindowCol;
    DestR.x = ( idx % m_WindowRow ) * DestR.w;
    DestR.y = ( idx / m_WindowRow ) * DestR.h;

    if ( !camInfo.isActive() ) return false;

    SDL_Texture *tex2M = camInfo.tex2M;
    if ( nullptr == tex2M )
    {
        tex2M = SDL_CreateTexture( ren, m_PixelFormat, SDL_TEXTUREACCESS_TARGET, camInfo.width,
                                   camInfo.height );
        if ( !tex2M )
        {
            RIDEHAL_ERROR( "SDL_CreateTextureFromSurface Error: %s", SDL_GetError() );
            return false;
        }
        camInfo.tex2M = tex2M;
    }

    auto pts = camInfo.camFrame.timestamp;
    {
        std::lock_guard<std::mutex> guard( *camInfo.mutex );
        if ( nullptr == camInfo.data() )
        {
            RIDEHAL_DEBUG( "display buffer is not yet available. Skipped one frame." );
            return false;
        }


        SDL_UpdateTexture( tex2M, nullptr, camInfo.data(), camInfo.stride() );
    }

    if ( camInfo.lastFPSUpdate < pts - NSEC_PER_SEC / 2 )
    {
        updateFPS( camInfo );
        camInfo.lastFPSUpdate = pts;
    }

    // render frame
    DestR.w = m_WindowW / m_WindowRow;
    DestR.h = m_WindowH / m_WindowCol;
    DestR.x = ( idx % m_WindowRow ) * DestR.w;
    DestR.y = ( idx / m_WindowRow ) * DestR.h;
    SDL_RenderCopy( ren, tex2M, nullptr, &DestR );

    SDL_SetRenderDrawColor( ren, camInfo.color.r, camInfo.color.g, camInfo.color.b,
                            camInfo.color.a );

    {
        std::lock_guard<std::mutex> guard( *camInfo.mutex );
        float scaleX = static_cast<float>( m_WindowW ) / camInfo.width / m_WindowRow;
        float scaleY = static_cast<float>( m_WindowH ) / camInfo.height / m_WindowCol;

#if 0
        auto widow = 2.5 * camInfo.lastCamFPS /
                     ( ( camInfo.lastLanFPS > 0 ) ? camInfo.lastLanFPS : camInfo.lastCamFPS );
        auto historyWindow = NSEC_PER_SEC / m_LastRenderFPS * widow;
        renderBB( pts, historyWindow, camInfo.laneBoundaryQueue, ren, DestR, scaleX, scaleY,
                  QRide::Stack::COLOR_GREEN, false );

        widow = 2.5 * camInfo.lastCamFPS /
                ( ( camInfo.lastRodFPS > 0 ) ? camInfo.lastRodFPS : camInfo.lastCamFPS );
        historyWindow = NSEC_PER_SEC / m_LastRenderFPS * widow;
        renderBB( pts, historyWindow, camInfo.roadObjectQueue, ren, DestR, scaleX, scaleY,
                  QRide::Stack::COLOR_RED, true );

        widow = 2.5 * camInfo.lastCamFPS /
                ( ( camInfo.lastTfsFPS > 0 ) ? camInfo.lastTfsFPS : camInfo.lastCamFPS );
        historyWindow = NSEC_PER_SEC / m_LastRenderFPS * widow;
        renderBB( pts, historyWindow, camInfo.trafficSignQueue, ren, DestR, scaleX, scaleY,
                  QRide::Stack::COLOR_YELLOW, true );
#endif
        // render status bar
        SDL_Rect statusBarR = DestR;
        statusBarR.h = 25;
        statusBarR.y = DestR.y + DestR.h - statusBarR.h;

        SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_BLEND );
        SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xBB );
        SDL_RenderFillRect( ren, &statusBarR );

        SDL_SetRenderDrawColor( ren, 0xFF, 0xFF, 0xFF, 0xAA );
        SDL_RenderDrawRect( ren, &statusBarR );
        SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_NONE );

        // render cam name and static text
        SDL_Rect textR;
        float scale = 0.22;
        textR.w = camInfo.camNameText.getWidth() * scale;
        textR.h = camInfo.camNameText.getHeight() * scale;
        textR.x = DestR.x + 4;
        textR.y = DestR.y + DestR.h - 4 - textR.h;
        SDL_RenderCopy( ren, camInfo.camNameText.getTexture(), NULL, &textR );

        if ( m_EnableCamFPSCap )
        {
            // render cam name and static text
            textR.w = camInfo.statusBarText.getWidth() * scale;
            textR.h = camInfo.statusBarText.getHeight() * scale;
            textR.x = DestR.x + DestR.w - textR.w;
            textR.y = DestR.y + DestR.h - 4 - textR.h;
            SDL_RenderCopy( ren, camInfo.statusBarText.getTexture(), NULL, &textR );

            // render individual FPS
            renderFPS( ren, DestR, DestR.w - textR.w + 82, camInfo.lastCamFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 172, camInfo.lastRodFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 262, camInfo.lastLanFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 348, camInfo.lastTfsFPS );
        }
    }

    camInfo.markRendered();
    return true;
}

void TinyViz::renderFPS( SDL_Renderer *ren, const SDL_Rect &DestR, uint32_t xShfit, float &fps )
{
    // The FPS texture is pre-calculated with 400 entries
    auto *fpsTexture = m_NumTextures.getTextInfo( fps * 10 );
    if ( !fpsTexture ) return;

    // render fps
    SDL_Rect textR;
    float scale = 0.22;
    textR.w = fpsTexture->getWidth() * scale;
    textR.h = fpsTexture->getHeight() * scale;
    textR.x = DestR.x + xShfit;
    textR.y = DestR.y + DestR.h - textR.h - 4;

    SDL_RenderCopy( ren, fpsTexture->getTexture(), NULL, &textR );
}

void TinyViz::updateFPS( const std::string &camName, std::deque<uint64_t> &queue,
                         uint64_t timestamp )
{
    // note: there could be multiple objects belong to the same timestamp
    if ( queue.empty() || queue[queue.size() - 1] != timestamp ) queue.push_back( timestamp );
    if ( queue.size() > 100 ) queue.pop_front();

    RIDEHAL_DEBUG( "Updated %s FPS info. queue size now %zu. timestamp:[%" PRIu64 ", %" PRIu64 "]",
                   camName, queue.size(), queue[queue.size() - 1], queue[0] );
}

void TinyViz::updateFPS( CamInfo &camInfo )
{
    constexpr size_t SIZE = 4;
    std::array<const std::deque<uint64_t> *, SIZE> queues{ &camInfo.camPTSs, &camInfo.lanPTSs,
                                                           &camInfo.rosPTSs, &camInfo.tfsPTSs };
    std::array<float *, SIZE> FPSs{ &camInfo.lastCamFPS, &camInfo.lastLanFPS, &camInfo.lastRodFPS,
                                    &camInfo.lastTfsFPS };

    for ( size_t idx = 0; idx < SIZE; idx++ )
    {
        auto &queue = *( queues[idx] );
        *FPSs[idx] = queue.empty() ? 0
                                   : queue.size() / ( ( queue[queue.size() - 1] - queue[0] ) /
                                                      static_cast<double>( NSEC_PER_SEC ) );

        RIDEHAL_DEBUG( "%s: queue size:%zu, timestamp (%" PRIu64 ", %" PRIu64 "), ",
                       camInfo.camName, queue.size(), queue.empty() ? 0 : queue[queue.size() - 1],
                       queue.empty() ? 0 : queue[0] );
    }
}

template<class DataType>
void TinyViz::renderBB( const uint64_t targetPTS, const uint64_t historyWindow,
                        std::map<uint64_t, std::list<DataType>> &queue, SDL_Renderer *ren,
                        const SDL_Rect &DestR, const float scaleX, const float scaleY,
                        const SDL_Color &color, const bool closeLoop )
{
    // move to the closest entry
    while ( queue.size() >= 2 && std::next( queue.begin() )->first <= targetPTS )
        queue.erase( queue.begin() );

    // remove entry if outdated
    if ( queue.size() == 1 && queue.begin()->first < ( targetPTS - historyWindow ) ) queue.clear();

    if ( queue.empty() || queue.begin()->first > targetPTS ) return;

    auto &mapItem = *queue.begin();
    for ( auto &l : mapItem.second )
    {
        auto &payload = l.payload();
        if ( payload.vertexCount > payload.MAX_VERTEX_SIZE )
        {
            RIDEHAL_ERROR( "Found invalid vertexCount at renderBB(). Max:%zu. vertexCount: %u",
                           payload.MAX_VERTEX_SIZE, payload.vertexCount );
            continue;
        }

        RIDEHAL_DEBUG( "renderBB: box %u vertexCount: %u : %f %f %f %f", closeLoop,
                       payload.vertexCount, payload.vertices[0], payload.vertices[1],
                       payload.vertices[2], payload.vertices[3] );

        std::vector<SDL_Point> points;
        for ( size_t p = 0; p < payload.vertexCount; p += 2 )
        {
            SDL_Point point;
            point.x = payload.vertices[p] * scaleX + DestR.x;
            point.y = payload.vertices[p + 1] * scaleY + DestR.y;
            points.push_back( point );
        }
        if ( closeLoop && payload.vertexCount >= 2 )
        {
            SDL_Point point;
            point.x = payload.vertices[0] * scaleX + DestR.x;
            point.y = payload.vertices[1] * scaleY + DestR.y;
            points.push_back( point );
        }

        glLineWidth( 4 / m_WindowCol );
        SDL_SetRenderDrawColor( ren, color.r, color.g, color.b, color.a );
        SDL_RenderDrawLines( ren, &points[0], points.size() );
        glLineWidth( 1 );
    }
}

}   // namespace Stack
}   // namespace QRide
