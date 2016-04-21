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
// Copyright (C) 2013 by Tom Lechner
//

#include <lax/units.h>
#include <lax/strmanip.h>

#include "laidoutprefs.h"
#include "language.h"
#include "configured.h"
#include "stylemanager.h"
#include "utils.h"


using namespace Laxkit;

namespace Laidout {


//Laidout object def:
//  tools
//  windows
//  global prefs
//  imposition types/generators
//  export filters
//  import filters
//  papersizes
//
//  project
//    documents -> set of Document
//    limbos    -> set of Group
//    papergroups -> set of PaperGroup
//    plain textobjects  -> set of PlainTextObjects
//
//todo:
//  resources


/*! \class LaidoutPreferences
 * 
 * Global preferences of Laidout. These things get input and output with laidoutrc.
 */


LaidoutPreferences::LaidoutPreferences()
 : icon_dirs(2)
{
	default_units=UNITS_Inches;
	unitname=newstr("inches");
	pagedropshadow=5;

	splash_image_file=newstr(ICON_DIRECTORY);
	appendstr(splash_image_file,"/laidout-splash.png");

    default_template=NULL;
    defaultpaper=get_system_default_paper();
    temp_dir=NULL;
	palette_dir=newstr("/usr/share/gimp/2.0/palettes");

	preview_size=400;

	autosave=0; //minutes
	autosave_path=newstr("./%f.autosave");

	experimental=false;

	exportfilename=newstr("%f-exported.whatever");
}

LaidoutPreferences::~LaidoutPreferences()
{
	delete[] autosave_path;
	delete[] unitname;
	delete[] splash_image_file;
	delete[] default_template;
	delete[] defaultpaper;
	delete[] temp_dir;
	delete[] palette_dir;
	delete[] exportfilename;
}

Value *LaidoutPreferences::duplicate()
{
	LaidoutPreferences *p=new LaidoutPreferences;

	p->preview_size=preview_size;
	p->default_units=default_units;
	makestr(p->unitname,unitname);
	p->pagedropshadow=pagedropshadow;
	makestr(p->splash_image_file,splash_image_file);
	makestr(p->default_template,default_template);
	makestr(p->defaultpaper,defaultpaper);
	makestr(p->temp_dir,temp_dir);
	p->autosave=autosave;
	makestr(p->autosave_path,autosave_path);
	makestr(p->exportfilename, exportfilename);
	p->experimental=experimental;

	return p;
}

ObjectDef *LaidoutPreferences::makeObjectDef()
{
	ObjectDef *def=stylemanager.FindDef("LaidoutPreferences");
	if (def) {
		def->inc_count();
		return def;
	}

	//StyleDef(const char *nname,const char *nName,const char *ntp, const char *ndesc,unsigned int fflags=STYLEDEF_CAPPED);
	def=new StyleDef(NULL, //extensd
			"LaidoutPreferences", //name
			_("General preferences"), //Name
			_("Laidout global configuration options go here. They are stored in ~/.config/laidout-(version)/laidoutrc."
			  "If you modify settings from within Laidout, the file will be overwritten,"
			  "and you'll lose any comments or formatting you have inserted directly in that file."),
			"class", //Value format
			NULL, //range
			NULL); //defaultvalue

	def->push("experimental",
			_("Experimental"),
			_("Whether to activate experimental features."),
			"boolean", NULL,"false",
			0,
			NULL);

	def->push("shortcutsfile",
			_("Shortcuts file"),
			_("File for shortcuts"),
			"File", NULL,NULL,
			0,
			NULL);

	def->push("splashimage",
			_("Splash image file"),
			_("Splash image file"),
			"File", NULL,NULL,
			0,
			NULL);

	def->push("previewsize",
			_("Preview size"),
			_("Pixel width or height to render cached previews of objects for on screen viewing. "),
			"real", NULL,"5",
			0,
			NULL);

	def->push("pagedropshadow",
			_("Page drop shadow"),
			_("How much to offset drop shadows around papers and pages"),
			"real", NULL,"5",
			0,
			NULL);

	def->push("defaulttemplate",
			_("Default template"),
			_("Default template to create new blank documents from"),
			"string", NULL,NULL,
			0,
			NULL);

	def->push("defaultunits",
			_("Default units"),
			_("Default units to use in all controls. Inside files, still uses inches."),
			"string", NULL,NULL,
			0,
			NULL);

	def->push("defaultpaper",
			_("Default paper"),
			_("Default paper"),
			"string", "letter",NULL,
			0,
			NULL);

	def->push("activate_color",
			_("Activate color"),
			_("Activate color, usually a green"),
			"Color", NULL,"rgbf(0,.78,0)",
			0,
			NULL);

	def->push("deactivate_color",
			_("Deactivate color"),
			_("Deactivate color, usually a red"),
			"Color", NULL,"rgbf(1,.39,.39)",
			0,
			NULL);

	def->push("icon_dirs",
			_("Icon directory"),
			_("A list of directories to look for icons in"),
			"set", "File",NULL,
			OBJECTDEF_ISSET,
			NULL);

	def->push("autosave",
			_("Autosave"),
			_("How often (in minutes) to autosave for recovery (0==never). Saving to actual file still must be manually done."),
			"real", "0",NULL,
			0,
			NULL);

	def->push("autosave_path",
			_("Autosave path"),
			_("Where and how to save when autosaving. Relative paths are to current file, %%f is current file name."),
			"string", NULL,NULL,
			0,
			NULL);

	def->push("export_file_name",
			_("Export file name"),
			_("Default base name (without extension) to use when exporting."),
			"string", NULL,"%f-exported",
			0,
			NULL);

	 //---------
//	def->push("",
//			_(""),
//			_(""),
//			VALUE_, NULL,"",
//			0,
//			NULL);
	return def;


//	fprintf(f,"laxprofile Light #Default built in profiles are Dark and Light. You can define others in the laxconfig section.\n");
//	fprintf(f,"laxconfig-sample #Remove the \"-sample\" part to redefine various default window behaviour settingss\n");
//	dump_out_rc(f,NULL,2,-1);
//	fprintf(f,"\n"
//			  "#laxcolors  #To set only the colors of laxconfig, use this\n"
//			  "#  panel\n"
//			  "#    ...\n"
//			  "#  menu\n"
//			  "#    ...\n"
//			  "#  edits\n"
//			  "#    ...\n"
//			  "#  buttons\n"
//			  "#    ...\n");

}


} // namespace Laidout

