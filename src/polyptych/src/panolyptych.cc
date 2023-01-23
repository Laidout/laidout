//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2011 by Tom Lechner



// polyhedron spherical texture arranger

#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <getopt.h>
#include <lax/anxapp.h>
#include <lax/vectors.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/freedesktop.h>
#include <lax/laxoptions.h>

#include <GraphicsMagick/Magick++.h>

////#define POLYPTYCH_TUIO
//#ifdef POLYPTYCH_TUIO
//#include <lax/laxtuio.h>
//#include <lax/laxtuio.cc> //it's not in laxkit proper yet
//#endif


#include "../language.h"
#include "glbase.h"
#include "gloverlay.h"
#include "poly.h"
#include "polyrender.h"
#include "panoviewwindow.h"


#include <iostream>


////template implementation:
//#include <lax/lists.cc>
//#include <lax/refptrstack.cc>


using namespace std;
using namespace Laxkit;
using namespace Polyptych;

#define DBG



//-------------------------- version() -------------------------------------
const char *version()
{
	return "Polyptych version 0.1\n"
		   "written by Tom Lechner, tomlechner.com";
}

//------------------------- Init attributes -----------------------------------


//default window size
#define WINDOWWIDTH  1000
#define WINDOWHEIGHT 800


char *spherefile=NULL;
Polyhedron poly;
int draw_texture=1;
Basis extra_basis;
double global_fontsize=20;

unsigned char *spheremap_data=NULL,
			  *spheremap_data_rotated=NULL;
//const char *spheremap="lincolnwalnut-256x128.ppm";
//const char *spheremap="wallet-512x256.ppm";
int spheremap_width,
	spheremap_height;

const char *consolefontfile="/usr/share/fonts/truetype/freefont/FreeSans.ttf";



//--------------------------- anXXApp -------------------------------

/*! the only thing this class is adding extra is really the GLXContext,
 * used by windows when calling glXMakeCurrent. need some clever way to
 * have this available without needing a whole new class for anXApp?
 *
 * like anXApp::makeReadyForDrawing(int subsystem, anXWindow *win)
 * and have stack of callbacks function[subsystem](win); ?
 */
class anXXApp : public anXApp
{
 public:
	GLXContext cx;
	XVisualInfo *glvi;
	virtual int addglwindow(anXWindow *win);
	virtual int glmakecurrent(anXWindow *win);
	virtual int init(int argc, char **argv);
};

 //Common GL initializing attributes
//static int attributeListSingle[] = {	
//		GLX_RGBA,		
//		GLX_RED_SIZE,   1,
//		GLX_GREEN_SIZE, 1,
//		GLX_BLUE_SIZE,  1,
//		GLX_DEPTH_SIZE, 1,
//		None
//	 };

static int  attributeListDouble[]  =  {
		GLX_RGBA,		
		GLX_DOUBLEBUFFER,		
		GLX_RED_SIZE,	1,	
		GLX_GREEN_SIZE,	1,
		GLX_BLUE_SIZE,  1,
		GLX_DEPTH_SIZE, 1,
		None 
	};

//! Setup the GLXContext in cx.
int anXXApp::init(int argc, char **argv)
{
	anXApp::init(argc,argv);
	glvi = glXChooseVisual(dpy, DefaultScreen(dpy), attributeListDouble);
	cx = glXCreateContext(dpy, glvi, 0, GL_TRUE);
	
	//vis=glvi->visual; //<-- why does this cause a crash when XCreateWindow?
	
	//*** would modify vis to correspond to glvi->visual, or exit
	
	if (!cx) {
		cout <<"*-*-* Error setting up GLXContext."<<endl;
		exit(0);
	}
	//Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
	return 0;
}

int anXXApp::addglwindow(anXWindow *win)
{
	return anXApp::addwindow(win);
	//***
}

//! Return 1 on success, 0 failure. *** this can be put in an aHedronWindow
int anXXApp::glmakecurrent(anXWindow *win)
{
	if (!win || !win->xlib_window) return 0;
	return glXMakeCurrent(dpy, win->xlib_window, cx);
}


//-------------------------------------------------------------------------------

//! Make a Polyhedron cube.
void defineCube()
{
	makestr(poly.name,"Cube");

	poly.vertices.push(spacepoint(1,1,1));
	poly.vertices.push(spacepoint(-1,1,1));
	poly.vertices.push(spacepoint(-1,-1,1));
	poly.vertices.push(spacepoint(1,-1,1));
	poly.vertices.push(spacepoint(-1,1,-1));
	poly.vertices.push(spacepoint(1,1,-1));
	poly.vertices.push(spacepoint(1,-1,-1));
	poly.vertices.push(spacepoint(-1,-1,-1));

	poly.faces.push(new Face("0 1 2 3",""));
	poly.faces.push(new Face("1 0 5 4",""));
	poly.faces.push(new Face("2 1 4 7",""));
	poly.faces.push(new Face("0 3 6 5",""));
	poly.faces.push(new Face("3 2 7 6",""));
	poly.faces.push(new Face("4 5 6 7",""));
	poly.connectFaces();
	poly.makeedges();
	//DBG poly.dump_out(stdout, 0, 0, NULL);
	poly.BuildExtra();
}

