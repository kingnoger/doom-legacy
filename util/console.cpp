// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2004 by DooM Legacy Team.
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
// Revision 1.16  2004/08/12 18:30:33  smite-meister
// cleaned startup
//
// Revision 1.15  2004/07/25 20:17:05  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.14  2004/07/11 14:32:01  smite-meister
// Consvars updated, bugfixes
//
// Revision 1.13  2004/07/05 16:53:30  smite-meister
// Netcode replaced
//
// Revision 1.12  2004/04/25 16:26:51  smite-meister
// Doxygen
//
// Revision 1.10  2004/01/10 16:03:00  smite-meister
// Cleanup and Hexen gameplay -related bugfixes
//
// Revision 1.9  2003/12/31 18:32:50  smite-meister
// Last commit of the year? Sound works.
//
// Revision 1.8  2003/12/09 01:02:02  smite-meister
// Hexen mapchange works, keycodes fixed
//
// Revision 1.7  2003/05/05 00:24:50  smite-meister
// Hexen linedef system. Pickups.
//
// Revision 1.6  2003/04/24 20:30:33  hurdler
// Remove lots of compiling warnings
//
// Revision 1.5  2003/01/25 21:33:06  smite-meister
// Now compiles with MinGW 2.0 / GCC 3.2.
// Builder can choose between dynamic and static linkage.
//
// Revision 1.4  2002/12/29 18:57:03  smite-meister
// MAPINFO implemented, Actor deaths handled better
//
// Revision 1.3  2002/12/16 22:19:36  smite-meister
// HUD fix
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Console for Doom LEGACY

#include <stdarg.h>

#include "doomdef.h"
#include "dstrings.h"
#include "d_event.h"

#include "command.h"
#include "console.h"

#include "g_game.h"
#include "g_input.h"
#include "hu_stuff.h"
#include "keys.h"
#include "r_data.h"
#include "sounds.h"
#include "screen.h"

#include "v_video.h"
#include "i_video.h"
#include "z_zone.h"
#include "i_system.h"
#include "w_wad.h"

const char CON_PROMPTCHAR = '>';
const int  CON_BUFFERSIZE = 16384;

Console con;

//language_t con_language = la_english; /// TODO Language for game output strings
int con_keymap   = la_english; /// Keyboard layout

// communication with HUD
int  con_clearlines; // top screen lines to refresh when view reduced
bool con_hudupdate;  // when messages scroll, we need a backgrnd refresh

int  con_clipviewtop;// clip value for planes & sprites, so that the
                     // part of the view covered by the console is not
                     // drawn when not needed, this must be -1 when
                     // console is off

//bool  con_forcepic=true;  // at startup toggle console transulcency when
                             // first off


//======================================================================
//                 KEYBOARD LAYOUTS FOR ENTERING TEXT
//======================================================================

/// currently supported keyboard layouts
static char *keymap_names[2] = {"english", "french"};

static char azerty_shiftmap[] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '?', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  '0', // shift-0
  '1', // shift-1
  '2', // shift-2
  '3', // shift-3
  '4', // shift-4
  '5', // shift-5
  '6', // shift-6
  '7', // shift-7
  '8', // shift-8
  '9', // shift-9
  '/',
  '.', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};

static char qwerty_shiftmap[] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  ' ', '!', '"', '#', '$', '%', '&',
  '"', // shift-'
  '(', ')', '*', '+',
  '<', // shift-,
  '_', // shift--
  '>', // shift-.
  '?', // shift-/
  ')', // shift-0
  '!', // shift-1
  '@', // shift-2
  '#', // shift-3
  '$', // shift-4
  '%', // shift-5
  '^', // shift-6
  '&', // shift-7
  '*', // shift-8
  '(', // shift-9
  ':',
  ':', // shift-;
  '<',
  '+', // shift-=
  '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '[', // shift-[
  '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK - Not Watcom but C :D
  ']', // shift-]
  '"', '_',
  '\'', // shift-`
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  '{', '|', '}', '~', 127
};


