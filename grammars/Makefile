# Auxiliary Makefile for Doom Legacy grammars
# Ville Bergholm 2005-2006

i = ../include

CFLAGS = $(CF) -I$(i) -I.

grammar_objects := $(subst $(objdir),../$(objdir),$(grammar_objects))

objdir := ../$(objdir)


all	: $(grammar_objects)



ntexture.tab.h	: ntexture.y $(i)/ntexture.h
	bison -d $<

ntexture.tab.c	: ntexture.y $(i)/ntexture.h
	bison -d $<

ntexture.yy.c	: ntexture.flex $(i)/ntexture.h ntexture.tab.h
	flex $<

$(objdir)/ntexture.o: ntexture.cpp $(i)/ntexture.h $(i)/w_wad.h ntexture.tab.h
	$(CC) -c $(CFLAGS) $< -o $@

# implied rules

$(objdir)/%.o	: %.c
	$(CC) -c $(CFLAGS) $< -o $@
