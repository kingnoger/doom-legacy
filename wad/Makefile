# Gnu Make makefile for Doom Legacy
# Copyright (C) 2004 by DooM Legacy Team.
#
# This auxiliary Makefile builds/updates legacy.wad
#

.PHONY	: all

all	:
	cp ../legacy.wad .
	./wadtool.exe -x legacy.wad
	cp ../resources/*.txt .
	cp ../resources/*.png .
	cp ../resources/*.h .
	cp ../resources/*.lmp .
	./d2h.exe doom2hexen.txt XDOOM.lmp
	./d2h.exe heretic2hexen.txt XHERETIC.lmp
	./wadtool.exe -c legacy.wad ../resources/legacy.wad.inventory
	rm *.txt *.png *.h *.lmp