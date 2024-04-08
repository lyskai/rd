//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef RIDEHAL_SAMPLE_TINYVIZ_HELP_WINDOW_HPP
#define RIDEHAL_SAMPLE_TINYVIZ_HELP_WINDOW_HPP

#include <vector>

#include "TextRenderer.hpp"

namespace ridehal
{
namespace sample
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

}   // namespace sample
}   // namespace ridehal

#endif   // #ifndef RIDEHAL_SAMPLE_TINYVIZ_HELP_WINDOW_HPP
