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
#include <lax/anxapp.h>

#include "laidoutprefs.h"
#include "../language.h"
#include "../configured.h"
#include "stylemanager.h"
#include "utils.h"
#include "../ui/externaltoolwindow.h"

#include <sys/file.h>
#include <clocale>
#include <cctype>
#include <unistd.h>


#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;


namespace Laidout {


/*! \class LaidoutPreferences
 * 
 * Global preferences of Laidout. These things get input and output with laidoutrc.
 */
/*! \var Laxkit::PtrStack<char> LaidoutPreferences::preview_file_bases
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
 * This can be used to search in XDG_CACHE_HOME/thumbnails/large/@ and XDG_CACHE_HOME/thumbnails/normal/@".
 * 
 * \todo be able to create preview files of different types by default... Should be able
 *   to more fully suggest preview generation perhaps? The thing with '@' is a little
 *   hacky maybe. Should be able to select something equivalent to 
 *   "Search by freedesktop thumb spec" which would automatically search both the large
 *   and normal dirs.. that also affects default max width for previews......
 */ 
/*! \var int LaidoutPreferences::preview_over_this_size
 * \brief The file size in kilobytes over which preview images should be used where possible.
 */


LaidoutPreferences::LaidoutPreferences()
 : preview_file_bases(LISTS_DELETE_Array),
   icon_dirs(LISTS_DELETE_Array),
   plugin_dirs(LISTS_DELETE_Array)
{
	rc_file = nullptr;
	autosave_prefs = true; //immediately on any change

	splash_image_file = nullptr; //default later is LoadIcon("LaidoutSplash")...  

	default_units  = UNITS_Inches;
	unitname       = newstr("inches");
	uiscale        = 1;
	pagedropshadow = 5;
	dont_scale_icons = true;
	use_monitor_ppi  = true;

    default_template = nullptr;
    defaultpaper = get_system_default_paper();
    temp_dir = nullptr;
	start_with_last = false;

	if (S_ISDIR(file_exists("/usr/share/gimp/2.0/palettes", 1, nullptr)))
		palette_dir = newstr("/usr/share/gimp/2.0/palettes");
	else palette_dir = nullptr;

	preview_size = 512;
	auto_generate_previews = false;

	clobber_protection = nullptr;

	autosave      = false;
	autosave_time = 5; //minutes
	autosave_path = newstr("./%f.autosave");
	//autosave_num=0; //0 is no limit

	experimental = false;

	exportfilename = newstr("%f-exported.whatever");

	shadow_color.rgbf(0.,0.,0.,1.0);
	current_page_color.rgbf(.8,0.,0.,1.0);
}

LaidoutPreferences::~LaidoutPreferences()
{
	delete[] rc_file;
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

Value *LaidoutPreferences::duplicateValue()
{
	LaidoutPreferences *p=new LaidoutPreferences;

	makestr(p->rc_file, rc_file);
	p->autosave_prefs = autosave_prefs;

	p->auto_generate_previews = auto_generate_previews;
	p->preview_size  = preview_size;
	for (int c = 0; c < preview_file_bases.n; c++) {
		p->preview_file_bases.push(newstr(preview_file_bases.e[c]), LISTS_DELETE_Array);
	}

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

	p->shadow_color = shadow_color;
	p->current_page_color = current_page_color;

	return p;
}


ObjectDef *LaidoutPreferences::makeObjectDef()
{
	ObjectDef *def=stylemanager.FindDef("LaidoutPreferences");
	if (def) {
		def->inc_count();
		return def;
	}

	def=new ObjectDef(nullptr, //extensd
			"LaidoutPreferences", //name
			_("General preferences"), //Name
			_("Laidout global configuration options go here. They are stored in ~/.config/laidout-(version)/laidoutrc."
			  "If you modify settings from within Laidout, the file will be overwritten,"
			  "and you'll lose any comments or formatting you have inserted directly in that file."),
			"class", //Value format
			nullptr, //range
			nullptr); //defaultvalue

	def->push("uiscale",
			_("UI Scale"),
			_("Values <= 0 mean to use default."),
			"real", "[-1,200]", "1",
			0,
			nullptr);
	makestr(def->last()->uihint, "NumSlider");

	def->push("dont_scale_icons",
			_("Don't scale icons"),
			_("Use native icon size, not scaled along with font size"),
			"boolean", nullptr,"true",
			0,
			nullptr);

	def->push("shortcuts_file",
			_("Shortcuts file"),
			_("File for shortcuts"),
			"File", nullptr,nullptr,
			0,
			nullptr);

	def->push("splashimage",
			_("Splash image file"),
			_("Splash image file"),
			"File", nullptr,nullptr,
			0,
			nullptr);

	def->push("auto_generate_previews",
			_("Auto generate previews"),
			_("If true, when you import an image, automatically create a smaller copy to speed up screen rendering"),
			"boolean", nullptr,"false",
			0,
			nullptr);

	def->push("preview_size",
			_("Preview size"),
			_("Pixel width or height to render cached previews of objects for on screen viewing. "),
			"real", nullptr,"512",
			0,
			nullptr);

	def->push("pagedropshadow",
			_("Page drop shadow"),
			_("How much to offset drop shadows around papers and pages"),
			"real", nullptr,"5",
			0,
			nullptr);

	def->push("shadow_color",
			_("Shadow color"),
			_("Color of page drop shadow"),
			"Color", nullptr,"rgbf(0,0,0)",
			0,
			nullptr);

	def->push("current_page_color",
			_("Current page outline"),
			_("Color of current page indicator"),
			"Color", nullptr,"rgbf(1,0,0)",
			0,
			nullptr);

	def->push("start_with_last",
			_("Start With Last Document"),
			_("Start with last document used."),
			"boolean", nullptr,nullptr,
			0,
			nullptr);

	def->push("defaulttemplate",
			_("Default template"),
			_("Default template to create new blank documents from"),
			"File", nullptr,nullptr,
			0,
			nullptr);

	def->push("defaultunits",
			_("Default units"),
			_("Default units to use in all controls. Inside files, still uses inches."),
			"string", nullptr,nullptr,
			0,
			nullptr);

	def->push("defaultpaper",
			_("Default paper"),
			_("Default paper"),
			"string", "letter",nullptr,
			0,
			nullptr);

//	def->push("activate_color",
//			_("Activate color"),
//			_("Activate color, usually a green"),
//			"Color", nullptr,"rgbf(0,.78,0)",
//			0,
//			nullptr);
//
//	def->push("deactivate_color",
//			_("Deactivate color"),
//			_("Deactivate color, usually a red"),
//			"Color", nullptr,"rgbf(1,.39,.39)",
//			0,
//			nullptr);

	def->push("preview_file_bases",
			_("Preview file names"),
			_("A list of possible preview file names. Use '@' for freedesktop hash, ^ for dir of document, % for file name without extension, and * for basename."),
			"set", "String", nullptr,
			OBJECTDEF_ISSET,
			nullptr);

	def->push("icon_dirs",
			_("Icon directories"),
			_("A list of directories to look for icons in"),
			"set", "File",nullptr,
			OBJECTDEF_ISSET,
			nullptr);

	def->push("plugin_dirs",
			_("Plugin directories"),
			_("A list of directories to look for plugins in"),
			"set", "File",nullptr,
			OBJECTDEF_ISSET,
			nullptr);

	def->push("autosave",
			_("Autosave"),
			_("Whether to autosave"),
			"boolean", "false",nullptr,
			0,
			nullptr);

	def->push("autosave_time",
			_("Autosave time"),
			_("How many minutes between autosaves (0==never)."),
			"real", "0",nullptr,
			0,
			nullptr);

//	def->push("autosave_num",
//			_("Number of autosaves"),
//			_("Don't have more than this number of autosave files. 0 means no limit."),
//			"int", "0",nullptr,
//			0,
//			nullptr);

	def->push("autosave_path",
			_("Autosave path"),
			_("Where and how to save when autosaving. Relative paths are to current file"),
			"string", "./%f.autosave",nullptr,
			0,
			nullptr);

	def->push("export_file_name",
			_("Export file name"),
			_("Default base name (without extension) to use when exporting."),
			"string", nullptr,"%f-exported",
			0,
			nullptr);

	def->push("experimental",
			_("Experimental"),
			_("Whether to activate experimental features. Requires restart."),
			"boolean", nullptr,"false",
			0,
			nullptr);

	def->pushFunction("edit_external_tools",
			_("Edit external tools..."),
			_("Bring up window to configure external tools"),
			nullptr
			);
	makestr(def->last()->uihint, "button");
	def->last()->stylefunc = ExternalToolWindow::SpawnExternalToolsWindow;
	
	 //---------
//	def->push("",  // id
//			_(""), // label
//			_(""), // description
//			"int", nullptr,"", //type, range, defaultvalue
//			0, // flags
//			nullptr,  //NewObjectFunc
//			nullptr); //ObjectFunc
	return def;


//	fprintf(f,"laxprofile Light #Default built in profiles are Dark and Light. You can define others in the laxconfig section.\n");
//	fprintf(f,"laxconfig-sample #Remove the \"-sample\" part to redefine various default window behaviour settingss\n");
//	dump_out_rc(f,nullptr,2,-1);
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

/*! Return 1 for success, 2 for success, but other contents changed too, -1 for unknown, 0 for total failure */
int LaidoutPreferences::assign(FieldExtPlace *ext,Value *v)
{
	const char *extstring = ext->e(0);

	if (!strcmp(extstring, "defaultunits")) {
		StringValue *str = dynamic_cast<StringValue*>(v);
		if (!str) return 0;
		UnitManager *units = GetUnitManager();
		int id = units->UnitId(str->str);
		if (id == UNITS_None) return 0;
		default_units = id;
		if (autosave_prefs) UpdatePreference(extstring, str->str, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "shortcuts_file")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(shortcuts_file, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(shortcuts_file, sv->str);
		}
		if (autosave_prefs) UpdatePreference(extstring, shortcuts_file, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "splashimage")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(splash_image_file, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(splash_image_file, sv->str);
		}
		if (autosave_prefs) UpdatePreference(extstring, splash_image_file, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "defaulttemplate")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(default_template, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(default_template, sv->str);
		}
		if (autosave_prefs) UpdatePreference(extstring, default_template, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "defaultpaper")) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv) return 0;
		makestr(defaultpaper, sv->str);
		if (autosave_prefs) UpdatePreference(extstring, defaultpaper, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "pagedropshadow")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		pagedropshadow = d;
		if (autosave_prefs) UpdatePreference(extstring, pagedropshadow, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "shadow_color")) {
		ColorValue *color = dynamic_cast<ColorValue*>(v);
		if (!color) return 0;
		shadow_color.rgbf(color->color.Red(), color->color.Green(), color->color.Blue(), color->color.Alpha());
		if (autosave_prefs) UpdatePreference(extstring, shadow_color, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "current_page_color")) {
		ColorValue *color = dynamic_cast<ColorValue*>(v);
		if (!color) return 0;
		current_page_color.rgbf(color->color.Red(), color->color.Green(), color->color.Blue(), color->color.Alpha());
		if (autosave_prefs) UpdatePreference(extstring, current_page_color, nullptr);
		return 1;
	}
	
	if (!strcmp(extstring, "uiscale")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		uiscale = d;
		anXApp::app->theme->ui_scale = uiscale;
		anXApp::app->ThemeReconfigure();
		if (autosave_prefs) UpdatePreference(extstring, uiscale, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "dont_scale_icons")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		dont_scale_icons = d;
		if (autosave_prefs) UpdatePreference(extstring, dont_scale_icons, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "experimental")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		experimental = d;
		// *** turn on/off experimental features? warning dialog about needing restart?
		if (autosave_prefs) UpdatePreference(extstring, experimental, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "start_with_last")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		start_with_last = d;
		if (autosave_prefs) UpdatePreference(extstring, start_with_last, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "temp_dir")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(temp_dir, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(temp_dir, sv->str);
		}
		if (autosave_prefs) UpdatePreference(extstring, temp_dir, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "autosave")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		autosave = d;
		if (autosave_prefs) UpdatePreference(extstring, autosave, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "autosave_time")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		autosave_time = d;
		if (autosave_prefs) UpdatePreference(extstring, autosave_time, nullptr);
		return 1;
	}

//	if (!strcmp(extstring, "autosave_num")) {
//		return new IntValue(autosave_num);
//	}

	if (!strcmp(extstring, "autosave_path")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(autosave_path, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(autosave_path, sv->str);
		}
		if (autosave_prefs) UpdatePreference(extstring, autosave_path, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "export_file_name")) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv) return 0;
		makestr(exportfilename, sv->str);
		if (autosave_prefs) UpdatePreference(extstring, exportfilename, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "auto_generate_previews")) {
		int e = 0;
		bool b = getBooleanValue(v, &e);
		if (e == 0) return 0;
		auto_generate_previews = b;
		if (autosave_prefs) UpdatePreference(extstring, auto_generate_previews, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "preview_size")) {
		double d;
		if (!isNumberType(v, &d)) return 0;
		if (d < 1.0) return 0;
		preview_size = d;
		if (autosave_prefs) UpdatePreference(extstring, preview_size, nullptr);
		return 1;
	}

	if (!strcmp(extstring, "palette_dir")) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (fv) {
			makestr(palette_dir, fv->filename);
		} else {
			StringValue *sv = dynamic_cast<StringValue*>(v);
			if (!sv) return 0;
			makestr(palette_dir, sv->str);
		}
		return 1;
	}

	if (!strcmp(extstring, "preview_file_bases")) {
		SetValue *sv = dynamic_cast<SetValue*>(v);
		if (!sv) return 0;
		for (int c = 0; c < sv->n(); c++) {
			Value *v = sv->e(c);
			if (v->type() != VALUE_String && v->type() != VALUE_File) return 0;
		}
		for (int c = 0; c < sv->n(); c++) {
			Value *v = sv->e(c);
			if (v->type() == VALUE_String) {
				StringValue *s = dynamic_cast<StringValue*>(v);
				preview_file_bases.push(newstr(s->str));
			} else { // VALUE_File
				FileValue *s = dynamic_cast<FileValue*>(v);
				preview_file_bases.push(newstr(s->filename));
			}
		}
		return 1;
	}

	if (!strcmp(extstring, "icon_dirs")) {
		// ***
		return 0;
	}

	if (!strcmp(extstring, "plugin_dirs")) {
		// ***
		return 0;
	}

	return -1;
}

