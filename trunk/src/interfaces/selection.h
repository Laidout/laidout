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
#ifndef SELECTION_H
#define SELECTION_H

#include <lax/interfaces/viewportwindow.h>
#include "../calculator/values.h"

namespace Laidout {

//--------------------------- SelectedObject -------------------------

/*! \class SelectedObject
 */
class SelectedObject
{
  public:
	int info;
	LaxInterfaces::SomeData *obj;
	LaxInterfaces::ObjectContext *oc;
	ValueHash properties;

	SelectedObject(LaxInterfaces::ObjectContext *noc, int ninfo);
	virtual ~SelectedObject();
};


//--------------------------- Selection -------------------------

class Selection : public Laxkit::anObject, public Laxkit::DoubleBBox
{
	Laxkit::PtrStack<SelectedObject> objects;
	int currentobject;
	anObject *base_object;

  public:
	Selection();
	virtual ~Selection();

	virtual Selection *duplicate();
	virtual int FindIndex(LaxInterfaces::ObjectContext *oc);
	virtual int Add(LaxInterfaces::ObjectContext *oc, int where, int ninfo=-1);
	virtual int AddNoDup(LaxInterfaces::ObjectContext *oc, int where, int ninfo=-1);
	virtual int Remove(int i);
	virtual void Flush();
	virtual LaxInterfaces::ObjectContext *CurrentObject();
	virtual int CurrentObjectIndex() { return currentobject; }
	virtual void CurrentObject(int which);

	virtual int n() { return objects.n; }
	virtual LaxInterfaces::ObjectContext *e(int i);
	virtual int e_info(int i);
	virtual ValueHash *e_properties(int i);

};


} //namespace Laidout

#endif

