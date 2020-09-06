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
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	affinevalue.o \
	bboxvalue.o \
	group.o \
	objectcontainer.o \
	objectfilter.o \
	drawableobject.o \
	datafactory.o \
	groupinterface.o \
	epsdata.o \
	helpertypes.o \
	imagevalue.o \
	limagepatch.o \
	limagedata.o \
	limageinterface.o \
	lgradientdata.o \
	lpathsdata.o \
	mysterydata.o \
	lsomedataref.o \
	lengraverfilldata.o \
	lperspectiveinterface.o \
	lcaptiondata.o \
	ltextonpath.o \
	lvoronoidata.o \
	printermarks.o 



dataobjects: $(objs)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean printing hidegarbage unhidegarbage
clean:
	rm -f *.o

