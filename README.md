LAIDOUT Version 0.096.1
=======================
http://laidout.org


WHAT IT CAN DO RIGHT NOW
------------------------
Laidout is desktop publishing software built from the ground up with
imposition in mind. Currently one may arrange pages into various 
impositions, such as a booklet, or even a dodecahedron. You can fill
pages with images, gradients (linear, radial, and mesh), mesh 
transformed images, engraving-like fill objects, and some basic text.
Export with varying degrees of success to Svg, Scribus, Pdf, and more.

Sometimes there are experimental tools you can activate by
running: `laidout --experimental` or `laidout -E`

Please post any complaints and other correspondence either to Laidout's
github issue tracker: https://github.com/Laidout/laidout/issues,
or to the Laidout general mailing list found at:
https://lists.sourceforge.net/lists/listinfo/laidout-general

Laidout is currently in the 'mostly works on my machine' stage of development.

Currently the only developer (and creator of Laidout) is:
Tom Lechner (http://www.tomlechner.com)



SYSTEM REQUIREMENTS
-------------------
Currently, Laidout only runs in variations of Linux. It is being developed
on a Debian Unstable system, thus that one is the most hassle free to setup.

Ubuntu 14.04 needs a more recent harfbuzz than is included, but more
recent Ubuntu should work well.

Laidout does not run out of the box on Macs, but it should not be difficult
to run under X11 with a few (currently undone) changes. I have no Mac to
test on. Donations of the newest, most expensive and lightest Macs are welcome!

Laidout will not currently run on Windows. Feel free to subsidize a port
by showering the developer with money.

Please see http://www.laidout.org/faq.html for some extra help. Let me
know if you get stuck trying to install. It is supposed to be easy!


COMPILING RELEASES
------------------

If you are compiling from development git, not from a release, please see
'Compiling from development git' below.


You will need a few extra development libraries on your computer in order
to compile Laidout. Running ./configure does a simple check for these,
but the check is so simple, that it may miss them or otherwise get confused.

For everything, you will need the header files for at least:
  Imlib2, harfbuzz, freetype2, fontconfig, cairo, x11, ssl, cups, sqlite3, 
  graphicsmagick++, ftgl, opengl 

If you are on a debian based system, you can probably install these with this command:

    apt-get install g++ pkg-config libpng12-dev libreadline-dev libx11-dev libxext-dev libxi-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev libcairo2-dev libharfbuzz-dev libsqlite3-dev libgraphicsmagick++1-dev mesa-common-dev libglu1-mesa-dev libftgl-dev zlib1g-dev

On Fedora, this list is more like this:

    sudo dnf install -y cairo-devel cups-devel fontconfig-devel ftgl-devel glibc-headers harfbuzz-devel imlib2-devel lcms-devel libpng-devel libX11-devel libXext-devel libXft-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel openssl-devel readline-devel sqlite-devel xorg-x11-proto-devel zlib-devel GraphicsMagick-c++-devel libstdc++-devel freetype-devel imake


For only the bare minimum, without the polyhedron unwrapper (requires opengl),
and without the ability to grab Fontmatrix font tags (requires sqlite), you 
only need the following. Note that you will also need to pass 
in "--nogl" and "--disable-sqlite" to ./configure below if you are not
compiling everything:

    apt-get install g++ pkg-config libpng12-dev libreadline-dev libx11-dev libxext-dev libxi-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev libcairo2-dev libharfbuzz-dev zlib1g-dev



To compile and install Laidout, just run these three easy steps:

    ./configure
    make
    make install
    (or sudo make install)

Note that if your computer has, say, 8 processors, you can compile much faster
using `make -j 8` instead of plain make.

Run `./configure --help` for a full list of configuration options.

`make install` will put
laidout-(whatever version) in (usually) /usr/local/bin, and make a
symbolic link /usr/local/bin/laidout point to it. To install elsewhere,
do: `./configure --prefix=/path/to/somewhere/else`

You do not need to `make install` in order to run. Simply run `src/laidout`.
Laidout does not really depend on any other files, but you may be missing
icons. If you do this , you can set a specific place or places to search
for icons by modifying the file `~/.config/laidout/(version)/laidoutrc`. This 
will have been created the first time you run Laidout. Open this file, and if
the magic worked, simply uncomment the line for `icon_dir` and set to the 
directory you want. For the uninstalled icons, this would 
be (this directory)/src/icons.


MAKING A DEB PACKAGE
--------------------
If you want to create a deb package of Laidout, make sure you have the fakeroot,
dpkg-dev, and debhelper packages installed, and have all the other packages listed 
from the COMPILING RELEASES section above, and do this from the top laidout directory:

    make deb

This makes sure there is a "debian" directory, then calls `dpkg-buildpackage -rfakeroot`.

If the magic works, you will find installable packages in the directory directly
above the Laidout directory. If it does not work, please let me know so I can fix it!

If the magic does NOT work, and you fail with this error:

    dpkg-shlibdeps: error: no dependency information found for /usr/lib/libGL.so.1

then you need to change: `dh_shlibdeps` in debian/rules to: `dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info`
Maybe something to do with non-packaged NVidia drivers?



COMPILING FROM DEVELOPMENT GIT
------------------------------
Compiling from git source requires a few more steps than compiling releases.
Be advised that the dev version may be rather buggy compared to "stable" versions!

Here is a fast and easy way to get Laidout up and running from development source:

1. Grab Laidout source from github (make sure the git package is installed):

        git clone https://github.com/Laidout/laidout.git

2. Laidout is currently built upon its own custom gui library called 
   the Laxkit (http://laxkit.sourceforge.net).

   Enter the laidout directory and get the Laxkit source:

        cd laidout
        git clone http://github.com/Laidout/laxkit.git laxkit

3. Compile the goods. Make sure you have all the packages from the Compiling Releases section 
   above, then do this:

        cd laxkit
        ./configure
        make
        cd ..
        ./configure --laxkit=`pwd`/laxkit/lax
        make

    Or all this in one command:

        cd laxkit  && ./configure  && make  && cd ..  && ./configure --laxkit=`pwd`/laxkit/lax  && make

    Do NOT `make install` yet.
    Also note that if your computer has, say, 8 processors, you can compile much faster
    using `make -j 8` instead of plain make.

4.  Icons.
    Steps 1-3 have (ideally) compiled everything. You can run Laidout (at src/laidout)
    without icons and also without installing it anywhere.

    When downloading development source code, the icons have not yet been generated. 
    Making the icons requires Inkscape (http://www.inkscape.org) to generate icons 
    from src/icons/icons.svg, and in laxkit/lax/icons/icons.svg. A python script there 
    calls Inkscape. cd to src/icons and run "make". Do the same for laxkit/lax/icons/:

        cd src/icons
        make
        cd ../../laxkit/lax/icons
        make
        cd ../../..

    The laxkit icons need to be put in the laidout icon area. do this with:

        cp laxkit/lax/icons/*png src/icons

    All this in one step:

        cd laxkit/lax/icons && make && cp *png ../../../src/icons && cd ../../../src/icons && make

    The current system can take a long time, depending on the speed of your computer, since
    the script has to start Inkscape separately for each icon. Some day, the icons will be
    generated directly from the base icons file by Laidout, for a large speed improvement, but
    much work needs to be done on the SVG importer before that can happen.

    You can use src/icons/makeimages.py to regenerate all icons, or single icons if you
    like. The master icon file is src/icons.svg. Each top level object in that file
    with ids that start with a capital letter will have an icon generated of the same
    name by running makeimages.py ThatName.

5.  Ok, now do `make install`.

6.  Running Laidout might spit out copious amounts of debugging info to stderr
    if you run from a terminal. If this is the case, you can turn this 
    off with `make hidegarbage` before doing `make` (the same goes for the 
    Laxkit). Be advised that this requires Perl to be installed. Or, you 
    can just run Laidout like this:

        laidout 2> /dev/null
    
    Also, if you make any changes to the include lines of source files, make sure
    to run `make depends`. This will update dependencies for the next time you compile.


INSTALLING DIFFERENT VERSIONS AT THE SAME TIME
----------------------------------------------

It is quite ok to install different versions of Laidout at the same time.
The ~/.config/laidout/ directory keeps all the config information in subdirectories
based on the version. The same applies for installed binaries and other
resources that get put in prefix/share/laidout. This way, you do not risk
clobbering or corrupting files from other versions. 

You can change the version number manually by specifying it when you run
./configure with somelike:

    ./configure --version=0.096-different

You should try to preserve the main version number (which is 0.096 in this
example), or it might confuse Laidout at some point.

HOWEVER, (todo) work needs to be done on "make uninstall" to not remove 
things that were installed to those directories outside of "make install". 
Right now, everything in prefix/share/laidout/version will be removed on uninstall
(but not anything in ~/.config/laidout). If this is causing you difficulty, please
let me know, and I'll try to finally fix this.

In any case, if you can, it's really safer to build the deb package, which 
automatically keeps track of such things, though with deb packages, you currently
can have only one at a time, without tinkering with debian/control.

Also, though you can have different versions coexist, there is not currently an
automatic way to convert resources from older laidout versions to newer ones.
This is on my long to-do list, but is a low priority. If this is a problem, let me know!


SOURCE CODE DOCUMENTATION and CONTRIBUTING
------------------------------------------

There is a lot of source code documentation available via doxygen, just type:

    make docs

or if you want all documentation for the Laxkit and Laidout in one place, then
make sure you have a link to the Laxkit source in the top Laidout directory,
and do:

    make alldocs
 
This will dump it all out to docs/html. In particular, you will have a 
rapidly expanding todo list. If you want to contribute, that is a good
place to start. That and the Laxkit todo list.

A down and dirty way to become aquainted with the source code is to 
download the Laxkit to the top laidout directory (if it is not there
already), and change the following lines in the Doxyfile-with-laxkit
to say YES rather than NO:

    SOURCE_BROWSER         = YES
    EXTRACT_ALL            = YES

Then from the docs directory, run `doxygen Doxyfile-with-laxkit`
(or do make alldocs from top directory), which will generate a clickable source
tree accessible from your favorite web browser.

By the way, all the code assumes tabs are 4 characters wide.
(in vim, :set tabstop=4)

Also check out the dev page on the Laidout website:
http://www.laidout.org/dev.html


Compiling on Mac OS X
---------------------
For the adventurous, you can help make Laidout work on OS X.
Straight from downloading, it will not compile all the way on OS X.
You'll still need all the libraries listed above.
I'm just guessing, since I don't have access to an OS X or other BSD
based machine, but it shouldn't be too hard to make Laidout work on them.
There are a few places where I use GNU-only functions, like getline(),
which shouldn't be very difficult to replace or define similar functions.


Compiling on Windows
--------------------
Sorry, you're out of luck at the moment. If this is important to you,
and you don't want to figure it out yourself, feel free to subsidize
a windows port with copious amounts of money.


