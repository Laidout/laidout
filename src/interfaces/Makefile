##############################################################
##############                                 ###############
#############  Laidout src/interfaces Makefile  ##############
##############                                 ###############
##############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	actionarea.o \
	nupinterface.o \
	aligninterface.o \
	anchorinterface.o \
	pagerangeinterface.o \
	graphicalshell.o \
	objectindicator.o \
	paperinterface.o \
	cloneinterface.o \
	animationinterface.o \
	pagemarkerinterface.o \
	objectfilterinterface.o \
	pathintersectionsinterface.o \



interfaces: $(objs)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean interfaces hidegarbage unhidegarbage
clean:
	rm -f *.o

