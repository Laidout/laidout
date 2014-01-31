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
// Copyright (C) 2014 by Tom Lechner
//

#include "selection.h"

#include <lax/lists.cc>

namespace Laidout {

//--------------------------- SelectedObject -------------------------

/*! \class SelectedObject
 */

SelectedObject::SelectedObject(LaxInterfaces::ObjectContext *noc)
{
	oc=noc->duplicate();
	if (oc) obj=oc->obj;
}

SelectedObject::~SelectedObject()
{
	if (oc) delete oc;
}




//--------------------------- Selection -------------------------

/*! \class Selection
 */

Selection::Selection()
{
	currentobject=-1;
	base_object=NULL;
}

Selection::~Selection()
{
	if (base_object) base_object->dec_count();
}

LaxInterfaces::ObjectContext *Selection::CurrentObject()
{
	if (currentobject<0) return NULL;
	return objects.e[currentobject]->oc;
}

/*! Set the current object to be this one. Set to top if which out of bounds.
 */
void Selection::CurrentObject(int which)
{
	if (which<0 || which>=objects.n) which=objects.n-1;
	currentobject=which;
}


/*! Return index of added oc in selection. Warning: does NOT check for duplicates!
 */
int Selection::Add(LaxInterfaces::ObjectContext *oc, int where)
{
	if (!oc) return -1;
	if (where<0 || where>objects.n) where=objects.n;
	currentobject=where;
	return objects.push(new SelectedObject(oc),-1,where);
}

/*! Like Add(), but do not add if is already in selection. In that case,
 * return -1. Return -2 if oc==NULL.
 */
int Selection::AddNoDup(LaxInterfaces::ObjectContext *oc, int where)
{
	if (!oc) return -2;
	if (where<0 || where>objects.n) where=objects.n;

	for (int c=0; c<objects.n; c++) {
		if (oc->isequal(objects.e[c]->oc)) return -1;
	}

	currentobject=where;
	return objects.push(new SelectedObject(oc),-1,where);
}

/*! Remove item at index i.
 */
int Selection::Remove(int i)
{
	if (i<0 || i>=objects.n) return 1;
	int c=objects.remove(i);
	if (currentobject==i) currentobject=i-1;
	if (currentobject<0) currentobject=objects.n-1;
	return c;
}


void Selection::Flush()
{
	currentobject=-1;
	objects.flush();
}


int Selection::FindIndex(LaxInterfaces::ObjectContext *oc)
{
	for (int c=0; c<objects.n; c++) {
		if (oc->isequal(objects.e[c]->oc)) return c;
	}
	return -1;
}

LaxInterfaces::ObjectContext *Selection::e(int i)
{
	if (i<0 || i>=objects.n) return NULL;
	return objects.e[i]->oc;
}

ValueHash *Selection::e_properties(int i)
{
	if (i<0 || i>=objects.n) return NULL;
	return &objects.e[i]->properties;
}

// *** complicated because currently objectcontext needs viewport to retrieve transforms
//void Selection::FindBBox()
//{
//}


} //namespace Laidout

