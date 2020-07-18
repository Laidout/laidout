Manually making deb package and src tarball for releases
========================================================


This file contains notes to help the developer(s) package Laidout for releases.
If you just want to make a deb package yourself, skip down to "build the package".

There are probably a number of problems and inefficiencies with this process.
Please submit patches to streamline!



## 1. Double check that these are current:

- `debian/laidout.1`  (use `laidout --helpman` to aid updating)
- `README.md`  <-  must have updated dependency list
- the `laidoutrc` description dump out in laidout.cc
- `features.md`, new notes edited from LEFT-OFF/DONE notes
- `QUICKREF.html` (make quickref).

Make sure all the examples work.


## 2. Update release branch from current master in github

WORK IN PROGRESS!!

	git clone https://github.com/Laidout/laidout.git
	git checkout release
	git merge -X theirs master


## 3. Modify source for new version

Make sure any references to the current date are accurate, currently:

	deb/laidout.1

Make sure version number is correct. Use updateversion.py for mostly auto updating.
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

Make sure configure defaults to 'prefix=/usr/local/'. This is what should be in a source tarball.

Make sure all examples have the correct version number, and actually load correctly

Make sure experimental shield is behaving properly  in src/interfaces.cc

Commit changes:

	git commit --all -m'Last minute touchups to this tag'


## 4. Hide the debugging garbage and commit to the release branch

	touch Makefile-toinclude
	make touchdepends
	make hidegarbage
	src/hidegarbage src/polyptych/src/*cc
	git commit --all -m'Hid debugging garbage'

	git push

Delay creating actual release tag until after you test compile, just in case new errors are uncovered.


## 5. Export a fresh copy of the new tag, make a tarball, and test.

1. Clone the new release, and delete the git dir.

	   git clone https://github.com/Laidout/laidout.git laidout-(version)
	   cd laidout-(version)
	   git checkout release
	   make tar

2. Now unpack this new tarball in some other dir, do a test run, and verify install:

	   mkdir test-build
	   cd test-build
	   tar xjvf ../laidout-version.tar.bz2
	   cd laidout-version
	   ./configure --prefix=`pwd`/../test-install
	   make -j 8
	   make install
	   ../test-install/bin/laidout


## 6. Build a deb package

In top laidout directory:

	dpkg-buildpackage -rfakeroot

This often will expose otherwise unknown errors, I think because making the deb packages uses lots more
compile flags. You can do `fakeroot debian/rules binary` to not have to recompile everything after fixing.

If you fail with this error:

	dpkg-shlibdeps: error: no dependency information found for /usr/lib/libGL.so.1

then you need to change:

	dh_shlibdeps

in debian/rules to:

	dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info

Maybe something to do with non-packaged NVidia drivers?


## 7. Test ON A DIFFERENT COMPUTER

If all clear, rejoice, and go to next step


## 8. Finalize git release tag

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


## 9. Update version number in master

Change version number to new dev version number in deb/updateversion.py and run it.

Move DONE things from LEFT---OFF to the DONE file, don't look back and head for the hills.

