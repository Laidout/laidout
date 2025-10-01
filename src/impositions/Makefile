###############################################################
##############                                  ###############
#############  Laidout src/impositions Makefile  ##############
##############                                  ###############
###############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2` -I$(POLYPTYCHBASEDIR) `pkg-config --cflags GraphicsMagick++`
#CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2` -I$(POLYPTYCHBASEDIR)


objs= \
	imposition.o \
	impositioneditor.o \
	accordion.o \
	singles.o \
	impositions.o \
	dodecahedron.o \
	box.o \
	netimposition.o \
	simplenet.o \
	signatures.o \
	signatureinterface.o \
	singlesinterface.o \
	netdialog.o \
	polyptychwindow.o

#frompolyptych= glbase.o  gloverlay.o  hedronwindow.o  nets.o  poly.o  polyrender.o


impositions: $(objs)

#nets.o:
#	g++ $(CPPFLAGS) -c ../polyptych/nets.cc -o nets.o
#
#poly.o:
#	g++ $(CPPFLAGS) -c ../polyptych/poly.cc -o poly.o

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. -I$(POLYPTYCHBASEDIR) *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc



.PHONY: clean impositions hidegarbage unhidegarbage
clean:
	rm -f *.o

