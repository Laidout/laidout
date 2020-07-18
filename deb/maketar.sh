#!/bin/bash

#
# Make distribution Laidout tarball.
#
# Do a FRESH clone of Laidout, then:
#   cd laidout
#   make tar
#


#
# Pull Laxkit
#
if [ ! -f laxkit ]; then
	echo Cloning Laxkit...
	git clone http://github.com/Laidout/laxkit.git laxkit
fi

#
# Setup Laxkit
#
cd laxkit
./configure
make touchdepends
make hidegarbage
make clean

#
# back to laidout top directory
#
cd ..
./configure
make touchdepends
make hidegarbage

#
# Build and generate icons
#
echo Building...
make -j 8
echo Creating icons...
make icons

#
# Remove git directories:
#
echo Cleaning up...
rm -rf .git
rm -rf laxkit/.git

#
# Clean up unnecssary stuff, but keep icons, version.h
#
make clean
rm src/configured.h Makefile-toinclude config.log

#
# Finally make tarball
#
echo Creating tarball...
cd ..
tar cjv laidout > laidout-$LAIDOUTVERSION.tar.bz2


echo "Done!"

