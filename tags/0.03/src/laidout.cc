//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/***  Laidout   ****/
/*** Tom Lechner ***/
/***   2006+-3   ***/


/*! \defgroup pools Builtin Pools
 * Here are the various pools of stuff and their generating functions.
 * The pools are available through the main LaidoutApp.
 */

//---------------------<< start program! >>-------------------------

#include <lax/anxapp.h>
#include <lax/version.h>
#include <lax/fileutils.h>
#include <Imlib2.h>
#include <getopt.h>

#define LAIDOUT_CC
#include "laidout.h"
#include "viewwindow.h"
#include "impositions/impositioninst.h"
#include "headwindow.h"
#include "version.h"
#include <lax/lists.cc>




#include <lax/refcounter.cc>
Laxkit::RefCounter<Laxkit::anObject> objectstack;
using namespace Laxkit;
using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 



//----------------------------------- pre-run misc -----------------------------------

//! "Laidout Version LAIDOUT_VERSION by Tom Lechner, sometime in 2006"
const char *LaidoutVersion() 
{ 
	static char *version_str=NULL;
	if (version_str==NULL) {
		makestr(version_str,"Laidout Version ");
		appendstr(version_str,LAIDOUT_VERSION);
		appendstr(version_str,"\nby Tom Lechner, sometime in 2006\n");
		appendstr(version_str,"Released under the GNU Public License, Version 2.\n");
		appendstr(version_str," (using Laxkit Version ");
		appendstr(version_str,LAXKIT_VERSION);
		appendstr(version_str,")\n");
	}
	return version_str; 
}

/*! \ingroup lmisc
 * Print usage and exit.
 */
void print_usage()
{
	cout <<LaidoutVersion()<<endl<<
		"\n laidout [options] [file1] [file2] ...\n\n"
		"Options:\n"
		"  -n --new \"letter,portrait,3pgs\"  Create new document\n"
		"    -n default (default not yet!)  default is single letter portrait, or whatever is in laidoutrc\n"
		"    -n pamphlet (not yet!)         other things can be tags to a doc style in .laidout/templates/*\n"
		"    -n whatever (not yet!)\n" 
		"  -f --rescan-fonts (not yet!)     Rescan font directories\n"
		"  -p --new-font-path dir (nope!)   Add dir to font path, and rescan fonts\n"
		"  --no-x (nope!)                   Start up command line, no other interface, useful for quick printing??\n"
		"  -v --version                     Print out version info, then exit.\n"
		"  -h --help                        Show this summary and exit.\n";
	exit(0);
}


//----------------------------------- misc -----------------------------------
/*! \defgroup lmisc Misc Random Clean Me
 * 
 * Random stuff that needs a home in the source tree that
 * is perhaps a little more meaningful or something...
 */



//----------------------- Main Control Panel: an unmapped window until further notice? -------------------------------

//class ControlPanel : public Laxkit::anXWindow
//{
// public:
//	virtual int DataEvent(EventData *data,const char *mes); 
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//};



//------------------------------- LaidoutApp --------------------------------------------

/*! \class LaidoutApp
 * \brief The Laidout application class
 *
 * This is the central control for the whole shebang.
 *
 * It keeps various pools of things, like
 * the interfaces for ViewWindow classes, impositions, papersizes, etc. This is to have
 * specific instances of available types, styles, or whatever. Other objects that use these
 * things will make copies of them to use, rather than use these things directly. These base
 * objects are kept in pools to have reference items initialized to any default settings (which might change
 * due to use input during the course of running).
 * 
 * A big TODO would be to figure out how much of this functionality can be put into static variables
 * of the various classes. In most cases, this would not be reasonable, as you might have a single class
 * that provides more than one type of behavior, like having a default BoldStyle and a default ItalicStyle,
 * but both might be instances of the same class...
 */
/*! \var Laxkit::PtrStack<Laxkit::anInterface> LaidoutApp::interfacepool
 * \ingroup pools
 * \brief Stack of available interfaces for ViewWindow objects.
 */
