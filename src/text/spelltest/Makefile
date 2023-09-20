########################################################
##############                           ###############
#############  Laidout src/text Makefile  ##############
##############                           ###############
########################################################

include ../../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


#objs= \
#
#	#streams.o
#
#all: $(objs)
#
#wip: streams.o

spelltest:
	g++ test-aspell.cc -laspell -o test-aspell

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

#include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: wip clean hidegarbage unhidegarbage
clean:
	rm -f *.o

