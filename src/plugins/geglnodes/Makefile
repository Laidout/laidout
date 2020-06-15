######################################################################
##############                                         ###############
#############  Laidout src/plugins/geglnodes Makefile   ##############
##############                                         ###############
######################################################################


include ../../../Makefile-toinclude
include makedepend

LD=g++
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(EXTRA_CPPFLAGS) -std=c++11 -fPIC -Wall $(DEBUGFLAGS) -I$(LAXDIR)/.. `pkg-config --cflags freetype2`



all: geglnodes.so


####------------------  Gegl  --------------------#####
GEGLCPPFLAGS= `pkg-config --cflags ${GEGLVERSION}`
GEGLLIBS=     `pkg-config --libs   ${GEGLVERSION}`

geglnodes.so: geglnodes.o svgnodes.o
	@echo "Compiling geglnodes with pkg-config path $(PKG_CONFIG_PATH)"
	@echo "gegl cflags: $(GEGLCPPFLAGS)"
	@echo "gegl libs  : $(GEGLLIBS)"
	g++ -shared -fPIC $(CPPFLAGS) $(GEGLCPPFLAGS) -L$(LAXDIR) svgnodes.o geglnodes.o -llaxkit $(GEGLLIBS) -o geglnodes.so

geglnodes.o: geglnodes.cc
	g++ -shared -fPIC $(CPPFLAGS) $(GEGLCPPFLAGS) -L$(LAXDIR) geglnodes.cc -llaxkit $(GEGLLIBS) -c -o $@

svgnodes.o: svg/svgnodes.cc
	g++ -shared -fPIC $(CPPFLAGS) $(GEGLCPPFLAGS) -L$(LAXDIR) svg/svgnodes.cc -llaxkit $(GEGLLIBS) -c -o $@



####--------------- maintenance functions ---------------------

depends:
	../../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

hidegarbage:
	../../hidegarbage *.cc

unhidegarbage:
	../../hidegarbage -0 *.cc


.PHONY: clean hidegarbage unhidegarbage lax laxinterfaces all plugins depends
clean:
	rm -f *.o *.so svg/*.o

