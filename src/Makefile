###################################################
##############                      ###############
#############  Laidout Src Makefile  ##############
##############                      ###############
###################################################

# You'll want to change the following to where the Laxkit headers are located
LAXDIR=/home/tom/p/sourceforge/lax
LAXIDIR=$(LAXDIR)/interfaces


LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lXpm -lpng `imlib2-config --libs` -L$(LAXDIR) -L$(LAXIDIR)
CPPFLAGS= -Wall -g -I. -I.. -I../.. -I$(LAXDIR)/..
DEBUGFLAGS= -g -DDEBUG


objs= \
	styles.o \
	papersizes.o \
	page.o \
	group.o \
	document.o \
	disposition.o \
	dispositioninst.o \
	dispositions.o \
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


laidout: lax laxinterface $(objs) printing
	$(LD) $(LDFLAGS) $(objs) printing/*.o -llaxinterfaces -llaxkit -o $@

lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

docs:
	cd ../docs && doxygen 

depends:
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc

include makedepend

printing: 
	cd printing && $(MAKE)

dispositions:
	cd dispositions && $(MAKE)


.PHONY: clean lax docs printing
clean:
	rm -f laidout *.o
	cd printing && $(MAKE) clean
	cd dispositions && $(MAKE) clean
	
