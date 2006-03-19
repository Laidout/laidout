###############################################################
##############                                  ###############
#############  Laidout src/impositions Makefile  ##############
##############                                  ###############
###############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I$(LAXDIR)/..
DEBUGFLAGS= -g


objs= \
	imposition.o \
	impositioninst.o \
	impositions.o \
	dodecahedron.o \
	nets.o \
	netimposition.o 



impositions: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage

unhidegarbage:
	../hidegarbage 0



.PHONY: clean impositions hidegarbage unhidegarbage
clean:
	rm -f *.o
	