static char azerty_keymap[128] =
{
  0,
  1,2,3,4,5,6,7,8,9,10,
  11,12,13,14,15,16,17,18,19,20,
  21,22,23,24,25,26,27,28,29,30,
  31,
  ' ','!','"','#','$','%','&','%','(',')','*','+',';','-',':','!',
  '0','1','2','3','4','5','6','7','8','9',':','M','<','=','>','?',
  '@','Q','B','C','D','E','F','G','H','I','J','K','L',',','N','O',
  'P','A','R','S','T','U','V','Z','X','Y','W','^','\\','$','^','_',
  '@','Q','B','C','D','E','F','G','H','I','J','K','L',',','N','O',
  'P','A','R','S','T','U','V','Z','X','Y','W','^','\\','$','^',127
};


/// mapping from SHIFT-keycodes to ASCII chars
char *shiftxform = qwerty_shiftmap;


// mapping from keycodes to ASCII chars
char KeyTranslation(unsigned char key)
{
  return (key < 128) ? azerty_keymap[key] : key;
}



//======================================================================
//   Global colormaps
//======================================================================

// Prepare a colormap for GREEN ONLY translucency over background

byte *whitemap;
byte *greenmap;
byte *graymap;

static void CON_SetupColormaps()
{
  int i, j, k;

  //
  //  setup the green translucent background colormap
  //
  greenmap = (byte *) Z_Malloc(256,PU_STATIC,NULL);
  whitemap = (byte *) Z_Malloc(256,PU_STATIC,NULL);
  graymap  = (byte *) Z_Malloc(256,PU_STATIC,NULL);

  byte *pal = (byte *)fc.CacheLumpName("PLAYPAL",PU_CACHE);

  for (i=0,k=0; i<768; i+=3,k++)
    {
      j = pal[i] + pal[i+1] + pal[i+2];

      if (game.mode == gm_heretic)
        {
          greenmap[k] = 209 + (byte)((float)j*15/(3*255));   //remaps to greens(209-224)
          graymap[k]  =       (byte)((float)j*35/(3*255));   //remaps to grays(0-35)
          whitemap[k] = 145 + (byte)((float)j*15/(3*255));   //remaps to reds(145-168)
        }
      else
        greenmap[k] = 127 - (j>>6);
    }

  //
  //  setup the white and gray text colormap
  //
  // this one doesn't need to be aligned, unless you convert the
  // V_DrawMappedPatch() into optimised asm.

  if (game.mode != gm_heretic)
    {
      for (i=0; i<256; i++)
        {
          whitemap[i] = i;        //remap each color to itself...
          graymap[i]  = i;
        }

      for (i=168;i<192;i++)
        {
          whitemap[i] = i-88;     //remaps reds(168-192) to whites(80-104)
          graymap[i]  = i-80;      //remaps reds(168-192) to gray(88-...)
        }
      whitemap[45] = 190-88; // the color[45]=color[190] !
      graymap [45] = 190-80;
      whitemap[47] = 191-88; // the color[47]=color[191] !
      graymap [47] = 191-80;
    }
}





//======================================================================
//                   CONSOLE VARS AND COMMANDS
//======================================================================


CV_PossibleValue_t CV_Positive[] = {{1,"MIN"}, {999999999,"MAX"}, {0,NULL}};

// how many seconds the hud messages lasts on the screen
consvar_t   cons_msgtimeout = {"con_hudtime","5", CV_SAVE,CV_Unsigned};

// number of lines console move per frame
consvar_t   cons_speed = {"con_speed","8", CV_SAVE,CV_Positive};

// percentage of screen height to use for console
consvar_t   cons_height = {"con_height","50", CV_SAVE,CV_Positive};

CV_PossibleValue_t backpic_cons_t[]={{0,"translucent"},{1,"picture"},{0,NULL}};
// whether to use console background picture, or translucent mode
consvar_t   cons_backpic = {"con_backpic","0", CV_SAVE,backpic_cons_t};


