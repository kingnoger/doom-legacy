// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2003 by DooM Legacy Team.
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
//
// $Log$
// Revision 1.4  2003/03/23 14:24:13  smite-meister
// Polyobjects, MD3 models
//
// Revision 1.3  2003/03/15 20:07:21  smite-meister
// Initial Hexen compatibility!
//
// Revision 1.2  2002/12/16 22:04:58  smite-meister
// Actor / DActor separation done!
//
// Revision 1.1.1.1  2002/11/16 14:18:24  hurdler
// Initial C++ version of Doom Legacy
//
//
// DESCRIPTION:
//   Menu widget stuff, episode selection and such.
//    
//-----------------------------------------------------------------------------


#ifndef m_menu_h
#define m_menu_h 1

#include "doomtype.h"

//
// MENUS
//

struct consvar_t;
struct event_t;
struct patch_t;
struct sfxinfo_t;

// flags for items in the menu
typedef enum
{
  // Group 1, 4 bits: menuitem action (what we do when a key is pressed)
  IT_SPACE          = 0x00,  // no handling
  IT_CALL           = 0x01,  // <enter>: call routine with menuitem number as param
  IT_CONTROL        = 0x02,  // like IT_CALL, but swallows also <backspace> to erase values
  IT_ARROWS         = 0x03,  // call routine with param 0 for <left> and 1 for <right> or <enter>
  IT_KEYHANDLER     = 0x04,  // call routine with the key as param (except <esc>, which always exits)

  // TODO this is badly designed, we should make a real textbox instead
  IT_BAD_KEYHANDLER = 0x05,  // call routine with the key as param (except <up>, <down>, <enter> and <esc>)

  IT_CVAR           = 0x06,  // <left>, <right> and <enter> change cvar, string cvar works more strangely
  IT_SUBMENU        = 0x07,  // <enter>: go to sub menu given by "routine"

  IT_TYPE_MASK      = 0x000F,

  // Group 2, 4 bits: display type
  IT_NOTHING        = 0x00,   // big space
  IT_DYBIGSPACE     = 0x10,   // same as nothing
  IT_PATCH          = 0x20,   // a patch or a string with big font
  IT_GRAYPATCH      = 0x30,   // grayed patch or big font string

  IT_STRING         = 0x40,   // little string (spaced with 10)
  IT_WHITESTRING    = 0x50,   // little string in white
  IT_STRING2        = 0x60,   // a simple string
  IT_DYLITLSPACE    = 0x70,   // little space

  IT_BIGSLIDER      = 0x80,   // sound volume uses this

  IT_DISPLAY_MASK   = 0x00F0,

  // Group 3: consvar specific
  IT_CV_NORMAL      = 0x000,
  IT_CV_SLIDER      = 0x100,
  IT_CV_STRING      = 0x200,
  IT_CV_NOMOD       = 0x300,
  IT_CV_NOPRINT     = 0x400,

  IT_CVAR_MASK      = 0x0F00,

  // in short for some common uses
  IT_BIGSPACE    = IT_SPACE + IT_DYBIGSPACE,
  IT_LITLSPACE   = IT_SPACE + IT_DYLITLSPACE,
  IT_CVARMAX     = IT_CVAR  + IT_CV_NOMOD,
  IT_DISABLED    = IT_SPACE + IT_GRAYPATCH,
  IT_CONTROLSTR  = IT_CONTROL + IT_STRING2
} menuflag_t;


class Menu;

typedef void (*menufunc_t)(int choice);


union itemaction_t
{
  consvar_t  *cvar;     // IT_CVAR
  Menu       *submenu;  // IT_SUBMENU
  menufunc_t  routine;  // IT_CALL, IT_KEYHANDLER, IT_ARROWS
};

struct menuitem_t
{
  // show IT_xxx
  short  flags;

  char  *patch;
  char  *text;  // used when FONTBxx lump is found

  //void      *itemaction;
 itemaction_t itemaction;

