# Auxiliary Makefile for Doom Legacy tools
# Ville Bergholm 2003

i = ../include

CFLAGS = $(CF) -I$(i)

objdir := ../$(objdir)

all	: ../d2h.exe ../wadtool.exe


../d2h.exe: $(objdir)/d2h.o
	$(LD) $(LDFLAGS) $< -o $@

../wadtool.exe: $(objdir)/wadtool.o
	$(LD) $(LDFLAGS) $< -o $@


depend	:
	$(CC) -MM $(CFLAGS) *.cpp | sed -e 's/^\([^ ]\)/$$(objdir)\/\1/' > tools.dep


# implied rules

$(objdir)/%.o	: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@


include tools.dep
