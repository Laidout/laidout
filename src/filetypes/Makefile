#############################################################
##############                                ###############
#############  Laidout src/filetypes Makefile  ##############
##############                                ###############
#############################################################

include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) $(EXTRA_CPPFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`


objs= \
	filefilters.o \
	filters.o \
	exportdialog.o \
	htmlgallery.o \
	importdialog.o \
	image.o \
	image-gs.o \
	laidoutimport.o \
	pageatlas.o \
	pdf.o \
	pdfpodofo.o \
	podofoimposeplan.o \
	postscript.o \
	ppt.o \
	scribus.o \
	svg.o



filetypes: $(objs)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean filetypes hidegarbage unhidegarbage
clean:
	rm -f *.o

