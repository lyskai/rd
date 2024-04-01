//  Copyright 2020 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")
#ifndef QRIDE_STACK_TINYVIZ_IF_HPP
#define QRIDE_STACK_TINYVIZ_IF_HPP

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ridehal/sample/DataTypes.hpp"

using namespace ridehal::sample;

namespace QRide
{
namespace Stack
{

class TinyVizIF
{
public:
    TinyVizIF() = default;
    TinyVizIF( const TinyVizIF & ) = delete;
    TinyVizIF &operator=( const TinyVizIF & ) = delete;
    virtual ~TinyVizIF() = default;

    enum class PixelFormat
    {
        YUY2,
        UYVY,
        NV12,
        YV12,
        RGB
    };

    virtual bool init( PixelFormat format = PixelFormat::YUY2, uint32_t winW = 1920,
                       uint32_t winH = 1024 ) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual bool addCamera( const std::string camName, uint32_t width, uint32_t height ) = 0;
    virtual bool addData( const std::string camName, CamFrame_t & ) = 0;
#if 0
    virtual bool addData( const std::string camName, DataTypes::LaneBoundary & ) = 0;
    virtual bool addData( const std::string camName, DataTypes::RoadDelimiter & ) = 0;
    virtual bool addData( const std::string camName, DataTypes::RoadSurface & ) = 0;
    virtual bool addData( const std::string camName, DataTypes::RoadObjects & ) = 0;
    virtual bool addData( const std::string camName, DataTypes::TrafficSign & ) = 0;
#endif
};
}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_IF_HPP