/*!	\var Laxkit::PtrStack<Imposition> LaidoutApp::impositionpool
 * \ingroup pools
 * \brief Stack of available impositions.
 */
/*!	\var Laxkit::PtrStack<PaperType> LaidoutApp::papersizes;
 * \ingroup pools
 * \brief Stack of available paper sizes.
 */
//class LaidoutApp : public anXApp
//{
// public:
////	ControlPanel *maincontrolpanel;
//	Project *project;
//	Document *curdoc;
////	PtrStack<Style> stylestack:
////	PtrStack<FontThing> fontstack;
////	PtrStack<Project> projectstack;
////	ScreenStyle *screen;
//	Laxkit::PtrStack<Laxkit::anInterface> interfacepool;
//	PtrStack<Imposition> impositionpool;
//	PtrStack<PaperType> papersizes;
//	LaidoutApp();
//	virtual ~LaidoutApp();
//	virtual int init(int argc,char **argv);
//	virtual void setupdefaultcolors();
//	void parseargs(int argc,char **argv);
//
//	Document *findDocument(const char *saveas);
//	int LoadDocument(const char *filename);
//	int NewDocument(DocumentStyle *docinfo);
//	int NewDocument(const char *spec);
//	int DumpWindows(FILE *f,int indent,Document *doc);
//
//	void notifyDocTreeChanged(anXWindow *callfrom=NULL);
//};

//---

//! Laidout constructor, just inits a few variables to 0.
/*! 
 */
LaidoutApp::LaidoutApp() : anXApp()
{	
	project=NULL;
	curdoc=NULL;
	tooltips=1000;
}

//! Destructor, only have to delete project!
LaidoutApp::~LaidoutApp() 
{
	DBG cout <<"Laidout destructor.."<<endl;
	 //these flush automatically, but are listed here for occasional debugging purposes...
//	papersizes.flush(); 
//	impositionpool.flush();
//	interfacepool.flush();
//	PathInterface::basepathops.flush();
	if (project) delete project;
}

//! Init pools, parse args, create a main control window.
/*! Pools must be initialized after the constructor completes, because some of the initialization
 * requires that laidout point to something meaningful.
 *
 *  <pre>
 *  	***load in any global,group,home configs
 *  	***load in any programs,modules,configs from argv
 *  	***if multiple files, use screenstyle from most recently loaded file
 *  </pre>
 * Currently existing pools are:\n
 *   impositionpool\n
 *   papersizes\n
 *   path operators\n
 *   viewer interfaces\n
 *  Planned pools are:\n
 *   fonts\n
 *   styles\n
 *   tool=name, interface, target, icon
 *
 *  Init pools, then parse args, then
 * if no windows made yet, then pop up a newdoc window. ***optionally go straight to just having
 * control window, but no open docs.
 *
 * Create a new project, document, and viewwindow as necessary.
 */
int LaidoutApp::init(int argc,char **argv)
{
	anXApp::init(argc,argv);
	setupdefaultcolors(); // ***pre-empt anything anXApp read in.....need cleaner way here!
	
	GetBuiltinImpositionPool(&impositionpool);
	GetBuiltinPaperSizes(&papersizes);
	PushBuiltinPathops(); // this must be called before getinterfaces because of pathops...
	GetBuiltinInterfaces(&interfacepool);
	
	// **** Init Imlib
	imlib_context_set_display(dpy);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(DefaultColormap(dpy, DefaultScreen(dpy)));
	imlib_set_cache_size(10 * 1024 * 1024); // in bytes

	 // Note parseargs has to come after initing all the pools and whatever else
	parseargs(argc,argv);
	
	 // Define default project if necessayr, and Pop something up if there hasn't been anything yet
	if (!project) project=new Project();
	 //*** set up main control window
	//maincontrolpanel=new ControlPanel(***);
	if (topwindows.n==0) // if no other windows have been launched yet, then launch newdoc window
		addwindow(new NewDocWindow(NULL,"New Document",ANXWIN_DELETEABLE|ANXWIN_LOCAL_ACTIVE,0,0,500,600, 0));
	
	
	return 0;
};

