// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2004 by DooM Legacy Team.
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
// Revision 1.6  2004/09/09 17:15:20  jussip
// Cleared out old joystick crap in preparation for brand new code.
//
// Revision 1.5  2004/08/12 18:30:29  smite-meister
// cleaned startup
//
// Revision 1.4  2004/07/25 20:18:47  hurdler
// Remove old hardware renderer and add part of the new one
//
// Revision 1.3  2004/07/14 16:13:13  smite-meister
// cleanup, commands
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

// menu only cvars (used when launching server, and in doing serversearch)
extern consvar_t cv_skill;
extern consvar_t cv_monsters;
extern consvar_t cv_nextmap;
extern consvar_t cv_newdeathmatch;
extern consvar_t cv_serversearch;


// server info
extern consvar_t cv_servername;
extern consvar_t cv_internetserver;
extern consvar_t cv_allownewplayers;
extern consvar_t cv_maxplayers;
//extern consvar_t cv_maxteams;

// server demo
extern consvar_t cv_playdemospeed;

// server game
extern consvar_t cv_deathmatch;
extern consvar_t cv_teamplay;
extern consvar_t cv_teamdamage;
extern consvar_t cv_hiddenplayers;

extern consvar_t cv_exitmode;
extern consvar_t cv_fraglimit;
extern consvar_t cv_timelimit;

extern consvar_t cv_allowjump;
extern consvar_t cv_allowrocketjump;
extern consvar_t cv_allowautoaim;
extern consvar_t cv_allowmlook;

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


// client info (server needs to know)
extern consvar_t cv_playername;
extern consvar_t cv_playercolor;
extern consvar_t cv_skin;
extern consvar_t cv_autoaim;
extern consvar_t cv_originalweaponswitch;
extern consvar_t cv_weaponpref;

extern consvar_t cv_splitscreen;

extern consvar_t cv_playername2;
extern consvar_t cv_playercolor2;
extern consvar_t cv_skin2;
extern consvar_t cv_autoaim2;
//extern consvar_t cv_originalweaponswitch2;
//extern consvar_t cv_weaponpref2;


// client input (g_input.cpp)
extern consvar_t cv_controlperkey;

extern consvar_t cv_autorun;
extern consvar_t cv_automlook;
extern consvar_t cv_usemouse;
extern consvar_t cv_invertmouse;
extern consvar_t cv_mousemove;
extern consvar_t cv_mousesensx;
extern consvar_t cv_mousesensy;

extern consvar_t cv_autorun2;
extern consvar_t cv_automlook2;
extern consvar_t cv_usemouse2;
extern consvar_t cv_invertmouse2;
extern consvar_t cv_mousemove2;
extern consvar_t cv_mousesensx2;
extern consvar_t cv_mousesensy2;
extern consvar_t   cv_mouse2port;
#ifdef LMOUSE2
 extern consvar_t cv_mouse2opt;
#endif

extern consvar_t cv_usejoystick;
extern consvar_t cv_joystickfreelook;


// client console (console.cpp)
extern consvar_t cons_msgtimeout;
extern consvar_t cons_speed;
extern consvar_t cons_height;
extern consvar_t cons_backpic;

// client hud (hu_stuff.cpp)
extern consvar_t cv_crosshair;
extern consvar_t cv_crosshair2;
extern consvar_t cv_showmessages;
extern consvar_t cv_showmessages2;
extern consvar_t cv_stbaroverlay;
//extern consvar_t cv_crosshairscale;

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
extern consvar_t cv_chasecam;
extern consvar_t cv_cam_dist;
extern consvar_t cv_cam_height;
extern consvar_t cv_cam_speed;

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
