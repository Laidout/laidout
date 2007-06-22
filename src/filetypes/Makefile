#############################################################
##############                                ###############
#############  Laidout src/filetypes Makefile  ##############
##############                                ###############
#############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I$(LAXDIR)/..
DEBUGFLAGS= -g


objs= \
	filefilters.o \
	filters.o \
	exportdialog.o \
	postscript.o \
	ppt.o 



filetypes: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean filetypes hidegarbage unhidegarbage
clean:
	rm -f *.o
	
