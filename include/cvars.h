// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004-2005 by DooM Legacy Team.
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
// Revision 1.11  2006/01/02 17:02:30  smite-meister
// small fixes
//
// Revision 1.10  2005/04/17 17:47:54  smite-meister
// netcode
//
// Revision 1.9  2005/03/16 21:16:08  smite-meister
// menu cleanup, bugfixes
//
// Revision 1.8  2004/11/28 18:02:23  smite-meister
// RPCs finally work!
//
// Revision 1.7  2004/11/13 22:38:59  smite-meister
// intermission works
//
// Revision 1.6  2004/09/09 17:15:20  jussip
// Cleared out old joystick crap in preparation for brand new code.
//
// Revision 1.4  2004/07/25 20:18:47  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.2  2004/07/13 20:23:37  smite-meister
// Mod system basics
//
// Revision 1.1  2004/06/25 15:18:37  smite-meister
// CVar cleanup
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Console variable prototypes.

#ifndef cvars_h
#define cvars_h 1

struct consvar_t;

// misc
extern consvar_t cv_masterserver;
extern consvar_t cv_netstat;

// server info
extern consvar_t cv_servername;
extern consvar_t cv_publicserver;
extern consvar_t cv_allownewplayers;
extern consvar_t cv_maxplayers;
//extern consvar_t cv_maxteams;

// server demo
extern consvar_t cv_playdemospeed;

// server rules
extern consvar_t cv_deathmatch;
extern consvar_t cv_teamplay;
extern consvar_t cv_teamdamage;
extern consvar_t cv_hiddenplayers;

extern consvar_t cv_exitmode;
extern consvar_t cv_fraglimit;
extern consvar_t cv_timelimit;

extern consvar_t cv_jumpspeed;
extern consvar_t cv_allowrocketjump;
extern consvar_t cv_allowautoaim;
extern consvar_t cv_allowmlook;
extern consvar_t cv_allowpause;

extern consvar_t cv_itemrespawn;
extern consvar_t cv_itemrespawntime;
extern consvar_t cv_respawnmonsters;
extern consvar_t cv_respawnmonsterstime;
extern consvar_t cv_fragsweaponfalling;

extern consvar_t cv_gravity;
extern consvar_t cv_nomonsters;
extern consvar_t cv_fastmonsters;
extern consvar_t cv_solidcorpse;
extern consvar_t cv_voodoodolls;
extern consvar_t cv_infighting;

// client info (server needs to know)
extern consvar_t cv_splitscreen;

// client input (g_input.cpp)
extern consvar_t cv_controlperkey;
extern consvar_t cv_usemouse[2];
extern consvar_t cv_mousesensx[2];
extern consvar_t cv_mousesensy[2];
extern consvar_t cv_automlook[2];
extern consvar_t cv_mousemove[2];
extern consvar_t cv_invertmouse[2];
extern consvar_t cv_mouse2port;
#ifdef LMOUSE2
 extern consvar_t cv_mouse2opt;
#endif

// client console (console.cpp)
extern consvar_t cons_msgtimeout;
extern consvar_t cons_speed;
extern consvar_t cons_height;
extern consvar_t cons_backpic;

// client hud (hu_stuff.cpp)
extern consvar_t cv_stbaroverlay;

// client chat (hu_stuff.cpp)
extern consvar_t cv_chatmacro1;
extern consvar_t cv_chatmacro2;
extern consvar_t cv_chatmacro3;
extern consvar_t cv_chatmacro4;
extern consvar_t cv_chatmacro5;
extern consvar_t cv_chatmacro6;
extern consvar_t cv_chatmacro7;
extern consvar_t cv_chatmacro8;
extern consvar_t cv_chatmacro9;
extern consvar_t cv_chatmacro0;


// client audio (s_sound.cpp)
extern consvar_t cv_soundvolume;
extern consvar_t cv_musicvolume;
extern consvar_t cd_volume;
extern consvar_t cv_numChannels;
extern consvar_t cv_stereoreverse;
extern consvar_t cv_surround;
extern consvar_t cv_precachesound;

// client renderer
extern consvar_t cv_viewheight;

extern consvar_t cv_scr_width;
extern consvar_t cv_scr_height;
extern consvar_t cv_scr_depth;
extern consvar_t cv_fullscreen;
extern consvar_t cv_usegamma;

extern consvar_t cv_viewsize;
extern consvar_t cv_detaillevel;
extern consvar_t cv_scalestatusbar;
//extern consvar_t cv_fov;

extern consvar_t cv_screenslink;
extern consvar_t cv_translucency;
extern consvar_t cv_fuzzymode;
extern consvar_t cv_splats;
extern consvar_t cv_bloodtime;
extern consvar_t cv_psprites;

// client opengl renderer
extern consvar_t cv_grsolvetjoin;
extern consvar_t cv_grrounddown;
extern consvar_t cv_grcrappymlook;
extern consvar_t cv_grdynamiclighting;
extern consvar_t cv_grstaticlighting;
extern consvar_t cv_grmblighting;
extern consvar_t cv_grcoronas;
extern consvar_t cv_grcoronasize;
extern consvar_t cv_grfov;
extern consvar_t cv_grpolygonsmooth;
extern consvar_t cv_grmd2;
extern consvar_t cv_grtranswall;
extern consvar_t cv_grfog;
extern consvar_t cv_grfogcolor;
extern consvar_t cv_grfogdensity;
extern consvar_t cv_grcontrast;
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;
extern consvar_t cv_grfiltermode;
extern consvar_t cv_grcorrecttricks;
extern consvar_t cv_grnearclippingplane;
extern consvar_t cv_grfarclippingplane;

#endif
