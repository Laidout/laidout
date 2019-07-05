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
// Copyright (C) 2004-2013 by Tom Lechner
//


/*! \defgroup pools Builtin Pools
 * Here are the various pools of stuff and their generating functions.
 * The pools are available through the main LaidoutApp.
 * 
 * \todo The pools might need to be rethought, so to allow for potentially loadable
 *   modules for file filters, for instance. The interface pool could be rewritten to
 *   be based on Attributes and interface object factories.
 */

//---------------------<< start program! >>-------------------------

#include <lax/anxapp.h>
#include <lax/version.h>
#include <lax/fileutils.h>
#include <lax/laximages.h>
#include <lax/laxoptions.h>
#include <lax/units.h>
#include <lax/bitmaputils.h>
#include <lax/freedesktop.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/utf8string.h>
#include <sys/file.h>

#define LAIDOUT_CC
#include "api/functions.h"
#include "configured.h"
#include "core/stylemanager.h"
#include "core/utils.h"
#include "filetypes/filters.h"
#include "impositions/impositioneditor.h"
#include "impositions/netimposition.h"
#include "impositions/singles.h"
#include "nodes/nodeeditor.h"
#include "printing/epsutils.h"
#include "ui/headwindow.h"
#include "ui/newdoc.h"
#include "ui/viewwindow.h"
#include "version.h"
#include "laidout.h"
#include "language.h"

// *** FOR DEBUGGING:
#include "ui/valuewindow.h"


//template implementations
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


#include <sys/stat.h>
#include <glob.h>
#include <cstdio>

#ifdef LAX_USES_CAIRO
//for debugging below
#include <cairo/cairo-xlib.h>
#endif


using namespace Laxkit;
using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 


//! The mother of all Laidout classes.
namespace Laidout {


ObjectDef stylemanager(NULL,"Laidout",_("Laidout"),_("Global Laidout namespace"),"namespace",NULL,NULL);



//----------------------------------- pre-run misc -----------------------------------

//! Return a const pointer to a string with Laidout version information.
/*! \ingroup lmisc
 */
const char *LaidoutVersion() 
{ 
	static char *version_str=NULL;
	if (version_str==NULL) {
		const char *outstr=
						_("Laidout Version %s\n"
						  "http://www.laidout.org\n"
						  "by Tom Lechner, sometime between 2006 and 2019\n"
						  "Released under the GNU Public License, Version 3.\n"
						  " (using Laxkit Version %s)");
		version_str=new char[1+strlen(outstr)+strlen(LAIDOUT_VERSION)+strlen(LAXKIT_VERSION)];
		sprintf(version_str,outstr,LAIDOUT_VERSION,LAXKIT_VERSION);
	}
	return version_str; 
}

//----------------------------------- misc -----------------------------------
/*! \defgroup lmisc Misc Random Clean Me
 * 
 * Random stuff that needs a home in the source tree that
 * is perhaps a little more meaningful or something...
 */

////! Redefinition of the default Laxkit preview generator.
///*! \ingroup misc
// *
// * This can create previews of eps files..
// *
// * Return 0 for success.
// */
//int laidout_preview_maker(const char *original, const char *preview, const char *format, int width, int height, int fit)
//{
//	// *** need to wrap eps handling into something else...
//	if (!GeneratePreviewFile(original,preview,format,width,height,fit)) return 0;
//
//	 //normal preview maker didn't work, so try something else...
//	DoubleBBox bbox;
//	char *title,*date;
//	int depth,w,h;
//	FILE *f=fopen(original,"r");
//	if (!f) return 1;
//	setlocale(LC_ALL,"C");
//	int c=scaninEPS(f,&bbox,&title,&date,NULL,&depth,&w,&h);
//	setlocale(LC_ALL,"");
//	fclose(f);
//	if (c) return 1; //not eps probably
//	return WriteEpsPreviewAsPng(GHOSTSCRIPT_BIN,
//						 original,w,h,
//						 preview,width,height,
//						 NULL);
//}




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
 * \todo would be nice for debugging to be able to restart laidout in any language,
 *   whatever the current locale actually is (?)
 */
/*! \var Laxkit::RefPtrStack<Laxkit::anInterface> LaidoutApp::interfacepool
 * \ingroup pools
 * \brief Stack of available interfaces for ViewWindow objects.
 */
/*!	\var Laxkit::PtrStack<ImpositionResource> LaidoutApp::impositionpool
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
/*! \var Laxkit::PtrStack<char> LaidoutApp::preview_file_bases
 * \brief The templates used to name preview files for images.
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
 * A '@' will expand to the freedesktop.org thumbnail management spec, namely
 * the md5 digest of "file:///path/to/file", with ".png" added to the end.
 * This can be used to search in "~/.thumbnails/large/@ and "~/.thumbnails/normal/@".
 * 
 * \todo be able to create preview files of different types by default... Should be able
 *   to more fully suggest preview generation perhaps? The thing with '@' is a little
 *   hacky maybe. Should be able to select something equivalent to 
 *   "Search by freedesktop thumb spec" which would automatically search both the large
 *   and normal dirs.. that also affects default max width for previews......
 */ 
/*! \var int LaidoutApp::preview_over_this_size
 * \brief The file size in kilobytes over which preview images should be used where possible.
 */

ColorManager *colorManager = nullptr;

//! Laidout constructor, just inits a few variables to 0.
/*! 
 */
LaidoutApp::LaidoutApp()
  : anXApp(),
	preview_file_bases(2)
{	
	colorManager = new ColorManager();
	colorManager->AddSystem(Create_sRGB_System(true), true);
	ColorManager::SetDefault(colorManager);
	//colorManager->dec_count();


	autosave_timerid = 0;
	force_new_dialog = false;

	icons=IconManager::GetDefault();

	runmode = RUNMODE_Normal;

	config_dir = ConfigDir();

	if (!strncmp(SHARED_DIRECTORY, "..", 2)) { 
		shared_dir = ExecutablePath();
		appendstr(shared_dir, "/../");
		appendstr(shared_dir, SHARED_DIRECTORY);
		simplify_path(shared_dir, 1);
		if (shared_dir[strlen(shared_dir)-1] != '/')
			appendstr(shared_dir, "/");
	} else shared_dir = newstr(SHARED_DIRECTORY);

	makestr(controlfontstr,"sans-15");

	curcolor=0;
	lastview=NULL;
	
	project=new Project;
	curdoc=NULL;
	tooltips=1000;

	experimental=0;

	 // laidoutrc defaults
	preview_over_this_size=250; 
	max_preview_length=200;
	max_preview_width=max_preview_height=-1;
	preview_transient=1; 

	defaultpaper=NULL;

	//ghostscript_binary=newstr(GHOSTSCRIPT_BIN);
	if (file_exists("/usr/bin/gs", 0, nullptr) == S_IFREG) ghostscript_binary = newstr("/usr/bin/gs");
	else ghostscript_binary = nullptr;

	calculator=NULL;
	GetUnitManager()->DefaultUnits(prefs.unitname);
	GetUnitManager()->PixelSize(1./72,UNITS_Inches);

	resources.SetAppName("laidout",LAIDOUT_VERSION);

	pipeout = false;
	pipeoutarg = NULL;
}

//! Destructor, only have to delete project!
LaidoutApp::~LaidoutApp() 
{
	DBG cerr <<"LaidoutApp destructor.."<<endl;

	 //these flush automatically, but are listed here for occasional debugging purposes...
//	papersizes.flush(); 
//	impositionpool.flush();
//	interfacepool.flush();
//	PathInterface::basepathops.flush();

	//we need special treatment of dls:
	while (plugins.n) {
		PluginBase *plugin = plugins.pop();
		plugin->Finalize();
		DeletePlugin(plugin);
	}

	dumpOutResources();

	if (defaultpaper)       defaultpaper->dec_count();
	if (curdoc)             curdoc->dec_count();
	if (project)            delete project;
	if (config_dir)         delete[] config_dir;
	if (ghostscript_binary) delete[] ghostscript_binary;
	if (calculator)		    calculator->dec_count();
	delete[] pipeoutarg;
}

int LaidoutApp::close()
{
	anXApp::close();
	return 0;
}

int LaidoutApp::Idle(int tid, double delta)
{
	if (tid==autosave_timerid) { Autosave(); return 0; }

	return 1;
}

/*! Call this when autosave settings are updated.
 * It will remove old autosave timer and add a new one. Note this starts the timer clock at 0 again.
 */
void LaidoutApp::UpdateAutosave()
{
	if (autosave_timerid) removetimer(this, autosave_timerid);

	if (prefs.autosave && prefs.autosave_time>0) {
		int ms=prefs.autosave_time*60*1000;
		DBG cerr <<"Will autosave every "<<int(prefs.autosave_time)<<":"<<prefs.autosave_time<<" min"<<endl;
		autosave_timerid=addtimer(this, ms,ms, -1);
	}
}

/*! Return 0 for success, or nonzero for unable to save.
 */
int LaidoutApp::Autosave()
{
	// *** TO DO: 
	//types of saves:
	//  save a backup when you do a normal save
	//  autosave actual files
	//  autosave as backups: 
	//    save just one
	//    save like file-autosave1.laidout, up to a maximum number or time frame
	//              file-autosave2.laidout
	//              file-autosave3.laidout
	//  save for crash recovery in some standard location, like with "lock" files or something
	//  saves documents only, not project at the moment

	const char *bname;
	const char *ext;
	char *fmt;
	char *fname;
	int status;
	Document *doc;
	ErrorLog log;

	for (int c=0; c<project->docs.n; c++) {
		doc=project->docs.e[c]->doc;
		if (!doc) continue;

		fmt=newstr(prefs.autosave_path);

		if (strstr(fmt, "%f")) {
			bname=lax_basename(doc->Saveas());
			fname=replaceallname(fmt, "%f", isblank(bname)?"untitled":bname);
			delete[] fmt;  fmt=fname;  fname=NULL;
		}

		if (strstr(fmt, "%e")) {
			ext=lax_extension(doc->Saveas());
			if (!isblank(ext)) {
				fname=replaceallname(fmt, "%e", ext);
				delete[] fmt;  fmt=fname;  fname=NULL;
			}
		}

		if (strstr(fmt, "%b")) {
			char *bbname=newstr(lax_basename(doc->Saveas()));
			ext=lax_extension(bbname);
			if (ext) {
				bbname[ext-bbname-1]='\0';
			}

			fname=replaceallname(fmt, "%b", bbname);
			delete[] fmt;  fmt=fname;  fname=NULL;
			delete[] bbname;
		}

		//if (strstr(fmt, "#")) {
		//}

		if (fname==NULL) { fname=fmt; fmt=NULL; }

		//expand with dir of doc->saveas
		//fname should be either absolute or relative to that saveas
		expand_home_inplace(fname);
		if (is_relative_path(fname)) {
			 //need to expand to a place relative to the original file
			char *path=lax_dirname(doc->Saveas(), 1);
			if (path) {
				if (path[strlen(path)-1]!='/') appendstr(path,"/");
				appendstr(path,fname);
				simplify_path(path);
				delete[] fname;
				fname=path;
			}
		}

		status=doc->SaveACopy(fname, true,true,log, false);

		if (status==0) {
			cerr <<" .... autosaved to: "<<fname<<endl;
			notifyPrefsChanged(NULL, PrefsJustAutosaved);

		} else {
			cerr <<" .... ERROR trying to autosave to: "<<fname<<endl;
		}

		delete[] fmt;   fmt=NULL;
		delete[] fname; fname=NULL;
	}

	//DBG cerr <<" *** need to finish implementing autosave!!"<<endl;

	return 1;
}


ObjectDef *LaidoutApp::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("Laidout");
	if (sd) {
		sd->inc_count();
		return sd;
	}

