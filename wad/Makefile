# Gnu Make makefile for Doom Legacy
# Copyright (C) 2004 by DooM Legacy Team.
#
# This auxiliary Makefile builds/updates legacy.wad
#

.PHONY	: all

all	:
	cp ../legacy.wad .
	./wadtool.exe -x legacy.wad
	cp ../resources/*.lmp .
	./d2h.exe ../resources/doom2hexen.txt XDOOM.lmp
	./d2h.exe ../resources/heretic2hexen.txt XHERETIC.lmp
	./wadtool.exe -c legacy.wad ../resources/legacy.wad.inventory
	rm *.lmp