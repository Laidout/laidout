###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

#LAXDIR=/home/tom/p/sourceforge/laxkit/lax
include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng -lcups `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I../.. -I$(LAXDIR)/..
DEBUGFLAGS= -g -DDEBUG


objs= \
	styles.o \
	papersizes.o \
	page.o \
	group.o \
	document.o \
	interfaces.o \
	commandwindow.o \
	headwindow.o \
	helpwindow.o \
	newdoc.o \
	objectcontainer.o \
	saveppt.o \
	viewwindow.o \
	extras.o \
	drawdata.o \
	spreadeditor.o \
	somedialogs.o \
	laidout-more.o \
	laidout.o 

all: laidout

laidout: lax laxinterface $(objs) printing impositions
	$(LD) $(LDFLAGS) $(objs) printing/*.o impositions/*.o -llaxinterfaces -llaxkit -o $@

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage
	cd printing && $(MAKE) hidegarbage
	cd impositions && $(MAKE) hidegarbage

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage 0
	cd printing && $(MAKE) hidegarbage
	cd impositions && $(MAKE) hidegarbage

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

.PHONY: clean lax docs printing impositions hidegarbage unhidegarbage
clean:
	rm -f laidout *.o
	cd printing && $(MAKE) clean
	cd impositions && $(MAKE) clean
	
