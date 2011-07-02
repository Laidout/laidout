#################################################
##############                    ###############
#############  Polyptych Makefile  ##############
##############                    ###############
#################################################


include ../../Makefile-toinclude

LD=g++
CPPFLAGS= -Wall -g -I. -I.. -I$(LAXDIR)/.. -I$(LAXIDIR) -I/usr/include/freetype2
LDFLAGS= -L/usr/X11R6/lib -lX11 -lftgl -lm -lpng -lcrypto -lGraphicsMagick++ `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR) -lXft -lXi


pobjs= glbase.o gloverlay.o poly.o nets.o polyrender.o hedronwindow.o


polyptych: lax $(pobjs) polyptych.o
	g++  $(CPPFLAGS) $(LDFLAGS) $(pobjs) polyptych.o  -llaxkit -llaxinterfaces -lGL -lGLU -o polyptych
	
spheretopoly: lax spheretopoly-gm.o nets.o poly.o 
	$(LD) $(LDFLAGS) -lGraphicsMagick++ spheretopoly-gm.o nets.o poly.o -llaxinterfaces -llaxkit -o $@

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
	rm -f polyptych spheretopoly spheretocube *.o
