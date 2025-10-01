###############################################################
##############                                  ###############
#############  Laidout src/dataobjects Makefile  ##############
##############                                  ###############
###############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	affinevalue.o \
	bboxvalue.o \
	objectcontainer.o \
	objectfilter.o \
	pointsetvalue.o \
	drawableobject.o \
	datafactory.o \
	groupinterface.o \
	epsdata.o \
	fontvalue.o \
	helpertypes.o \
	imagevalue.o \
	lcaptiondata.o \
	lellipsedata.o \
	lengraverfilldata.o \
	lgradientdata.o \
	limagedata.o \
	limageinterface.o \
	limagepatch.o \
	lmirrorinterface.o \
	lpathsdata.o \
	lperspectiveinterface.o \
	lroundedrectdata.o \
	lsimplepath.o \
	lsomedataref.o \
	ltextonpath.o \
	lvoronoidata.o \
	mysterydata.o \
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