Value *LaidoutPreferences::dereference(const char *extstring, int len)
{
	if (!strcmp(extstring, "defaultunits")) {
		UnitManager *units = GetUnitManager();
		char *name = nullptr;
		units->UnitInfoId(default_units, nullptr, nullptr,nullptr,&name,nullptr,nullptr);
		StringValue *s = new StringValue(name);
		return s;
	}

	if (!strcmp(extstring, "shortcuts_file")) {
		return shortcuts_file ? new FileValue(shortcuts_file) : nullptr;
	}

	if (!strcmp(extstring, "splashimage")) {
		return splash_image_file ? new FileValue(splash_image_file) : nullptr;
	}

	if (!strcmp(extstring, "defaulttemplate")) {
		return default_template ? new FileValue(default_template) : nullptr;
	}

	if (!strcmp(extstring, "defaultpaper")) {
		return defaultpaper ? new StringValue(defaultpaper) : nullptr;
	}

	if (!strcmp(extstring, "pagedropshadow")) {
		return new IntValue(pagedropshadow);
	}

	if (!strcmp(extstring, "shadow_color")) {
		return new ColorValue(shadow_color);
	}

	if (!strcmp(extstring, "current_page_color")) {
		return new ColorValue(current_page_color);
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
		return temp_dir ? new StringValue(temp_dir) : nullptr;
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
		return autosave_path ? new StringValue(autosave_path) : nullptr;
	}

	if (!strcmp(extstring, "export_file_name")) {
		return exportfilename ? new StringValue(exportfilename) : nullptr;
	}

	if (!strcmp(extstring, "palette_dir")) {
		return palette_dir ? new StringValue(palette_dir) : nullptr;
	}

	if (!strcmp(extstring, "auto_generate_previews")) {
		return new BooleanValue(auto_generate_previews);
	}

	if (!strcmp(extstring, "preview_size")) {
		return new IntValue(preview_size);
	}

	if (!strcmp(extstring, "preview_file_bases")) {
		SetValue *set = new SetValue("String");
		for (int c = 0; c < preview_file_bases.n; c++) {
			set->Push(new StringValue(preview_file_bases.e[c]), 1);
		}
		return set; 
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


	return nullptr;
}

/*! Note this totally clobbers old file. */
int LaidoutPreferences::SavePrefs(const char *file)
{
	if (!file) return 2;

	FILE *f=fopen(file,"w");
	if (f) {
		setlocale(LC_ALL,"C");
		dump_out(f,0,0,nullptr);
		fclose(f);
		setlocale(LC_ALL,"");
		return 0;
	}

	return 1;
}


/*! Add path to the list of directories used by resource.
 * Currently resource can be "icons" or "plugins".
 *
 * Checks if path is a valid, existing directory. Fails with 1 if not.
 * Return 0 on success.
 */
int LaidoutPreferences::AddPath(const char *resource, const char *path)
{
	if (file_exists(path, 1, nullptr) != S_IFDIR) {
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

/*! tool->category must already be in external_categories, or else nothing is done and 1 is returned.
 * Return 0 if added successfully.
 *  Takes ownership of tool, so don't worry about dec_count UNLESS 1 IS RETURNED. 
 */
int LaidoutPreferences::AddExternalTool(ExternalTool *tool)
{
	return external_tool_manager.AddExternalTool(tool);
}

/*! Takes ownership of category, so don't worry about dec_count. */
int LaidoutPreferences::AddExternalCategory(ExternalToolCategory *category)
{
	return external_tool_manager.AddExternalCategory(category);
}

const char *LaidoutPreferences::GetToolCategoryName(int id, const char **idstr)
{
	return external_tool_manager.GetToolCategoryName(id, idstr);
}

ExternalToolCategory *LaidoutPreferences::GetToolCategory(int category)
{
	return external_tool_manager.GetToolCategory(category);
}

/*! Find existing tool from str like "Print: lp" -> "category: command_name".
 */
ExternalTool *LaidoutPreferences::FindExternalTool(const char *str)
{
	return external_tool_manager.FindExternalTool(str);
}

/*! Return the first tool in category, or null. */
ExternalTool *LaidoutPreferences::GetDefaultTool(int category)
{
	return external_tool_manager.GetDefaultTool(category);
}

/*! Returns a null terminated list.
 * You should delete the returned list with deletestrs(strs, 0).
 * file should be full path for file.
 */
char **LaidoutPreferences::DefaultPreviewLocations(const char *file, const char *doc_path, int index)
{
	char **bases = new char*[preview_file_bases.n + 1];
	bases[preview_file_bases.n] = nullptr;

	int i = 0;
	for (int c = 0; c < preview_file_bases.n; c++) {
		bases[i] = PreviewFileName(file, preview_file_bases.e[c], doc_path, index);
		if (bases[i] != nullptr) i++;
	}
	for ( ; i < preview_file_bases.n; i++)
		bases[i] = nullptr;

	return bases;
}


int LaidoutPreferences::UpdatePreference(const char *which, const ScreenColor &color, const char *laidoutrc)
{
	char scratch[100];
	sprintf(scratch, "rgbf(%.10g,%.10g,%.10g,%.10g)", color.Red(), color.Green(), color.Blue(), color.Alpha());
	return UpdatePreference(which, scratch, laidoutrc);
}

int LaidoutPreferences::UpdatePreference(const char *which, double value, const char *laidoutrc)
{
	char scratch[50];
	sprintf(scratch, "%.10g", value);
	return UpdatePreference(which, scratch, laidoutrc);
}

int LaidoutPreferences::UpdatePreference(const char *which, int value, const char *laidoutrc)
{
	char scratch[50];
	sprintf(scratch, "%d", value);
	return UpdatePreference(which, scratch, laidoutrc);
}

int LaidoutPreferences::UpdatePreference(const char *which, bool value, const char *laidoutrc)
{
	return UpdatePreference(which, value ? "true" : "false", laidoutrc);
}

/*! Update global preference by writing out a new laidoutrc.
 * This only works when which/value is supposed to be on one line. value shouldn't have any newlines.
 *
 * The line containing which at the beginning of the line or "#which" will be entirely replaced with "which value".
 *
 * Return 0 for success or nonzero for error.
 */
int LaidoutPreferences::UpdatePreference(const char *which, const char *value, const char *laidoutrc)
{
	if (!laidoutrc) laidoutrc = rc_file;

	int fd=open(laidoutrc, O_RDONLY, 0);
	if (fd<0) { return 1; }
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"r");
	if (!f) { close(fd); return 2; }

	char *outfile=newstr(laidoutrc);
	appendstr(outfile, "-TEMP");
	while (file_exists(outfile, 1, nullptr)) {
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

	char *line=nullptr;
	size_t n=0;
	int c;
	int found=0;

	char *vvalue=nullptr;
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

/*! Return 1 for uncommented match, -1 for commented match, 0 for no match. */
static int match_key(const char *line, const char *which, int slen)
{
	if (strncmp(line, which, slen) == 0 && (isspace(line[slen]) || !line[slen])) return 1;
	if (*line != '#') return 0;
	line++;
	while (isspace(*line)) line++;
	if (strncmp(line, which, slen) == 0 && (isspace(line[slen]) || !line[slen])) return -1;
	return 0;
}

static void fprint_list(FILE *out, const char *which, PtrStack<char> &stack)
{
	for (int cc = 0; cc < stack.n; cc++) {
		const char *value = stack.e[cc];
		char *vvalue = nullptr;
		if (strpbrk(value, "\"\'#\t\n\r")) {
			vvalue = escape_string(value, '"', true);
			value = vvalue;
		}
		fprintf(out, "%s %s\n", which, value);
		delete[] vvalue;
	}
}

/*! Update global preference by writing out a new laidoutrc.
 * This only works when which/value is supposed to be a list of several items of the same name.
 *
 * The line containing which at the beginning of the line will be entirely replaced with the whole
 * list formatted as "which value". Any subsequent "which ..." lines will be removed.
 * Any "#which" lines will not be touched.
 *
 * Return 0 for success or nonzero for error.
 */
int LaidoutPreferences::UpdatePreference(const char *which, PtrStack<char> &stack, const char *laidoutrc)
{
	if (!laidoutrc) laidoutrc = rc_file;
	if (!laidoutrc) return 1;

	int fd = open(laidoutrc, O_RDONLY, 0);
	if (fd < 0) { return 1; }
	flock(fd, LOCK_EX);
	FILE *f = fdopen(fd,"r");
	if (!f) { close(fd); return 2; }

	char *outfile = newstr(laidoutrc);
	appendstr(outfile, "-TEMP");
	while (file_exists(outfile, 1, nullptr)) {
		char *nout = increment_file(outfile);
		delete[] outfile;
		outfile = nout;
	}

	int slen = strlen(which);

	FILE *out = fopen(outfile, "w");
	if (!out) {
		fclose(f); //also closes fd
		delete[] outfile;
		return 3;
	}

	setlocale(LC_ALL,"C");

	//read each line, if "^which ..." is found, replace with "which value"

	char *line = nullptr;
	size_t n = 0;
	int c;
	int found = 0;

	while (!feof(f)) {
		c = getline(&line, &n, f);
		if (c <= 0) continue;

		int match = match_key(line, which, slen);

		if (!found && match == 1) {
			found = 1;
			fprint_list(out, which, stack);

		} else if (found && match == 1) {
			// found an uncommented option, but we already wrote out, so ignore
			// fprintf(out, "#%s", line);

		} else if (!found && match == -1) {
			// found a commented option, and we haven't written out yet
			// leave it commented just in case
			found = 1;
			fprint_list(out, which, stack);
			fwrite(line, 1, strlen(line), out);

		} else {
			fwrite(line, 1, strlen(line), out);
		}
	}

	if (!found) fprint_list(out, which, stack);
	
	if (line) free(line);

	fclose(out);
	flock(fd,LOCK_UN);
	fclose(f); //also closes the fd	
	setlocale(LC_ALL,"");

	 //finally move temp file to real file
	unlink(laidoutrc);
	rename(outfile, laidoutrc);
	delete[] outfile;

	return 0;	
}



} // namespace Laidout

