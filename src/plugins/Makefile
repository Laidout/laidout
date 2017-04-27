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
CPPFLAGS= -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. -I/usr/include/freetype2


objs= \
	plugin.o
	#antiprism.o


dataobjects: $(objs)

depends:
	makedepend -fmakedepend -I.. -I$(LAXDIR)/.. -Y *.cc

include makedepend

test: plugin.o test.o exampleplugin.so
	g++ -rdynamic $(CPPFLAGS) plugin.o test.o -llaxkit $(LDFLAGS) -ldl -o test

exampleplugin.so:
	g++ $(CPPFLAGS) -shared -fPIC exampleplugin.cc -L$(LAXDIR) -o exampleplugin.so
	#g++ $(CPPFLAGS) -shared -fPIC exampleplugin.cc -L$(LAXDIR) -llaxkit -o exampleplugin.so

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc


.PHONY: clean printing hidegarbage unhidegarbage
clean:
	rm -f *.o *.so test

