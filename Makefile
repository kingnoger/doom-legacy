# Gnu Make makefile for Doom Legacy
# Copyright (C) 2002-2014 by Doom Legacy Team.
#
# Use 'make OPT=1' to make optimized version, else you'll get debug info.

# C++ compiler
# FIXME why will CC = gcc not work?
CC = g++
CXX = g++
# CC := clang --analyze # and comment out the linker last line for sanity

# linker
LD = $(CXX)


SRCDIR := src
BUILDDIR := build

# Find every *.cpp file in the src tree. We don't want this since there is unused stuff there.
#SOURCES := $(shell find $(SRCDIR) -type f -name *.cpp)
# Only use listed source files.
SOURCES := $(shell cat $(SRCDIR)/src_files.txt)

# Corresponding object files.
OBJECTS := $(patsubst $(SRCDIR)/%, $(BUILDDIR)/%, $(SOURCES:.cpp=.o))

PARSER_OBJS := $(BUILDDIR)/ntexture.parser.o \
  $(BUILDDIR)/ntexture.lexer.o \
  $(BUILDDIR)/decorate.parser.o \
  $(BUILDDIR)/decorate.lexer.o


# Platform autodetect
UNAME = $(shell uname)
ifeq ($(UNAME),Linux)
  export LINUX=1
else
  export WIN=1
endif


###  C flags

# Basic flags (language version, warnings, dependency file generation)
CF = -std=c++11 -Wall -MP -MMD

# Debugging and optimization
ifdef OPT
  CF += -O
else
  CF += -g -O0
endif

# Dynamic or static linkage? I like static.
ifdef DYNAMIC
  CF += -DDYNAMIC_LINKAGE
endif

# Freeform commandline defines
ifdef DEF
CF += -D$(DEF)
endif

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
#
# Miscellaneous options: use as many as you like
# NO_SHADERS : do not include support for GLSL shaders in the build
# NO_MIXER   : do not include SDL_mixer in the build


###  Platform specific part.
ifdef LINUX

# compiler
 platform  = -DLINUX
 interface = -DSDL $(shell sdl-config --cflags)
# linker
 LIBS	= $(shell sdl-config --libs) -lSDL_mixer -lpng -ljpeg -lz -ldl -Llib -ltnl -ltomcrypt  # -lSDL_ttf
 OPENGLLIBS = -lGL -lGLU
 LDFLAGS = -Wall
# executable
 exename = doomlegacy2

else # assume WIN32 is defined

# compiler
 platform  = -D__WIN32__
 interface = -DSDL -DNO_MIXER $(shell sdl-config --cflags)
 CF += -mwindows
# linker
 LIBS	= -lmingw32 $(shell sdl-config --libs) SDL_mixer.lib -lpng -jpeg -lz -L. -ltnl -ltomcrypt -lwsock32
 OPENGLLIBS = -lopengl32 -lglu32
 LDFLAGS = -Wall -mwindows
# executable
 exename = doomlegacy2.exe
endif

CFLAGS := $(CF) $(platform) $(interface) -Iinclude

ifndef DYNAMIC
  LIBS += $(OPENGLLIBS)
endif


.PHONY	: clean docs tools wad tnl


###  Main executable

TARGET := bin/$(exename)

# link the executable
$(TARGET): $(OBJECTS) $(PARSER_OBJS)
	@echo " Linking..."
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

# most object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# d_main.cpp with Git revision string
$(BUILDDIR)/engine/d_main.o: $(SRCDIR)/engine/d_main.cpp
	$(CC) $(CFLAGS) -DSVN_REV=\"`git describe --tags`\" -c $< -o $@


### Lexers and parsers

# Lemon parser executable
LEMON := $(BUILDDIR)/lemon

# Lemon parser
$(LEMON): lemon/lemon.c
	@echo " Building Lemon..."; 
	$(CC) $< -o $@
	cp lemon/lempar.c $(BUILDDIR)

$(BUILDDIR)/%.lexer.o: $(BUILDDIR)/%.lexer.c $(BUILDDIR)/%.parser.h include/parser_driver.h
	$(CXX) $(CFLAGS) -c $< -o $@

# TODO combine these
$(BUILDDIR)/ntexture.lexer.c: $(SRCDIR)/grammars/ntexture.lexer.flex
	flex -PNTEXTURE_ -t $<  > $@ 

$(BUILDDIR)/decorate.lexer.c: $(SRCDIR)/grammars/decorate.lexer.flex
	flex -PDECORATE_ -t $<  > $@ 

$(BUILDDIR)/%.parser.o: $(BUILDDIR)/%.parser.c $(BUILDDIR)/%.parser.h include/parser_driver.h
	$(CXX) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.parser.h: $(BUILDDIR)/%.parser.c 

$(BUILDDIR)/%.parser.c: $(SRCDIR)/grammars/%.parser.y $(LEMON)
	cp $< $(BUILDDIR)
	$(LEMON) $(BUILDDIR)/$(notdir $<)
	ls build
# TODO why does %.parser.c vanish when it's built?



### Tools

bin/doom2hexen.exe: $(BUILDDIR)/tools/doom2hexen.o
	$(LD) $(LDFLAGS) $^ -o $@

bin/wadtool.exe: $(BUILDDIR)/tools/wadtool.o $(BUILDDIR)/util/md5.o
	$(LD) $(LDFLAGS) $^ -o $@

bin/convert_deh.exe: $(BUILDDIR)/tools/convert_deh.o $(BUILDDIR)/util/mnemonics.o
	$(LD) $(LDFLAGS) $^ -o $@

# tool objects 
$(BUILDDIR)/tools/%.o: tools/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


### Phony targets

clean:
	@echo " Cleaning..."; 
	$(RM) -r $(BUILDDIR) $(TARGET)

docs:	Doxyfile
	doxygen

tools:	bin/doom2hexen.exe bin/wadtool.exe bin/convert_deh.exe

# wad building directory
WD = $(BUILDDIR)/wad

wad	: bin/wadtool.exe bin/doom2hexen.exe
ifdef WAD
	@echo "Building a new legacy.wad using an old version $(WAD)..."
	@mkdir -p $(WD)
	bin/wadtool.exe -x $(WAD) -d $(WD)
	cp resources/*.txt $(WD)
	cp resources/*.png $(WD)
	cp resources/*.h $(WD)
	cp resources/*.lmp $(WD)
	bin/doom2hexen.exe $(WD)/doom2hexen.txt $(WD)/XDOOM.lmp
	bin/doom2hexen.exe $(WD)/heretic2hexen.txt $(WD)/XHERETIC.lmp
	bin/wadtool.exe -c bin/legacy.wad -d $(WD) resources/legacy.wad.inventory
	@echo "Finished building legacy.wad."
else
	@echo "Usage: make wad WAD=/path/to/existing/legacy.wad"
endif

# OpenTNL
ifdef TNL
tnl	:
	@echo "Building TNL using source tree at $(TNL)..."
	ln -s $(TNL)/tnl include
	patch -d $(TNL) -p0 < libtnl_patch.diff
	make -C $(TNL)/libtomcrypt
	make -C $(TNL)/tnl
	mv $(TNL)/tnl/libtnl.a lib
	mv $(TNL)/libtomcrypt/libtomcrypt.a lib
	@echo "Finished building TNL."
else
tnl	:
	@echo "Usage: make tnl TNL=path_to_tnl_source"
endif