	sd=new ObjectDef(NULL,
						  "Laidout",
						  _("Laidout"),
						  _("Main Laidout container"),
						  "namespace",
						  NULL,NULL);
	sd->push("documents",
			_("Documents"),
			_("List of available documents."),
			"set", "Document", NULL,
			0,NULL);
	sd->push("limbos",
			_("Limbos"),
			_("List of available limbo areas."),
			"set", "Group", NULL,
			0,NULL);
	sd->push("resources",
			_("Resources"),
			_("List of available resources."),
			"set", "Resource", NULL,
			0,NULL);
	sd->push("windows",
			_("Windows"),
			_("List of current top level windows."),
			"set", "HeadWindow", NULL,
			0,NULL);
	sd->push("settings",
			_("Settings"),
			_("List of various settings. These get loaded and saved in a laidoutrc file."),
			"LaidoutPreferences", NULL, NULL,
			0,NULL);

	return sd;
}

Value *LaidoutApp::duplicate()
{
	inc_count();
	return this;
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
 *   argv[0] or the /proc/pid/exe link.  
 */
int LaidoutApp::init(int argc,char **argv)
{
	anXApp::init(argc,argv); //setupdefaultcolors() is called here
	
	 //------------ make adjustments to some standard dirs 
	 //             when running before installing
 
	 // find the current executable path
	Utf8String execdir(ExecutablePath(), -1, true);
	execdir = lax_dirname(execdir.c_str(),0);

	 // Check executabledir/icons first
	Utf8String iconpath = execdir;
	iconpath.Append("/icons");
	if (file_exists(iconpath.c_str(), 1, nullptr) == S_IFDIR) {
		icons->AddPath(iconpath.c_str());
		DBG cerr <<"Added uninstalled icon dir "<<iconpath<<" to icon path"<<endl;
	}

	if (!strncmp(ICON_DIRECTORY, "..", 2)) {
		//configured dir is relative to executable
		iconpath = execdir;
		iconpath.Append("/");
		iconpath.Append(ICON_DIRECTORY);
		char *path = simplify_path(iconpath.c_str());
		if (file_exists(path, 1, nullptr) == S_IFDIR) {
			icons->AddPath(path);
			DBG cerr <<"Added relocatabale icon dir "<<path<<" to icon path"<<endl;
		}
		delete[] path;

	} else {
		if (file_exists(ICON_DIRECTORY, 1, nullptr) == S_IFDIR) {
			icons->AddPath(ICON_DIRECTORY);
			DBG cerr <<"Added installed icon dir "<<ICON_DIRECTORY<<" to icon path"<<endl;
		}
	}


	 //-----read in laidoutrc
	if (!readinLaidoutDefaults()) {
		createlaidoutrc();
	}


	 //------setup initial pools
	 
	DBG cerr <<"---file filters init"<<endl;
	installFilters();
	
	DBG cerr <<"---imposition pool init"<<endl;
	GetBuiltinImpositionPool(&impositionpool);
	DBG cerr <<"---imposition pool init done"<<endl;
	
	DBG cerr <<"---papersizes pool init"<<endl;
	GetBuiltinPaperSizes(&papersizes);
	
	DBG cerr <<"---interfaces pool init"<<endl;
	PushBuiltinPathops(); // this must be called before getinterfaces because of pathops...
	GetBuiltinInterfaces(&interfacepool);


	 //-----establish user accesible api
	DBG cerr<<"---install functions"<<endl;
	InitFunctions();
	InitObjectDefinitions();

	 //-----initialize the main calculator
	DBG cerr<<"---init main calculator"<<endl;
	InitInterpreters();
	
	 //------load plugins
	InitializePlugins();

	 //-----read in resources
	char configfile[strlen(config_dir)+20];
	sprintf(configfile,"%s/autolaidoutrc",config_dir);
	FILE *f;
	if (readable_file(configfile,&f)) { //***i suppose this should have a flock capability?
		app_resources.dump_in(f,0,NULL);
		fclose(f);
	}

	 //-----add Templates dir as default bookmark
	Attribute *att=app->AppResource("Bookmarks");
    if (att) {
		if (att->find(_("Templates"))==NULL) {
			char *tdir=default_path_for_resource("templates");
			att->push(_("Templates"), tdir);
			delete[] tdir;
		}
	}


	 //-----if no bases defined add freedesktop style
	if (!preview_file_bases.n) {
		preview_file_bases.push(newstr("~/.thumbnails/large/@"));
		preview_file_bases.push(newstr("~/.thumbnails/normal/@"));
		preview_file_bases.push(newstr(".laidout-%.jpg"));
		preview_file_bases.push(newstr(".laidout-%.jpg"));
	}
	
	 //-----define default icon
	for (int c=0; c<icons->NumPaths(); c++) {
		Utf8String path = newstr(icons->GetPath(c));
		path.Append("/laidout-48x48.png");
		if (file_exists(path.c_str(), 1, nullptr) == S_IFREG) {
			DefaultIcon(path.c_str());
			break;
		}
	}


	 // Note parseargs has to come after initing all the pools and whatever else
	DBG cerr <<"---init: parse args"<<endl;
	parseargs(argc,argv);
	
	 // Define default project if necessary, and Pop something up if there hasn't been anything yet
	if (!project) project=new Project();

	if (runmode == RUNMODE_Normal) {
		 //try to load the default template if no windows are up
		if (topwindows.n == 0 && !force_new_dialog && prefs.start_with_last) {
			MenuInfo recent;
	        recently_used(NULL, NULL, "Laidout", 0, &recent);
			if (recent.n()) {
				const char *file_to_load = recent.e(0)->name;
				if (!isblank(file_to_load)) {
					Document *doc = NULL;
					int index = topwindows.n;

					if (Load(file_to_load, generallog) == 0) doc = curdoc;
					else {
						generallog.AddError(0,0,0, _("Could not load %s!"), file_to_load);
					}

					if (doc != nullptr && topwindows.n == index) {
						if (!doc && project->docs.n) doc = project->docs.e[0]->doc;
						if (doc) addwindow(newHeadWindow(doc));
					}
				}
			}
		}

		if (topwindows.n==0 && !force_new_dialog && prefs.default_template) {
			ErrorLog log;
			LoadTemplate(prefs.default_template,log);
		}
		
		 // if no other windows have been launched yet, then launch newdoc window
		if (topwindows.n==0)
			//addwindow(new NewDocWindow(NULL,"New Document",ANXWIN_LOCAL_ACTIVE,0,0,0,0, 0));
			addwindow(BrandNew());

		if (prefs.autosave>0) {
			UpdateAutosave();
		}

		 //let the user know of any snafus during startup
		NotifyGeneralErrors(NULL);

	} else if (runmode == RUNMODE_Impose_Only) {
		//***

	} else if (runmode == RUNMODE_Nodes_Only) {
		//***

	} else if (runmode == RUNMODE_Shell) {
		calculator->RunShell();
		return 0;
	}

	DBG cerr <<"---done with init"<<endl;
	
	return 0;
};

//! Initialize and install built in interpreters. Returns number added.
int LaidoutApp::InitInterpreters()
{
	if (!calculator) calculator = new LaidoutCalculator();
	interpreters.push(calculator, LISTS_DELETE_Refcount, 0); //list should be blank, but just in case push to 0

    return 1;
}

int LaidoutApp::AddInterpreter(Interpreter *i, bool absorb_count)
{
  if (!i) return 1;
  interpreters.push(i);
  if (absorb_count) i->dec_count();
  return 0;
}

int LaidoutApp::RemoveInterpreter(Interpreter *i)
{
  if (!i) return 1;
  interpreters.remove(interpreters.findindex(i));
  return 0;
}

Interpreter *LaidoutApp::FindInterpreter(const char *name)
{
	for (int c=0; c<interpreters.n; c++) {
		if (!strcmp(name, interpreters.e[c]->Id())) return interpreters.e[c];
	}
	return NULL;
}

//! Write out resources to ~/.laidout/(version)/autolaidoutrc.
void LaidoutApp::dumpOutResources()
{
	if (!app_resources.attributes.n) return;

	char configfile[strlen(config_dir)+20];
	sprintf(configfile,"%s/autolaidoutrc",config_dir);
	
	setlocale(LC_ALL,"C");
	int fd=open(configfile,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
	if (fd<0) { setlocale(LC_ALL,""); return; }
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"w");
	if (!f) { setlocale(LC_ALL,""); ::close(fd); return; }

	fprintf(f,"## THIS FILE IS AUTOMATICALLY GENERATED BY LAIDOUT\n");
	fprintf(f,"## It is read when Laidout starts, and rewritten when Laidout exits.\n\n");
	app_resources.dump_out(f,0);

	flock(fd,LOCK_UN);
	fclose(f);// this closes fd too
	setlocale(LC_ALL,"");
}

//! Return 1 is saving as a single project, or 0 if saving as individual documents.
int LaidoutApp::IsProject()
{
	return !isblank(project->filename);
}

//! Called from init(), creates a user's laidoutrc if readinLaidoutDefaults failed.
/*! Return 0 for created ok, else non-zero error.
 *
 * See also dump_out_file_format().
 *
 * \todo should separate the laidoutrc writing functions to allow easy dumping to any stream like stdout.
 * \todo maybe be able to preserve user comments in a laidoutrc?
 */
int LaidoutApp::createlaidoutrc()
{
	DBG cerr <<"-------------Creating $HOME/.config/laidout/(version)/laidoutrc----------"<<endl;

	 // ensure that ~/.config/ladiout/(version) exists
	 //   if not, create, and put in a README explaining what's what:
	 //   	laidoutrc
	 //   	icons/
	 //   	palettes/
	 //   	templates/
	 //   	templates/default
	 //   	impositions/
	 //   	plugins/
	 // if no ~/.laidout/(version)/laidoutrc exists, possibly import
	 //   from other installed laidout versions
	 //   otherwise perhaps touch laidoutrc, and put in a much
	 //   commented empty version
	
	int t=check_dirs(config_dir,1);
	if (t==-1) { // dirs were ok
		 // create "~/.config/laidout/(version)/laidoutrc"
		char path[strlen(config_dir)+20];
		sprintf(path,"%s/laidoutrc",config_dir);
		setlocale(LC_ALL,"C");
		FILE *f=fopen(path,"w");
		if (f) {
			fprintf(f,"#Laidout %s laidoutrc\n",LAIDOUT_VERSION);
			fprintf(f,"\n"
					  "# Laidout global configuration options go in here.\n"
					  "# If you modify settings from within Laidout, this file will be overwritten,\n"
					  "# and you'll lose any comments or formatting you have inserted.\n"
					  "\n"
					  "\n"

					   //enable experimental
					  "## Uncomment to enable any features that are experimental.\n"
					  "#experimental\n"
					  "\n"

					   //shortcuts
					  " #By default when you modify shortcuts in Laidout, they are saved in ./shortcuts.\n"
					  " #Listing another file here will load keys from that file first, THEN the ./shortcuts will\n"
					  " #load on top of that. For the file format, you may do a dump of current keys by\n"
					  " #running \"laidout -S\".\n"
					  "#shortcuts shortcutsfile\n"
					  "\n"

					   //splash image
					  " #If you want to use a custom splash image:\n"
					  "#splashimage /path/to/it\n"
					  "\n"

					   //drop shadow
					  "# Customize how some things get displayed or entered:\n"
					  "#pagedropshadow 5    #how much to offset drop shadows around papers and pages \n"
					  "\n"

					   //autosave
					  " #Autosave settings\n"
					  "autosave false #Whether to autosave. true or false.\n"
					  "autosave_time 0  #number of minutes (such as 1.5) between autosaves. 0 means no autosave\n"
					  "autosave_path ./%%f.autosave  #default location for autosave, relative to actual file\n"
					  "                             #%%f is filename, %%f is full path, %%b is name without extension\n"
					  "                             #%%e is extension, # or ### is autosave number (or padded number)\n"
					  "autosave_num 0  #number of autosave files to maintain. 0 means no limit. Ignored if no '#' in autosave_path.\n"
					  "\n"

					   //default exported file name
					  " #Default export file name base. An extension is added automatically by the exporter.\n"
					  " #Use \"%%f\" to substitute document file name. Everything here after a '.' till the \n"
					  " #end of the string is replaced when the file extension is changed.\n"
					  "#export_file_name %%f-exported\n"
					  "\n"

					   //units
					  " #The default units presented to the user. Within files on disk, it is currently always inches.\n"
					  "#defaultunits inches\n"
					  "\n"

					   //default paper
					  "#defaultpapersize letter #Name of default paper to use\n"
					  "\n"

					   //default template
					  " #if the following is commented out, then running \"laidout\" will\n"
					  " #always bring up the new document dialog. If it is uncommented, then the\n"
					  " #specified file is loaded with the filename removed so that trying to save will\n"
					  " #force entering a new name and location. If the file is not an absolute path,\n"
					  " #then it is assumed to be relative to ~/.laidout/(version)/templates.\n"
					  "#default_template default\n"
					  "\n"

					   //resource directories
					  " # Some assorted directories:\n");
			fprintf(f,"#icon_dir %s/icons\n", shared_dir);
			fprintf(f,"#plugin_dir %s/plugins #configdir/plugins, and this dir are always consulted after any dir in this config\n", shared_dir);
			fprintf(f,"#palette_dir /usr/share/gimp/2.0/palettes\n"
					  "\n");


					   ////colors
					  //"#colors (***ToDO)\n"
					  //"#  activate   rgbf  0 .78  0    #Some controls have green/red to indicate go/no-go. Redefine here\n"
					  //"#  deactivate rgbf  1 .39 .39   # for instance, to compensate for red/green color blindness.\n"
					  //"\n"

			fprintf(f,"laxprofile Light #Default built in profiles are Dark, Light, and Gray. You can define others in the laxconfig section.\n");
			fprintf(f,"laxconfig-sample #Remove the \"-sample\" part to redefine various default window behaviour settingss\n");
			dump_out_rc(f,NULL,2,-1);
			fprintf(f,"\n"
					  "#laxcolors  #To set only the colors of laxconfig, use this\n"
					  "#  panel\n"
					  "#    ...\n"
					  "#  menu\n"
					  "#    ...\n"
					  "#  edits\n"
					  "#    ...\n"
					  "#  buttons\n"
					  "#    ...\n");

			fprintf(f,"\n"
					  "\n"
					  "\n"

//					   //preview generation
//					  //" # Alternately, you can specify the maximum width and height separately:\n"
//					  //"#maxPreviewWidth 200\n"
//					  //"#maxPreviewHeight 200\n"
//					  "#*** note, the following preview stuff is maybe not so accurate, code is in flux:\n"
//					  " #The size a file (unless specified, default is kilobytes) must be to trigger\n"
//					  " #the automatic creation of a smaller preview image file.\n"
//					  "#previewThreshhold 200kb\n"
//					  "#previewThreshhold never\n"
//					  "\n"
//					  " #You can have previews that are created during the program\n"
//					  " #be deleted when the program exits, or have newly created previews remain:\n"
//					  "#temporaryPreviews yes  #yes to delete new previews on exit\n"
//					  "#temporaryPreviews      #same as: temporaryPreviews yes\n"
//					  "#permanentPreviews yes  #yes to not delete new preview images on exit\n"
//					  "\n"
//					  " #When preview files are not specified, Laidout tries to find one\n"
//					  " #based on a sort of name template. Say an image is filename.tiff, then\n"
//					  " #the first line implies using a preview file called filename-s.jpg,\n"
//					  " #where the '%%' stands for the original filename minus its the final suffix.\n"
//					  " #The second looks for .laidout-previews/filename.tiff.jpg, relative to the\n"
//					  " #directory of the original file. The '*' stands for the entire original filename.\n"
//					  " #The final example is an absolute path, and will try to create all preview files\n"
//					  " #in that place, in this case, it would try ~/laidout/tmp/filename.tiff.jpg.\n"
//					  " #A '@' will expand to the freedesktop.org thumbnail management defined md5\n"
//					  " #representation of the original file. In other words, ~/.thumbnails/large/@\n"
//					  " #is a valid preview name. When you have many previewName, then all are selectable\n"
//					  " #in various dialogs, and the first one is the default.\n"
//					  "#previewName %%-s.jpg\n"
//					  "#previewName .laidout-previews/*.jpg\n"
//					  "#previewName ~/.laidout/tmp/*.jpg\n"
//					  "#previewName ~/.thumbnails/large/@    #these two cover all the current freedesktop\n"  
//					  "#previewName ~/.thumbnails/normal/@   #thumbnail locations\n"
//					  "\n"
//					  "\n# The maximum width or height for preview images\n"
//					  "#maxPreviewLength 200\n"
//					  "\n"
					  "\n");
			fclose(f);
			setlocale(LC_ALL,"");
		}

		 // create the other relevant directories
		sprintf(path,"%s/addons",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/templates",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/impositions",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/palettes",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/fonts",config_dir);
		mkdir(path,0755);
		sprintf(path,"%s/plugins",config_dir);
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

	DBG cerr <<"-------------Checking $HOME/.laidout/(version)/laidoutrc----------"<<endl;
	int foundkeys=0;

	FILE *f=NULL;
	char configfile[strlen(config_dir)+20];
	sprintf(configfile,"%s/laidoutrc",config_dir);
	if (!readable_file(configfile,&f)) return 0;

	 //now f is open, must read in...
	setlocale(LC_ALL,"C");
	Attribute att;
	att.dump_in(f,0,NULL);
	char *name,*value;

	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;
		if (!name) continue;

		//DBG cerr <<(name?name:"(no name)")<<": "<<(value?value:"(no value)")<<endl;

		if (!strcmp(name,"laxconfig")) {
			dump_in_rc(att.attributes.e[c],NULL);

		} else if (!strcmp(name,"laxprofile")) {
			if (value) {
				makestr(app_profile,value);
				if (!strcmp(value,"Dark") || !strcmp(value,"Gray") || !strcmp(value,"Light"))
					setupdefaultcolors();
			}
			
		} else if (!strcmp(name,"theme")) {
			if (isblank(app_profile) || (!isblank(app_profile) && value && !strcmp(app_profile, value))) {
				Theme *thme = new Theme(app_profile);
				thme->dump_in_atts(att.attributes.e[c], 0, NULL);
				if (theme) theme->dec_count();
				theme = thme;
			}

		} else if (!strcmp(name,"shortcuts")) {
			foundkeys=1;
			InitializeShortcuts();
			ShortcutManager *m=GetDefaultShortcutManager();
			m->Load(value);

		} else if (!strcmp(name,"experimental")) {
			experimental = 1;

		} else if (!strcmp(name,"start_with_last")) {
			prefs.start_with_last = 1;

		} else if (!strcmp(name,"default_template")) {
			if (file_exists(value,1,NULL)==S_IFREG) makestr(prefs.default_template,value);
			else {
				char *fullname=full_path_for_resource(value,"templates");
				//if (file_exists(fullname,1,NULL)==S_IFREG) makestr(prefs.default_template,value);
				if (file_exists(fullname,1,NULL)==S_IFREG) makestr(prefs.default_template,fullname);
				delete[] fullname;
			}
			//default to config_dir/templates/default?

		} else if (!strcmp(name,"splashimage")) {
			makestr(prefs.splash_image_file,value);

		} else if (!strcmp(name,"defaultpapersize")) {
			 //custom(10cm, 20cm)
			 //letter,portrait
			makestr(prefs.defaultpaper,value); //*** bit hacky, should have custom width/height, whatever, etc
		
		} else if (!strcmp(name,"template")) {
			cout <<"***imp me! readinlaidoutrc: template"<<endl;
			//*** for multiple startup templates not found in config_dir/templates,
			// template nickname /file/path
			// > laidout --template consumptionIssue
			// > laidout --template 1paperPamphlet
		
		} else if (!strcmp(name,"ghostscript_binary")) {
			if (file_exists(value,1,NULL)==S_IFREG) makestr(ghostscript_binary,value);

		} else if (!strcmp(name,"icon_dir")) {
			if (file_exists(value,1,NULL) == S_IFDIR) {
				icons->AddPath(value);
			}
		
		} else if (!strcmp(name,"plugin_dir")) {
			if (file_exists(value,1,NULL) == S_IFDIR) {
				prefs.AddPath("plugins", value);
			}

		} else if (!strcmp(name,"palette_dir")) {
			if (file_exists(value,1,NULL) == S_IFDIR) makestr(prefs.palette_dir,value);
		
		} else if (!strcmp(name,"temp_dir")) {
			//**** default "config_dir/temp/pid/"?
			//				or projectdir/.laidouttemp/previews
			//	make sure supplied tempdir is writable before using.
			cout <<" *** imp temp_dir in laidoutrc"<<endl;

		} else if (!strcmp(name,"pagedropshadow")) {
			IntAttribute(value,&prefs.pagedropshadow);
		
		} else if (!strcmp(name,"defaultunits")) {
			if (value) {
				int id=0;
				UnitManager *units=GetUnitManager();
				if (units->UnitInfo(value,&id,NULL,NULL,NULL,NULL,NULL)==0) {
					makestr(prefs.unitname,value);
					prefs.default_units=id;
					units->DefaultUnits(value);
					notifyPrefsChanged(NULL,PrefsDefaultUnits);
				}
			}

		} else if (!strcmp(name,"export_file_name")) {
			if (!isblank(value)) makestr(prefs.exportfilename, value);

		} else if (!strcmp(name,"autosave_path")) {
			if (!isblank(value)) makestr(prefs.autosave_path, value);

		} else if (!strcmp(name,"autosave_time")) {
			DoubleAttribute(value, &prefs.autosave_time, NULL);

		//} else if (!strcmp(name,"autosave_num")) {
		//	IntAttribute(value, &prefs.autosave_num, NULL);

		} else if (!strcmp(name,"autosave")) {
			prefs.autosave = BooleanAttribute(value);


		 //--------------preview related options:
		} else if (!strcmp(name,"previewThreshhold")) {
			if (value && !strcmp(value,"never")) preview_over_this_size=INT_MAX;
			else IntAttribute(value,&preview_over_this_size);
		
		} else if (!strcmp(name,"permanentPreviews")) {
			preview_transient = !BooleanAttribute(value);
		
		} else if (!strcmp(name,"temporaryPreviews")) {
			preview_transient = BooleanAttribute(value);
		
		} else if (!strcmp(name,"previewName")) {
			preview_file_bases.push(newstr(value));
			DBG cerr<<"preview_file_bases local for top="<<(int)preview_file_bases.islocal[preview_file_bases.n-1]<<endl;
		
		} else if (!strcmp(name,"maxPreviewLength")) {
			IntAttribute(value,&max_preview_length);

		}
	}
	
	fclose(f);
	setlocale(LC_ALL,"");
	DBG cerr <<"-------------Done with $HOME/.laidout/(version)/laidoutrc----------"<<endl;

	if (foundkeys==0) {
		char *keys=newstr(laidout->config_dir);
		appendstr(keys,"/default.keys");
		if (readable_file(keys)) {
			DBG cerr <<" ----- reading in default shortcuts file ------"<<endl;

			InitializeShortcuts();
			ShortcutManager *m=GetDefaultShortcutManager();
			m->Load(keys);
		}
		delete[] keys;
	}

	return 1;
}

int LaidoutApp::AddPlugin(const char *path)
{
	// *** todo!
	return 1;
}

int LaidoutApp::RemovePlugin(const char *name)
{
	// *** todo!
	return 1;
}

/*! Returns the number of plugins added.
 */
int LaidoutApp::InitializePlugins()
{
	int n=0;

	DBG cerr <<"Initializing plugins..."<<endl;

	 //try to load all in prefs.plugin_dirs/*.so

	 //always add default plugin dir to end of list
	char scratch[PATH_MAX];

	sprintf(scratch,"%splugins",config_dir);
	prefs.AddPath("plugins", scratch);

	sprintf(scratch,"%splugins", shared_dir);
	prefs.AddPath("plugins", scratch);

	glob_t globbuf;
	char *path = NULL;
	const char *file;
	struct stat statbuf;
	int status;

	for (int c=0; c<prefs.plugin_dirs.n; c++) {

		makestr(path, prefs.plugin_dirs.e[c]);
		appendstr(path, "/*.so");

		DBG cerr <<"scanning for plugins in "<<path<<"..."<<endl;

		glob(path, GLOB_ERR, NULL, &globbuf);

		for (int c2=0; c2<(int)globbuf.gl_pathc; c2++) {
			status = stat(globbuf.gl_pathv[c2], &statbuf);
			if (status != 0) continue;
			if (!S_ISREG(statbuf.st_mode)) continue;

			file = globbuf.gl_pathv[c2];

			PluginBase *plugin = LoadPlugin(file, generallog); //loads but doesn't initialize
			if (plugin) {
				//need to check for plugin with same name already loaded! fail if so... do not allow same name plugins!
				for (int c3=0; c3<plugins.n; c3++) {
					if (!strcmp(plugins.e[c3]->PluginName(), plugin->PluginName())) {
						char scratch[strlen(_("Plugin called \"%s\" exists already, skipping!")) + strlen(plugin->PluginName()) + 5];
						sprintf(scratch, _("Plugin called \"%s\" exists already, skipping!"), plugin->PluginName());

						generallog.AddMessage(0, NULL, file, scratch, Laxkit::ERROR_Warning);
						DeletePlugin(plugin);
						plugin = NULL;
						break;
					}
				}

				if (plugin) {
					plugins.push(plugin);
					plugin->dec_count();
					plugin->Initialize();
					n++;
				}
			}
			if (ColorManager::GetDefault(false) == nullptr) {
				cerr << " *** ColorManager wiped after dlopen!! This shouldn't happen!!!!! Aaaaaaagh!!"<<endl;
				ColorManager::SetDefault(colorManager);
			}
		}
		globfree(&globbuf);
	}
	delete[] path;

	return n;
}

/*! Pop up a box showing any errors in log (or generallog if log==NULL). Flushes log afterwards.
 * This is done, for instance, at startup, to notify of any plugin load errors.
 * If there aren't any, then do nothing.
 */
void LaidoutApp::NotifyGeneralErrors(ErrorLog *log)
{
	if (log==NULL) log = &generallog;
	Laidout::NotifyGeneralErrors(log);
}


/*! Calling code should increment count on returned object if they want to use it in place.
 * Ordinarily, most code is duplicating the returned object.
 */
PaperStyle *LaidoutApp::GetDefaultPaper()
{
	if (defaultpaper) return defaultpaper;

	if (!strchr(prefs.defaultpaper,',')) { 
       for (int c=0; c<laidout->papersizes.n; c++) {
            if (strcasecmp(prefs.defaultpaper, laidout->papersizes.e[c]->name)==0) {
				defaultpaper=laidout->papersizes.e[c];
				defaultpaper->inc_count();
				return defaultpaper;
            }
        }
	}

	defaultpaper=new PaperStyle();
	defaultpaper->SetFromString(prefs.defaultpaper);

	return defaultpaper;
}

//! Set the builtin Laxkit colors. This is called from anXApp::init().
/*! ***should switch to color(COLOR_BG), etc.. or color("bg")?? or some other
 * resource system to keep track of first click, directories, bookmarks, etc.
 */
void LaidoutApp::setupdefaultcolors()
{
	//char *oldpf=app_profile;
	//app_profile=NULL; //force it to light colors
	anXApp::setupdefaultcolors();
	//app_profile=oldpf;
}

LaxOptions options;

//! Initialize a LaxOptions object to contain Laidout's command line option summary.
/*! \ingroup lmisc
 */
void InitOptions()
{
	 //command line options are somewhat low level, so NOT localized (for now)
	options.HelpHeader(LaidoutVersion());
	options.UsageLine("laidout [options] [file1] [file2] ...");
	options.Add("export-formats",     'X', 0, "List all the available export formats",       0, NULL);
	options.Add("list-export-options",'O', 1, "List all the options for the given format",   0, "format");
	options.Add("export"             ,'e', 1, "Export a document based on the given options",0, "\"format=EPS start=3\"");
	options.Add("template",           't', 1, "Start laidout from this template in ~/.laidout/(version)/templates",0,"templatename");
	options.Add("no-template",        'N', 0, "Do not use a default template or load from last", 0, NULL);
	options.Add("new",                'n', 1, "Create new document",                         0, "\"letter,portrait,3pgs\"");
	options.Add("file-format",        'F', 0, "Print out a pseudocode mockup of the file format, then exit",0,NULL);
	options.Add("command",            'c', 1, "Run one or more commands without the gui",    0, "\"newdoc net\"");
	options.Add("script",             's', 1, "Like --command, but the commands are in the given file",     0, "/some/file");
	options.Add("shell",              'L', 0, "Enter a command line shell. Can be used with --command and --script.", 0, NULL);
	options.Add("default-units",      'u', 1, "Use the specified units.",                    0, "(in|cm|mm|m|ft|yards)");
	options.Add("load-dir",           'l', 1, "Start in this directory.",                    0, "path");
	options.Add("experimental",       'E', 0, "Enable any compiled in experimental features",0, NULL);
	//options.Add("backend",            'B', 1, "Either cairo or xlib (xlib very deprecated).",0, NULL);
	options.Add("impose-only",        'I', 1, "Run only as a file imposer, not full Laidout",0, NULL);
	options.Add("nodes-only",         'o', 1, "Run only as a node editor on argument",       0, NULL);
	options.Add("pipein",             'p', 1, "Start with a document piped in on stdin",     0, "default");
	options.Add("pipeout",            'P', 1, "On exit, export document[0] to stdout",       0, "default");
	options.Add("list-shortcuts",     'S', 0, "Print out a list of current keyboard bindings, then exit",0,NULL);
	options.Add("theme",              'T', 1, "Set theme. Currently, one of Light, Dark, or Gray",0,NULL);
	options.Add("helphtml",           'H', 0, "Output an html fragment of key shortcuts.",   0, NULL);
	options.Add("helpman",             0 , 0, "Output a man page fragment of options.",      0, NULL);
	options.Add("version",            'v', 0, "Print out version info, then exit.",          0, NULL);
	options.Add("help",               'h', 0, "Show this summary and exit.",                 0, NULL);

}

//! Parse command line options, and load up initial documents or projects.
/*! 
 */
void LaidoutApp::parseargs(int argc,char **argv)
{
	DBG cerr <<"---------start options"<<endl;
	//InitOptions(); <- this is done in main()
	//c=options.Parse(argc,argv, &index); <- now down in main also
	
	char *exprt = NULL;
	bool pipein = false;
	const char *pipeinarg = NULL;


	LaxOption *o;
	for (o=options.start(); o; o=options.next()) {
		switch(o->chr()) {
			case 'h': // Show usage summary, then exit
				options.Help(stdout);
				exit(0);
			case 'v':  // Show version info, then exit
				cout <<LaidoutVersion()<<endl;
				exit(0);

			case 'E': { // experimental
				experimental=1;
			  } break; 

			case 'T': { // theme
				makestr(app_profile, o->arg());
			  } break; 

			case 't': { // load in template
					ErrorLog log;
					LoadTemplate(o->arg(),log);
				} break;

			case 'L': { // --shell
					donotusex=2;
					runmode=RUNMODE_Shell;
				} break;

			case 's': { // --script
					donotusex=1;
					int size=file_size(o->arg(),1,NULL);
					if (size<=0) {
						cerr <<_("No input script!")<<endl;
						exit(1);
					}
					char *instr=new char[size+1];
					FILE *f=fopen(o->arg(),"r");
					if (!f) {
						cerr <<_("Could not open ")<<o->arg()<<endl;
						exit(1);
					}
					fread(instr,1,size,f);
					instr[size]='\0';
					runmode = RUNMODE_Commands;
					char *str = calculator->In(instr, NULL);
					if (str) cout <<str<<endl;
					if (runmode != RUNMODE_Shell) runmode = RUNMODE_Quit;
				} break;

			case 'c': { // --command
					donotusex=1;
					runmode = RUNMODE_Commands;
					char *str = calculator->In(o->arg(), NULL);
					if (str) cout <<str<<endl;
					if (runmode != RUNMODE_Shell) runmode = RUNMODE_Quit;
				} break;

			case 'n': { // --new "letter singles 3 pages blah blah blah"
					if (NewDocument(o->arg())==0) {
						if (curdoc) curdoc->dec_count();
						curdoc=project->docs.e[project->docs.n-1]->doc;
						if (curdoc) curdoc->inc_count();
					}
				} break;

			case 'l': { // load dir
					makestr(load_dir,o->arg());
				} break;

			case 'N': { // do not use a default template
					//if (prefs.default_template) { delete[] prefs.default_template; prefs.default_template=NULL; }
					force_new_dialog = true;
				} break;

			case 'F': { // dump out file format
					if (dump_out_file_format("-",0)) exit(1);
					exit(0);
				} break;

			case 'S': { // dump out shortcuts
					dump_out_shortcuts(stdout,0,0);
					exit(0);
				} break;

			case 'H': { // dump out shortcuts
					dump_out_shortcuts(stdout,0,1);
					exit(0);
				} break;

			case 'e': { // export
					exprt=newstr(o->arg());
				} break;

			case 'O': { // list export options for a given format
					//DBG cout <<"   ***** --list-export-options IS A HACK!! Code me right! ***"<<endl;
					//printf("format   = \"theformat\"    #the format to export as\n");
					//printf("tofile   = /file/to/export/to\n");
					//printf("tofiles  = \"/files/like###.this\"  #the \"###\" section is replaced with the spread index\n");
					//printf("                                  #Either tofile or tofiles should be present, not both\n");
					//printf("layout   = papers  #usually singles, papers, or pages, but full range of possible values\n");
					//printf("                   #    depends on the particular imposition used by the document\n");
					//printf("start    = 3       #the starting index to export, counting from 0\n");
					//printf("end      = 5       #the ending index to export, counting from 0\n");
					//---------
					if (!o->arg()) {
						//cerr << "Must specify one of:"<<endl;
						//for (int c=0; c<exportfilters.n; c++) cerr << exportfilters.e[c]->VersionName() <<endl;
						cout << "Must specify one of:"<<endl;
						for (int c=0; c<exportfilters.n; c++) cout << exportfilters.e[c]->VersionName() <<endl;
						exit(1);
					}

					ExportFilter *filter=FindExportFilter(o->arg(),false);
					if (!filter) {
						//cerr << "Filter "<<o->arg()<<" not found!"<<endl;
						cout << "Filter "<<o->arg()<<" not found!"<<endl;
						exit(1);
					}

					DocumentExportConfig *config=filter->CreateConfig(NULL);
					config->filter=filter;
					cout << filter->VersionName() << " options:" << endl;
					config->dump_out(stdout, 2, -1, NULL);
					exit(0);
				} break;

			case 'X': { // list export formats
					for (int c=0; c<exportfilters.n; c++) 
						cout << exportfilters.e[c]->VersionName() <<endl;
					exit(0);
				} break;

			case 'I': { // impose-only
					runmode = RUNMODE_Impose_Only;
					anXWindow *editor = newImpositionEditor(NULL,"impedit",_("Impose..."),0,NULL,
														  NULL,"SignatureImposition",NULL,o->arg(),
														  NULL);
					addwindow(editor);
				} break;

			case 'o': { // nodes-only
					runmode = RUNMODE_Nodes_Only;
					anXWindow *editor = newNodeEditor(NULL,"nodeedit",_("Nodes"), 0,NULL, NULL,0, o->arg());
					addwindow(editor);
				} break;

			case 'p': { //pipein
					pipein = true;
					pipeinarg = o->arg();
			    } break;

			case 'P': { //pipeout
					pipeout = true;
					makestr(pipeoutarg, o->arg());
			    } break;

			case 'u': { // default units
					UnitManager *units=GetUnitManager();
					int id=0;
					if (units->UnitInfo(o->arg(),&id,NULL,NULL,NULL,NULL,NULL)==0) {
						makestr(prefs.unitname,o->arg());
						prefs.default_units=id;
						units->DefaultUnits(prefs.unitname);
						notifyPrefsChanged(NULL,PrefsDefaultUnits);
						DBG cerr <<"laidout:units->DefaultUnits:"<<prefs.default_units<<"  from um:"<<units->DefaultUnits(prefs.unitname)<<endl;
					}

				} break;
		} //switch
	}

	if (runmode==RUNMODE_Quit) exit(0);

//	int readin=0;
//	if (optind<argc && argv[optind][0]=='-' && argv[optind][0]=='\0') { 
//		cout << "**** must implement read in doc from stdin\n";
//		readin=1;
//	}


	 //export doc if found, then exit
	if (exprt) {
		//*** this should probably be moved to its own function so command line pane can call it
		//*** is this obsoleted by --command?

		DBG cout << "export: "<<exprt<<endl;

		 //parse the config string into a config
		Attribute att;
		NameValueToAttribute(&att,exprt,'=',0);
		DBG att.dump_out(stdout,2);

		 //figure out where to export from
		const char *filename=NULL;
		o=options.remaining();
		if (o) filename=o->arg();


		 //-------export
		if (!filename) {
			cout <<_("No file to export from specified!")<<endl;
			exit(1);
		}

		 //load in document to pass with config
		ErrorLog error;		 
		Document *doc=NULL;

		if (Load(filename,error)==0) doc=curdoc;
		if (error.Total()) {
			if (!doc) {
				dumperrorlog(_("Fatal errors loading document:"),error);
				exit(1);
			}
			dumperrorlog(_("Warnings encountered while loading document:"),error);
		}

		const char *format=att.findValue("filter");
		DBG cout << "Exporting with \""<<(format ? format : "unknown filter") <<"\""<<endl;
		ExportFilter *filter = FindExportFilter(format,false);
		if (!filter) {
			cout <<_("Filter not found!")<<endl;
			exit(1);
		}

		DocumentExportConfig *config=filter->CreateConfig(NULL);
		//config->dump_in_atts(&att,0,NULL);
		 
		config->doc=doc;
		config->doc->inc_count();
		config->filter=filter;
		config->dump_in_atts(&att,0,NULL);//second time with doc!

		int err=export_document(config,error);
		if (err>0) {
			dumperrorlog(_("Export failed."),error);
			exit(1);
		} else if (err<0) {
			dumperrorlog(_("Export finished with warnings:"),error);
		} else {
			cout <<_("Exported.")<<endl;
		}
		exit(0);
	} //if exprt

	 // options are now basically parsed, must handle any resulting commands like export
	DBG o=options.remaining();
	DBG if (o) {
	DBG 	cerr <<"Read in these files:"<<endl;
	DBG     int num=options.more()+1;
	DBG 	for (int c=1 ; o; o=options.next(), c++) 
	DBG 		cerr <<"  "<<c<<"/"<<num<<":  "<<o->arg()<<endl;
	DBG }


	 // load in any projects or documents after the args
	Document *doc=NULL;
	int index=topwindows.n;
	if (!project) project=new Project;
	ErrorLog log;

	for (o=options.remaining(); o; o=options.next()) {
		DBG cerr <<"----Read in:  "<<o->arg()<<endl;
		doc = NULL;
		if (Load(o->arg(),log)==0) doc = curdoc;
		else {
			generallog.AddError(0,0,0, _("Could not load %s!"), o->arg());
		}

		if (topwindows.n == index) {
			if (!doc && project->docs.n) doc = project->docs.e[0]->doc;
			if (doc && runmode==RUNMODE_Normal) addwindow(newHeadWindow(doc));
		}
	}

	if (pipein) {
		int n=0;
		DBG cerr << "Piping in..."<<endl;
		char *data = pipe_in_whole_file(stdin, &n);

		//*** //load from string data
		cerr << " *** need to finish implementing pipein!!"<<endl;
		if (!strcasecmp(pipeinarg, "default")) {
			 //assume is laidout file

//			if (Load(NULL, log, data) == 0) doc = curdoc;
//			if (topwindows.n == index) {
//				if (!doc && project->docs.n) doc = project->docs.e[0]->doc;
//				if (doc && runmode == RUNMODE_Normal) addwindow(newHeadWindow(doc));
//			}
		} else {
			 //parse import settings
			Attribute att;
			NameValueToAttribute(&att, pipeinarg, '=', ',');

			ImportConfig config;
			config.dump_in_atts(&att, 0, NULL);

			//ErrorLog log;
			import_document(&config, generallog, data,n);

			NotifyGeneralErrors(NULL);
		}

		delete[] data;

	} //end pipein

	DBG cerr <<"---------end options"<<endl;
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

//! Find the doc with the given obect_id. Does not increment its count.
Document *LaidoutApp::findDocumentById(unsigned long id)
{
	if (!project) return NULL;
	for (int c=0; c<project->docs.n; c++) {
		if (project->docs.e[c]->doc && project->docs.e[c]->doc->object_id==id)
			return project->docs.e[c]->doc;
	}
	return NULL;
}

//! Find the doc with saveas. Does not increment its count.
Document *LaidoutApp::findDocument(const char *saveas)
{
	if (!project) return NULL;

	 //search for full name
	for (int c=0; c<project->docs.n; c++) {
		if (project->docs.e[c]->doc
			 && project->docs.e[c]->doc->saveas 
			 && !strcmp(saveas,project->docs.e[c]->doc->saveas))
			return project->docs.e[c]->doc;
	}

	while (*saveas=='.') saveas++;

	 //search for partial name match
	for (int c=0; c<project->docs.n; c++) {
		if (project->docs.e[c]->doc
			 && project->docs.e[c]->doc->saveas 
			 && strstr(project->docs.e[c]->doc->saveas,saveas))
			return project->docs.e[c]->doc;
	}
	return NULL;
}

//! Return the path corresponding to the requested program.
/*! Currently, this responds only to "gs", which returns the current path for 
 * ghostscript, if any.
 *
 * \todo the undocumented laidoutrc attribute ghostscript_binary will set that variable in LaidoutApp. 
 *   In future, should probably have section devoted to known, modifiable external executables like
 *   gs, inkscape, gimp, tex, etc.
 */
const char *LaidoutApp::binary(const char *what)
{
	if (!strcmp(what,"gs")) return ghostscript_binary;
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
 *
 * If dir/name exists, and that file is readable, return the full path to it in a new char[].
 * Otherwise return NULL.
 */
char *LaidoutApp::full_path_for_resource(const char *name,const char *dir)//dir=NULL
{
	int c=0;
	if (!strncmp(name,"file://",7)) { name+=7; c=1; }
	char *fullname=newstr(name);

	if (c || !strncmp(fullname,"/",1) || !strncmp(fullname,"./",2) || !strncmp(fullname,"../",3)) {
		 // is path with filename
		convert_to_full_path(fullname,NULL);
		if (readable_file(fullname)) return fullname;
		delete[] fullname;
		return NULL;

	} else {
		 // else is a name
		if (dir) {
			prependstr(fullname,"/");
			prependstr(fullname,dir);
			prependstr(fullname,"/");
			prependstr(fullname,config_dir);
		} 
		convert_to_full_path(fullname,NULL);
		if (readable_file(fullname)) return fullname;

		DBG cerr <<" *** need to fully implement full_path_for_resource() name search!"<<endl;
	}
	delete[] fullname;//***
	fullname=NULL;//***

	return fullname;
}

//! Return the default absolute path for the given resource. 
char *LaidoutApp::default_path_for_resource(const char *resource)
{
	char *path=newstr(config_dir);
	appendstr(path,"/");
	appendstr(path,resource);
	simplify_path(path, 1);
	return path;
}

////! ***todo Find a resource by the human readable name, rather than by filename
//char *full_path_for_resource_by_name(const char *name,char *dir)//dir=NULL
//{
//}

//! Similar to Load(), but only for Document, not Project, and forces a rename.
/*! If a doc with the same filename is already loaded, it is ignored. Templates will only
 * be created from files straight from disk.
 *
 * Note that this does not automatically create a new window, but it does add the
 * new Document to LaidoutApp::project.
 *
 * \todo deal with returned error when load failed
 */
Document *LaidoutApp::LoadTemplate(const char *name, ErrorLog &log)
{
	 // find absolute path to a file
	char *fullname=full_path_for_resource(name,"templates");
	if (file_exists(fullname,1,NULL)!=S_IFREG) {
		delete[] fullname;
		return NULL;
	}
		
	Document *doc=new Document(NULL,fullname);
	
	 //must push before Load to not screw up setting up windows and other controls
	project->Push(doc);
	doc->dec_count();//so now has 1 count for project
	if (doc->Load(fullname,log)==0) { // load failed
		project->Pop(NULL);
		//doc->dec_count();
		return NULL;
	}
	
	makestr(doc->saveas,NULL); // this forces a change of name, must be done after Load
	delete[] fullname;
	if (curdoc && curdoc!=doc) {
		curdoc->dec_count();
		curdoc=doc;
		curdoc->inc_count();
	}
	return doc;
}

//! Load a project or document from filename, make it curdoc and return on successful load.
/*! If a doc with the same filename is already loaded, then make that curdoc.
 *
 * If there are fatal errors, then an error message is returned in error_ret, and NULL is returned.
 * Sometimes there are merely warnings, in which case those are returned in error_ret, but
 * the document or project is still loaded.
 *
 * Returns 0 for document loaded or document already loaded,
 * 1 for project loaded, or a negative number for fatal error
 * encountered.
 */
int LaidoutApp::Load(const char *filename, ErrorLog &log)
{
	if (!strncmp(filename,"file://",7)) filename+=7;
	char *fullname=newstr(filename);
	convert_to_full_path(fullname,NULL);
	Document *doc=findDocument(fullname);
	if (doc) {
		delete[] fullname;
		if (curdoc) curdoc->dec_count();
		curdoc=doc;
		if (curdoc) curdoc->inc_count();
		return 0;
	}

	FILE *f=open_laidout_file_to_read(fullname,"Project",&log, false);
	if (f) {
		fclose(f);
		if (project) project->clear();
		if (project->Load(fullname,log)==0) {
			delete[] fullname;
			return 1;
		} 
		delete[] fullname;
		return -1;
	}


	doc=new Document(NULL,fullname);
	if (!project) project=new Project;
	project->Push(doc); //important: this must be before doc->Load()
	doc->dec_count();
	
	if (doc->Load(fullname, log)==0) {
		 //load failed
		project->Pop(NULL);
		//doc->dec_count();
		doc=NULL;
	}
	delete[] fullname;
	if (doc) { 
		if (curdoc) curdoc->dec_count();
		curdoc=doc; 
		doc->inc_count(); //count for curdoc link
		return 0; 
	}
	return -1;
}

//! Create a new document from spec and call up a new ViewWindow.
/*! spec would be something passed in on the command line,
 * like (case doesn't matter) TODO:\n
 * <pre>
 *  laidout -n "saveas blah.doc, letter, 3 pages, booklet"
 *  laidout -n "3 pages, net(box,3,5,9)"
 *  laidout -n 'net("Dodecahedron")'
 * </pre>
 * 
 * For now, only paper size, number of pages, and very basic imposition type
 * are implemented. Does a comparison only of the characters in the field, so
 * for instance 'let' would match 'letter', and 'sing' would match 'singles'.
 * 
 * The option parser then calls this function with spec="letter, 3 pages, booklet".
 * At a minimum, you must specify the paper size and imposition.
 * "default" or a blank spec currently maps to "letter, portrait, 1 page, Singles".
 *
 * This is a looser version of the interpreter's command NewDocument.
 *
 * \todo *** default should be a setting in the laidoutrc.
 *
 * Return 0 on success, nonzero for fail.
 * 
 */
int LaidoutApp::NewDocument(const char *spec)
{
	if (!spec) return 1;
	if (!strcmp(spec,"default")) spec="letter, portrait, singles";
	DBG cerr <<"------create new doc from \""<<spec<<"\""<<endl;

	char *saveas=NULL;
	Imposition *imp=NULL;
	PaperStyle *paper=NULL;
	int numpages=1;

	//Attribute *spec_parameters=parse_fields(NULL,spec,NULL);


//---------------------****
	 // break down the spec
	char **fields=split(spec,',',NULL),
		 *field=NULL;
	if (!fields) { 
		DBG cout <<"*** broken spec"<<endl;
		return 2; 
	}
	int c,c2,n; // n is length of first alnum word in field
	int landscape=0;
	for (c=0; fields[c]; c++) {
		field=fields[c];
		while (field && isspace(*field)) field++;
		n=0;
		while (isalnum(field[n])) n++;
		if (!field) continue;

		if (strcasestr(field,"box")==field && isspace(field[3])) {
			NetImposition *nimp=new NetImposition;
			nimp->SetNet(field);
			imp=nimp;
			continue;
		}

		if (isdigit(*field)) { // assume number of pages
			char *ee=NULL;
			int i=strtol(field,&ee,10);
			while (isspace(*ee)) ee++;
			if (*ee=='\0' || !strcasecmp(ee,_("pages"))) {
				numpages=i;
				if (numpages<=0) numpages=1;
				continue;
			}
		}

		 // check for new filename
		if (!strncasecmp(field,"saveas",n)) {
			field+=n;
			n=0;
			while (isspace(*field)) field++;
			while (isalnum(field[n]) || field[n]=='.' || field[n]=='_' || field[n]=='-' || field[n]=='+') n++;
			saveas=newnstr(field,n);
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

		 // check imposition resources
		if (!imp) for (c2=0; c2<impositionpool.n; c2++) {
			if (!strncasecmp(field,impositionpool.e[c2]->name,n)) {
				imp=impositionpool.e[c2]->Create();
				break;
			}
		}
		if (c2!=impositionpool.n) continue;
	}
	
	if (!imp) imp=new Singles(); //either way, imp has count of 1 now
	 
	if (!paper) paper=papersizes.e[0];
	unsigned int flags=paper->flags;
	paper->flags=(paper->flags&~1)|landscape;
	if (!strcmp("NetImposition",imp->whattype())) {
		NetImposition *neti=dynamic_cast<NetImposition *>(imp);
		if (!neti->nets.n) {
			neti->SetNet("Dodecahedron");
		}
	}
	imp->SetPaperSize(paper); // makes a duplicate of paper
	if (numpages==0) numpages=1;
	imp->NumPages(numpages);
	paper->flags=flags;
	
	if (!saveas) { // make a unique temporary name...
		makestr(saveas,Untitled_name());
	}
	
	Document *newdoc=new Document(imp,saveas);
	if (imp) imp->dec_count();
	if (!project) project=new Project();
	project->Push(newdoc); //adds count to newdoc
	newdoc->dec_count();
	if (saveas) delete[] saveas;

	addwindow(newHeadWindow(newdoc));
	return 0;
}

//! Create a new ViewWindow for a new Document based on imposition.
/*! Puts it in the current project, creates new project if none exists.
 *
 * The imposition is passed onto Document, which increments its count.
 *
 * filename is where is should be saved, and is assumed to not exist. If the calling
 * code is not sure if something exists there, then filename should be passed NULL.
 * The file is not saved here. The filename is merely where the document will
 * be saved to at the next save.
 *
 * Return 0 for success, or nonzero for error and document not added.
 */
int LaidoutApp::NewDocument(Imposition *imposition, const char *filename)
{
	if (!imposition) return 1;

	Document *newdoc=new Document(imposition,filename);
	if (!project) project=new Project();
	project->Push(newdoc); //adds count to newdoc

	DBG cerr <<"***** just pushed newdoc using imposition "<<newdoc->imposition->Name()<<", must make viewwindow *****"<<endl;
	newdoc->dec_count();
	
	
	if (!laidout->donotusex) {
		anXWindow *blah=newHeadWindow(newdoc); 
		addwindow(blah);
	}
	return 0;
}

/*! Return 0 for success or nonzero for error.
 */
int LaidoutApp::NewProject(Project *proj, ErrorLog &log)
{
	if (!proj) return 1;
	if (proj==project) return 0;
	delete project;
	project=proj;

	//***update windows to not reference old documents, limbos, or other resources
	
	int n=0;
	for (int c=0; c<topwindows.n; c++) {
		if (dynamic_cast<HeadWindow *>(topwindows.e[c])) n++;
	}
	
	if (!laidout->donotusex && !n) addwindow(newHeadWindow(NULL,"ViewWindow"));

	project->initDirs();
	project->Save(log);

	return 0;
}

/*! Return the export filter with name as its ExportFilter::VersionName().
 * First search exact matches to name, but if !exact_only, then also do a
 * case insensitive search for part of name in VersionName().
 */
ExportFilter *LaidoutApp::FindExportFilter(const char *name, bool exact_only)
{
	if (!name) return NULL;

	 //search for exact format match first
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcasecmp(laidout->exportfilters.e[c]->VersionName(),name)) {
			return laidout->exportfilters.e[c];
		}
	}

	if (exact_only) return NULL;

	 //if no match, search for first case insensitive match
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strncasecmp(laidout->exportfilters.e[c]->VersionName(),name,strlen(name))) {
			return laidout->exportfilters.e[c];
		}
	}

	return NULL;
}

