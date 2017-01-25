#################################################
##############                    ###############
#############  Polyptych Makefile  ##############
##############                    ###############
#################################################


##Comment the next 2 lines out if you don't want to include tuio capability
#POLYPTYCH_TUIO=-DPOLYPTYCH_TUIO
#TUIOLIBS=-llo


BINDIR=$(PREFIX)/bin

include ../../../Makefile-toinclude

LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. -I$(LAXIDIR) `freetype-config --cflags` `pkg-config GraphicsMagick++ --cflags`
LDFLAGS= -L/usr/X11R6/lib -lsqlite3 -lXft -lXi -lXext -lX11 -lftgl -lfontconfig -lm -lpng -lcairo -lcrypto `pkg-config GraphicsMagick++ --libs` `freetype-config --libs` `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)


pobjs= glbase.o gloverlay.o poly.o nets.o polyrender.o


all: polyptych spheretopoly spheretocube convertahedron panolyptych


panolyptych: lax $(pobjs) panoviewwindow.o panolyptych.o
	g++ $(pobjs) panolyptych.o panoviewwindow.o -llaxinterfaces -llaxkit $(TUIOLIBS) $(LDFLAGS) -lGL -lGLU $(CPPFLAGS) -o $@

panolyptych.o: panolyptych.cc
	g++ -c $(POLYPTYCH_TUIO)  $(CPPFLAGS) panolyptych.cc -o $@


forlaidout: nets.o poly.o

forlaidoutgl: nets.o poly.o glbase.o gloverlay.o polyrender.o hedronwindow.o


polyptych: lax $(pobjs) hedronwindow.o polyptych.o
	g++ $(pobjs) polyptych.o hedronwindow.o -llaxinterfaces -llaxkit $(TUIOLIBS) $(LDFLAGS) -lGL -lGLU $(CPPFLAGS) -o polyptych
	
polyptych.o: polyptych.cc
	g++ -c $(POLYPTYCH_TUIO)  $(CPPFLAGS) polyptych.cc -o polyptych.o


convertahedron: poly.o nets.o convertahedron.cc
	g++ convertahedron.cc  poly.o nets.o -llaxinterfaces -llaxkit $(LDFLAGS)  $(CPPFLAGS) -o $@

spheretopoly: lax spheretopoly-gm.o nets.o poly.o 
	$(LD) spheretopoly-gm.o nets.o poly.o -llaxinterfaces -llaxkit $(LDFLAGS) -lGraphicsMagick++ -o $@

spheretocube: spheretocube.o
	$(LD) spheretocube.o -lGraphicsMagick++ -llaxkit $(LDFLAGS) -o $@

spheretocubemap: spheretocubemap.o
	$(LD) spheretocubemap.o -lGraphicsMagick++ $(LAXDIR)/vectors.o -o $@

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
