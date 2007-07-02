###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng -lcups `imlib2-config --libs` -lXft -L$(LAXIDIR) -L$(LAXDIR)
CPPFLAGS= $(HIDEGARBAGE) -Wall -g -I. -I.. -I../.. -I$(LAXDIR)/.. -I/usr/include/freetype2
DEBUGFLAGS= -g -DDEBUG


dirs= \
	dataobjects \
	impositions \
	interfaces \
	filetypes \
	printing 

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
	viewwindow.o \
	palettes.o \
	extras.o \
	drawdata.o \
	spreadeditor.o \
	laidout-more.o \
	importimages.o \
	laidout.o 

all: laidout

laidout: lax laxinterface $(objs) $(dirs) 
	$(LD) $(LDFLAGS) $(objs) [a-z]*/*.o -llaxinterfaces -llaxkit -lssl -o $@

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME hidegarbage; done

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage -0 *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME unhidegarbage; done

lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

test: lax laxinterface stylewindow.o  styles.o stylemanager.o test.o
	$(LD) $(LDFLAGS) test.o stylewindow.o styles.o stylemanager.o -llaxinterfaces -llaxkit -o $@

docs:
	cd ../docs && doxygen 

alldocs:
	cd ../docs && doxygen Doxyfile-with-laxkit

depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME depends; done

include makedepend

interfaces: 
	cd interfaces && $(MAKE)

filetypes: 
	cd filetypes && $(MAKE)

printing: 
	cd printing && $(MAKE)

impositions:
	cd impositions && $(MAKE)

dataobjects: 
	cd dataobjects && $(MAKE)

.PHONY: clean lax docs $(dirs) hidegarbage unhidegarbage
clean:
	rm -f laidout *.o
	for NAME in $(dirs); do $(MAKE) -C $$NAME clean; done
	