//! Push onto exportfilters, in alphabetical order.
void LaidoutApp::PushExportFilter(ExportFilter *filter)
{
	if (!filter) return;
//	for (int c=0; c<exportfilters.n; c++) {
//		if (strcmp(filter->VersionName(),exportfilters.e[c]->VersionName())<0) {
//			exportfilters.push(filter,-1,c);
//			return;
//		}
//	}
	exportfilters.push(filter);
}

//! Push onto importfilters, in alphabetical order.
void LaidoutApp::PushImportFilter(ImportFilter *filter)
{
	if (!filter) return;
//	for (int c=0; c<importfilters.n; c++) {
//		if (strcmp(filter->VersionName(),importfilters.e[c]->VersionName())<0) {
//			importfilters.push(filter,-1,c);
//			return;
//		}
//	}
	importfilters.push(filter);
}

//! Call dump_out() on all HeadWindow objects in topwindows.
/*! This writes out:
 * <pre>
 *  window
 *    ....whatever HeadWindow puts out
 * </pre>
 *
 * If doc!=0 then only output headwindows that only have doc in them. 
 * 
 * \todo might be better to have some special check when
 *   loading so that if doc is being loaded from a project load, then all windows
 *   in the doc are ignored?, or if doc is only doc open?
 * \todo implement dump context
 */
