######################################################
##############                         ###############
#############  Laidout src/ui Makefile  ##############
##############                         ###############
######################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	about.o \
	autosavewindow.o \
	buttonbox.o \
	commandwindow.o \
	externaltoolwindow.o \
	findwindow.o \
	headwindow.o \
	metawindow.o \
	importimagesdialog.o \
	impositionselectwindow.o \
	newdoc.o \
	objecttree.o \
	palettes.o \
	papersizewindow.o \
	plaintextwindow.o \
	pluginwindow.o \
	settingswindow.o \
	spreadeditor.o \
	viewwindow.o \
	valuewindow.o \
	interfaces.o 



ui: $(objs)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean depends hidegarbage unhidegarbage
clean:
	rm -f *.o

