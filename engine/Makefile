# Auxiliary Makefile for Doom Legacy engine
# Ville Bergholm 2002

i = ../include

CFLAGS = $(CF) -I$(i)

engine_objects := $(subst $(objdir),../$(objdir),$(engine_objects))

objdir := ../$(objdir)


all	: $(engine_objects)

depend	:
	$(CC) -MM $(CFLAGS) *.cpp > engine.dep
	$(CC) -MM $(CFLAGS) animation/*.cpp >> engine.dep
	$(CC) -MM $(CFLAGS) automap/*.cpp >> engine.dep
	$(CC) -MM $(CFLAGS) hud/*.cpp >> engine.dep
	$(CC) -MM $(CFLAGS) menu/*.cpp >> engine.dep
	$(CC) -MM $(CFLAGS) scripting/*.cpp >> engine.dep


# implied rules, one for each subdirectory. sad but true.

#$(objdir)/%.o	: %.c
#	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: animation/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: automap/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: hud/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: menu/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

$(objdir)/%.o	: scripting/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include engine.dep
