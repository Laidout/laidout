#################################################
##############                    ###############
#############  Polyptych Makefile  ##############
##############                    ###############
#################################################


##Comment the next 2 lines out if you don't want to include tuio capability
POLYPTYCH_TUIO=-DPOLYPTYCH_TUIO
TUIOLIBS=-llo


BINDIR=$(PREFIX)/bin

include ../../../Makefile-toinclude

LD=g++
CPPFLAGS= -Wall -g -I. -I.. -I$(LAXDIR)/.. -I$(LAXIDIR) -I/usr/include/freetype2 -I/usr/include/GraphicsMagick/
LDFLAGS= -L/usr/X11R6/lib -lX11 -lftgl -lm -lpng -lcrypto -lGraphicsMagick++ `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR) -lXft -lXi


pobjs= glbase.o gloverlay.o poly.o nets.o polyrender.o hedronwindow.o

all: polyptych spheretopoly spheretocube convertahedron

forlaidout: nets.o poly.o

forlaidoutgl: nets.o poly.o glbase.o gloverlay.o polyrender.o hedronwindow.o

polyptych: lax $(pobjs) polyptych.o
	g++ $(pobjs) polyptych.o -llaxinterfaces -llaxkit $(TUIOLIBS) $(LDFLAGS) -lGL -lGLU $(CPPFLAGS) -o polyptych
	
polyptych.o: polyptych.cc
	g++ -c $(POLYPTYCH_TUIO)  $(CPPFLAGS) polyptych.cc -o polyptych.o

convertahedron: poly.o nets.o convertahedron.cc
	g++ convertahedron.cc  poly.o nets.o -llaxinterfaces -llaxkit $(LDFLAGS)  $(CPPFLAGS) -o $@

spheretopoly: lax spheretopoly-gm.o nets.o poly.o 
	$(LD) spheretopoly-gm.o nets.o poly.o -llaxinterfaces -llaxkit $(LDFLAGS) -lGraphicsMagick++ -o $@

spheretocube: spheretocube.o
	$(LD) -lGraphicsMagick++ spheretocube.o $(LAXDIR)/vectors.o -o $@

lax:
	cd $(LAXDIR) && $(MAKE)
	cd $(LAXDIR)/interfaces && $(MAKE)

laxinterface:
	cd $(LAXDIR)/interfaces && $(MAKE)


depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc

include makedepend


glbase.o:


.PHONY: clean lax laxinterface
clean:
	rm -f polyptych convertahedron spheretopoly spheretocube *.o
