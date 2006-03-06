############################################################
##############                               ###############
#############  Laidout src/printing Makefile  ##############
##############                               ###############
############################################################

LAXDIR=/home/tom/p/sourceforge/lax
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= -Wall -g -I. -I.. -I$(LAXDIR)/..
DEBUGFLAGS= -g


objs= \
	dodecahedron.o \
	nets.o \
	netdisposition.o 



dispositions: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend



.PHONY: clean printing
clean:
	rm -f *.o
	
