# Auxiliary Makefile for Doom Legacy utils
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

util_objects := $(subst $(objdir),../$(objdir),$(util_objects))

objdir := ../$(objdir)


all	: $(util_objects)

depend	:
	$(CC) -MM $(CFLAGS) *.cpp > util.dep

# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

#$(objdir)/%.o	: %.c
#	$(CC) -c $(CFLAGS) $< -o $@

include util.dep
