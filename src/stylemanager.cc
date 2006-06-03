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


#include "stylemanager.h"
#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


//from laxkit in RefCounted: typedef int (*DeleteRefCountedFunc)(RefCounted *obj);

////! Func to be called from destructor of obj.
//int RemoveStyleDefFromManager(RefCounted *obj)
//{
//	return stylemanager.styledefs.remove(stylemanager.styledefs.findindex(obj));
//}

//-------------------------- StyleDefinition/StyleManager -------------------------------

typedef Style *(*StyleCreateFunc)(StyleDef *def);
		
/*! \class StyleManager
 * \brief Holds style definitions and style instances.
 */
//class StyleManager
//{
// protected:
//	
// public:
//	PtrStack<StyleDef> styledefs;
//	PtrStack<Style> styles;
//	int AddStyleDef(StyleDef *def);
////	void deleteStyle(Style *style);
//	Style *newStyle(const char *styledef);
//	Style *newStyle(Style *baseonthis); <--create a generic?
//	StyleDef *FindDef(const char *styledef);
//	void flush();
//};
//

//! Flush styles and styledefs.
void StyleManager::flush()
{
	styledefs.flush();
	styles.flush();
}

//! Currently, just does return styledefs.pushnodup(def).
/*! Return 1 for style exists already. 0 for success.
 */
int StyleManager::AddStyleDef(StyleDef *def)
{
	int c;
	for (c=0; c<styledefs.n; c++) {
		if (!strcmp(def->name,styledefs.e[c]->name)) return 1;
	}
	//def->deleteMe=RemoveStyleDefFromManager;
	return styledefs.pushnodup(def,0);
}

//! Find the styledef with StyleDef::name same as styledef.
StyleDef *StyleManager::FindDef(const char *styledef)
{
	for (int c=0; c<styledefs.n; c++)
		if (!strcmp(styledef,styledefs.e[c]->name)) return styledefs.e[c];
	return NULL;
}

/*! \todo need to have generic style create when styledef found but 
 *    newfunc not found
 */
Style *StyleManager::newStyle(const char *styledef)
{
	StyleDef *s=FindDef(styledef);
	if (s && s->newfunc) return s->newfunc(s);
	return NULL;
}

//! Create a generic?
Style *StyleManager::newStyle(Style *baseonthis)
{
	cout << "*** imp StyleManager::newStyle(Style *baseonthis)!!"<<endl;
	return NULL;
}