//--------------------------------- main(), help(), version() ----------------------------

LaxOptions options;

//! Initialize a LaxOptions object to contain Laidout's command line option summary.
/*! \ingroup lmisc
 */
void InitOptions()
{
	options.HelpHeader(version());
//	options.HelpHeader("\nProject an equirectangular sphere map onto a polyhedron.\n"
//					   "Output parameters for spheretopoly to generate faces.");
	options.UsageLine("panolyptych [options] [equirectangular image file]");

	options.Add("fontsize",  'f', 1, "Default console font size, in pixels",      0, "20");
	options.Add("tuio",      'T', 1, "Set up a tuio listener on the specified port (such as 3333)");
	options.Add("version",   'v', 0, "Print out version of the program and exit", 0, NULL);
	options.Add("help",      'h', 0, "Print out this help and exit",              0, NULL);
}


int main(int argc, char **argv) 
{
	//anXXApp app;
	anXApp app;
	app.SetTheme("Dark");
	app.init(argc,argv);
	Magick::InitializeMagick(*argv);
	const char *tuio=NULL;

	 // parse options
	int c,index, t;

	InitOptions();
	c=options.Parse(argc,argv, &index);
	if (c==-2) {
		cerr <<"Missing parameter for "<<argv[index]<<"!!"<<endl;
		exit(0);
	}
	if (c==-1) {
		cerr <<"Unknown option "<<argv[index]<<"!!"<<endl;
		exit(0);
	}

	LaxOption *o;
	for (o=options.start(); o; o=options.next()) {
		switch(o->chr()) {
			case 'h':    // Show usage summary, then exit
				options.Help(stdout);
				exit(0);
			case 'v':   // Show version info, then exit
				cout <<version()<<endl;
				exit(0);

			case 'f': {
				t=strtod(o->arg(),NULL);
				if (t>0) global_fontsize=t;
				break; 
			  }
					  
			case 'T': tuio=o->arg(); break;

		}
	}


	for (o=options.remaining(); o; o=options.next()) {
		if (!spherefile) {
			spherefile=newstr(o->arg());
			convert_to_full_path(spherefile,NULL);
			break;
		}
	}

	 //determine sphere image
	if (spherefile) {

		 //test for loadability only, hedron window does actual loading
		if (ImageLoader::Ping(spherefile, NULL,NULL,NULL,NULL) != 0) {
			cerr << "Can't load sphere file "<<spherefile<<"!"<<endl;
			exit(1);
		}

	} else {
		 //generate globe latitude+longitude image
		cerr <<"No sphere image given, using generic lines..."<<endl;
	}




	if (tuio) {
#ifdef POLYPTYCH_TUIO
		SetupTUIOListener(tuio);
		app.SetMaxTimeout(50000);
#endif
	}


	 //Create and configure the new window
	Polyptych::PanoViewWindow *w=new Polyptych::PanoViewWindow(NULL,
									 spherefile?spherefile:"Panolyptych",
									 spherefile?spherefile:"Panolyptych",
									 0,
									 0,0, WINDOWWIDTH,WINDOWHEIGHT, 0);
									 //1910-WINDOWWIDTH,0, WINDOWWIDTH,WINDOWHEIGHT, 0); //put on right side of screen
	 //set ui stuff
	w->FontAndSize(consolefontfile,global_fontsize);

	 //set up initial image
	if (spherefile) w->installImage(spherefile);
	else w->UseGenericImageData();
	w->extra_basis=extra_basis;


	 //Add window and Run!
	//app.addglwindow(w);
	app.addwindow(w);
	app.run();

	//XAutoRepeatOn(app.dpy);
	//XSync(app.dpy,True); // must sync so the autorepeat takes effect!!
	app.close();

	DBG cerr <<"-----------------program done, cleanup follows--------------------"<<endl;
	DBG InstallShortcutManager(nullptr);

	//DBG cerr << "extra_basis:"<<endl
	//DBG	 << extra_basis.p.x << ',' <<extra_basis.p.y << ',' <<extra_basis.p.z << " \\" <<endl
	//DBG	 << extra_basis.x.x << ',' <<extra_basis.x.y << ',' <<extra_basis.x.z << " \\" <<endl
	//DBG	 << extra_basis.y.x << ',' <<extra_basis.y.y << ',' <<extra_basis.y.z << " \\" <<endl
	//DBG	 << extra_basis.z.x << ',' <<extra_basis.z.y << ',' <<extra_basis.z.z << endl;
}

