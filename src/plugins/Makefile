############################################################
##############                               ###############
#############  Laidout src/plugins Makefile   ##############
##############                               ###############
############################################################


include ../../Makefile-toinclude

LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= $(EXTRA_LDFLAGS) -L/usr/X11R6/lib -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` `freetype-config --libs`\
		 `cups-config --libs` -lXft -L$(LAXIDIR) -L$(LAXDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(EXTRA_CPPFLAGS) -fPIC -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. -I/usr/include/freetype2


####--  Gegl flags  --#####
GEGLCPPFLAGS= `pkg-config --cflags gegl-0.3`
GEGLLIBS=     `pkg-config --libs gegl-0.3`


#
# If you want a plugin built by default, include in objs here.
#
objs= \
	plugin.o 

plugins= \
	geglnodes.so \
	#exampleplugin.so \
	#antiprism.o


#dataobjects: $(objs)
dataobjects: $(objs) $(plugins)

plugins: $(plugins)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

test: lax laxinterface plugin.o test.o exampleplugin.so
	g++ -rdynamic $(CPPFLAGS) plugin.o test.o -llaxkit $(LDFLAGS) -ldl -o test

geglnodes.so: lax laxinterface geglnodes.cc
	g++ -shared -fPIC $(CPPFLAGS) $(GEGLCPPFLAGS) -L$(LAXDIR) geglnodes.cc -llaxkit $(GEGLLIBS) -o geglnodes.so

exampleplugin.so: lax laxinterface exampleplugin.o
	g++ $(CPPFLAGS) -shared -fPIC exampleplugin.o -L$(LAXDIR) -llaxkit -o exampleplugin.so
	#g++ $(CPPFLAGS) -shared -fPIC exampleplugin.cc -L$(LAXDIR) -o exampleplugin.so

lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean printing hidegarbage unhidegarbage lax laxinterfaces
clean:
	rm -f *.o *.so test