static char *bindtable[NUMINPUTS]; // console key bindings


//  Clear console text buffer
void Command_Clear_f()
{
  con.Clear();
}


// Choose keyboard layout
void Command_Keymap_f()
{
  if (COM_Argc() != 2)
    {
      CONS_Printf("Usage: keymap <language>\n");
      return;
    }

  int i;
  char *kmap = COM_Argv(1);
  for (i=0; i<2; i++)
    if (!strcasecmp(kmap, keymap_names[i]))
      break;

  switch (i)
    {
    case la_french:
      shiftxform = azerty_shiftmap;
      con_keymap = la_french;
      CONS_Printf("French keymap.\n");
      break;

    default:
      shiftxform = qwerty_shiftmap;
      con_keymap = la_english;
      CONS_Printf("English keymap.\n");
    }
}


void Command_Bind_f()
{
  int na = COM_Argc();
  int key;

  if (na != 2 && na != 3)
    {
      CONS_Printf ("bind <keyname> [<command>]\n");
      CONS_Printf("\2bind table :\n");
      na = 0;
      for (key=0; key<NUMINPUTS; key++)
        if (bindtable[key])
          {
            CONS_Printf("%s : \"%s\"\n", G_KeynumToString(key), bindtable[key]);
            na = 1;
          }
      if (!na)
        CONS_Printf("Empty\n");
      return;
    }

  key = G_KeyStringtoNum(COM_Argv(1));
  if (!key)
    {
      CONS_Printf("Invalid key name\n");
      return;
    }

  if (bindtable[key] != NULL)
    {
      Z_Free(bindtable[key]);
      bindtable[key] = NULL;
    }

  if (na == 3)
    bindtable[key] = Z_StrDup(COM_Argv(2));
}



//======================================================================
//   Console class implementation
//======================================================================

Console::Console()
{
  refresh = true;
  recalc = true;

  con_destheight = 0;
  active = true; // start with console active, but unseen
  graphic = false;

  con_cols = con_lines = 0;
  con_line = NULL;
  con_buffer = NULL;

  memset(inputlines, 0, sizeof(inputlines));
  for (int i=0; i<32; i++)
    inputlines[i][0] = CON_PROMPTCHAR;
  input_cy = 0;
  input_cx = 1;

  for (int i=0;i<NUMINPUTS;i++)
    bindtable[i] = NULL;
}


Console::~Console()
{
  if (con_buffer)
    delete con_buffer;
}


// Client init (not executed on dedicated server!): Setup the console text buffer

void Console::Init()
{
  // initialize output buffer
  con_buffer = new char[CON_BUFFERSIZE];
  memset(con_buffer, 0, CON_BUFFERSIZE);

  // set console full screen for game startup MAKE SURE VID_Init() done !!!
  con_destheight = con_height = vid.height;

  // make sure it is ready for the loading screen
  RecalcSize();

  CON_SetupColormaps();

  con_clipviewtop = -1;     // -1 does not clip

  con_hudlines = CON_MAXHUDLINES;

  // load console background pic
  con_backpic = tc.GetPtr("CONSBACK");

  // borders MUST be there
  con_lborder  = tc.GetPtr("CBLEFT");
  con_rborder = tc.GetPtr("CBRIGHT");

  cons_msgtimeout.Reg();
  cons_speed.Reg();
  cons_height.Reg();
  cons_backpic.Reg();

  graphic = true;
}



