# Auxiliary Makefile for Doom Legacy grammars
# Ville Bergholm 2005

i = ../include

CFLAGS = $(CF) -I$(i) -I.

grammar_objects := $(subst $(objdir),../$(objdir),$(grammar_objects))

objdir := ../$(objdir)


all	: $(grammar_objects)


ntexture.tab.c	: ntexture.y $(i)/ntexture.h
	@echo "Need to run Bison!"
	bison $<

ntexture.yy.c	: ntexture.flex $(i)/ntexture.h ntexture.tab.h
	flex $<

# implied rules

$(objdir)/%.o	: %.c
	$(CC) -c $(CFLAGS) $< -o $@

