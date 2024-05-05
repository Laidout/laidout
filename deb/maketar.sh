#!/bin/bash

#
# Make distribution Laidout source tarball: laidout-$LAIDOUTVERSION.tar.bz2
#
# Do a FRESH clone of Laidout, then:
#   cd laidout
#   make tar
#
# Note, clone needs to be fresh because the .git directory is removed along the way,
# and you probably don't want to do that to your working directory.
#
# Also note, this tarball is for building from source.
# It does NOT contain an executable.
#

LAIDOUTVERSION=$1
LAIDOUTCONFIG=$2

echo $LAIDOUTVERSION $LAIDOUTCONFIG

echo Creating laidout-$LAIDOUTVERSION.tar.bz2

exit_if_error() {
  local exit_code=$1
  shift
  if [ $exit_code != 0 ] ; then
	  echo
      echo "Fail($exit_code)!  $1"
      exit $exit_code
  fi
}

#
# Configure Laidout
#
echo Configuring Laidout...
#Note this will clone Laxkit if it is missing
./configure $LAIDOUTCONFIG
exit_if_error $? 'Could not configure Laidout'
make touchdepends
make hidegarbage

#
# Setup Laxkit
#
echo Setting up Laxkit...
cd laxkit
exit_if_error $? 'Laxkit missing!'
make touchdepends
make hidegarbage
cd ..

#
# Build and generate icons
#
echo Building Laidout to generate icons...
make -j 8
exit_if_error $? 'Could not build Laxkit'
echo Generating icons...
make icons
exit_if_error $? 'Could not generate icons'

#
# Remove git directories:
#
echo Cleaning up... in `pwd`
rm -rf .git
rm -rf laxkit/.git

#
# Clean up unnecssary stuff, but keep icons, version.h
#
make clean
rm src/configured.h Makefile-toinclude config.log
cd laxkit
make clean
cd ..

#
# Finally make tarball
#
echo Creating tarball...
LODIR=$(basename $(pwd))
cd ..
tar cjv $LODIR > laidout-$LAIDOUTVERSION.tar.bz2
exit_if_error $? 'Could not create tar'

echo
echo "Wrote to: laidout-$LAIDOUTVERSION.tar.bz2"
echo "Done!"

