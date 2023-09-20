############################################################
##############                               ###############
#############  Laidout src/printing Makefile  ##############
##############                               ###############
############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	print.o \
	psfilters.o \
	pscolorpatch.o \
	psgradient.o \
	psimage.o \
	psimagepatch.o \
	pspathsdata.o \
	pseps.o \
	epsutils.o \
	psout.o



printing: $(objs)

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

