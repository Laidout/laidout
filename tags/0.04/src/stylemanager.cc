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
//------------------------------
//StyleManagerNode {
//	StyleDef *def;
//	PtrStack<Style> styles;
//}
//-------------------------- StyleDefinition/StyleManager -------------------------------

typedef Style *(*StyleCreateFunc)(StyleDef *def);
		
/*! \class StyleManager
 * \brief Holds style definitions and style instances.
 */


//! Mainly For debugging at the moment... w&1 defs, w&2 styles...
/*! In the future, this could be used to automatically dump out a 
 * file format description, analogous to dtds.
 */
void StyleManager::dump(FILE *f,int w)
{
	if (w&1) {
		for (int c=0; c<styledefs.n; c++) {
			fprintf(f,"styledef %d\n",c);
			styledefs.e[c]->dump_out(f,2,0);
		}
	}
	if (w&2) {
		for (int c=0; c<styles.n; c++) {
			fprintf(f,"style %d\n",c);
			styles.e[c]->dump_out(f,2,0);
		}
	}
}

//! Flush styles and styledefs.
void StyleManager::flush()
{
	styledefs.flush();
	styles.flush();
}

//! Currently, just does return styledefs.pushnodup(def,0).
/*! Return 1 for style exists already. 0 for success.
 *
 * If it does not exist already, then def->inc_count() is called.
 *
 * If absorb, then the def was just created, and one of its count
 * refers to def. The calling code does not want to worry about that
 * count whether or not the style was taken already. So if the style was
 * taken already, then def->dec_count() is called, which will typically
 * destroy def. If the style is new, then its count will increase by one
 * here, which refers to the stylemanager reference.
 *
 * If absorb==0, the initial tick for the def pointer remains whether or
 * not the style existed already.
 * 
 * \todo decide whether StyleManager gets its own count on the def,
 *   or if it puts a special deleteMe in the def to tell manager to
 *   remove all reference to it..
 */
int StyleManager::AddStyleDef(StyleDef *def, int absorb)
{
	int c;
	for (c=0; c<styledefs.n; c++) {
		if (!strcmp(def->name,styledefs.e[c]->name)) {
			if (absorb) def->dec_count();
			return 1;
		}
	}
	//def->deleteMe=RemoveStyleDefFromManager;
	styledefs.push(def,0);
	if (!absorb) def->inc_count();
	return 0;
}

//! Find the styledef with StyleDef::name same as styledef.
/*! This returns the pointer, but does NOT increase the count on it.
 */
StyleDef *StyleManager::FindDef(const char *styledef)
{
	for (int c=0; c<styledefs.n; c++)
		if (!strcmp(styledef,styledefs.e[c]->name)) return styledefs.e[c];
	return NULL;
}

//! Find the style with Style::stylename same as style.
/*! This returns the pointer, but does NOT increase the count on it.
 */
Style *StyleManager::FindStyle(const char *style)
{
	for (int c=0; c<styles.n; c++)
		if (!strcmp(style,styles.e[c]->Stylename())) return styles.e[c];
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