// Toggle console on and off
void Console::Toggle(bool forceoff)
{
  if (dedicated)
    {
      CONS_Printf("Some idiot tried to close the console\n");
      return; // dedicated server cannot close console!
    }

  // console key was pushed
  if (active || forceoff)
    {
      // toggle console off
      ClearHUD();
      con_destheight = 0;
      active = false;

      if (forceoff)
        {
          // close it NOW!
          con_height = 0;
          con_clipviewtop = -1;       //remove console clipping of view
        }
    }
  else
    {
      // toggle console on
      con_destheight = (cons_height.value * vid.height)/100;
      if (con_destheight < 20)
        con_destheight = 20;
      else if (con_destheight > vid.height - hud.stbarheight)
        con_destheight = vid.height-hud.stbarheight;

      con_destheight &= ~0x3;      // multiple of text row height
      active = true;
    }
}



void Console::Clear()
{
  con_cx = 0;
  con_cy = con_lines-1;
  con_scrollup = 0;

  if (con_buffer)
    {
      memset(con_buffer, 0, CON_BUFFERSIZE);
      con_line = &con_buffer[con_cy*con_cols];
    }
}



//
//  Called after a screen size change to set the rows and line size of the
//  console text buffer and rewrap it.
//
void Console::RecalcSize()
{
  recalc = false;

  int conw = (vid.width >> 3) - 2; // 8-pixel font

  // check for change of video width
  if (conw == con_cols)
    return; // didn't change

  int old_cols  = con_cols;
  int old_lines = con_lines;
  int old_cy    = con_cy;

  char tmp_buffer[CON_BUFFERSIZE];
  memcpy(tmp_buffer, con_buffer, CON_BUFFERSIZE);

  if (conw < 1)
    con_cols = (BASEVIDWIDTH >> 3) - 2;
  else
    con_cols = conw;

  con_lines = CON_BUFFERSIZE / con_cols;
  memset(con_buffer, ' ', CON_BUFFERSIZE);

  con_cx = 0;
  con_cy = con_lines - 1;
  con_scrollup = 0;

  con_line = &con_buffer[con_cy * con_cols];

  if (!old_cols)
    return; // not the first time

  // re-arrange console text buffer to keep text
  for (int i = old_cy+1; i < old_cy+old_lines; i++)
    {
      if (tmp_buffer[(i % old_lines) * old_cols])
        {
          char temp[1000]; // a line, this should be enough

          memcpy(temp, &tmp_buffer[(i % old_lines)*old_cols], old_cols);
          int j = old_cols - 1;
          while (temp[j] == ' ' && j)
            j--;
          temp[j+1] = '\n';
          temp[j+2] = '\0';
          Print(temp);
        }
    }
}



//  Clear time of console heads up messages
void Console::ClearHUD()
{
  for (int i=0; i<con_hudlines; i++)
    con_hudtime[i] = 0;
}



//  Console ticker : handles console move in/out, cursor blinking
void Console::Ticker()
{
  extern int viewwindowy;

  // cursor blinking
  con_tick++;
  con_tick &= 7;

  // console movement
  if (con_destheight != con_height)
    {
      // up/down move to dest
      if (con_height < con_destheight)
        {
          con_height += cons_speed.value;
          if (con_height > con_destheight)
            con_height = con_destheight;
        }
      else if (con_height > con_destheight)
        {
          con_height -= cons_speed.value;
          if (con_height < con_destheight)
            con_height = con_destheight;
        }
    }

  // clip the view, so that the part under the console is not drawn
  con_clipviewtop = -1;
  if (cons_backpic.value)   // clip only when using an opaque background
    {
      if (con_height > 0)
        con_clipviewtop = con_height - viewwindowy - 1 - 10;
//NOTE: BIG HACK::SUBTRACT 10, SO THAT WATER DON'T COPY LINES OF THE CONSOLE
//      WINDOW!!! (draw some more lines behind the bottom of the console)
      if (con_clipviewtop < 0)
        con_clipviewtop = -1;   //maybe not necessary, provided it's <0
    }

  // make overlay messages disappear after a while
  for (int i=0; i<con_hudlines; i++)
    {
      if (--(con_hudtime[i]) < 0)
        con_hudtime[i] = 0;
    }
}



