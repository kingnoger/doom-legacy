// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2007 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Font system.

#include "console.h"
#include "g_game.h"

#include "r_data.h"
#include "screen.h"
#include "v_video.h"
#include "i_video.h"

#include "w_wad.h"
#include "z_zone.h"


#define NO_TTF 1 // TrueType fonts off by default


#define HU_FONTSTART    '!'     // the first font character
#define HU_FONTEND      '_'     // the last font character
#define HU_FONTSIZE (HU_FONTEND - HU_FONTSTART + 1) // default font size

// Doom:
// STCFN033-95 + 121 : small red font

// Heretic:
// FONTA01-59 : medium silver font
// FONTB01-58 : large green font, some symbols empty

// Hexen:
// FONTA01-59 : medium silver font
// FONTAY01-59 : like FONTA but yellow
// FONTB01-58 : large red font, some symbols empty

font_t *hud_font;
font_t *big_font; // TODO used width-1 instead of width...


font_t::~font_t()
{}


//======================================================================
//                          RASTER FONTS
//======================================================================

/// \brief Doom-style raster font
class rasterfont_t : public font_t
{
protected:
  char  start, end;     ///< first and last ASCII characters included in the font
  std::vector<class Material*> font; ///< one Material per symbol

public:
  rasterfont_t(int startlump, int endlump, char firstchar = '!');
  virtual ~rasterfont_t();

  /// Write a single character (draw WHITE if bit 7 set)
  virtual float DrawCharacter(float x, float y, char c, int flags);
  /// Write a string using the font.
  virtual float DrawString(float x, float y, const char *str, int flags);
  /// Returns the width of the string in unscaled pixels
  virtual float StringWidth(const char *str);
  virtual float StringWidth(const char *str, int n);
};



rasterfont_t::rasterfont_t(int startlump, int endlump, char firstchar)
{
  if (startlump < 0 || endlump < 0)
    I_Error("Incomplete font!\n");

  int truesize = endlump - startlump + 1; // we have this many lumps
  char lastchar = firstchar + truesize - 1;

  // the font range must include '!' and '_'. We will duplicate letters if need be.
  start = min(firstchar, '!');
  end = max(lastchar, '_');
  int size = end - start + 1;

  font.resize(size);

  for (int i = start; i <= end; i++)
    if (i < firstchar || i > lastchar)
      // replace the missing letters with the first char
      font[i - start] = materials.GetLumpnum(startlump);
    else
      font[i - start] = materials.GetLumpnum(i - firstchar + startlump);

  // use the character '0' as a "prototype" for the font
  if (start <= '0' && '0' <= end)
    {
      height = font['0' - start]->worldheight + 1;
      width = font['0' - start]->worldwidth;
    }
  else
    {
      height = font[0]->worldheight + 1;
      width = font[0]->worldwidth;
    }
}


rasterfont_t::~rasterfont_t()
{
  // TODO release font materials
}


// Writes a single ASCII character
float rasterfont_t::DrawCharacter(float x, float y, char c, int flags)
{
  if (flags & V_WHITEMAP)
    {
      current_colormap = whitemap;
      flags |= V_MAP;
    }

  c = toupper(c & 0x7f); // just ASCII
  if (c < start || c > end)
    return 0;

  Material *m = font[c - start];
  m->Draw(x, y, flags);

  return m->worldwidth;
}



//  Draw a string using the font
//  NOTE: the text is centered for screens larger than the base width
float rasterfont_t::DrawString(float x, float y, const char *str, int flags)
{
  if (flags & V_WHITEMAP)
    {
      current_colormap = whitemap;
      flags |= V_MAP;
    }

  float dupx = 1;
  float dupy = 1;

  if (rendermode == render_opengl)
    {
      if (flags & V_SSIZE)
	{
	  dupx = vid.fdupx;
	  dupy = vid.fdupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.fdupx;
	  y *= vid.fdupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}
    }
  else
    {
      if (flags & V_SSIZE)
	{
	  dupx = vid.dupx;
	  dupy = vid.dupy;
	}

      if (flags & V_SLOC)
	{
	  x *= vid.dupx;
	  y *= vid.dupy;
	  flags &= ~V_SLOC; // not passed on to Texture::Draw
	}
    }

  // cursor coordinates
  float cx = x;
  float cy = y;
  float rowheight = height * dupy;

  while (1)
    {
      int c = *str++;
      if (!c)
        break;

      if (c == '\n')
        {
          cx = x;
	  cy += rowheight;
          continue;
        }

      c = toupper(c);
      if (c < start || c > end)
        {
          cx += 4*dupx;
          continue;
        }

      Material *m = font[c - start];
      m->Draw(cx, cy, flags);

      cx += m->worldwidth * dupx;
    }

  return cx - x;
}


