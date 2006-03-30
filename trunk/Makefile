###############################################
##############                  ###############
#############  Laidout Makefile  ##############
##############                  ###############
###############################################


 # Where to install stuff, currently:
 #   prefix/bin
#PREFIX=/usr/local
PREFIX=testinstall


 # where the main executable goes
BINDIR=$(PREFIX)/bin


 ### If you want to be sure that an install does not clobber anything that exists
 ### already, then uncomment the line with the '--backup=t' and comment out the other.
#INSTALL=install -D --backup=t 
INSTALL=install -D


##----------- you shouldn't have to modify anything below here --------------

LAIDOUTVERSION=0.01
LAIDOUTNAME=laidout-$(LAIDOUTVERSION)

laidout: 
	cd src && $(MAKE)

all: laidout docs

docs:
	cd docs && doxygen
	

install: laidout
	echo 'Installing to $(BINDIR)/laidout which points to $(BINDIR)/$(LAIDOUTNAME)'
	$(INSTALL) -m755 src/laidout $(BINDIR)/$(LAIDOUTNAME)
	rm -f $(BINDIR)/laidout
	ln -s $(LAIDOUTNAME) $(BINDIR)/laidout

uninstall: 
	echo 'Uninstalling laidout.'
	rm -f $(BINDIR)/laidout
	rm -f $(BINDIR)/$(LAIDOUTNAME)




.PHONY: all laidout clean docs install uninstall
clean:
	cd src && $(MAKE) clean
	
