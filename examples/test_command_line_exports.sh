#!/bin/sh

#
# This script provides an example to test command line exporters.
# For a full range of export options per format, you can do this (for instance):
#   laidout --list-export-options svg
#
# Current exporters available in 0.095 are:
#
# Pdf 1.4
# Svg 1.1
# Scribus 1.3.3.12
# Image
# Podofoimpose PLAN
# Passepartout
# Postscript LL3
# EPS 3.0
#


LAIDOUT=../src/laidout


$LAIDOUT -e 'format=pdf' test-all-objects.laidout
$LAIDOUT -e 'format=scribus' test-all-objects.laidout
$LAIDOUT -e 'format=svg' test-all-objects.laidout 
$LAIDOUT -e 'format=image' test-all-objects.laidout
$LAIDOUT -e 'format=podofo' test-all-objects.laidout
$LAIDOUT -e 'format=passepartout' test-all-objects.laidout
$LAIDOUT -e 'format=postscript' test-all-objects.laidout
$LAIDOUT -e 'format=eps' test-all-objects.laidout


