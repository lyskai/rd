//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef QRIDE_STACK_TINYVIZ_CAM_INFO_HPP
#define QRIDE_STACK_TINYVIZ_CAM_INFO_HPP

#include <deque>
#include <mutex>
#include <vector>

#include "TextRenderer.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/sample/DataTypes.hpp"

using namespace ridehal::sample;

namespace QRide
{
namespace Stack
{

class CamInfo
{
public:
    CamInfo();   // dummy constructor is needed for map initialization
    CamInfo( std::string _camName, uint32_t width, uint32_t height, uint32_t pitch );
    CamInfo( const CamInfo &copy ) = default;
    CamInfo( CamInfo &&copy ) = default;
    CamInfo &operator=( const CamInfo &copy ) = default;
    CamInfo &operator=( CamInfo &&copy ) = default;

    bool initText( SDL_Renderer *ren, TTF_Font *font );
    bool closeText();
    void clearQueue();

    void setActive() { inactiveCount = 0; }
    bool isActive() { return inactiveCount < 30; }
    void markRendered() { inactiveCount++; }

    uint8_t *data();
    size_t size();

    std::string camName;
    SDL_Color color;
    uint32_t width = 1920;
    uint32_t height = 1020;
    uint32_t pitch = 1920 * 2;

    // text
    TextInfo camNameText;
    TextInfo statusBarText;

    uint64_t lastFPSUpdate = 0;
    std::deque<uint64_t> camPTSs;
    float lastCamFPS = 0;
    std::deque<uint64_t> lanPTSs;
    float lastLanFPS = 0;
    std::deque<uint64_t> rosPTSs;
    float lastRodFPS = 0;
    std::deque<uint64_t> tfsPTSs;
    float lastTfsFPS = 0;

    // For protecting display buffer & bbBoxes
    // wrap with unique_ptr is needed for map initialization (mutex is not move-able)
    std::unique_ptr<std::mutex> mutex;
    CamFrame_t CamFrame;

    SDL_Texture *tex2M = nullptr;

#if 0
    std::map<uint64_t, std::list<QRide::Stack::DataTypes::LaneBoundary>> laneBoundaryQueue;
    std::map<uint64_t, std::list<QRide::Stack::DataTypes::RoadObjects>> roadObjectQueue;
    std::map<uint64_t, std::list<QRide::Stack::DataTypes::TrafficSign>> trafficSignQueue;
#endif
private:
    size_t inactiveCount = 0;
};

}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_CAM_INFO_HPP
