###############################################################
##############                                  ###############
#############  Laidout src/dataobjects Makefile  ##############
##############                                  ###############
###############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= -Wall -g -I. -I.. -I$(LAXDIR)/.. -I/usr/include/freetype2
DEBUGFLAGS= -g


objs= \
	group.o \
	objectcontainer.o \
	drawableobject.o \
	datafactory.o \
	groupinterface.o \
	epsdata.o \
	limagepatch.o \
	limagedata.o \
	limageinterface.o \
	lgradientdata.o \
	lpathsdata.o \
	mysterydata.o \
	lsomedataref.o \
	lengraverfilldata.o \
	printermarks.o 



dataobjects: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean printing hidegarbage unhidegarbage
clean:
	rm -f *.o
	
