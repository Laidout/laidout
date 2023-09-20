########################################################
##############                           ###############
#############  Laidout src/core Makefile  ##############
##############                           ###############
########################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	document.o \
	objectiterator.o \
	drawdata.o \
	externaltools.o \
	fieldplace.o \
	guides.o \
	importimage.o \
	laidoutprefs.o \
	page.o \
	papersizes.o \
	plaintext.o \
	project.o \
	spreadview.o \
	stylemanager.o \
	utils.o 



core: $(objs)

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

