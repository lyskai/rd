//  Copyright 2020-2024 Qualcomm Technologies, Inc. All rights reserved.
//  Confidential & Proprietary - Qualcomm Technologies, Inc. ("QTI")

#include "TextRenderer.hpp"

#include <hogl/post.hpp>

namespace QRide
{
namespace Stack
{

TextInfo::TextInfo()
{
    m_HoglArea = hogl::add_area( "TINY_VIZ_TEXTINFO" );
}

TextInfo::TextInfo( std::string const &_text, SDL_Color _color )
    : m_Color( _color ),
      m_Text( _text )
{
    m_HoglArea = hogl::add_area( "TINY_VIZ_TEXTINFO" );
}

bool TextInfo::init( SDL_Renderer *ren, TTF_Font *font )
{

    if ( !ren || !font )
    {
        hogl::post( m_HoglArea, m_HoglArea->ERROR, "InitText() Found nullptr" );
        return false;
    }
    m_Surface = TTF_RenderText_Solid( font, m_Text.c_str(), m_Color );
    m_Texture = SDL_CreateTextureFromSurface( ren, m_Surface );
    SDL_QueryTexture( m_Texture, NULL, NULL, &m_Width, &m_Height );

    return true;
}

bool TextInfo::close()
{
    if ( m_Texture ) SDL_DestroyTexture( m_Texture );
    if ( m_Surface ) SDL_FreeSurface( m_Surface );

    return true;
}

bool NumTextCollect::init( uint32_t range, SDL_Renderer &ren, TTF_Font &font )
{
    for ( uint32_t idx = 0; idx < range; idx++ )
    {
        TextInfo info( std::to_string( idx / 10 ) + "." + std::to_string( idx % 10 ), COLOR_WHITE );
        info.init( &ren, &font );
        m_NumTextInfos.emplace_back( std::move( info ) );
    }

    // saftey guard for anything larger than [0..range]
    TextInfo info( "inv", COLOR_WHITE );
    info.init( &ren, &font );
    m_NumTextInfos.emplace_back( std::move( info ) );

    return true;
}

bool NumTextCollect::release()
{
    for ( auto &t : m_NumTextInfos ) t.close();

    return true;
}

TextInfo *NumTextCollect::getTextInfo( uint32_t idx, hogl::area *area )
{
    // the last entry is the over-boundary warning
    if ( idx >= m_NumTextInfos.size() - 1 )
    {
        hogl::post( area, area->ERROR, "NumTextCollect getTexture() index %u out-of-bound",
                    m_NumTextInfos.size() );
        return &m_NumTextInfos[m_NumTextInfos.size() - 1];
    }

    return &m_NumTextInfos[idx];
}

}   // namespace Stack
}   // namespace QRide
