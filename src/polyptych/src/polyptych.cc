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

////template implementation:
//#include <lax/lists.cc>
//#include <lax/refptrstack.cc>


#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxFiles;

#include "../language.h"
#include "glbase.h"
#include "gloverlay.h"
#include "poly.h"
#include "polyrender.h"
#include "hedronwindow.h"

#define DBG

using namespace Polyptych;


//-------------------------- version() -------------------------------------
const char *version()
{
	return "Polyptych version 0.1\n"
		   "written by Tom Lechner, tomlechner.com";
}

//------------------------- Init attributes -----------------------------------


//default window size
#define SCREENWIDTH  1000
#define SCREENHEIGHT 800


RefPtrStack<Net> nets;
char *polyptychfile=NULL;
char *polyhedronfile=NULL,
	 *spherefile=NULL;
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
	options.UsageLine("polyptych [options] [polyptychfile]");

	options.Add("image",     'i', 0, "An equirectangular sphere image",           0, "file");
	options.Add("polyhedron",'p', 1, "An OFF, Obj, or Polyp polyhedron file",     0, "file");
	options.Add("fontsize",  'f', 1, "Default console font size, in pixels",      0, "20");
	options.Add("touch",     't', 0, "Run in touch mode, where input is all single click",   0, "");
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
	int touchmode=0;

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
			case 't': { touchmode=1; break; }

			case 'p': { //polyhedron
				makestr(polyhedronfile,o->arg());
				convert_to_full_path(polyhedronfile,NULL);
			  } break;
					  
			case 'T': tuio=o->arg(); break;

			case 'i': { //image
				makestr(spherefile,o->arg());
				convert_to_full_path(spherefile,NULL);
			  } break;
		}
	}


	for (o=options.remaining(); o; o=options.next()) {
		if (!polyptychfile) {
			polyptychfile=newstr(o->arg());
			break;
		}
	}

	convert_to_full_path(polyptychfile,NULL);

	if (polyhedronfile) {
		 //overrides polyptych file
		char *error=NULL;
		if (poly.dumpInFile(polyhedronfile,&error)) { //file not polyhedron
			cerr <<"Could not load polyhedron: "<<polyhedronfile<<endl;
			if (error) cerr <<" with error: "<<error<<endl;
			exit(1);
		}
		poly.makeedges();
		poly.BuildExtra();

		cerr <<"Read in "<<polyhedronfile<<""<<endl;

	} else if (polyptychfile) {
		 //load in polyptych file if and only if no polyhedron file given
		Attribute *att=NULL;
		Attribute polyatt;
		polyatt.dump_in(polyptychfile, 0);
		makestr(polyhedronfile,NULL);

		touch_recently_used_xbel(polyptychfile, "application/x-polyptych-doc",
								"Polyptych", "polyptych",
								"Polyptych",
								true, //visited
								false,//modified
								NULL);

		char *name, *value;
		for (int c=0; c<polyatt.attributes.n; c++) {
			name=polyatt.attributes.e[c]->name;
			value=polyatt.attributes.e[c]->value;
			att=polyatt.attributes.e[c];

			if (!polyhedronfile && !strcmp(name,"polyhedronfile")) {
				makestr(polyhedronfile,value);
			} else if (!spherefile && !strcmp(name,"spherefile")) {
				makestr(spherefile,value);
			} else if (!strcmp(name,"net")) {
				Net *net=new Net;
				net->_config=1;
				net->dump_in_atts(att,0,NULL);
				net->basenet=&poly;
				poly.inc_count();
				nets.push(net);
				net->dec_count();
				 //--further initialization done below after hedron read in

			} else if (!strcmp(name,"basis")) {
				for (int c2=0; c2<att->attributes.n; c2++) {
					name= att->attributes.e[c2]->name;
					value=att->attributes.e[c2]->value;
					double d[3];
					int n=DoubleListAttribute(value,d,3,NULL);
					if (n!=3) continue;
					if (!strcmp(name,"p")) {
						extra_basis.p=spacepoint(d[0],d[1],d[2]);

					} else if (!strcmp(name,"x")) {
						extra_basis.x=spacepoint(d[0],d[1],d[2]);

					} else if (!strcmp(name,"y")) {
						extra_basis.y=spacepoint(d[0],d[1],d[2]);

					} else if (!strcmp(name,"z")) {
						extra_basis.z=spacepoint(d[0],d[1],d[2]);
					}
				}
			}
		}

		char *error=NULL;
		if (polyhedronfile && poly.dumpInFile(polyhedronfile,&error)) { //file not polyhedron
			cerr <<"Could not load polyhedron: "<<polyhedronfile<<endl;
			if (error) cerr <<" with error: "<<error<<endl;
			exit(1);
		}
		if (polyhedronfile) {
			touch_recently_used_xbel(polyhedronfile, "application/x-polyhedron-doc", "Polyhedron", "polyptych", "Polyhedron", true, false, NULL);
		}
		if (!poly.faces.n) defineCube();

		poly.BuildExtra();

		//DBG cerr<<"poly dump after read in:"<<endl;
		//DBG poly.dump_out(stdout,0,0,NULL);
		if (nets.n) {
			 //continued initialization from loading in net above
			 //make sure facemode for all the faces are correct!
			for (int c=0; c<nets.n; c++) {
				for (int c2=0; c2<nets.e[c]->faces.n; c2++) {
					if (nets.e[c]->faces.e[c2]->tag!=FACE_Actual) continue;
					poly.faces.e[nets.e[c]->faces.e[c2]->original]->cache->facemode=nets.e[c]->object_id;				
				}
				 //set seed face for this net
				poly.faces.e[nets.e[c]->info]->cache->facemode=-nets.e[c]->object_id;
			}
		}


	} else {
		 //make a cube if no polyhedron file given
		cerr <<"Warning, no polyhedron file given, generating a cube..."<<endl;
		defineCube();
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
#ifndef POLYPTYCH_TUIO
		cerr << " *** warning: compiled without tuio support"<<endl;
#else 
		SetupTUIOListener(tuio);
		app.SetMaxTimeout(50000);
#endif
	}


	 //Create and configure the new window
	Polyptych::HedronWindow *w=new Polyptych::HedronWindow(NULL,
									 polyptychfile?polyptychfile:"unwrapper",
									 polyptychfile?polyptychfile:"unwrapper",
									 0, 0,0,SCREENWIDTH,SCREENHEIGHT, 0,
									 &poly);
	w->touchmode=touchmode;
	 //set ui stuff
	w->FontAndSize(consolefontfile,global_fontsize);
	w->SetFiles(polyhedronfile,spherefile,polyptychfile);

	 //copy over nets
	if (nets.n) {
		for (int c=0; c<nets.n; c++) {
			w->nets.push(nets.e[c]);
		}
	}

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
	DBG cerr <<"delete "<<nets.n<<" nets..."<<endl;
	DBG nets.flush();
	DBG FinalizeShortcutManager();
	DBG cerr <<"done deleting nets."<<endl;

	//DBG cerr << "extra_basis:"<<endl
	//DBG	 << extra_basis.p.x << ',' <<extra_basis.p.y << ',' <<extra_basis.p.z << " \\" <<endl
	//DBG	 << extra_basis.x.x << ',' <<extra_basis.x.y << ',' <<extra_basis.x.z << " \\" <<endl
	//DBG	 << extra_basis.y.x << ',' <<extra_basis.y.y << ',' <<extra_basis.y.z << " \\" <<endl
	//DBG	 << extra_basis.z.x << ',' <<extra_basis.z.y << ',' <<extra_basis.z.z << endl;
}