//! Set the builtin Laxkit colors.
/*! ***should switch to color(COLOR_BG), etc.. or color("bg")?? or some other
 * resource system to keep track of first click, directories, bookmarks, etc.
 */
void LaidoutApp::setupdefaultcolors()
{
	color_fg=          rgbcolor(32,32,32);
	color_bg=          rgbcolor(192,192,192);
	color_text=        rgbcolor(64,64,64);
	color_textbg=      rgbcolor(255,255,255);
	color_htext=       rgbcolor(0,0,0);
	color_htextbg=     rgbcolor(127,127,127);
	color_border=      rgbcolor(128,128,128);
	color_aborder=     rgbcolor(0,0,0);
	color_menutext=    rgbcolor(0,0,0);
	color_menuhtext=   rgbcolor(255,0,0);
	color_menuhbg=     rgbcolor(127,127,127);
	color_menuofftext= rgbcolor(127,0,0);
	color_button=      rgbcolor(192,192,192);
	color_buttontext=  rgbcolor(0,0,0);
	color_buttonmo=    rgbcolor(164,164,164);
	color_pad=         rgbcolor(128,128,128);
	color_tooltipbg=   rgbcolor(255,255,128);
	color_tooltiptext= rgbcolor(0,0,0);
	color_bw=          1;
}

//! Parse command line options, and load up initial documents or projects.
/*! 
 * <pre>
 *  Command line options
 *  
 *  laidout file1 file2 ...
 *  
 *     -f --rescan-fonts                 Rescan font directories
 *     -p --new-font-path dir            Add dir to font path, scan it and add to already stored list
 *     -n --new "letter, portrait 3pgs"  Open and create new document
 *        -n default                      default is single letter portrait, or whatever is in laidoutrc
 *        -n pamphlet                     other things can be tags to a doc style in .laidout/templates dir
 *        -n whatever
 *     --no-x                            Start up command line, no other interface, useful for quick printing??
 *     -v --version                      Print out version info, then exit.
 *     -h --help                         Show this summary.
 *  </pre>
 */
void LaidoutApp::parseargs(int argc,char **argv)
{
	DBG cout <<"---------start options"<<endl;
	 // parse args -- option={ "long-name", hasArg, int *vartosetifoptionfound, returnChar }
	static struct option long_options[] = {
			{ "rescan-fonts",  0, 0, 'f' },
			{ "new-font-path", 1, 0, 'p' },
			{ "new",           1, 0, 'n' },
			{ "load-dir",      1, 0, 'l' },
			{ "no-x",          0, 0, 'x' },
			{ "version",       0, 0, 'v' },
			{ "help",          0, 0, 'h' },
			{ 0,0,0,0 }
		};
	int c,index;
	while (1) {
		c=getopt_long(argc,argv,"fp:n:vh",long_options,&index);
		if (c==-1) break;
		switch(c) {
			case ':': cout <<"missing parameter..."<<endl; exit(1); // missing parameter
			case '?': cout <<"Unknown option...."<<endl; exit(1);  // unknown option
			case 'h': print_usage(); // Show usage summary, then exit
			case 'v':  // Show version info, then exit
				cout <<LaidoutVersion()<<endl;
				exit(0);
					  
			case 'f': { // --rescan-fonts
					cout << "**** rescan font path "<< endl;
				} break;
			case 'p': { // --new-font-path
					cout << "**** add \""<< optarg << "\" to font path"<< endl;
				} break;
			case 'x': { // --no-x
					cout << "**** do not use X " << endl;
				} break;
			case 'n': { // --new "letter singes 3 pages blah blah blah"
					//cout << " make new doc from: \""<< optarg << "\""<< endl;
					if (NewDocument(optarg)==0) curdoc=project->docs.e[project->docs.n-1];
				} break;
			case 'l': { // load dir
					makestr(load_dir,optarg);
				} break;
		}
	}
	int readin=0;
	if (optind<argc && argv[optind][0]=='-') { 
		DBG cout << "**** read in doc from stdin\n";
		readin=1;
	}


	// load in any docs after the args
	if (optind<argc) cout << "First non-option argv[optind]="<<argv[optind] << endl;
	DBG cout <<"*** read in these files:"<<endl;
	Document *doc;
	index=topwindows.n;
	if (!project) project=new Project;
	for (c=optind; c<argc; c++) {
		DBG cout <<"----Read in:  "<<argv[c]<<endl;
		doc=LoadDocument(argv[c]);
		if (doc && topwindows.n==index) addwindow(newHeadWindow(doc));
	}
	
	DBG cout <<"---------end options"<<endl;
}

