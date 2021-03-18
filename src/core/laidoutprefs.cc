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
// Copyright (C) 2013 by Tom Lechner
//

#include <lax/units.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/iconmanager.h>

#include "laidoutprefs.h"
#include "../language.h"
#include "../configured.h"
#include "stylemanager.h"
#include "utils.h"

#include <sys/file.h>
#include <clocale>
#include <cctype>
#include <unistd.h>

//template implementation:
#include <lax/lists.cc>

#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxFiles;


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
//
//todo:
//  resources


/*! \class LaidoutPreferences
 * 
 * Global preferences of Laidout. These things get input and output with laidoutrc.
 */


LaidoutPreferences::LaidoutPreferences()
 : icon_dirs(2),
   plugin_dirs(2)
{
	splash_image_file = newstr("./laidout-splash.png");

	default_units  = UNITS_Inches;
	unitname       = newstr("inches");
	uiscale        = 1;
	pagedropshadow = 5;
	dont_scale_icons = true;

    default_template = NULL;
    defaultpaper = get_system_default_paper();
    temp_dir = NULL;
	start_with_last = false;

	if (S_ISDIR(file_exists("/usr/share/gimp/2.0/palettes", 1, NULL)))
		palette_dir=newstr("/usr/share/gimp/2.0/palettes");
	else palette_dir = NULL;

	preview_size = 400;

	clobber_protection = nullptr;

	autosave      = false;
	autosave_time = 5; //minutes
	autosave_path = newstr("./%f.autosave");
	//autosave_num=0; //0 is no limit

	experimental = false;

	exportfilename = newstr("%f-exported.whatever");
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
	delete[] clobber_protection;
}

Value *LaidoutPreferences::duplicate()
{
	LaidoutPreferences *p=new LaidoutPreferences;

	p->preview_size  = preview_size;
	p->default_units = default_units;
	makestr(p->unitname,unitname);
	p->pagedropshadow= pagedropshadow;
	makestr(p->splash_image_file,splash_image_file);
	makestr(p->default_template,default_template);
	makestr(p->defaultpaper,defaultpaper);
	makestr(p->temp_dir,temp_dir);
	p->autosave = autosave;
	p->autosave_time = autosave_time;
	//p->autosave_num = autosave_num;
	makestr(p->autosave_path,autosave_path);
	makestr(p->exportfilename, exportfilename);
	p->start_with_last = start_with_last;
	p->experimental = experimental;
	p->uiscale = uiscale;
	p->dont_scale_icons = dont_scale_icons;

	for (int c=0; c<icon_dirs.n; c++) {
		p->icon_dirs.push(newstr(icon_dirs.e[c]), LISTS_DELETE_Array);
	}

	for (int c=0; c<plugin_dirs.n; c++) {
		p->plugin_dirs.push(newstr(plugin_dirs.e[c]), LISTS_DELETE_Array);
	}

	return p;
}

