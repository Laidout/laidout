###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

include ../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` \
		 `cups-config --libs` -lXft -L$(LAXIDIR) -L$(LAXDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(HIDEGARBAGE) -Wall $(DEBUGFLAGS) -I. -I.. -I../.. -I$(LAXDIR)/.. -I/usr/include/freetype2 -I$(POLYPTYCHBASEDIR)


#---polyptych related:
ifeq ($(LAIDOUT_USING_GL), yes)
#---if gl:
POLYPTYCHLIBS= -lGraphicsMagick++ -lGL -lGLU -lftgl 
POLYPTYCH_GL_OBJS=polyptych/src/hedronwindow.o  polyptych/src/glbase.o  polyptych/src/gloverlay.o  polyptych/src/polyrender.o
POLYPTYCHFORLAIDOUT=polyptychgl

else
#---if no gl:
POLYPTYCHLIBS= 
POLYPTYCH_GL_OBJS= 
POLYPTYCHFORLAIDOUT=polyptych
endif

#---either
POLYPTYCHOBJS=polyptych/src/nets.o polyptych/src/poly.o


dirs= \
	calculator \
	api \
	dataobjects \
	impositions \
	interfaces \
	filetypes \
	printing

objs= \
	utils.o \
	fieldplace.o \
	styles.o \
	stylemanager.o \
	papersizes.o \
	plaintext.o \
	page.o \
	spreadview.o \
	document.o \
	project.o \
	guides.o \
	interfaces.o \
	about.o \
	buttonbox.o \
	commandwindow.o \
	plaintextwindow.o \
	headwindow.o \
	helpwindow.o \
	newdoc.o \
	viewwindow.o \
	palettes.o \
	drawdata.o \
	spreadeditor.o \
	laidout-more.o \
	importimage.o \
	importimages.o \
	laidoutprefs.o \
	laidout.o 

all: laidout

laidout: lax laxinterface $(dirs) $(objs) $(POLYPTYCHFORLAIDOUT)
	$(LD) $(objs) [a-z]*/*.o $(POLYPTYCH_GL_OBJS) $(POLYPTYCHOBJS) -llaxinterfaces -llaxkit -lcairo -lfontconfig $(LDFLAGS) -lreadline -lcrypto $(POLYPTYCHLIBS) -o $@

icons:
	cd src/icons && $(MAKE)
									 
polyptych:
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) forlaidout

polyptychgl:
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) forlaidoutgl

api:
	cd api && $(MAKE)

calculator:
	cd calculator && $(MAKE)

dataobjects:
	cd dataobjects && $(MAKE)

impositions:
	cd impositions && $(MAKE)

interfaces:
	cd interfaces && $(MAKE)

filetypes:
	cd filetypes && $(MAKE)

printing:
	cd printing && $(MAKE)

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

test2: lax laxinterface interfaces test2.o papersizes.o
	$(LD) test2.o styles.o dataobjects/group.o dataobjects/objectcontainer.o papersizes.o interfaces/paperinterface.o $(LDFLAGS)  -llaxinterfaces -llaxkit -o $@

test: lax laxinterface stylewindow.o  styles.o stylemanager.o test.o
	$(LD) test.o stylewindow.o styles.o stylemanager.o $(LDFLAGS) -llaxinterfaces -llaxkit -o $@

docs:
	cd ../docs && doxygen 

alldocs:
	cd ../docs && doxygen Doxyfile-with-laxkit

depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME depends; done
	cd $(POLYPTYCHBASEDIR)/polyptych/src && $(MAKE) depends

include makedepend


.PHONY: clean lax docs $(dirs) hidegarbage unhidegarbage polyptych polyptychgl
clean:
	rm -f laidout *.o
	for NAME in $(dirs); do $(MAKE) -C $$NAME clean; done
	
