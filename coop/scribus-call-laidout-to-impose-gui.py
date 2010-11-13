#!/usr/bin/env python
# -*- coding: utf-8  -*-


""" 
 This launches "laidout --impose-only ..." to let you use Laidout's
 signature editor to reimpose a Scribus document.

 Written by Tom Lechner, tomlechner.com, 2010.
 You may modify this script and use it for any purpose at all.

 version: $Id$
"""


import sys
import tempfile

try:
    # Please do not use 'from scribus import *' . If you must use a 'from import',
    # Do so _after_ the 'import scribus' and only import the names you need, such
    # as commonly used constants.
    import scribus
except ImportError,err:
    print "This Python script is written for the Scribus scripting interface."
    print "It can only be run from within Scribus."
    sys.exit(1)

import os

def main(argv):
    if not scribus.haveDoc():
        sys.exit(1)

    laidoutexecutable="laidout"  # <---Change this to be where the laidout program is located!!
    laidoutexecutable="/home/tom/p/sourceforge/laidout/src/laidout"

    thecommand=""
    infile=os.getcwd()+"/this-is-a-temporary-file.sla"  #note this saves to a very random area usually!!
    reimposed=os.getcwd()+"/this-is-reimposed-from-laidout.sla" #  seems like directory of the plugin

    #infile =tempfile.NamedTemporaryFile()  <-- maybe this can be used instead?? how to create, get filename, then let laidout open??
    #outfile=tempfile.NamedTemporaryFile()  <-- maybe this can be used instead??

    size=scribus.getPageSize()
    units=scribus.getUnit()
    width=0
    height=0
    if (units==scribus.UNIT_MILLIMETERS):
        width=size[0]/10/2.54
        height=size[1]/10/2.54
    elif (units==scribus.UNIT_PICAS):
        width=size[0]/6
        height=size[1]/6
    elif (units==scribus.UNIT_POINTS):
        width=size[0]/72
        height=size[1]/72

    #thecommand="/bin/ls"
    thecommand=laidoutexecutable+" --impose-only " \
                         +"'in=\""+infile+"\" " \
                         +" out=\""+reimposed+"\" " \
                         +" prefer=\"booklet\"" \
                         +"'"
    #print "command:\n", thecommand

    try:
        print "Creating temporary file ",infile,"..."
        scribus.saveDocAs(infile)
    except:
        print "Could not save as ",infile
        sys.exit(1)


    try:
        print "Trying: "+thecommand
        os.system(thecommand)

    except:
        print "An error occured trying to run the system command:\n",thecommand
        sys.exit(1)

    #scribus.closeDoc() #closes the "original" document, which has been rename to infile upon saveDocAs()
    scribus.openDoc(reimposed) #open the brand spanking new reimposed document

    #if (os.path.isfile(reimposed)): os.remove(reimposed) <-- keep this around
    print "Removing temporary file ",infile,"..."
    #os.remove(infile)



def main_wrapper(argv):
    """The main_wrapper() function disables redrawing, sets a sensible generic
    status bar message, and optionally sets up the progress bar. It then runs
    the main() function. Once everything finishes it cleans up after the main()
    function, making sure everything is sane before the script terminates."""
    try:
        scribus.statusMessage("Running script...")
        scribus.progressReset()
        main(argv)
    finally:
        # Exit neatly even if the script terminated with an exception,
        # so we leave the progress bar and status bar blank and make sure
        # drawing is enabled.
        if scribus.haveDoc():
            scribus.setRedraw(True)
        scribus.statusMessage("")
        scribus.progressReset()

# This code detects if the script is being run as a script, or imported as a module.
# It only runs main() if being run as a script. This permits you to import your script
# and control it manually for debugging.
if __name__ == '__main__':
    main_wrapper(sys.argv)

