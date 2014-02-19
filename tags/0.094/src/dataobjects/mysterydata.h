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
// Copyright (C) 2007-2013 by Tom Lechner
//
#ifndef MYSTERYDATA_H
#define MYSTERYDATA_H

#include "drawableobject.h"
#include <lax/interfaces/rectinterface.h>
#include <lax/attributes.h>


namespace Laidout {



//--------------------------------------- MysteryData ---------------------------------
class MysteryData : virtual public DrawableObject,
					virtual public LaxInterfaces::SomeData
{
  public:
	char *importer;
	char *name;
	long nativeid;
	int numpoints;
	flatpoint *outline;
	LaxFiles::Attribute *attributes;
	MysteryData(const char *gen=NULL);
	virtual ~MysteryData();
	virtual const char *whattype() { return "MysteryData"; }
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);
	virtual int installAtts(LaxFiles::Attribute *att);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

} //namespace Laidout

#endif


