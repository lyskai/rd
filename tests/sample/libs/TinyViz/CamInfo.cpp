//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "CamInfo.hpp"
#include "Macros.hpp"

namespace ridehal
{
namespace sample
{

CamInfo::CamInfo() : mutex( new std::mutex )
{
    camFrame.timestamp = 0;
}

CamInfo::CamInfo( std::string _camName )
    : camName( _camName ),
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

uint32_t CamInfo::width()
{
    uint32_t w = 0;

    if ( nullptr != camFrame.buffer )
    {
        w = camFrame.buffer->sharedBuffer.imgProps.width;
    }

    return w;
}

uint32_t CamInfo::height()
{
    uint32_t h = 0;

    if ( nullptr != camFrame.buffer )
    {
        h = camFrame.buffer->sharedBuffer.imgProps.actualHeight[0];
    }

    return h;
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

RideHal_ImageFormat_e CamInfo::format()
{
    RideHal_ImageFormat_e fmt = RIDEHAL_IMAGE_FORMAT_MAX;

    if ( nullptr != camFrame.buffer )
    {
        fmt = camFrame.buffer->sharedBuffer.imgProps.format;
    }

    return fmt;
}


}   // namespace sample
}   // namespace ridehal
