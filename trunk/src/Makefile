###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

#LAXDIR=/home/tom/p/sourceforge/laxkit/lax
include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= $(CPPFLAGS) -Wall -g -I. -I.. -I../.. -I$(LAXDIR)/..
DEBUGFLAGS= -g -DDEBUG


objs= \
	styles.o \
	papersizes.o \
	page.o \
	group.o \
	document.o \
	interfaces.o \
	newdoc.o \
	objectcontainer.o \
	saveppt.o \
	viewwindow.o \
	extras.o \
	drawdata.o \
	spreadeditor.o \
	laidout-more.o \
	laidout.o 

all: laidout

laidout: lax laxinterface $(objs) printing impositions
	$(LD) $(LDFLAGS) $(objs) printing/*.o impositions/*.o -llaxinterfaces -llaxkit -o $@

lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

docs:
	cd ../docs && doxygen 

depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc
	cd printing && $(MAKE) depends
	cd impositions && $(MAKE) depends

include makedepend

printing: 
	cd printing && $(MAKE)

impositions:
	cd impositions && $(MAKE)


.PHONY: clean lax docs printing impositions
clean:
	rm -f laidout *.o
	cd printing && $(MAKE) clean
	cd impositions && $(MAKE) clean
	
