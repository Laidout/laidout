#!/bin/sh

#
# This script provides an example to test command line exporters.
#
# List all available exporters with:
#   laidout --export-formats
#
# For a full range of export options per format, you can do this (for instance):
#   laidout --list-export-options svg
#


LAIDOUT=../src/laidout

$LAIDOUT -X | while read -r line ; do
	echo "do something with: $line";
	$LAIDOUT -e filter='"'$line'"' test-all-objects.laidout
done


