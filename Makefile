###############################################
##############                  ###############
#############  Laidout Makefile  ##############
##############                  ###############
###############################################

objs=papersizes.o newdoc.o laidout.o


laidout: 
	cd src && $(MAKE)

all: laidout docs

docs:
	cd docs && doxygen
	

install: laidout docs
	echo '<<< Install goes here >>>'

uninstall: 
	echo '<<< Uninstall goes here >>>'




.PHONY: all laidout clean docs install uninstall
clean:
	rm laidout *.o
	