//  Handles console key input
bool Console::Responder(event_t *ev)
{
  if (chat_on)
    return false;

  // let go keyup events, don't eat them
  if (ev->type != ev_keydown)
    return false;

  int key = ev->data1;

  // check for console toggle key
  if (key == gamecontrol[gc_console][0] ||
      key == gamecontrol[gc_console][1])
    {
      Toggle();
      return true;
    }

  //  check other keys only if console prompt is active
  if (!active)
    {
      if (key < NUMINPUTS && bindtable[key])
        {
          COM_BufAddText(bindtable[key]);
          COM_BufAddText("\n");
          return true;
        }
      return false;
    }

  // eat shift if console is active
  if (key == KEY_SHIFT)
    return true;

  // escape key toggles off console
  if (key == KEY_ESCAPE)
    {
      Toggle();
      return true;
    }


  // sequential completions a la 4dos
  static char completion[80];
  static int  comskips, varskips;

  // command completion forward (tab) and backward (shift-tab)
  if (key == KEY_TAB)
    {
      // TOTALLY UTTERLY UGLY NIGHT CODING BY FAB!!! :-)
      //
      // sequential command completion forward and backward

      // remember typing for several completions (…-la-4dos)
      if (inputlines[input_cy][input_cx-1] != ' ')
        {
          if (strlen (inputlines[input_cy]+1)<80)
            strcpy (completion, inputlines[input_cy]+1);
          else
            completion[0] = 0;

          comskips = varskips = 0;
        }
      else
        {
          if (shiftdown)
            {
              if (comskips<0)
                {
                  if (--varskips<0)
                    comskips = -(comskips+2);
                }
              else
                if (comskips>0)
                  comskips--;
            }
          else
            {
              if (comskips<0)
                varskips++;
              else
                comskips++;
            }
        }

      const char *cmd;
      if (comskips>=0)
        {
          cmd = COM_CompleteCommand(completion, comskips);
          if (!cmd)
            // dirty:make sure if comskips is zero, to have a neg value
            comskips = -(comskips+1);
        }
      if (comskips<0)
        cmd = consvar_t::CompleteVar(completion, varskips);

      if (cmd)
        {
          memset(inputlines[input_cy]+1,0,CON_MAXPROMPTCHARS-1);
          strcpy (inputlines[input_cy]+1, cmd);
          input_cx = strlen(cmd)+1;
          inputlines[input_cy][input_cx] = ' ';
          input_cx++;
          inputlines[input_cy][input_cx] = 0;
        }
      else
        {
          if (comskips>0)
            comskips--;
          else if (varskips>0)
            varskips--;
        }

      return true;
    }

  // move up (backward) in console textbuffer
  if (key == KEY_PGUP)
    {
      if (con_scrollup < con_lines - ((con_height - 16) >> 3))
        con_scrollup += 5;
      return true;
    }
  else if (key == KEY_PGDN)
    {
      if (con_scrollup >= 5)
        con_scrollup -= 5;
      else
        con_scrollup = 0;
      return true;
    }

  // oldset text in buffer
  if (key == KEY_HOME)
    {
      con_scrollup = (con_lines - ((con_height - 16) >> 3));
      return true;
    }
  else if (key == KEY_END)
    {
      // most recent text in buffer
      con_scrollup = 0;
      return true;
    }

  // command enter
  if (key == KEY_ENTER)
    {
      if (input_cx < 2)
        return true;

      // push the command
      COM_BufAddText(inputlines[input_cy]+1);
      COM_BufAddText("\n");

      CONS_Printf("%s\n", inputlines[input_cy]);

      input_cy = (input_cy + 1) & 31;
      input_hist = input_cy;

      memset(inputlines[input_cy],0,CON_MAXPROMPTCHARS);
      inputlines[input_cy][0] = CON_PROMPTCHAR;
      input_cx = 1;

      return true;
    }

  // backspace command prompt
  if (key == KEY_BACKSPACE)
    {
      if (input_cx > 1)
        {
          input_cx--;
          inputlines[input_cy][input_cx] = 0;
        }
      return true;
    }

  // move back in input history
  if (key == KEY_UPARROW)
    {
      // copy one of the previous inputlines to the current
      do
        {
          input_hist = (input_hist - 1) & 31;   // cycle back
        }
      while (input_hist != input_cy && !inputlines[input_hist][1]);

      // stop at the last history input line, which is the
      // current line + 1 because we cycle through the 32 input lines
      if (input_hist == input_cy)
        input_hist = (input_cy + 1) & 31;

      memcpy(inputlines[input_cy], inputlines[input_hist], CON_MAXPROMPTCHARS);
      input_cx = strlen(inputlines[input_cy]);

      return true;
    }

  // move forward in input history
  if (key == KEY_DOWNARROW)
    {
      if (input_hist == input_cy)
        return true;

      do
        {
          input_hist = (input_hist + 1) & 31;
        }
      while (input_hist != input_cy && !inputlines[input_hist][1]);

      memset(inputlines[input_cy], 0, CON_MAXPROMPTCHARS);

      // back to current line
      if (input_hist == input_cy)
        {
          inputlines[input_cy][0] = CON_PROMPTCHAR;
          input_cx = 1;
        }
      else
        {
          strcpy(inputlines[input_cy],inputlines[input_hist]);
          input_cx = strlen(inputlines[input_cy]);
        }
      return true;
    }

  // convert keycode to ASCII
  if (key >= KEY_KEYPAD7 && key <= KEY_KPADDEL)
    {
      // allow people to use keypad in console (good for typing IP addresses) - Calum
      char keypad_translation[] =
      {
        '7','8','9','-',
        '4','5','6','+',
        '1','2','3',
        '0','.'
      };

      key = keypad_translation[key - KEY_KEYPAD7];
    }
  else if (key == KEY_KPADSLASH)
    key = '/';
  else if (shiftdown)
    key = shiftxform[key];
  else if (con_keymap != la_english)
    key = KeyTranslation(key);

  // enter a char into the command prompt
  if (key < 32 || key > 127)
    return false;

  // add key to cmd line here
  if (input_cx < CON_MAXPROMPTCHARS)
    {
      // make sure letters are lowercase for commands & cvars
      if (key >= 'A' && key <= 'Z')
        key = key + 'a' - 'A';

      inputlines[input_cy][input_cx] = key;
      inputlines[input_cy][input_cx+1] = 0;
      input_cx++;
    }

  return true;
}


