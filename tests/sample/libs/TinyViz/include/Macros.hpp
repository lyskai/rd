//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#ifndef QRIDE_STACK_TINYVIZ_MACROS_HPP
#define QRIDE_STACK_TINYVIZ_MACROS_HPP

#include <SDL2/SDL.h>

namespace QRide
{
namespace Stack
{

constexpr SDL_Color COLOR_BLACK = { 0, 0, 0, 0xFF };
constexpr SDL_Color COLOR_RED = { 0xFF, 0, 0, 0xFF };
constexpr SDL_Color COLOR_GREEN = { 0, 0xFF, 0, 0xFF };
constexpr SDL_Color COLOR_BLUE = { 0, 0, 0xFF, 0xFF };
constexpr SDL_Color COLOR_LIGHT_BLUE = { 0, 0xFF, 0xFF, 0xFF };
constexpr SDL_Color COLOR_DARK_GREEN = { 0, 0x88, 0, 0xFF };
constexpr SDL_Color COLOR_PURPLE = { 0xCC, 0x99, 0xFF, 0xFF };
constexpr SDL_Color COLOR_WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
constexpr SDL_Color COLOR_YELLOW = { 0xFF, 0xFF, 0, 0xFF };
constexpr SDL_Color COLOR_PINK = { 0xFF, 0, 0xFF, 0xFF };
constexpr SDL_Color COLOR_LIGHTGRAY = { 0xCC, 0xCC, 0xCC, 0xFF };

}   // namespace Stack
}   // namespace QRide

#endif   // #ifndef QRIDE_STACK_TINYVIZ_MACROS_HPP
