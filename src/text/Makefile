########################################################
##############                           ###############
#############  Laidout src/text Makefile  ##############
##############                           ###############
########################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


# uncomment these when they are actually ready for Laidout inclusion. Otherwise put them in wip
objs= \
	#cssparse.o
	#streams.o

all: $(objs)

wip: streams.o cssparse.o opentype.o

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: wip clean hidegarbage unhidegarbage
clean:
	rm -f *.o