int LaidoutApp::DumpWindows(FILE *f,int indent,Document *doc)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c,n=0;
	HeadWindow *head;
	for (c=0; c<topwindows.n; c++) {
		head=dynamic_cast<HeadWindow *>(topwindows.e[c]);
		if (!head || (doc && !head->HasOnlyThis(doc))) continue;
		fprintf(f,"%swindow\n",spc);
		head->dump_out(f,indent+2,0,NULL);
		n++;
	}
	return n;
}


} // namespace Laidout


using namespace Laidout;

//---------------------------------------- main() ----------------------- This is where it all begins!
int main(int argc,char **argv)
{
	 //initialize gettext
	char scratch[100];
	sprintf(scratch,"laidout-%s",LAIDOUT_VERSION);
	setlocale(LC_ALL,"");
	if (!strncmp(LANGUAGE_PATH, "..", 2)) {
		char *path = ExecutablePath();
		appendstr(path, "/../"); //since path points to binary file, assume it's */bin/laidout
		appendstr(path, LANGUAGE_PATH);
		simplify_path(path, 1);
		bindtextdomain(scratch,path);
		delete[] path;
	} else bindtextdomain(scratch,LANGUAGE_PATH);
	textdomain(scratch);

	DBG cerr<<"---------Intl settings----------"<<endl;
	DBG cerr<<"Text domain: "<<textdomain(NULL)<<endl;
	DBG cerr<<"Domain dir:  "<<bindtextdomain(scratch,NULL)<<endl;
	DBG cerr<<"Locale:      "<<setlocale(LC_MESSAGES,NULL)<<endl;
	DBG cerr<<"--------------------------------"<<endl;


	InitOptions();

	int c,index;
	c = options.Parse(argc,argv, &index); //parses into arguments, but does not process

	if (c==-2) {
		cerr <<"Missing parameter for "<<argv[index]<<"!!"<<endl;
		exit(0);
	}
	if (c==-1) {
		cerr <<"Unknown option "<<argv[index]<<"!!"<<endl;
		exit(0);
	}


	 //process a few things before anything else happens,
	 //since laxkit spews out debugging stuff straight away.
	 //laidout->parseargs() snags the rest
	LaxOption *o=options.find("helpman",0);
	if (o && o->parsed_present) {
		options.HelpMan(stdout);
		exit(0);
	}

	o=options.find("help",0);
	if (o && o->parsed_present) {
		options.Help(stdout); // Show usage summary
		exit(0);
	}

	o=options.find("version",0);
	if (o && o->parsed_present) {
		 // Show version info, then exit
		cout <<LaidoutVersion()<<endl;
		exit(0);
	}

	const char *backend="cairo";
	o=options.find("backend",0);
	if (o && o->parsed_present) {
		if (!strcasecmp(o->arg(),"xlib")) backend="xlib";
		else if (!strcasecmp(o->arg(),"cairo")) backend="cairo";
	}

	const char *theme=NULL;
	o=options.find("theme",0);
	if (o && o->parsed_present) {
		theme = o->arg();
	}


	 //redefine Laxkit's default preview maker
	//generate_preview_image = laidout_preview_maker;

	laidout=new LaidoutApp();
	if (theme) laidout->SetTheme(theme);
	if (backend) laidout->Backend(backend);
	o = options.find("experimental",0);
	if (o && o->parsed_present) laidout->experimental = 1;


	laidout->init(argc,argv);

	//DBG cerr <<"------------ stylemanager->dump --------------------"<<endl;
	//DBG stylemanager.dump(stderr,3);
	//DBG cerr <<"---------- stylemanager->dump end ---------------------"<<endl;

	laidout->run();

	DBG cerr <<"---------Laidout Close--------------"<<endl;
	 //for debugging purposes, spread out closing down various things....
	 //..this is a lot of stuff that should probably have a better finalizing
	 //  architecture behind it
	laidout->close();

	DBG cerr <<"---------  close interface manager..."<<endl;
	LaxInterfaces::InterfaceManager::SetDefault(NULL,true);
	DBG cerr <<"---------  close undo manager..."<<endl;
	Laxkit::SetUndoManager(NULL);
	DBG cerr <<"---------  close shortcut manager..."<<endl;
	Laxkit::InstallShortcutManager(NULL); //forces deletion of shortcut lists in Laxkit
	DBG cerr <<"---------  close icon manager..."<<endl;
	Laxkit::IconManager::SetDefault(NULL);
	DBG cerr <<"---------  close image processor..."<<endl;
	Laxkit::ImageProcessor::SetDefault(NULL);
	DBG cerr <<"---------  close image loaders manager..."<<endl;
	Laxkit::ImageLoader::FlushLoaders();
	DBG cerr <<"---------  close units manager..."<<endl;
	Laxkit::SetUnitManager(NULL);
	DBG cerr <<"---------  final dec_count of laidout app..."<<endl;
	laidout->dec_count();

	DBG cerr <<"---------  close stylemanager"<<endl;
	DBG cerr <<"  stylemanager.getNumFields()="<<(stylemanager.getNumFields())<<endl;
	//DBG cerr <<"  stylemanager.styles.n="<<(stylemanager.styles.n)<<endl;
	stylemanager.Clear();

	


//#ifdef LAX_USES_CAIRO
//	DBG cerr <<"---------  cairo_debug_reset_static_data()..."<<endl;
//	DBG cairo_debug_reset_static_data();
//#endif

	DBG cerr <<"-----------------------------Bye!--------------------------"<<endl;
	DBG cerr <<"------------end of code, default destructors follow--------"<<endl;

	return 0;
}




