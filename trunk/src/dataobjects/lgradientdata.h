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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef LGRADIENTDATA_H
#define LGRADIENTDATA_H

#include <lax/interfaces/gradientinterface.h>
#include "drawableobject.h"



namespace Laidout {



//------------------------------- LGradientData ---------------------------------------

class LGradientData : public DrawableObject, public LaxInterfaces::GradientData
{
  public:
	LGradientData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LGradientData();
	virtual const char *whattype() { return "GradientData"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup=NULL);
};



} //namespace Laidout

#endif

