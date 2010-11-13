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
#include <lax/refptrstack.cc>

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


StyleManager::StyleManager()
{}

StyleManager::~StyleManager()
{}

//! Mainly For debugging at the moment... w&1 defs, w&2 styles...
/*! In the future, this could be used to automatically dump out a 
 * file format description, analogous to dtds.
 */
void StyleManager::dump(FILE *f,int w)
{
	if (w&1) {
		for (int c=0; c<functions.n; c++) {
			fprintf(f,"function %d\n",c);
			functions.e[c]->dump_out(f,2,0,NULL);
		}
		for (int c=0; c<styledefs.n; c++) {
			fprintf(f,"styledef %d\n",c);
			styledefs.e[c]->dump_out(f,2,0,NULL);
		}
	}
//	if (w&2) {
//		for (int c=0; c<styles.n; c++) {
//			fprintf(f,"style %d\n",c);
//			styles.e[c]->dump_out(f,2,0,NULL);
//		}
//	}
}

//! Flush styles and styledefs.
void StyleManager::flush()
{
	functions.flush();
	styledefs.flush();
	//styles.flush();
}

//! Make the stylemanager remember the StyleDef.
/*! Return 1 for style exists already. 0 for success.
 *
 * If def->format==Element_Function, then push it onto functions.
 * Otherwise push it onto styledefs.
 *
 * If it does not exist already, then def->inc_count() is called.
 *
 * It is assumed that def has one count referring to the pointer
 * used to call the function with. 
 * 
 * If absorb, then the calling code does not want to worry about def
 * anymore, whether or not the StyleDef is in the manager. So if the
 * def was there already, def->dec_count() is called. If the def was not
 * in the manager, then that initial 1 count will become the stylemanager's
 * reference.
 *
 * If absorb==0, the initial tick for the def pointer remains whether or
 * not the styledef existed already. The count of def is increased only when
 * the def was not already in the manager.
 *
 * \todo add sorted pushing for faster access
 */
int StyleManager::AddStyleDef(StyleDef *def, int absorb)
{
	int c;
	RefPtrStack<StyleDef> *stack;
	if (def->format==Element_Function) stack=&functions; else stack=&styledefs;
	for (c=0; c<stack->n; c++) {
		if (!strcmp(def->name,stack->e[c]->name)) {
			if (absorb) def->dec_count();
			return 1; // def was already there
		}
	}
	//def->deleteMe=RemoveStyleDefFromManager;
	stack->push(def,3); // this incs count
	if (absorb) def->dec_count();
	return 0;
}

//! Find the styledef with StyleDef::name same as styledef.
/*! This returns the pointer, but does NOT increase the count on it.
 *
 * If which==1, then only look in functions. If which==2, only look in
 * objects. If which==3 (the default), then look in either.
 */
StyleDef *StyleManager::FindDef(const char *styledef, int which)
{
	if (which&1) for (int c=0; c<functions.n; c++)
		if (!strcmp(styledef,functions.e[c]->name)) return functions.e[c];
	if (which&2) for (int c=0; c<styledefs.n; c++)
		if (!strcmp(styledef,styledefs.e[c]->name)) return styledefs.e[c];
	return NULL;
}

//! Find the style with Style::stylename same as style.
/*! This returns the pointer, but does NOT increase the count on it.
 */
Style *StyleManager::FindStyle(const char *style)
{
//	for (int c=0; c<styles.n; c++)
//		if (!strcmp(style,styles.e[c]->Stylename())) return styles.e[c];
	return NULL;
}

/*! \todo need to have generic style create when styledef found but 
 *    newfunc not found
 */
Style *StyleManager::newStyle(const char *styledef)
{
	StyleDef *s=FindDef(styledef,2);
	if (!s) return NULL;
	if (s->newfunc) return s->newfunc(s);
	if (!s->stylefunc) return NULL;

	//s->stylefunc()

	return NULL;
}

//! Create a generic?
Style *StyleManager::newStyle(Style *baseonthis)
{
	cout << "*** imp StyleManager::newStyle(Style *baseonthis)!!"<<endl;
	return NULL;
}

