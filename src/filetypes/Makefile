#############################################################
##############                                ###############
#############  Laidout src/filetypes Makefile  ##############
##############                                ###############
#############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. -I/usr/include/freetype2


objs= \
	filefilters.o \
	filters.o \
	exportdialog.o \
	importdialog.o \
	image.o \
	laidoutimport.o \
	pdf.o \
	podofoimposeplan.o \
	postscript.o \
	ppt.o \
	scribus.o \
	svg.o



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

