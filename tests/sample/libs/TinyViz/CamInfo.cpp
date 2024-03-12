//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include <hogl/post.hpp>

#include "CamInfo.hpp"
#include "Macros.hpp"

namespace QRide
{
namespace Stack
{

CamInfo::CamInfo() : mutex( new std::mutex )
{
    CamFrame.timestamp = 0;
}

CamInfo::CamInfo( std::string _camName, uint32_t _width, uint32_t _height, uint32_t _pitch )
    : camName( _camName ),
      width( _width ),
      height( _height ),
      pitch( _pitch ),
      camNameText( camName, COLOR_WHITE ),
      statusBarText( "FPS:{ Cam:       /s, ROD:        /s, LAN:        /s, TFS:        /s }",
                     COLOR_LIGHTGRAY ),
      mutex( new std::mutex )
{
    CamFrame.timestamp = 0;
}

bool CamInfo::initText( SDL_Renderer *ren, TTF_Font *font )
{
    camNameText.init( ren, font );
    statusBarText.init( ren, font );
    return true;
}

bool CamInfo::closeText()
{
    camNameText.close();
    statusBarText.close();
    return true;
}

uint8_t *CamInfo::data()
{
    return (uint8_t *) CamFrame.buffer->sharedBuffer.data();
}

size_t CamInfo::size()
{
    return CamFrame.buffer->sharedBuffer.size;
}


}   // namespace Stack
}   // namespace QRide
