# Gnu Make makefile for Doom Legacy
# Copyright (C) 2002-2006 by DooM Legacy Team.
#
# $Id$
#
# This primary Makefile calls auxiliary Makefiles in subdirectories.
# Use 'make OPT=1' to make optimized version, else you'll get debug info.

# Platform autodetect.

UNAME = $(shell uname)

ifeq ($(UNAME),Linux)
export LINUX=1
else
export WIN=1
endif

# Debugging and optimization
ifdef OPT
CF = -O
else
CF = -g -O0
endif

# Dynamic or static linkage? I like static.
ifdef DYNAMIC
linkage = -DDYNAMIC_LINKAGE
else
linkage = 
endif

# Freeform defines
ifdef DEF
CF += -D$(DEF)
endif


# ----------- platform specific part begins
ifdef LINUX

# file removal utility
 RM = rm
# assembler
 NASM = nasm
 nasmformat = elf
# compiler
 platform  = -DLINUX
 interface = -DSDL #-DNO_MIXER
# linker
 LIBS	= -lSDLmain -lSDL -lSDL_mixer -lpng -ljpeg -lz -L. -ltnl -ltomcrypt
 OPENGLLIBS = -lGL -lGLU
# CF += -m32
 LDFLAGS = -Wall #-m32
# executable
 exename = Legacy

else # assume WIN32 is defined

# file removal utility
 RM = rm
# assembler
 NASM 	= nasmw.exe
 nasmformat = win32
# compiler
 platform  = -D__WIN32__
 interface = -DSDL -DNO_MIXER
 CF += -mwindows
# linker
 LIBS	= -lmingw32 -lSDLmain -lSDL SDL_mixer.lib -lpng -jpeg -lz -L. -ltnl -ltomcrypt -lwsock32
 OPENGLLIBS = -lopengl32 -lglu32
 LDFLAGS = -Wall -mwindows
# executable
 exename = Legacy.exe

endif
# ----------- platform specific part ends

export RM
export LDFLAGS

# C++ compiler (usually g++)
export CC = g++

# Defines.
#
# Automatic defines: Automatically defined by the compiler in the corresponding environment.
# __WIN32__, __WIN32, _WIN32, WIN32 : defined in Win32 environment
# __DJGPP__ : defined by DJGPP
# _MACOS_ : ???
# __OS2__ : ???
#
# Platform: use _exactly_ one of the following:
# LINUX : Linux platform
# __WIN32__ : Win32 platform (automatic, no need to define)
#
# Multimedia interface: use ONLY _one_ of the following:
# SDL : compile the SDL version of Legacy (use SDL for multimedia interface, SDL_mixer for music)
# WIN32_DIRECTX : compile the Win32 native version of Legacy. Use DirectX for multimedia interface
#
# Miscellaneous options: use as many as you like
# NO_OPENGL : do not include OpenGL renderer in the build
# NO_MIXER : do not include SDL_mixer in the build
# USEASM : use assembler routines where possible

export CF += -Wall $(platform) $(interface) $(linkage)

INCLUDES = -Iinclude
CFLAGS = $(CF) $(INCLUDES)

# linker
export LD = $(CC)

export objdir = objs


export engine_objects = \
	$(objdir)/g_game.o \
	$(objdir)/g_state.o \
	$(objdir)/g_demo.o \
	$(objdir)/g_input.o \
	$(objdir)/g_type.o \
	$(objdir)/g_level.o \
	$(objdir)/g_mapinfo.o \
	$(objdir)/g_map.o \
	$(objdir)/g_player.o \
	$(objdir)/g_team.o \
	$(objdir)/g_think.o \
	$(objdir)/g_actor.o \
	$(objdir)/g_pawn.o \
	$(objdir)/g_decorate.o \
	$(objdir)/p_tick.o \
	$(objdir)/p_setup.o \
	$(objdir)/p_saveg.o \
	$(objdir)/p_spec.o \
	$(objdir)/p_events.o \
	$(objdir)/p_floor.o \
	$(objdir)/p_plats.o \
	$(objdir)/p_ceilng.o \
	$(objdir)/p_doors.o \
	$(objdir)/p_genlin.o \
	$(objdir)/p_things.o \
	$(objdir)/p_lights.o \
	$(objdir)/p_anim.o \
	$(objdir)/p_switch.o \
	$(objdir)/p_poly.o \
	$(objdir)/p_acs.o \
	$(objdir)/a_action.o \
	$(objdir)/p_pspr.o \
	$(objdir)/p_hpspr.o \
	$(objdir)/p_xpspr.o \
	$(objdir)/p_enemy.o \
	$(objdir)/p_henemy.o \
	$(objdir)/p_xenemy.o \
	$(objdir)/ai_mobjinfo.o \
	$(objdir)/ai_states.o \
	$(objdir)/t_oper.o \
	$(objdir)/t_parse.o \
	$(objdir)/t_prepro.o \
	$(objdir)/t_spec.o \
	$(objdir)/t_vari.o \
	$(objdir)/t_script.o \
	$(objdir)/t_func.o \
	$(objdir)/p_map.o \
	$(objdir)/p_maputl.o \
	$(objdir)/p_sight.o \
	$(objdir)/p_hsight.o \
	$(objdir)/p_telept.o \
	$(objdir)/p_heretic.o \
	$(objdir)/p_camera.o \
	$(objdir)/p_user.o \
	$(objdir)/p_inter.o \
	$(objdir)/am_map.o \
	$(objdir)/menu.o \
	$(objdir)/f_finale.o \
	$(objdir)/f_wipe.o \
	$(objdir)/wi_stuff.o \
	$(objdir)/st_lib.o \
	$(objdir)/st_stuff.o \
	$(objdir)/hu_stuff.o \
	$(objdir)/m_cheat.o \
	$(objdir)/p_fab.o \
	$(objdir)/p_hacks.o \
	$(objdir)/d_items.o \
	$(objdir)/d_main.o \
	$(objdir)/dstrings.o \
	$(objdir)/acbot.o \
	$(objdir)/b_bot.o \
	$(objdir)/b_path.o

