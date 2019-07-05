#!/usr/bin/python


#
# If there's new places that use a version number, add to places list below.
# This runs sed on these files to replace the numbers.
#
# Double check with git diff to make sure you are not changing things you don't want to.
#


import os


#debian/changelog debian/laidout.1 docs/doxygen/laidoutintro.txt docs/Doxyfile docs/Doxyfile-with-laxkit configure README.md
previousversion = "0.097"
newversion = "0.097.1"


basedir="../"

places = [ 
       [ "deb/changelog" ],
       [ "deb/laidout.1" ],
       [ "docs/doxygen/laidoutintro.txt" ],
       [ "docs/Doxyfile" ],
       [ "docs/Doxyfile-with-laxkit" ],
       [ "configure" ],
       [ "README.md" ]
    ] #... and all example docs


previousversion = previousversion.replace(".", "\\.")

for place in places:
    fname = basedir + place[0]
    #pattern = place[1]

    command = "sed -i s/"+previousversion+"/"+newversion+"/ " + fname
    print (command)

    os.system(command)


