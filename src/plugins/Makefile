############################################################
##############                               ###############
#############  Laidout src/plugins Makefile   ##############
##############                               ###############
############################################################


include ../../Makefile-toinclude

LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)
LD=g++
LDFLAGS= $(EXTRA_LDFLAGS) -L/usr/X11R6/lib -lXi -lXext -lX11 -lm -lpng `imlib2-config --libs` `pkg-config --libs freetype2`\
		 `cups-config --libs` -lXft -L$(LAXIDIR) -L$(LAXDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(EXTRA_CPPFLAGS) -std=c++11 -fPIC -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`



objs= \
	addonaction.o \
	plugin.o 

#
# If you want a plugin built by default, uncomment or add here.
#
plugins= \
	geglnodes \
	#example \
	#python \

	#gmic           #ToDO!
	#graphicsmagick #ToDO!
	#antiprism      #ToDO!


all: $(objs) plugins

allwithlax: lax laxinterface $(objs) plugins

plugins:
	for NAME in $(plugins); do \
		$(MAKE) -C $$NAME $$NAME.so; \
		cp $$NAME/*.so . ; \
	done


lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXIDIR) && $(MAKE)

depends:
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc
	for NAME in $(plugins); do $(MAKE) -C $$NAME depends ; done

include makedepend

test: lax laxinterface plugin.o test.o exampleplugin.so
	g++ -rdynamic $(CPPFLAGS) plugin.o test.o -llaxkit $(LDFLAGS) -ldl -o test


####------------------  Python  --------------------#####
PYTHONCPPFLAGS= `${PYTHONVERSION}-config --cflags`
PYTHONLIBS=     `${PYTHONVERSION}-config --ldflags`

python.so: python.cc
	@echo "Compiling python plugin with pkg-config path $(PKG_CONFIG_PATH)"
	@echo "  python cflags: $(PYTHONCPPFLAGS)"
	@echo "  python libs  : $(PYTHONLIBS)"
	g++ -shared -fPIC $(CPPFLAGS) $(PYTHONCPPFLAGS) -L$(LAXDIR) python.cc -llaxkit $(PYTHONLIBS) -o python.so


####------------------  ExamplePlugin  --------------------#####
exampleplugin.so: lax laxinterface exampleplugin.o
	g++ $(CPPFLAGS) -shared -fPIC exampleplugin.o -L$(LAXDIR) -llaxkit -o exampleplugin.so


hidegarbage:
	../hidegarbage *.cc
	for NAME in $(plugins); do $(MAKE) -C $$NAME hidegarbage ; done

unhidegarbage:
	../hidegarbage -0 *.cc
	for NAME in $(plugins); do $(MAKE) -C $$NAME unhidegarbage ; done


.PHONY: clean hidegarbage unhidegarbage lax laxinterfaces all plugins depends
clean:
	rm -f *.o *.so test
	for NAME in $(plugins); do $(MAKE) -C $$NAME clean ; done

