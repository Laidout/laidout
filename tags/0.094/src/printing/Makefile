############################################################
##############                               ###############
#############  Laidout src/printing Makefile  ##############
##############                               ###############
############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I$(LAXDIR)/.. -I/usr/include/freetype2
DEBUGFLAGS= -g


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
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean printing hidegarbage unhidegarbage
clean:
	rm -f *.o
	