ObjectDef *LaidoutPreferences::makeObjectDef()
{
	ObjectDef *def=stylemanager.FindDef("LaidoutPreferences");
	if (def) {
		def->inc_count();
		return def;
	}

	def=new ObjectDef(NULL, //extensd
			"LaidoutPreferences", //name
			_("General preferences"), //Name
			_("Laidout global configuration options go here. They are stored in ~/.config/laidout-(version)/laidoutrc."
			  "If you modify settings from within Laidout, the file will be overwritten,"
			  "and you'll lose any comments or formatting you have inserted directly in that file."),
			"class", //Value format
			NULL, //range
			NULL); //defaultvalue

	def->push("uiscale",
			_("UI Scale"),
			_("UI Scale"),
			"real", NULL,"1",
			0,
			NULL);

	def->push("dont_scale_icons",
			_("Don't scale icons"),
			_("Use native icon size, not scaled along with font size"),
			"boolean", NULL,"true",
			0,
			NULL);

	def->push("shortcuts_file",
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
			"real", NULL,"400",
			0,
			NULL);

	def->push("pagedropshadow",
			_("Page drop shadow"),
			_("How much to offset drop shadows around papers and pages"),
			"real", NULL,"5",
			0,
			NULL);

	def->push("start_with_last",
			_("Start With Last Document"),
			_("Start with last document used."),
			"boolean", NULL,NULL,
			0,
			NULL);

	def->push("defaulttemplate",
			_("Default template"),
			_("Default template to create new blank documents from"),
			"File", NULL,NULL,
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

//	def->push("activate_color",
//			_("Activate color"),
//			_("Activate color, usually a green"),
//			"Color", NULL,"rgbf(0,.78,0)",
//			0,
//			NULL);
//
//	def->push("deactivate_color",
//			_("Deactivate color"),
//			_("Deactivate color, usually a red"),
//			"Color", NULL,"rgbf(1,.39,.39)",
//			0,
//			NULL);

	def->push("icon_dirs",
			_("Icon directories"),
			_("A list of directories to look for icons in"),
			"set", "File",NULL,
			OBJECTDEF_ISSET,
			NULL);

	def->push("plugin_dirs",
			_("Plugin directories"),
			_("A list of directories to look for plugins in"),
			"set", "File",NULL,
			OBJECTDEF_ISSET,
			NULL);

	def->push("autosave",
			_("Autosave"),
			_("Whether to autosave"),
			"boolean", "false",NULL,
			0,
			NULL);

	def->push("autosave_time",
			_("Autosave time"),
			_("How many minutes between autosaves (0==never)."),
			"real", "0",NULL,
			0,
			NULL);

//	def->push("autosave_num",
//			_("Number of autosaves"),
//			_("Don't have more than this number of autosave files. 0 means no limit."),
//			"int", "0",NULL,
//			0,
//			NULL);

	def->push("autosave_path",
			_("Autosave path"),
			_("Where and how to save when autosaving. Relative paths are to current file"),
			"string", "./%f.autosave",NULL,
			0,
			NULL);

	def->push("export_file_name",
			_("Export file name"),
			_("Default base name (without extension) to use when exporting."),
			"string", NULL,"%f-exported",
			0,
			NULL);

	def->push("experimental",
			_("Experimental"),
			_("Whether to activate experimental features. Requires restart."),
			"boolean", NULL,"false",
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

Value *LaidoutPreferences::dereference(const char *extstring, int len)
{
	if (!strcmp(extstring, "defaultunits")) {
		UnitManager *units=GetUnitManager();
		char *name=NULL;
		units->UnitInfoId(default_units, NULL, NULL,NULL,&name,NULL);
		StringValue *s=new StringValue(name);
		return s;
	}

	if (!strcmp(extstring, "shortcuts_file")) {
		return shortcuts_file ? new FileValue(shortcuts_file) : NULL;
	}

	if (!strcmp(extstring, "splashimage")) {
		return splash_image_file ? new FileValue(splash_image_file) : NULL;
	}

	if (!strcmp(extstring, "defaulttemplate")) {
		return default_template ? new FileValue(default_template) : NULL;
	}

	if (!strcmp(extstring, "defaultpaper")) {
		return defaultpaper ? new StringValue(defaultpaper) : NULL;
	}

	if (!strcmp(extstring, "pagedropshadow")) {
		return new IntValue(pagedropshadow);
	}

	if (!strcmp(extstring, "uiscale")) {
		return new DoubleValue(uiscale);
	}

	if (!strcmp(extstring, "dont_scale_icons")) {
		return new BooleanValue(dont_scale_icons);
	}

	if (!strcmp(extstring, "experimental")) {
		return new BooleanValue(experimental);
	}

	if (!strcmp(extstring, "start_with_last")) {
		return new BooleanValue(start_with_last);
	}

	if (!strcmp(extstring, "temp_dir")) {
		return temp_dir ? new StringValue(temp_dir) : NULL;
	}

	if (!strcmp(extstring, "autosave")) {
		return new BooleanValue(autosave);
	}

	if (!strcmp(extstring, "autosave_time")) {
		return new DoubleValue(autosave_time);
	}

//	if (!strcmp(extstring, "autosave_num")) {
//		return new IntValue(autosave_num);
//	}

	if (!strcmp(extstring, "autosave_path")) {
		return autosave_path ? new StringValue(autosave_path) : NULL;
	}

	if (!strcmp(extstring, "export_file_name")) {
		return exportfilename ? new StringValue(exportfilename) : NULL;
	}

	if (!strcmp(extstring, "palette_dir")) {
		return palette_dir ? new StringValue(palette_dir) : NULL;
	}

	if (!strcmp(extstring, "previewsize")) {
		return new IntValue(preview_size);
	}

	if (!strcmp(extstring, "icon_dirs")) {
		SetValue *set = new SetValue("File");
		IconManager *icons=IconManager::GetDefault();
		for (int c=0; c<icons->NumPaths(); c++) {
			set->Push(new FileValue(icons->GetPath(c)), 1);
		}
		return set; 
	}

	if (!strcmp(extstring, "plugin_dirs")) {
		SetValue *set = new SetValue("File");
		for (int c=0; c<plugin_dirs.n; c++) {
			set->Push(new FileValue(plugin_dirs.e[c]), 1);
		}
		return set; 
	}


	return NULL;
}

int LaidoutPreferences::SavePrefs(const char *file)
{
	if (!file) return 2;

	FILE *f=fopen(file,"w");
	if (f) {
		setlocale(LC_ALL,"C");
		dump_out(f,0,0,NULL);
		fclose(f);
		setlocale(LC_ALL,"");
		return 0;
	}

	return 1;
}

/*! Update global preference by writing out a new laidoutrc.
 * This only works when which/value is supposed to be on one line. value shouldn't have any newlines.
 *
 * The line containing which at the beginning of the line or "#which" will be entirely replaced with "which value".
 */
int UpdatePreference(const char *which, const char *value, const char *laidoutrc)
{
	int fd=open(laidoutrc, O_RDONLY, 0);
	if (fd<0) { return 1; }
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"r");
	if (!f) { close(fd); return 2; }

	char *outfile=newstr(laidoutrc);
	appendstr(outfile, "-TEMP");
	while (file_exists(outfile, 1, NULL)) {
		char *nout = increment_file(outfile);
		delete[] outfile;
		outfile = nout;
	}

	FILE *out=fopen(outfile, "w");
	if (!out) {
		fclose(f); //also closes fd
		delete[] outfile;
		return 3;
	}

	setlocale(LC_ALL,"C");

	//read each line, if "^which ..." is found, replace with "which value"
	//If not found, but there is a "^#which ...", then replace that line
	// *** Note that if you do a lot of tinkering with the laidoutrc, this may wipe
	//     out comments you put in on those specific lines

	char *line=NULL;
	size_t n=0;
	int c;
	int found=0;

	char *vvalue=NULL;
	if (strpbrk(value, "\"\'#\t\n\r")) {
		vvalue=escape_string(value, '"', true);
		value=vvalue;
	}

	while (!feof(f)) {
		c=getline(&line, &n, f);
		if (c<=0) continue;

		if (!found && strncmp(line, which, strlen(which))==0 && isspace(line[strlen(which)])) {
			found=1;
			fprintf(out, "%s %s\n", which, value);

		} else if (found && strncmp(line, which, strlen(which))==0 && isspace(line[strlen(which)])) {
			 //found the option, but we already wrote out, so comment out this one
			fprintf(out, "#%s", line);

		} else if (!found && line[0]=='#' && strncmp(line+1, which, strlen(which))==0 && isspace(line[1+strlen(which)])) {
			found=1;
			fprintf(out, "%s %s\n", which, value);

		} else {
			fwrite(line, 1, strlen(line), out);
		}
	}

	if (!found) {
		fprintf(out, "%s %s\n", which, value);
	}
	
	if (line) free(line);

	fclose(out);
	flock(fd,LOCK_UN);
	fclose(f); //also closes the fd	
	setlocale(LC_ALL,"");

	 //finally move temp file to real file
	unlink(laidoutrc);
	rename(outfile, laidoutrc);
	delete[] outfile;

	delete[] vvalue;

	return 0;
}

/*! Add path to the list of directories used by resource.
 * Currently resource can be "icons" or "plugins".
 *
 * Checks if path is a valid, existing directory. Fails with 1 if not.
 * Return 0 on success.
 */
int LaidoutPreferences::AddPath(const char *resource, const char *path)
{
	if (file_exists(path, 1, NULL) != S_IFDIR) {
		DBG cerr << "LaidoutPreferences::AddPath("<<resource<<", "<<path<<"): not a directory!"<<endl;
		return 1;
	}

	if (!strcmp(resource, "icons")) {
		DBG cerr <<"Adding icon path: "<<path<<endl;
		icon_dirs.push(newstr(path), LISTS_DELETE_Array);
		return 0;

	} else if (!strcmp(resource, "plugins")) {
		DBG cerr <<"Adding plugin path: "<<path<<endl;
		plugin_dirs.push(newstr(path), LISTS_DELETE_Array);
		return 0;
	}

	return 1;
}

} // namespace Laidout

