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
#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <lax/lists.h>
#include "styles.h"

class StyleManager
{
 protected:
	int firstuserdef;
 public:
	Laxkit::PtrStack<StyleDef> styledefs;
	Laxkit::PtrStack<Style> styles;
	int AddStyleDef(StyleDef *def,int absorb=0);
	//void deleteStyle(Style *style);
	Style *newStyle(const char *styledef);
	Style *newStyle(Style *baseonthis);// <--create a generic?
	StyleDef *FindDef(const char *styledef);
	Style *FindStyle(const char *style);
	void flush();

	void dump(FILE *f,int w=1);
};

#ifndef LAIDOUT_CC
extern StyleManager stylemanager;
#endif

#endif