export util_objects = \
	$(objdir)/command.o \
	$(objdir)/console.o \
	$(objdir)/dehacked.o \
	$(objdir)/m_argv.o \
	$(objdir)/m_archive.o \
	$(objdir)/m_bbox.o \
	$(objdir)/m_dll.o \
	$(objdir)/m_fixed.o \
	$(objdir)/m_misc.o \
	$(objdir)/m_random.o \
	$(objdir)/m_swap.o \
	$(objdir)/md5.o \
	$(objdir)/parser.o \
	$(objdir)/tables.o \
	$(objdir)/vfile.o \
	$(objdir)/wad.o \
	$(objdir)/w_wad.o \
	$(objdir)/z_cache.o \
	$(objdir)/z_zone.o

export audio_objects = \
	$(objdir)/qmus2mid.o \
	$(objdir)/s_sound.o \
	$(objdir)/s_sndseq.o \
	$(objdir)/sounds.o

export video_objects = \
	$(objdir)/md3.o \
	$(objdir)/png.o \
	$(objdir)/jpeg.o \
	$(objdir)/screen.o \
	$(objdir)/v_video.o \
	$(objdir)/r_render.o \
	$(objdir)/r_bsp.o \
	$(objdir)/r_data.o \
	$(objdir)/r_draw.o \
	$(objdir)/r_draw8.o \
	$(objdir)/r_draw16.o \
	$(objdir)/r_main.o \
	$(objdir)/r_plane.o \
	$(objdir)/r_segs.o \
	$(objdir)/r_sky.o \
	$(objdir)/r_splats.o \
	$(objdir)/r_sprite.o \
	$(objdir)/r_things.o \
	$(objdir)/hw_trick.o \
	$(objdir)/hwr_render.o \
	$(objdir)/hwr_bsp.o \
	$(objdir)/hwr_geometry.o \
	$(objdir)/hwr_states.o \
	$(objdir)/oglrenderer.o \
	$(objdir)/oglhelpers.o 

export net_objects = \
	$(objdir)/n_interface.o \
	$(objdir)/n_connection.o \
	$(objdir)/sv_main.o \
	$(objdir)/sv_cmds.o \
	$(objdir)/cl_main.o

export sdl_objects = \
	$(objdir)/endtxt.o \
	$(objdir)/i_cdmus.o \
	$(objdir)/i_main.o \
	$(objdir)/i_net.o \
	$(objdir)/i_sound.o \
	$(objdir)/i_system.o \
	$(objdir)/i_video.o \
	$(objdir)/ogl_sdl.o \
	$(objdir)/searchp.o

export grammar_objects = \
	$(objdir)/ntexture.parser.o \
	$(objdir)/ntexture.lexer.o \
	$(objdir)/decorate.parser.o \
	$(objdir)/decorate.lexer.o \
	$(objdir)/parser_driver.o


objects = $(engine_objects) $(util_objects) $(audio_objects) $(video_objects) \
	$(net_objects) $(sdl_objects) $(grammar_objects)



# explicit rules

all	: mkdirobjs $(exename)

mkdirobjs:
	mkdir -p objs

.PHONY	: clean depend engine util audio video net sdl tools grammars

clean	:
	$(RM) $(objects)

depend:
	touch engine/engine.dep
	$(MAKE) -C engine depend
	touch video/video.dep
	$(MAKE) -C video depend
	touch audio/audio.dep
	$(MAKE) -C audio depend
	touch util/util.dep
	$(MAKE) -C util depend
	touch net/net.dep
	$(MAKE) -C net depend
	touch grammars/grammars.dep
	$(MAKE) -C grammars depend
	touch interface/sdl/sdl.dep
	$(MAKE) -C interface/sdl depend
	touch tools/tools.dep
	$(MAKE) -C tools depend

dep	: depend

docs	: Doxyfile
	doxygen

wad	: tools
	$(MAKE) -C wad

engine	:
	$(MAKE) -C engine

util	:
	$(MAKE) -C util

audio	:
	$(MAKE) -C audio

video	:
	$(MAKE) -C video

net	:
	$(MAKE) -C net

sdl	:
	$(MAKE) -C interface/sdl

tools	:
	$(MAKE) -C tools

grammars	:
	$(MAKE) -C grammars



ifdef DYNAMIC
# main program
$(exename) : engine util audio video net sdl grammars
	$(LD) $(LDFLAGS) $(objects) $(LIBS) -o $@
else
# all in one
$(exename) : engine util audio video net sdl grammars
	$(LD) $(LDFLAGS) $(objects) $(LIBS) $(OPENGLLIBS) -o $@
endif
