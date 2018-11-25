RELEASES
========

This file contains notes to help the developer(s) package Laidout for releases.
If you just want to make a deb package yourself, skip down to "build the package".

There are probably a number of problems and inefficiencies with this process.
Please submit patches to streamline!


MAKING LAIDOUT DEB PACKAGE AND SRC TARBALL
------------------------------------------
(if anyone has a better way of doing this, let me know)


1. Double check that these are current:
    debian/laidout.1  (use laidout --helpman to aid updating)
    README.md  <-  must have updated dependency list
    the laidoutrc description dump out in laidout.cc
    features.md
    QUICKREF.html (make quickref).

    make sure all the examples work.


2.  -----update release branch from current master in github-----
    WORK IN PROGRESS!!
        git clone https://github.com/Laidout/laidout.git
        git checkout release
        git merge -X theirs master


3.  ----modify source---
    make sure any references to the current date are accurate, currently:
        deb/laidout.1

    make sure version number is correct. Use updateversion.py for mostly auto updating.
    Currently these files are:
        deb/changelog
        deb/laidout.1                 :Version ****, for laying out books
        docs/doxygen/laidoutintro.txt :-- Version ***** --\n
        docs/Doxyfile                 :PROJECT_NUMBER         = *****
        docs/Doxyfile-with-laxkit     :PROJECT_NUMBER         = *****
        configure
        README.md                     :LAIDOUT Version *****
        all the example docs
        --> vi debian/changelog debian/laidout.1 docs/doxygen/laidoutintro.txt docs/Doxyfile docs/Doxyfile-with-laxkit configure README.md

    make sure configure defaults to 'prefix=/usr/local/'. This is what should be in a source tarball

    Make sure all examples have the correct version number, and actually load correctly

    make sure experimental shield is behaving properly  in src/interfaces.cc

    git commit --all -m'Last minute touchups to this tag'


4.  ---hide the debugging garbage and commit to the release branch---
    touch Makefile-toinclude; make touchdepends;
    make hidegarbage
    src/hidegarbage src/polyptych/src/*cc
    git commit --all -m'Hid debugging garbage'

    git push

    Delay creating actual release tag until after you test compile, just in case new errors are uncovered.


5. ---Export a fresh copy of the new tag and make a tarball.---
  a) Clone the new release, and delete the git dir.

       git clone https://github.com/Laidout/laidout.git laidout-(version)
       cd laidout-(version)
       git checkout release

     If Laxkit is to be included, you should export that to the top laidout dir:

      git clone http://github.com/Laidout/laxkit.git laxkit

      make sure in laidout/configure: LAXDIR=`pwd`/laxkit/lax

  b) Do 'cd laxkit; ./configure; make depends'
     cd to laidout top directory, we need Makefile-toinclude, so do:
       ./configure
       make touchdepends
       make clean

  c) make hidegarbage if you haven't already, in src AND in laxkit

  d) in laidout top dir: rm src/configured.h Makefile-toinclude config.log

  e) make icons  (we make before packaging for convenience, in case people don't have inkscape installed):
      cd laxkit/lax/icons; make;    # these use Inkscape to render from svg files to png
      cp *png ../../../src/icons    # <- copy the laxkit icons to the Laidout icon dir
      cd ../../../src/icons; make   # <- this will then overwrite any icons from Laidout supercede Laxkit

  f) cd to dir above laidout.
     Remove git directories:
       rm -rf .git
       rm -rf laxkit/.git

     Create final tarball:
       tar cjv (the dir) > laidout-version.tar.bz2

     This should be the distributed tarball, unpack in some other dir and do a test compile:
       mkdir test-build
       cd test-build
       tar xjvf ../laidout-version.tar.bz2
       cd laidout-version
       ./configure --prefix=`pwd`/../test-install
       make -j 8
       make install
       ../test-install/bin/laidout


6. ---build a deb package---

In top laidout directory:

  dpkg-buildpackage -rfakeroot

This often will expose otherwise unknown errors, I think because making the deb packages uses lots more
compile flags. You can do "fakeroot debian/rules binary" to not have to recompile everything after fixing.

If you fail with this error:
  dpkg-shlibdeps: error: no dependency information found for /usr/lib/libGL.so.1
then you need to change:
  dh_shlibdeps
in debian/rules to:
  dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info
Maybe something to do with non-packaged NVidia drivers?


7. --- Test ON A DIFFERENT COMPUTER ---

    If all clear, rejoice, and go to next step


8. --- finalize git release tag ---

    In github, create new release on release branch with new version tag,
    uploading tarball and deb file(s).

    After file release is uploaded, do not forget to:
     - add release tarball and deb to github or whereever
     - update the help and screenshots sections on the website, and the website in general
     - update the coop section to have links to current scripts
     - update the laidout rss feed
     - announce on the laidout mailing list, main website/rss, and g+


    git commands to remember:
        git tag -l    #<-- list all available tags
        git branch    #<-- list all available branches, use -a for more than all

