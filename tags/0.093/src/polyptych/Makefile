######################################################
##############                         ###############
#############  Polyptych Main Makefile  ##############
##############                         ###############
######################################################


POLYPTYCH_NAME     =polyptych
CONVERTAHEDRON_NAME=convertahedron
SPHERETOPOLY_NAME  =spheretopoly
SPHERETOCUBE_NAME  =spheretocube


##  
## Change this to the correct prefix
##
#PREFIX=/usr/local
PREFIX=$(DESTDIR)


INSTALL=install -D
INSTALLDIR=install -d
BINDIR=$(PREFIX)/bin



LD=g++
CPPFLAGS= -Wall -g -I. -I.. -I$(LAXDIR)/.. -I$(LAXIDIR) -I/usr/include/freetype2 -I/usr/include/GraphicsMagick/
LDFLAGS= -L/usr/X11R6/lib -lX11 -lftgl -lm -lpng -lcrypto -lGraphicsMagick++ `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR) -lXft -lXi


pobjs= glbase.o gloverlay.o poly.o nets.o polyrender.o hedronwindow.o

all: 
	cd src && $(MAKE) all

forlaidout:
	cd src && $(MAKE) forlaidout

polyptych: 
	cd src && $(MAKE) polyptych

convertahedron: 
	cd src && $(MAKE) convertahedron

spheretocube: 
	cd src && $(MAKE) spheretocube

spheretopoly: 
	cd src && $(MAKE) spheretopoly

lax:
	cd $(LAXDIR) && $(MAKE)
	cd $(LAXDIR)/interfaces && $(MAKE)

laxinterface:
	cd $(LAXDIR)/interfaces && $(MAKE)

depends:
	cd src && $(MAKE) depends


install:
	echo 'Installing to $(BINDIR)/laidout which points to $(BINDIR)/$(POLYPTYCHNAME)'
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m755 src/polyptych      $(BINDIR)/$(POLYPTYCH_NAME)
	$(INSTALL) -m755 src/convertahedron $(BINDIR)/$(CONVERTAHEDRON_NAME)
	$(INSTALL) -m755 src/spheretopoly   $(BINDIR)/$(SPHERETOPOLY_NAME)
	$(INSTALL) -m755 src/spheretocube   $(BINDIR)/$(SPHERETOCUBE_NAME)


.PHONY: clean lax laxinterface all forlaidout polyptych convertahedron spheretocube spheretopoly depends
clean:
	cd src && $(MAKE) clean