// returns the width of the string (unscaled)
float rasterfont_t::StringWidth(const char *str)
{
  float w = 0;

  for (int i = 0; str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}


// returns the width of the next n chars of str
float rasterfont_t::StringWidth(const char *str, int n)
{
  float w = 0;

  for (int i = 0; i<n && str[i]; i++)
    {
      int c = toupper(str[i]);
      if (c < start || c > end)
        w += 4;
      else
        w += font[c - start]->worldwidth;
    }

  return w;
}





//======================================================================
//                         TRUETYPE FONTS
//======================================================================

#ifndef NO_TTF
#include "SDL_ttf.h"

/// \brief TrueType font
class ttfont_t : public font_t
{
protected:
  TTF_Font *font;

public:
  ttfont_t(int lump)
  {
    byte *data = static_cast<byte*>(fc.CacheLumpNum(lump, PU_DAVE));
    font = TTF_OpenFontRW(SDL_RWFromConstMem(data, fc.LumpLength(lump)), true, 16);
    Z_Free(data);

    height = TTF_FontLineSkip(font); // monospace the font in y direction
    width = 0;
  }

  virtual ~ttfont_t()
  {
    TTF_CloseFont(font);
    font = NULL;
  }

  const char *GetFaceFamilyName() { return TTF_FontFaceFamilyName(font); }
  const char *GetFaceStyleName() { return TTF_FontFaceStyleName(font); }

  /// Returns the width of the UTF-8 string in unscaled pixels.
  virtual float StringWidth(const char *str)
  {
    int w, h;
    return TTF_SizeUTF8(font, str, &w, &h) ? 0 : w;
  }

  /// Returns the width of the first n characters of an UTF-8 string in unscaled pixels.
  virtual float StringWidth(const char *str, int n)
  {
    char temp[n+1];
    strncpy(temp, str, n);
    temp[n] = 0;
    int w, h;
    return TTF_SizeUTF8(font, temp, &w, &h) ? 0 : w;
  }

  /// Write a single ASCII character, return width
  virtual float DrawCharacter(float x, float y, char c, int flags)
  {
    char temp[2] = {c, 0};
    return DrawString(x, y, temp, flags);
    //TTF_RenderGlyph_Solid(font, c, color);
  }

  /// Write an UTF-8 string using the font, return string width.
  virtual float DrawString(float x, float y, const char *str, int flags)
  {
    SDL_Color c = {255, 255, 255}; // white
    SDL_Surface *text = TTF_RenderUTF8_Solid(font, str, c);
    //SDL_Surface *text32 = TTF_RenderUTF8_Blended(font, str, c);
    if (!text)
      {
	CONS_Printf("TTF rendering error: %s\n", TTF_GetError());
	return 0;
      }

    float w = text->w;
    SDL_Rect dest = {int(x), int(y)};
    SDL_BlitSurface(text, NULL, vidSurface, dest);
    SDL_FreeSurface(text);
    return w;
  }
};
#endif




// initialize the font system, load the fonts
void font_t::Init()
{
  int startlump, endlump;
  // TODO add legacy default font (in legacy.wad) (truetype!)

  // "hud font"
  switch (game.mode)
    {
    case gm_heretic:
    case gm_hexen:
      startlump = fc.GetNumForName("FONTA01");
      endlump  =  fc.GetNumForName("FONTA59");
      break;
    default:
      startlump = fc.GetNumForName("STCFN033");
      endlump =   fc.GetNumForName("STCFN095");
    }

  hud_font = new rasterfont_t(startlump, endlump);

  // "big font"
  if (fc.FindNumForName("FONTB_S") < 0)
    big_font = NULL;
  else
    {
      startlump = fc.FindNumForName("FONTB01");
      endlump   = fc.GetNumForName("FONTB58");
      big_font = new rasterfont_t(startlump, endlump);
    }

#ifndef NO_TTF
  if (TTF_Init() < 0)
    {
      CONS_Printf("TTF_Init error: %s\n", TTF_GetError());
      return;
    }

  ttfont_t *tt = new ttfont_t(fc.GetNumForName("TTF_TEST"));

  const char *p = tt->GetFaceFamilyName();
  if (p)
    CONS_Printf("TT font face family name: %s\n", p);

  p = tt->GetFaceStyleName();
  if (p)
    CONS_Printf("TT font face style name: %s\n", p);

  // TTF_Quit();
#endif
}
