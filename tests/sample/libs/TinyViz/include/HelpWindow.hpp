//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef QRIDE_STACK_TINYVIZ_HELP_WINDOW_HPP
#define QRIDE_STACK_TINYVIZ_HELP_WINDOW_HPP

#include <vector>

#include "TextRenderer.hpp"

namespace QRide
{
namespace Stack
{

class HelpWindowInfo
{
public:
    std::vector<TextInfo> textInfoList;
    SDL_Color color = COLOR_WHITE;
    SDL_Color bgColor = { 0x22, 0x22, 0x22, 0xCC };
    SDL_Color borderColor = COLOR_BLACK;

    SDL_Rect window = { 5, 5, 50, 50 };

    bool init( SDL_Renderer *ren, TTF_Font *font );
    bool render( SDL_Renderer *ren );
    bool renderF1( SDL_Renderer *ren );
    bool close();
};

}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_HELP_WINDOW_HPP
