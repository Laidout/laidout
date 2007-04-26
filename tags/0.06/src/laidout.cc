//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2005-2007 by Tom Lechner
//


/*! \defgroup pools Builtin Pools
 * Here are the various pools of stuff and their generating functions.
 * The pools are available through the main LaidoutApp.
 * 
 * *** The interface pool is necessary to be able to draw anything any old time.
 * The others could probably be absorbed by the stylemanager.
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
#include "stylemanager.h"
#include "configured.h"
#include "printing/epsutils.h"
#include <lax/lists.cc>

#include <sys/stat.h>
#include <cstdio>

StyleManager stylemanager;


using namespace Laxkit;
using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 



//----------------------------------- pre-run misc -----------------------------------

//! "Laidout Version LAIDOUT_VERSION by Tom Lechner, sometime in 2007"
/*! \ingroup lmisc
 */
const char *LaidoutVersion() 
{ 
	static char *version_str=NULL;
	if (version_str==NULL) {
		makestr(version_str,"Laidout Version ");
		appendstr(version_str,LAIDOUT_VERSION);
		appendstr(version_str,"\nby Tom Lechner, sometime in 2007\n");
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
		"  -t --template templatename       Start laidout from this template in the .laidout/templates\n"
		"  -n --new \"letter,portrait,3pgs\"  Create new document\n"
		"    -n whatever (not yet!)         Start up laidout with a Whatever imposition\n" 
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

//! Redefinition of the default Laxkit preview generator.
/*! \ingroup misc
 *
 * This can create previews of eps files..
 *
 * Return 0 for success.
 */
int laidout_preview_maker(const char *original, const char *preview, int width, int height, int fit)
{
	if (!_laximlib_generate_preview(original,preview,width,height,fit)) return 0;

	 //normal preview maker didn't work, so try something else...
	DoubleBBox bbox;
	char *title,*date;
	int depth,w,h;
	FILE *f=fopen(original,"r");
	if (!f) return 1;
	int c=scaninEPS(f,&bbox,&title,&date,NULL,&depth,&w,&h);
	fclose(f);
	if (c) return 1; //not eps probably
	return WriteEpsPreviewAsPng(GHOSTSCRIPT_BIN,
						 original,w,h,
						 preview,width,height,
						 NULL);
}





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
 *
 * \todo When Laxkit::FileDialog is more fully developed, palette_dir should be more
 *   like palette_path, and the paths beyound the first dir would be bookmarks.
 */
/*! \var Laxkit::PtrStack<Laxkit::anInterface> LaidoutApp::interfacepool
 * \ingroup pools
 * \brief Stack of available interfaces for ViewWindow objects.
 */
/*!	\var Laxkit::PtrStack<Imposition> LaidoutApp::impositionpool
 * \ingroup pools
 * \brief Stack of available impositions.
 */
/*!	\var Laxkit::PtrStack<PaperStyle> LaidoutApp::papersizes;
 * \ingroup pools
 * \brief Stack of available paper sizes.
 */
/*! \var char LaidoutApp::preview_transient
 * \brief Whether newly created previews should be deleted when no longer in use.
 */
/*! \var char *LaidoutApp::temp_dir
 * \brief Where to store temporary files.
 *
 * \todo *** imp me! Maybe: create laidoutrc->temp_dir/pid, then upon program termination
 *   or completion, delete that directory.
 */
/*! \var char *LaidoutApp::preview_file_base
 * \brief The template used to name preview files for images.
 *
 * When Laidout generates new preview images for existing image files,
 * the original suffix for the image file is stripped and inserted
 * into this string. So say a file is "image.tiff" and preview_file_base
 * is "%-s.jpg" (which is the default), then the preview file will
 * be "image-s.jpg". Generated preview files are typically jpg files. If
 * the base is "../thumbs/%-s.jpg" then the preview file will be
 * generated at "../thumbs" relative to where the image file is located.
 *
 * If the base is ".laidout-*.jpg", using a '*' rather than a '%', when
 * the full filename is substituted, rather than just the basename.
 *
 * \todo be able to create preview files of different types by default...
 */ 
/*! \var int LaidoutApp::preview_over_this_size
 * \brief The file size in kilobytes over which preview images should be used where possible.
 */


//! Laidout constructor, just inits a few variables to 0.
/*! 
 */
LaidoutApp::LaidoutApp() : anXApp()
{	
	config_dir=newstr(getenv("HOME"));
	appendstr(config_dir,"/.laidout/");
	appendstr(config_dir,LAIDOUT_VERSION);
	
	curcolor=0;
	lastview=NULL;
	
	project=new Project;
	curdoc=NULL;
	tooltips=1000;

	 // laidoutrc defaults
	defaultpaper=NULL;
	palette_dir=newstr("/usr/share/gimp/2.0/palettes");
	temp_dir=NULL;
	default_template=NULL;
	
	preview_over_this_size=250; 
	preview_file_base=newstr(".laidout-%.jpg");
	max_preview_length=200;
	max_preview_width=max_preview_height=-1;
	preview_transient=1; 

//	MenuInfo *menu=new MenuInfo("Main Menu");
//	 menu->AddItem("File",1);
//	 menu->SubMenu("File");
//	  menu->AddItem("Open");
//	  menu->AddItem("Open Project");
//	  menu->AddItem("Save");
//	  menu->AddItem("Save As...");
//	  menu->AddItem("Save All");
//	  menu->AddItem("Quit");
//	  menu->AddSep();
//	  menu->AddItem("Print...");
//	 menu->EndSubMenu();
//	 menu->AddItem("Viewer");
//	 menu->SubMenu("Viewer");
//	  menu->AddItem("Object Tool");
//	  menu->AddItem("Path Tool");
//	  menu->AddItem("Image Tool");
//	  menu->AddItem("Image Patch Tool");
//	  menu->AddItem("Gradient Tool");
//	  menu->AddItem("Text Tool");
//	  menu->AddItem("Magnify Tool");
//	  menu->AddSep();
//	 menu->EndSubMenu();
//	 menu->AddSep();
//	 menu->AddItem("About");
//	 menu->AddItem("Help");
//	dumpmenu(menu);
}

//! Destructor, only have to delete project!
LaidoutApp::~LaidoutApp() 
{
	//DBG cout <<"Laidout destructor.."<<endl;
	 //these flush automatically, but are listed here for occasional debugging purposes...
//	papersizes.flush(); 
//	impositionpool.flush();
//	interfacepool.flush();
//	PathInterface::basepathops.flush();
	if (project) delete project;
	if (config_dir) delete[] config_dir;
	
	if (defaultpaper) delete[] defaultpaper;
	if (palette_dir) delete[] palette_dir;
	if (temp_dir) delete[] temp_dir;
	
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
 *
 * \todo  manually adding a couple of pagestyles here there should be a better way to handle initializations.
 *   this should be cleared when plugin architecture is functional
 * \todo when the program is run before installing, should be able to automatically detect that
 *   and find icons and other resources (***NOT WORKING RIGHT NOW!!!) this seems to require checking
 *   argv[0]. If it is an absolute pathname or a definite pathname like "dir/blah" then it's
 *   location is clear. If it is merely "blah" then the PATH environmental variable must be
 *   used to search for the first user executable path1/blah, path2/blah, etc. However, care must
 *   be taken to traverse links properly, and not get caught in never ending link loops.
 */
int LaidoutApp::init(int argc,char **argv)
{
	anXApp::init(argc,argv); //setupdefaultcolors() is called here
	
	 //------------ make adjustments to some standard dirs 
	 //             when running before installing
 
	 // find the current executable path
	char *curexecpath;
	if (argv[0][0]!='/') {
		char *d=get_current_dir_name();
		curexecpath=newstr(d);
		appendstr(curexecpath,"/");
		appendstr(curexecpath,argv[0]);
		free(d);
		simplify_path(curexecpath);
	} else curexecpath=newstr(argv[0]);
	
	 // add either configured icon_dir to icons 
	 // or ./icons if the installed executable path is not the same as current executable path
	 //*******this doesn't work, curexecpath resolves to $curdir/laidout when you run. CRAP!!
	if (strcmp(BIN_PATH,curexecpath)) {
		char *iconpath=lax_dirname(curexecpath,0);
		appendstr(iconpath,"/icons");
		icons.addpath(iconpath);
		//DBG cout <<"Added uninstalled icon dir "<<iconpath<<" to icon path"<<endl;
		delete[] iconpath;
	} else {
		//DBG cout <<"Added installed icon dir "<<ICON_DIRECTORY<<" to icon path"<<endl;
		icons.addpath(ICON_DIRECTORY);
	}
	delete[] curexecpath;


	 //------setup initial pools
	//DBG cout <<"---imposition pool init"<<endl;
	GetBuiltinImpositionPool(&impositionpool);
	
	//DBG cout <<"---papersizes pool init"<<endl;
	GetBuiltinPaperSizes(&papersizes);
	
	//DBG cout <<"---interfaces pool init"<<endl;
	PushBuiltinPathops(); // this must be called before getinterfaces because of pathops...
	GetBuiltinInterfaces(&interfacepool);

	
	 // manually adding a couple of pagestyles
	 // *** why?
	//DBG cout <<"---manually adding a couple of pagestyles"<<endl;
	PageStyle *ps=new PageStyle;
	StyleDef *sd=ps->makeStyleDef();
	stylemanager.AddStyleDef(sd,1);
	delete ps;
	ps=new RectPageStyle(RECTPAGE_LRTB);
	sd=ps->makeStyleDef();
	stylemanager.AddStyleDef(sd,1);
	delete ps;

	 //read in laidoutrc
	if (!readinLaidoutDefaults()) {
		createlaidoutrc();
	}
	
	//****this should be done in a more automatic way via Laxkit: Init Imlib
	imlib_context_set_display(dpy);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(DefaultColormap(dpy, DefaultScreen(dpy)));
	imlib_set_cache_size(10 * 1024 * 1024); // in bytes

	 // Note parseargs has to come after initing all the pools and whatever else
	parseargs(argc,argv);
	
	 // Define default project if necessary, and Pop something up if there hasn't been anything yet
	if (!project) project=new Project();

	 //*** set up main control window
	//maincontrolpanel=new ControlPanel(***);
	
	 //try to load the default template if no windows are up
	if (topwindows.n==0 && default_template) {
		LoadTemplate(default_template);
	}
	
	 // if no other windows have been launched yet, then launch newdoc window
	if (topwindows.n==0)
		addwindow(new NewDocWindow(NULL,"New Document",ANXWIN_DELETEABLE|ANXWIN_LOCAL_ACTIVE,0,0,500,600, 0));
	
	return 0;
};

//! Called from init(), creates a user's laidoutrc if readinLaidoutDefaults failed.
/*! Return 0 for created ok, else non-zero error.
 *
 * \todo should separate the laidoutrc writing functions to allow easy dumping to any
 *   stream like stdout.
 */
int LaidoutApp::createlaidoutrc()
{
	//DBG cout <<"-------------Creating $HOME/.laidout/(version)/laidoutrc----------"<<endl;

	 // ensure that ~/.ladiout/(version) exists
	 //   if not, create, and put in a README explaining what's what:
	 //   	laidoutrc
	 //   	icons/
	 //   	palettes/
	 //   	templates/
	 //   	templates/default
	 //   	impositions/
	 // if no ~/.laidout/(version)/laidoutrc exists, possibly import
	 //   from other installed laidout versions
	 //   otherwise perhaps touch laidoutrc, and put in a much
	 //   commented empty version
	
	int t=check_dirs(config_dir,1);
	if (t==-1) { // dirs were ok
		 // create "~/.laidout/(version)/laidoutrc"
		char path[strlen(config_dir)+20];
		sprintf(path,"%s/laidoutrc",config_dir);
		FILE *f=fopen(path,"w");
		if (f) {
			fprintf(f,"#Laidout %s laidoutrc\n",LAIDOUT_VERSION);
			fprintf(f,"\n"
					  "# Laidout global configuration options go in here.\n"
					  "\n"
					  " #The number of kilobytes a file must be to trigger\n"
					  " #the automatic creation of a smaller preview image file.\n"
					  "#previewThreshhold 200\n"
					  "#previewThreshhold never\n"
					  "\n"
					  " #You can have previews that are created during the program\n"
					  " #be deleted when the program exits, or have newly created previews remain:\n"
					  "#temporaryPreviews yes  #yes to delete new previews on exit\n"
					  "#temporaryPreviews      #same as: temporaryPreviews yes\n"
					  "#permanentPreviews yes  #yes to not delete new preview images on exit\n"
					  "\n"
					  " #When preview files are not specified, Laidout tries to find one\n"
					  " #based on a sort of name template. Say an image is filename.tiff, then\n"
					  " #the first line implies using a preview file called filename-s.jpg,\n"
					  " #where the '%%' stands for the original filename minus its the final suffix.\n"
					  " #The second looks for .laidout-previews/filename.tiff.jpg, relative to the\n"
					  " #directory of the original file. The '*' stands for the entire original filename.\n"
					  " #The final example is an absolute path, and will try to create all preview files\n"
					  " #in that place, in this case, it would try ~/laidout/tmp/filename.tiff.jpg.\n"
					  "#defaultPreviewName %%-s.jpg\n"
					  "#defaultPreviewName .laidout-previews/*.jpg\n"
					  "#defaultPreviewName ~/.laidout/tmp/*.jpg\n"
					  "\n"
					  "\n# The maximum width or height for preview images\n"
					  "#maxPreviewLength 200\n"
					  //" # Alternately, you can specify the maximum width and height separately:\n"
					  //"#maxPreviewWidth 200\n"
					  //"#maxPreviewHeight 200\n"
					  "\n"

					  " #if the following is commented out, then running \"laidout\" will\n"
					  " #always bring up the new document dialog. If it is uncommented, then the\n"
					  " #specified file is loaded with the filename removed so that trying to save will\n"
					  " #force entering a new name and location. If the file is not an absolute path,\n"
					  " #then it is assumed to be relative to ~/laidout/(version)/templates.\n"
					  "#default_template ./templates/default\n"
					  "\n"
					  " # Some assorted directories:\n");
			fprintf(f,"#icon_dir %s/icons\n",SHARED_DIRECTORY);
			fprintf(f,"#palette_dir /usr/share/gimp/2.0/palettes\n"
					  "\n"
					  "\n");
			fclose(f);
		}
		 // create the other relevant directories
		sprintf(path,"%s/templates",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/impositions",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/palettes",config_dir);
		mkdir(path,0755);
	} else return -1;
	return 0;
}

//! Read in various startup defaults. Essentially read in the laidoutrc.
/*! 
 * Return 0 if laidoutrc doesn't exist, 1 if ok.
 * The default location is $HOME/.laidout/(version)/laidoutrc.
 */
int LaidoutApp::readinLaidoutDefaults()
{
	//DBG cout <<"-------------Checking $HOME/.laidout/(version)/laidoutrc----------"<<endl;
	FILE *f=NULL;
	char configfile[strlen(config_dir)+20];
	sprintf(configfile,"%s/laidoutrc",config_dir);
	if (!readable_file(configfile,&f)) return 0;

	 //now f is open, must read in...
	Attribute att;
	att.dump_in(f,0);
	char *name,*value;
	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;
		if (!name) continue;

		//DBG cout <<(name?name:"(no name)")<<": "<<(value?value:"(no value)")<<endl;
		if (!strcmp(name,"appcolors")) {
			cout <<"***imp me! readinlaidoutrc: appcolors"<<endl;
			
		} else if (!strcmp(name,"default_template")) {
			if (file_exists(value,1,NULL)==S_IFREG) makestr(default_template,value);
			else {
				char *fullname=full_path_for_resource(value,"templates");
				if (file_exists(fullname,1,NULL)==S_IFREG) makestr(default_template,value);
				delete[] fullname;
			}
			//default to config_dir/templates/default?

		} else if (!strcmp(name,"defaultpapersize")) {
			makestr(defaultpaper,value); //*** bit hacky, should have custom width/height, whatever, etc
		
		} else if (!strcmp(name,"template")) {
			cout <<"***imp me! readinlaidoutrc: template"<<endl;
			//*** for multiple startup templates not found in config_dir/templates,
			// template nickname /file/path
			// > laidout --template consumptionIssue
			// > laidout --template 1paperPamphlet
		
		} else if (!strcmp(name,"icon_dir")) {
			if (file_exists(value,1,NULL)==S_IFDIR) makestr(icon_dir,value);
			cout <<" *** must implement icon_dir in laidoutrc!!"<<endl;
		
		} else if (!strcmp(name,"palette_dir")) {
			if (file_exists(value,1,NULL)==S_IFDIR) makestr(palette_dir,value);
		
		} else if (!strcmp(name,"temp_dir")) {
			//**** default "config_dir/temp/pid/"?
			//				or projectdir/.laidouttemp/previews
			//	make sure supplied tempdir is writable before using.
			cout <<" *** imp temp_dir in laidoutrc"<<endl;

		 //--------------preview related options:
		} else if (!strcmp(name,"previewThreshhold")) {
			if (value && !strcmp(value,"never")) preview_over_this_size=INT_MAX;
			else IntAttribute(value,&preview_over_this_size);
		
		} else if (!strcmp(name,"permanentPreviews")) {
			preview_transient=!BooleanAttribute(value);
		
		} else if (!strcmp(name,"temporaryPreviews")) {
			preview_transient=BooleanAttribute(value);
		
		} else if (!strcmp(name,"defaultPreviewName")) {
			makestr(preview_file_base,value);
		
		} else if (!strcmp(name,"maxPreviewLength")) {
			IntAttribute(value,&max_preview_length);
		}
	}
	
	fclose(f);
	//DBG cout <<"-------------Done with $HOME/.laidout/(version)/laidoutrc----------"<<endl;
	return 1;
}

//! Set the builtin Laxkit colors. This is called from anXApp::init().
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
	//DBG cout <<"---------start options"<<endl;
	 // parse args -- option={ "long-name", hasArg, int *vartosetifoptionfound, returnChar }
	static struct option long_options[] = {
			{ "rescan-fonts",  0, 0, 'f' },
			{ "new-font-path", 1, 0, 'p' },
			{ "new",           1, 0, 'n' },
			{ "load-dir",      1, 0, 'l' },
			{ "no-x",          0, 0, 'x' },
			{ "template",      1, 0, 't' },
			{ "version",       0, 0, 'v' },
			{ "help",          0, 0, 'h' },
			{ 0,0,0,0 }
		};
	int c,index;
	while (1) {
		c=getopt_long(argc,argv,"t:fp:n:vh",long_options,&index);
		if (c==-1) break;
		switch(c) {
			case ':': cout <<"missing parameter..."<<endl; exit(1); // missing parameter
			case '?': cout <<"Unknown option...."<<endl; exit(1);  // unknown option
			case 'h': print_usage(); // Show usage summary, then exit
			case 'v':  // Show version info, then exit
				cout <<LaidoutVersion()<<endl;
				exit(0);

			case 't': { // load in template
					LoadTemplate(optarg);
				} break;
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
		//DBG cout << "**** read in doc from stdin\n";
		readin=1;
	}


	// load in any docs after the args
	//DBG if (optind<argc) cout << "First non-option argv[optind]="<<argv[optind] << endl;
	//DBG cout <<"*** read in these files:"<<endl;
	Document *doc;
	index=topwindows.n;
	if (!project) project=new Project;
	for (c=optind; c<argc; c++) {
		//DBG cout <<"----Read in:  "<<argv[c]<<endl;
		doc=LoadDocument(argv[c]);
		if (doc && topwindows.n==index) addwindow(newHeadWindow(doc));
	}
	
	//DBG cout <<"---------end options"<<endl;
}

//! Return whether win is in topwindows.
/*! This is used by HeadWindow to verify that a previously marked window
 * is still around. Note that this sort of query is not threadsafe,
 * but window access happens so soon after calling this function, it is 
 * not likely to crash the program.
 */
int LaidoutApp::isTopWindow(anXWindow *win)
{
	return topwindows.findindex(win)>=0;
}

//! Find the doc with saveas.
Document *LaidoutApp::findDocument(const char *saveas)
{
	for (int c=0; c<project->docs.n; c++) {
		if (project->docs.e[c]->saveas && !strcmp(saveas,project->docs.e[c]->saveas)) return project->docs.e[c];
	}
	return NULL;
}

//! Given a Laidout resource name, find the absolute file path for it. Returns a new char[].
/*! \todo *** this should be expanded to be a more full featured search for resources.
 *     this could involve find("blah","templates") or file("palette_name","palette_dir_or_path").
 *     could have an index file in base of the resource directory? that would be a map of
 *     human readable aliases to actual files?
 * \todo right now just searches in the user's home directory. Should also search in
 *   system wide directory
 * \todo allow "~/whatever" as an absolute path
 *
 * If dir!=NULL, then look in ~/.laidout/(version)/dir for the file. 
 *
 * name should be either an absolute file path or a more general name. If it is an
 * absolute path, then dir is ignored. Absolute paths will begin with "file://", "./",
 * "../" or "/".
 * 
 * If name is not an absolute path, then it is something like "blah/file".
 * In that case, blah should be an actual
 * directory name, a subdirectory of dir. If name is not found, then name minus
 * a final suffix is searched for. So if name is "thing", and file "thing" is not
 * found, then if "thing.laidout" is found, then that is used.
 */
char *LaidoutApp::full_path_for_resource(const char *name,char *dir)//dir=NULL
{
	int c=0;
	if (!strncmp(name,"file://",7)) { name+=7; c=1; }
	char *fullname=newstr(name);
	
	if (c || !strncmp(fullname,"/",1) || !strncmp(fullname,"./",2) || !strncmp(fullname,"../",3)) {
		 // is filename
		full_path_for_file(fullname,NULL,0);
		if (readable_file(fullname)) return fullname;
		delete[] fullname;
		return NULL;
	} else {
		 // else is a name
		if (dir) {
			appendstr(fullname,"/",1);
			appendstr(fullname,dir,1);
			appendstr(fullname,"/",1);
			appendstr(fullname,config_dir,1);
		} 
		full_path_for_file(fullname,NULL,0);
		if (readable_file(fullname)) return fullname;
		
		cout <<"imp full_path_for_resource for name not file! ***"<<endl;
	}
	delete[] fullname;//***
	fullname=NULL;//***

	return fullname;
}

////! ***todo Find a resource by the human readable name, rather than by filename
//char *full_path_for_resource_by_name(const char *name,char *dir)//dir=NULL
//{
//}
	
//! Similar to LoadDocument(const char *), but remove the saveas, so as to force a rename.
/*! If a doc with the same filename is already loaded, it is ignored. Templates will only
 * be created from files straight from disk.
 *
 * Note that this does not automatically create a new window, but it does add the
 * new Document to LaidoutApp::project.
 */
Document *LaidoutApp::LoadTemplate(const char *name)
{
	 // find absolute path to a file
	char *fullname=full_path_for_resource(name,"templates");
		
	Document *doc=new Document(NULL,fullname);
	
	 //must push before Load to not screw up setting up windows and other controls
	project->docs.push(doc); // docs is a plain Laxkit::PtrStack of Document
	if (doc->Load(fullname)==0) { // load failed
		project->docs.pop();
		delete doc;
		return NULL;
	}
	
	makestr(doc->saveas,NULL); // this forces a change of name, must be done after Load
	delete[] fullname;
	curdoc=doc;
	return doc;
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
	//DBG cout <<"------create new doc from \""<<spec<<"\""<<endl;
	
	char *saveas=NULL;
	Imposition *imp=NULL;
	PaperStyle *paper=NULL;
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
 *
 * filename is where is should be saved, and is assumed to not exist. If the calling
 * code is not sure if something exists there, then filename should be passed NULL.
 */
int LaidoutApp::NewDocument(DocumentStyle *docinfo, const char *filename)
{
	if (!docinfo) return 1;

	Document *newdoc=new Document(docinfo,filename);
	if (!project) project=new Project();
	project->docs.push(newdoc);
	//DBG cout <<"***** just pushed newdoc using docinfo->"<<docinfo->imposition->Stylename()<<", must make viewwindow *****"<<endl;
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
				
	 //refedine Laxkit's default preview maker
	generate_preview_image=laidout_preview_maker;

	laidout=new LaidoutApp();
	
	laidout->init(argc,argv);

	//DBG cout <<"------------ stylemanager->dump --------------------"<<endl;
	//DBG stylemanager.dump(stdout,3);
	//DBG cout <<"---------- stylemanager->dump end ---------------------"<<endl;

	laidout->run();

	//DBG cout <<"---------Laidout Close--------------"<<endl;
	laidout->close();
	delete laidout;
	
	//DBG cout <<"---------------stylemanager-----------------"<<endl;
	//DBG cout <<"  stylemanager.styledefs.n="<<(stylemanager.styledefs.n)<<endl;
	//DBG cout <<"  stylemanager.styles.n="<<(stylemanager.styles.n)<<endl;
	stylemanager.flush();

	cout <<"-----------------------------Bye!--------------------------"<<endl;
	//DBG cout <<"------------end of code, default destructors follow--------"<<endl;

	return 0;
}



