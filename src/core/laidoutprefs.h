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
#ifndef LAIDOUTPREFS_H
#define LAIDOUTPREFS_H


#include "externaltools.h"
#include "../calculator/values.h"


namespace Laidout {


class LaidoutPreferences : public Value
{
  public:
  	char *rc_file;

	double uiscale;
	bool dont_scale_icons;
	bool use_monitor_ppi;

	int default_units;
	char *unitname;

	int pagedropshadow;
	Laxkit::ScreenColor shadow_color;
	Laxkit::ScreenColor current_page_color;
	
	int preview_size;
	bool start_with_last;
	char *shortcuts_file;
	char *splash_image_file;
	char *default_template;
	char *defaultpaper;
	char *temp_dir;
	char *palette_dir;
	char *clobber_protection;
	bool   autosave;
	double autosave_time;
	char  *autosave_path;
	char *exportfilename;
	Laxkit::PtrStack<char> icon_dirs;
	Laxkit::PtrStack<char> plugin_dirs;
	bool experimental;

	bool autosave_prefs; //immediately on any change

	ExternalToolManager external_tool_manager;
	
	LaidoutPreferences();
	virtual ~LaidoutPreferences();

	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicateValue();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
	virtual int SavePrefs(const char *file=NULL);

	virtual int AddPath(const char *resource, const char *path);

	virtual int AddExternalTool(ExternalTool *tool);
	virtual int AddExternalCategory(ExternalToolCategory *category);
	virtual ExternalToolCategory *GetToolCategory(int category);
	virtual const char *GetToolCategoryName(int id, const char **idstr = nullptr);
	virtual ExternalTool *FindExternalTool(const char *str);
	virtual ExternalTool *GetDefaultTool(int category);
	
	int UpdatePreference(const char *which, const char *value, const char *laidoutrc);
	int UpdatePreference(const char *which, double value, const char *laidoutrc);
	int UpdatePreference(const char *which, int value, const char *laidoutrc);
	int UpdatePreference(const char *which, bool value, const char *laidoutrc);
	int UpdatePreference(const char *which, const Laxkit::ScreenColor &color, const char *laidoutrc);
};


} //namespace Laidout

#endif

