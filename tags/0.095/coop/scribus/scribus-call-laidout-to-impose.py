#!/usr/bin/env python
# -*- coding: utf-8  -*-


""" 
 This script is designed to be called from Scribus.

 This will create a new document called reimposed-from-laidout.sla.
 As this is, your Scribus document is a Letter size paper, and it gets
 imposed onto tabloid sized papers. Really the page sizes don't matter.
 To change the resulting paper size, just change where it says "Tabloid"
 below to the name of some other common paper.

 The reimposed document will be opened, and the original document will be closed.
 I would have both open, but I can't figure out how to only save a copy
 of the current document, or retrieve the filename of the current document.

 Written by Tom Lechner, tomlechner.com, 2010.
 You may modify this script and use it for any purpose at all.

 version: $Id$
"""


import sys

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

    thecommand=""
    infile=os.getcwd()+"/this-is-a-temporary-file.sla"
    reimposed=os.getcwd()+"/reimposed-from-laidout.sla"

    thecommand="laidout --command 'Import(filename=\"" \
                +infile+"\", keepmystery=2)" \
                         +" Reimpose(orientation=landscape, paper=\"Tabloid\", imposition=\"Booklet\") " \
                         +" Export(filter=ScribusExportConfig(filename=\"" \
                +reimposed+"\", layout=\"Papers\"))' "
    #print "command:\n", thecommand

    try:
        print "Creating temporary file ",infile,"..."
        scribus.saveDocAs(infile)
    except:
        print "Could not save as ",infile
        sys.exit(1)


    try:
        os.system("/home/tom/p/sourceforge/laidout/src/laidout")
        #os.system(thecommand)

    except:
        print "An error occured trying to run the system command:\n",thecommand
        sys.exit(1)

    scribus.closeDoc() #closes the "original" document, which has been rename to infile upon saveDocAs()
    scribus.openDoc(reimposed) #open the brand spanking new reimposed document

    #if (os.path.isfile(reimposed)): os.remove(reimposed) <-- keep this around
    print "Removing temporary file ",infile,"..."
    os.remove(infile)



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

