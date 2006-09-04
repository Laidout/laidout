###############################################
##############                  ###############
#############  Laidout Makefile  ##############
##############                  ###############
###############################################


 # Where to install stuff, currently:
 #   prefix/bin
#PREFIX=/usr/local
 # use this one for dpkg-buildpackage:
#PREFIX=$(DESTDIR)/usr
PREFIX=testinstall


 # where the main executable goes
BINDIR=$(PREFIX)/bin
 # like /usr/local/share/, examples go here: $SHAREDIR/laidout
SHAREDIR=$(PREFIX)/share


 ### If you want to be sure that an install does not clobber anything that exists
 ### already, then uncomment the line with the '--backup=t' and comment out the other.
#INSTALL=install -D --backup=t 
INSTALL=install -D
INSTALLDIR=install -d


##----------- you shouldn't have to modify anything below here --------------

LAIDOUTVERSION=0.04
LAIDOUTNAME=laidout-$(LAIDOUTVERSION)

laidout: 
	cd src && $(MAKE)

all: laidout docs

docs:
	cd docs && doxygen
	

#install: laidout <-- don't want to link twice, so assume 'make install' called separate
install: 
	echo 'Installing to $(BINDIR)/laidout which points to $(BINDIR)/$(LAIDOUTNAME)'
	$(INSTALL) -m755 src/laidout $(BINDIR)/$(LAIDOUTNAME)
	$(INSTALLDIR) $(SHAREDIR)/laidout/examples
	$(INSTALL) -m644 -t $(SHAREDIR)/laidout/examples examples/*
	rm -f $(BINDIR)/laidout
	ln -s $(LAIDOUTNAME) $(BINDIR)/laidout

uninstall: 
	echo 'Uninstalling laidout.'
	rm -f  $(BINDIR)/laidout
	rm -fr $(SHAREDIR)/laidout
	rm -f  $(BINDIR)/$(LAIDOUTNAME)

hidegarbage: touchdepends
	cd src && $(MAKE) hidegarbage

unhidegarbage: touchdepends
	cd src && $(MAKE) unhidegarbage

depends: touchdepends
	cd src && $(MAKE) depends

touchdepends:
	touch src/makedepend
	touch src/impositions/makedepend
	touch src/printing/makedepend

.PHONY: all laidout clean docs install uninstall hidegarbage unhidegarbage depends touchdepends
clean:
	cd src && $(MAKE) clean
	
