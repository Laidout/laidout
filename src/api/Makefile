#######################################################
##############                          ###############
#############  Laidout src/api Makefile  ##############
##############                          ###############
#######################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	reimpose.o \
	openandnew.o \
	importexport.o \
	buildicons.o \
	functions.o 



api: $(objs)

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

