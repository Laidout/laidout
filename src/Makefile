###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng -lcups `imlib2-config --libs` -L$(LAXIDIR) -L$(LAXDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I../.. -I$(LAXDIR)/..
DEBUGFLAGS= -g -DDEBUG


objs= \
	styles.o \
	stylemanager.o \
	papersizes.o \
	page.o \
	group.o \
	document.o \
	project.o \
	interfaces.o \
	iconmanager.o \
	about.o \
	buttonbox.o \
	commandwindow.o \
	headwindow.o \
	helpwindow.o \
	newdoc.o \
	objectcontainer.o \
	saveppt.o \
	viewwindow.o \
	palettes.o \
	extras.o \
	drawdata.o \
	spreadeditor.o \
	somedialogs.o \
	laidout-more.o \
	laidout.o 

all: laidout

laidout: lax laxinterface $(objs) dataobjects printing impositions 
	$(LD) $(LDFLAGS) $(objs) dataobjects/*.o printing/*.o impositions/*.o \
			-llaxinterfaces -llaxkit -o $@
#	$(LD) $(LDFLAGS) $(objs) dataobjects/*.o printing/*.o impositions/*.o \
#			-llaxinterfaces -llaxkit -o $@

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage *.cc
	cd printing && $(MAKE) hidegarbage
	cd impositions && $(MAKE) hidegarbage
	cd dataobjects && $(MAKE) hidegarbage

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage -0 *.cc
	cd printing && $(MAKE) unhidegarbage
	cd impositions && $(MAKE) unhidegarbage
	cd dataobjects && $(MAKE) unhidegarbage

lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

test: lax laxinterface stylewindow.o  styles.o stylemanager.o test.o
	$(LD) $(LDFLAGS) test.o stylewindow.o styles.o stylemanager.o -llaxinterfaces -llaxkit -o $@

docs:
	cd ../docs && doxygen 

depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc
	cd printing && $(MAKE) depends
	cd impositions && $(MAKE) depends
	cd dataobjects && $(MAKE) depends

include makedepend

printing: 
	cd printing && $(MAKE)

impositions:
	cd impositions && $(MAKE)

dataobjects: 
	cd dataobjects && $(MAKE)

.PHONY: clean lax docs printing impositions hidegarbage unhidegarbage dataobjects
clean:
	rm -f laidout *.o
	cd printing && $(MAKE) clean
	cd impositions && $(MAKE) clean
	cd dataobjects && $(MAKE) clean
	