//! Find the doc with saveas.
Document *LaidoutApp::findDocument(const char *saveas)
{
	for (int c=0; c<project->docs.n; c++) {
		if (!strcmp(saveas,project->docs.e[c]->saveas)) return project->docs.e[c];
	}
	return NULL;
}

//! Load a document from filename, putting in project, make it curdoc and return on successful load.
/*! If a doc with the same filename is already loaded, then make that curdoc, and return it.
 */
Document *LaidoutApp::LoadDocument(const char *filename)
{
	if (!strncmp(filename,"file://",7)) filename+=7;
	char *fullname=newstr(filename);
	full_path_for_file(fullname);
	Document *doc=findDocument(fullname);
	if (doc) {
		delete[] fullname;
		return curdoc=doc;
	}
		
	doc=new Document(NULL,fullname);
	project->docs.push(doc);
	
	if (doc->Load(fullname)==0) {
		project->docs.pop();
		delete doc;
		return NULL;
	}
	delete[] fullname;
	curdoc=doc;
	return doc;
}

//! Create a new document from spec and call up a new ViewWindow.
/*! spec would be something passed in on the command line,
 * like (case doesn't matter):\n
 * <tt> laidout -n "saveas blah.doc, letter, 3 pages, booklet"</tt>
 * 
 * For now, only paper size, number of pages, and very basic imposition type
 * are implemented. Does a comparison only of the characters in the field, so
 * for instance 'let' would match 'letter', and 'sing' would match 'singles'.
 * 
 * The option parser then calls this function with spec="letter, 3 pages, booklet".
 * At a minimum, you must specify the paper size and imposition.
 * "default" maps to "letter, portrait, 1 page, Singles". *** default should be
 * a setting in the laidoutrc.
 *
 * Return 0 on success, nonzero for fail.
 * 
 * ***--or--?\n
 * DocumentStyle *LaidoutApp::NewDocument(const char *spec)
 */
int LaidoutApp::NewDocument(const char *spec)
{
	if (!spec) return 1;
	if (!strcmp(spec,"default")) spec="letter, portrait, singles";
	DBG cout <<"------create new doc from \""<<spec<<"\""<<endl;
	
	char *saveas=NULL;
	Imposition *imp=NULL;
	PaperType *paper=NULL;
	int numpages=1;
	
	 // break down the spec
	char **fields=split(spec,',',NULL),*field;
	if (!fields) { cout <<"*** broken spec"<<endl; return 2; }
	int c,c2,n; // n is length of first alnum word in field
	int landscape=0;
	for (c=0; fields[c]; c++) {
		field=fields[c];
		while (field && isspace(*field)) field++;
		n=0;
		while (isalnum(field[n])) n++;
		if (!field) continue;

		 // check for new filename
		if (!strncasecmp(field,"saveas",n)) {
			field+=n;
			n=0;
			while (isspace(*field)) field++;
			while (isalnum(field[n]) || field[n]=='.' || field[n]=='_' || field[n]=='-' || field[n]=='+') n++;
			saveas=newnstr(field,n);
			continue;
		}
		
		if (isdigit(*field)) { // assume number of pages
			numpages=atoi(field); 
			if (numpages<=0) numpages=1;
			continue;
		}
		 // check papertypes
		for (c2=0; c2<papersizes.n; c2++) {
			if (!strncasecmp(field,papersizes.e[c2]->name,n)) {
				paper=papersizes.e[c2];
				break;
			}
		}
		if (c2!=papersizes.n) continue;
		if (!strncasecmp(field,"landscape",n)) {
			landscape=1;
			continue;
		} else if (!strncasecmp(field,"portrait",n)) {
			landscape=0;
			continue;
		}

		 // check imposition types
		for (c2=0; c2<impositionpool.n; c2++) {
			if (!strncasecmp(field,impositionpool.e[c2]->Stylename(),n)) {
				imp=impositionpool.e[c2];
				break;
			}
		}
		if (c2!=impositionpool.n) continue;
	}
	
	if (imp) imp=(Imposition *)imp->duplicate();
	else imp=new Singles();
	 
	if (!paper) paper=papersizes.e[0];
	unsigned int flags=paper->flags;
	paper->flags=(paper->flags&~1)|landscape;
	if (paper) imp->SetPaperSize(paper); // makes a duplicate of paper
	else imp->SetPaperSize(papersizes.e[0]);
	if (numpages==0) numpages=1;
	imp->NumPages(numpages);
	paper->flags=flags;
	
	if (!saveas) { // make a unique temporary name...
		makestr(saveas,"untitled");
	}
	
	DocumentStyle *docinfo=new DocumentStyle(imp); // copies over imp, not duplicate
	Document *newdoc=new Document(docinfo,saveas);
	if (!project) project=new Project();
	project->docs.push(newdoc);
	delete[] saveas;

	addwindow(newHeadWindow(newdoc));
	return 0;
}

