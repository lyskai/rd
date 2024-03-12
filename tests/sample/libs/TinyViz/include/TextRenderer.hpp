//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef QRIDE_STACK_TINYVIZ_TEXT_RENDERER_HPP
#define QRIDE_STACK_TINYVIZ_TEXT_RENDERER_HPP

#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Macros.hpp"
#include <hogl/area.hpp>

namespace QRide
{
namespace Stack
{

class TextInfo
{
public:
    TextInfo();
    TextInfo( std::string const &_text, SDL_Color _color );
    TextInfo( TextInfo && ) = default;

    bool init( SDL_Renderer *ren, TTF_Font *font );
    bool close();

    size_t getWidth() { return m_Width; }
    size_t getHeight() { return m_Height; }
    SDL_Texture *getTexture() { return m_Texture; }

private:
    SDL_Surface *m_Surface = nullptr;
    SDL_Texture *m_Texture = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    SDL_Color m_Color = COLOR_WHITE;
    std::string m_Text;
    hogl::area *m_HoglArea = nullptr;
};

class NumTextCollect
{
public:
    NumTextCollect() = default;
    bool init( uint32_t range, SDL_Renderer &ren, TTF_Font &font );
    bool release();
    TextInfo *getTextInfo( uint32_t idx, hogl::area *area );

private:
    std::vector<TextInfo> m_NumTextInfos;
};

}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_TEXT_RENDERER_HPP
