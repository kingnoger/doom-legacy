# Auxiliary Makefile for Doom Legacy video
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

video_objects := $(subst $(objdir),../$(objdir),$(video_objects))

objdir := ../$(objdir)


all	: $(video_objects)

depend	:
	$(CC) -MM $(CFLAGS) *.cpp hardware/*.cpp | sed -e 's/^\([^ ]\)/$$(objdir)\/\1/' > video.dep


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: hardware/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include video.dep