//! Create a new ViewWindow for a new Document based on docinfo
/*! Puts it in the current project, creates new project if none exists.
 *
 * The docinfo is passed onto Document, which takes control of it. The calling
 * code should not delete it.
 */
int LaidoutApp::NewDocument(DocumentStyle *docinfo, const char *filename)
{
	if (!docinfo) return 1;

	Document *newdoc=new Document(docinfo,filename);
	if (!project) project=new Project();
	project->docs.push(newdoc);
	DBG cout <<"***** just pushed newdoc using docinfo->"<<docinfo->imposition->Stylename()<<", must make viewwindow *****"<<endl;
	anXWindow *blah=newHeadWindow(newdoc); 
	addwindow(blah);
	return 0;
}

//! Call dump_out() on all HeadWindow objects in topwindows.
/*! This writes out:
 * <pre>
 *  window
 *    ....whatever HeadWindow puts out
 * </pre>
 *
 * If doc!=0 then only output headwindows that only have doc in them. 
 * (this is unimplemented, might be better to have some special check when
 * loading so that if doc is being loaded from a project load, then all windows
 * in the doc are ignored?, or if doc is only doc open?)
 */
int LaidoutApp::DumpWindows(FILE *f,int indent,Document *doc)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c,n=0;
	HeadWindow *head;
	for (c=0; c<topwindows.n; c++) {
		head=dynamic_cast<HeadWindow *>(topwindows.e[c]);
		if (!head || !head->HasOnlyThis(doc)) continue;
		fprintf(f,"%swindow\n",spc);
		head->dump_out(f,indent+2,0);
		n++;
	}
	return n;
}

//---------------------------------------- main() ----------------------- This is where it all begins!
int main(int argc,char **argv)
{
	//this is rather a hacky way to do this, but:
	//process help and version before anything else happens,
	//since laxkit spews out debugging stuff straight away
	if (argc>1) {
		if (!strcmp(argv[1],"-h") || !strcmp(argv[1],"--help"))
			print_usage(); // Show usage summary, then exit
		if (!strcmp(argv[1],"-v") || !strcmp(argv[1],"--version")) {
			 // Show version info, then exit
			cout <<LaidoutVersion()<<endl;
			exit(0);
		}
	}
				
	laidout=new LaidoutApp();
	
	laidout->init(argc,argv);

	laidout->run();

	DBG cout <<"---------Laidout Close--------------"<<endl;
	laidout->close();
	delete laidout;
	
	DBG cout <<"---------------objectstack-----------------"<<endl;
	DBG cout <<"  objectstack.n="<<(objectstack.n())<<endl;
	objectstack.flush();

	cout <<"---------------Bye!-----------------"<<endl;

	return 0;
}