//  Insert a new line in the console text buffer
void Console::Linefeed(int player)
{
  // set time for heads up messages
  con_hudtime[con_cy % con_hudlines] = cons_msgtimeout.value*TICRATE;
  // for which local player is it meant?
  con_lineowner[con_cy % con_hudlines] = player;

  // move to next line in buffer, clear it
  con_cy++;
  con_cx = 0;

  con_line = &con_buffer[(con_cy % con_lines)*con_cols];
  memset(con_line, ' ', con_cols);

  // make sure the view borders are refreshed if hud messages scroll
  con_hudupdate = true; // see HU_Erase()
}


//  Outputs text into the console text buffer
void Console::Print(char *msg)
{
  int mask = 0;
  int p = 1;

  //TODO: finish text colors
  if (*msg < 5)
    {
      if (*msg=='\2')  // set white color
        mask = 128;
      else if (*msg=='\3')
        {
          mask = 128;                         // white text + sound
          S_StartAmbSound(sfx_message);
        }
      else if (*msg == '\4') //Splitscreen: This message is for the second player
        p = 2;
    }

  while (*msg)
    {
      // skip non-printable characters and white spaces
      while (*msg && *msg <= ' ')
        {
          // carriage return
          if (*msg == '\r')
            {
              con_cy--;
              Linefeed(p);
            }
          else if (*msg == '\n')
            Linefeed(p);
          else if (*msg == ' ')
            {
              con_line[con_cx++] = ' ';
              if (con_cx >= con_cols)
                Linefeed(p);
            }
          else if (*msg == '\t')
            {
              //adds tab spaces for nice layout in console
              do {
                con_line[con_cx++] = ' ';
              } while (con_cx % 4 != 0);

              if (con_cx >= con_cols)
                Linefeed(p);
            }
          msg++;
        }

      if (*msg == 0)
        return;

      int l;
      // printable character
      for (l=0; l<con_cols && msg[l] > ' '; l++)
        ;

      // word wrap
      if (con_cx + l > con_cols)
        Linefeed(p);

      // a word at a time
      for ( ; l>0; l--)
        con_line[con_cx++] = *(msg++) | mask;
    }
}



