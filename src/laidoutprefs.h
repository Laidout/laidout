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
#ifndef LAIDOUTPREFS_H
#define LAIDOUTPREFS_H


#include "calculator/values.h"

namespace Laidout {


class LaidoutPreferences : public Value
{
  public:
	int default_units;
	char *unitname;
	int pagedropshadow;
	int preview_size;
	char *splash_image_file;
	char *default_template;
	char *defaultpaper;
	char *temp_dir;
	char *palette_dir;
	double autosave;
	char *autosave_path;
	char *exportfilename;
	//PtrStack<char> palette_dirs;
	Laxkit::PtrStack<char> icon_dirs;
	bool experimental;

	LaidoutPreferences();
	virtual ~LaidoutPreferences();

	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
	virtual Value *dereference(const char *extstring, int len);
	virtual int SavePrefs(const char *file=NULL);
};

int UpdatePreference(const char *which, const char *value, const char *laidoutrc);

} //namespace Laidout

#endif

