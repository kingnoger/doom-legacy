# Auxiliary Makefile for Doom Legacy SDL interface
# Ville Bergholm 2002

i = ../../include

CFLAGS = $(CF) -I$(i)

sdl_objects := $(subst $(objdir),../../$(objdir),$(sdl_objects))

objdir := ../../$(objdir)


all	: $(sdl_objects)


depend	:
	$(CC) -MM $(CFLAGS) *.cpp > sdl.dep

# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include sdl.dep