//======================================================================
//                          CONSOLE DRAW
//======================================================================


//  Scale a pic_t at 'startx' pos, to 'destwidth' columns.
//                startx,destwidth is resolution dependent
//  Used to draw console borders, console background.
//  The pic must be sized BASEVIDHEIGHT height.
//
//  TODO: ASM routine!!! lazy Fab!!
//
/*
static void CON_DrawBackpic (pic_t *pic, int startx, int destwidth)
{
    int         x, y;
    int         v;
    byte        *src, *dest;
    int         frac, fracstep;

    dest = vid.buffer+startx;

    for (y=0 ; y<con_height ; y++, dest += vid.width)
    {
        // scale the picture to the resolution
        v = SHORT(pic->height) - ((con_height - y)*(BASEVIDHEIGHT-1)/vid.height) - 1;

        src = pic->data + v*SHORT(pic->width);

        // in case of the console backpic, simplify
        if (pic->width==destwidth)
            memcpy (dest, src, destwidth);
        else
        {
            // scale pic to screen width
            frac = 0;
            fracstep = (SHORT(pic->width)<<16)/destwidth;
            for (x=0 ; x<destwidth ; x+=4)
            {
                dest[x] = src[frac>>16];
                frac += fracstep;
                dest[x+1] = src[frac>>16];
                frac += fracstep;
                dest[x+2] = src[frac>>16];
                frac += fracstep;
                dest[x+3] = src[frac>>16];
                frac += fracstep;
            }
        }
    }

}
*/


// draw the last lines of console text to the top of the screen
void Console::DrawHudlines()
{
  if (con_hudlines <= 0)
    return;

  int y, y2 = 0;

  if (chat_on)
    y = 8;   // leave place for chat input in the first row of text
  else
    y = 0;

  for (int i = con_cy - con_hudlines + 1; i <= con_cy; i++)
    {
      if (i < 0)
        continue;
      if (con_hudtime[i%con_hudlines] == 0)
        continue;

      char *p = &con_buffer[(i%con_lines)*con_cols];

      for (int x=0; x<con_cols; x++)
        {
#if 0 //FIXME: Hurdler, must be replaced by something compatible with the new renderer
#ifdef HWRENDER
          // TODO: Hurdler: see why we need to have a separate code here
          extern float gr_viewheight;
          if (con_lineowner[i%con_hudlines] == 2)
            V_DrawCharacter(x<<3, int(y2+gr_viewheight), p[x]);
          else
#endif
#endif
            V_DrawCharacter(x<<3, y, p[x]);
        }
      if (con_lineowner[i%con_hudlines] == 2)
        y2 += 8;
      else
        y += 8;
    }

  // top screen lines that might need clearing when view is reduced
  con_clearlines = y;      // this is handled by HU_Erase ();
}



