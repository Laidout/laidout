############################################################
##############                               ###############
#############  Laidout src/plugins Makefile   ##############
##############                               ###############
############################################################


include ../../Makefile-toinclude
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` `freetype-config --libs`\
		 `cups-config --libs` -lXft -L$(LAXIDIR) -L$(LAXDIR)
DEBUGFLAGS= -g -gdwarf-2

#               /usr/lib/x86_64-linux-gnu/glib-2.0/include/
GEGLCPPFLAGS= -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -I/usr/include/babl-0.1/ -I/usr/include/gegl-0.3/ -I/usr/include/glib-2.0 -L/usr/lib/x86_64-linux-gnu/ -I/usr/lib64/glib-2.0/include/
GEGLLIBS=     -lgegl-0.3 -lbabl-0.1 -lglib-2.0 -lgobject-2.0

CPPFLAGS= -fPIC -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. -I/usr/include/freetype2

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

