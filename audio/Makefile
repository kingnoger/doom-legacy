# Auxiliary Makefile for Doom Legacy audio
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

audio_objects := $(subst $(objdir),../$(objdir),$(audio_objects))

objdir := ../$(objdir)


all	: $(audio_objects)

depend	:
	$(CC) -MM $(CFLAGS) *.cpp > audio.dep
	$(CC) -MM $(CFLAGS) hardware/*.cpp >> audio.dep

# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: hardware/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include audio.dep