// draw the console background, text, and prompt if enough place
void Console::DrawConsole()
{
  //FIXME: refresh borders only when console bg is translucent
  con_clearlines = con_height;    // clear console draw from view borders
  con_hudupdate = true;           // always refresh while console is on

  // draw console background
  int x, y = int(con_height - 200*vid.fdupy);
  if (cons_backpic.value)
    con_backpic->Draw(0, y, V_SSIZE);
  else
    {
      int w = con_rborder->width*vid.dupx;
      x = vid.width - w;
      con_lborder->Draw(0, y, V_SSIZE);
      con_rborder->Draw(x, y, V_SSIZE);

      V_DrawFadeConsBack(w,0,x,con_height); // translucent background
    }

  // draw console text lines from bottom to top
  // (going backward in console buffer text)

  if (con_height < 20)       //8+8+4
    return;

  int i = con_cy - con_scrollup;

  // skip the last empty line due to the cursor being at the start
  // of a new line
  if (!con_scrollup && !con_cx)
    i--;

  for (y = con_height-20; y >= 0; y -= 8, i--)
    {
      if (i < 0)
        i = 0;

      char *p = &con_buffer[(i % con_lines) * con_cols];

      for (x = 0; x < con_cols; x++)
        V_DrawCharacter((x+1) << 3, y, p[x]);
    }


  // draw prompt if enough space (not while game startup)
  if (con_height == con_destheight && con_height >= 20 && !refresh)
    {
      // input line scrolls left if it gets too long
      char *p = inputlines[input_cy];
      if (input_cx >= con_cols)
        p += input_cx - con_cols + 1;

      int y = con_height - 12;

      for (x=0; x<con_cols; x++)
        V_DrawCharacter((x+1)<<3, y, p[x]);

      // draw the blinking cursor
      int x = (input_cx>=con_cols) ? con_cols - 1 : input_cx;
      if (con_tick < 4)
        V_DrawCharacter((x+1) << 3, y, 0x80 | '_');
    }
}


//  Console refresh drawer, called each frame
void Console::Drawer()
{
  if (!graphic)
    return;

  if (recalc)
    RecalcSize();

  //Fab: bighack: patch 'I' letter leftoffset so it centers
  //hud.font['I'-HU_FONTSTART]->leftoffset = -2;

  if (con_height > 0)
    DrawConsole();
  else if (game.state == GameInfo::GS_LEVEL)
    DrawHudlines();

  //hud.font['I'-HU_FONTSTART]->leftoffset = 0;
}







//======================================================================
//   Wrappers
//======================================================================

//
//  Console print! Wahooo! Lots o fun!
//
void CONS_Printf(char *fmt, ...)
{
  va_list     argptr;
  char        txt[512];

  va_start (argptr,fmt);
  vsprintf (txt,fmt,argptr);
  va_end   (argptr);

  // echo console prints to log file

  if (!con.graphic)
    {
      I_OutputMsg("%s", txt); // This function MUST be found in EVERY interface / platform in i_system.c
      return;
    }
  else
    {
      I_OutputMsg("%s",txt); //  FIXME make copies of console messages to stdout for debugging
      con.Print(txt); // write message in con text buffer
    }

  // make sure new text is visible
  con.con_scrollup = 0;

  // if not in display loop, force screen update
  if (con.refresh)
    {
      /*
      // this #if structure should be replaced with a platform independent function call
#if !defined (SDL) && (defined( __WIN32__) || defined( __OS2__))
      // show startup screen and message using only 'software' graphics
      // (rendermode may be hardware accelerated, but the video mode is not set yet)
      CON_DrawBackpic (con_backpic, 0, vid.width);    // put console background
      I_LoadingScreen ( txt );  // draw the message on it using Win32 API?
#else
      */
      // here we display the console background and console text
      // (no hardware accelerated support for these versions)
      con.Drawer();
      I_FinishUpdate(); // page flip or blit buffer
    }
}


//  Print an error message, and wait for ENTER key to continue.
//  To make sure the user has seen the message
void CONS_Error(char *msg)
{
  CONS_Printf("\2%s",msg);   // write error msg in different colour
  CONS_Printf("Press ENTER to continue\n");

  // dirty quick hack, but for the good cause
  while (I_GetKey() != KEY_ENTER)
    ;
}
