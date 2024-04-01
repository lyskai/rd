//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "CamInfo.hpp"
#include "Macros.hpp"

namespace QRide
{
namespace Stack
{

CamInfo::CamInfo() : mutex( new std::mutex )
{
    camFrame.timestamp = 0;
}

CamInfo::CamInfo( std::string _camName, uint32_t _width, uint32_t _height )
    : camName( _camName ),
      width( _width ),
      height( _height ),
      camNameText( camName, COLOR_WHITE ),
      statusBarText( "FPS:{ Cam:       /s, ROD:        /s, LAN:        /s, TFS:        /s }",
                     COLOR_LIGHTGRAY ),
      mutex( new std::mutex )
{
    camFrame.timestamp = 0;
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
    uint8_t *pData = nullptr;
    if ( nullptr != camFrame.buffer )
    {
        pData = (uint8_t *) camFrame.buffer->sharedBuffer.data();
    }
    return pData;
}

size_t CamInfo::size()
{
    size_t sz = 0;
    if ( nullptr != camFrame.buffer )
    {
        sz = camFrame.buffer->sharedBuffer.size;
    }
    return sz;
}

uint32_t CamInfo::stride()
{
    uint32_t st = 0;

    if ( nullptr != camFrame.buffer )
    {
        st = camFrame.buffer->sharedBuffer.imgProps.stride[0];
    }

    return st;
}


}   // namespace Stack
}   // namespace QRide
