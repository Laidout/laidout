######################################################
##############                         ###############
#############  Laidout src/ui Makefile  ##############
##############                         ###############
######################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	about.o \
	autosavewindow.o \
	buttonbox.o \
	commandwindow.o \
	headwindow.o \
	helpwindow.o \
	metawindow.o \
	importimagesdialog.o \
	newdoc.o \
	palettes.o \
	plaintextwindow.o \
	settingswindow.o \
	spreadeditor.o \
	viewwindow.o \
	interfaces.o 



ui: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean depends hidegarbage unhidegarbage
clean:
	rm -f *.o