  // hotkey in menu
  // or y of the item 
  byte alphaKey;
};



typedef enum 
{
  MM_NOTHING = 0,   // is just displayed until the user do someting
  MM_YESNO,         // routine is called with only 'y' or 'n' in param
  MM_EVENTHANDLER   // the same of above but without 'y' or 'n' restriction
                    // and routine is void routine(event_t *) (ex: set control)
} menumessage_t;

typedef bool (* eventhandler_t)(event_t *ev);

class MsgBox
{
  friend class Menu;
private:
  bool  active;
  int   x, y;
  int   rows, columns;
  char *text;
  menumessage_t type;
  union
  {
    menufunc_t     routine;
    eventhandler_t handler;
  } func;
public:

  // shows a message in a box
  void Set(const char *str, menufunc_t routine, menumessage_t btype);
  void SetHandler(const char *str, eventhandler_t routine);
  void SetText(const char *str);
  void Draw();
  void Close();
  bool Active() { return active; };
};



class Menu
{
  friend class MsgBox;
private:
  // video and audio resources
  // FIXME make a real font system, fix v_video.cpp (text output)
  static int     menufontbase;
  static patch_t **smallfont;

  static short AnimCount;  // skull animation counter
  static short whichSkull; // which skull to draw
  static int   SkullBaseLump;


  // currently active menu
  static Menu  *currentMenu;
  // menu item that is highlighted
  static short  itemOn; 

  typedef void (Menu::* drawfunc_t)();
  typedef bool (* quitfunc_t)();


private:
  Menu        *parent;      // previous menu
  short        x, y;        // menu coords
  const char  *titlepic;    // title patch name
  const char  *title;       // title as string for display with bigfont if present
  short        numitems;    // # of menu items
  menuitem_t  *items;       // array of menu items
  short        lastOn;      // last active item
  drawfunc_t   drawroutine; // draw routine
  quitfunc_t   quitroutine; // called before quit a menu return true if we can

public:
  // sounds
  static char *s_fail;   // disappointment
  static char *s_adjust; // a slider moves or a consvar is changed
  static char *s_move;   // move up/down in a menu
  static char *s_choose; // a choice is made
  static char *s_open;   // start something
  static char *s_close;  // exit or end something


  static bool active; // menu is currently open

  Menu(const char *tp, const char *t, int ni, Menu *up,
       menuitem_t *it, drawfunc_t df, short mx, short my,
       short on = 0, quitfunc_t qf = NULL);

  // opens the menu
  static void Open();

  // closes the menu
  static void Close(bool callexitmenufunc);

  // tics the menu (skull cursor) animation and video mode testing
  static void Ticker();

  // eats events
  static bool Responder(event_t *ev);

  // starts up the menu system
  static void Startup();

  // resets the menu system according to current game.mode
  static void Init();

  // draws the menus directly into the screen buffer.
  static void Drawer();

  // changes menu node
  static void SetupNextMenu(Menu *m);

  // the actual drawing
  void DrawMenuTitle();
  void DrawGenericMenu();

  void HereticMainMenuDrawer();
  void HexenMainMenuDrawer();
  void DrawClassMenu();
  void DrawConnectMenu();
  void DrawSetupPlayerMenu();
  void DrawVideoMode();
  void DrawReadThis1();
  void DrawReadThis2();
  void DrawSave();
  void DrawLoad();
  void DrawControl();
  void DrawOpenGLMenu();
  void OGL_DrawFogMenu();
  void OGL_DrawColorMenu();
};

// opens the menu and creates a message box
void M_StartMessage(const char *str, menufunc_t f, menumessage_t boxtype);

// Draws a box with a texture inside as background for messages
void M_DrawTextBox (int x, int y, int width, int lines);
// show or hide the setup for player 2 (called at splitscreen change)
void M_SwitchSplitscreen();

// Called by linux_x/i_video_xshm.c
void M_QuitResponse(int ch);

extern Menu menu; // main menu

#endif
