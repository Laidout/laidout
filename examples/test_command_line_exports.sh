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

$LAIDOUT -X | while read -r line ; do
	echo "do something with: $line";
	$LAIDOUT -e filter='"'$line'"' test-all-objects.laidout
done

#$LAIDOUT -e 'filter=pdf'          test-all-objects.laidout
#$LAIDOUT -e 'filter=scribus'      test-all-objects.laidout
#$LAIDOUT -e 'filter=svg'          test-all-objects.laidout 
#$LAIDOUT -e 'filter=image'        test-all-objects.laidout
#$LAIDOUT -e 'filter="image via ghostscript"' test-all-objects.laidout
#$LAIDOUT -e 'filter=podofo'       test-all-objects.laidout
#$LAIDOUT -e 'filter=passepartout' test-all-objects.laidout
#$LAIDOUT -e 'filter=postscript'   test-all-objects.laidout
#$LAIDOUT -e 'filter=eps'          test-all-objects.laidout


