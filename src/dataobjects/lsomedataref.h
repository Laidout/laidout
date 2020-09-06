//
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
#ifndef LSOMEDATAREF_H
#define LSOMEDATAREF_H

#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/somedataref.h>
#include "drawableobject.h"


namespace Laidout {


//------------------------------- LSomeDataRef ---------------------------------------

class LSomeDataRef : public DrawableObject,
				     public LaxInterfaces::SomeDataRef
{
  public:
	LSomeDataRef(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LSomeDataRef();
	virtual const char *whattype() { return "SomeDataRef"; }
	virtual const char *Id();
	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);

	 //from Value:
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};



//------------------------------- LSomeDataRef ---------------------------------------

class LBleedProxy : public LSomeDataRef
{
  public:
	LBleedProxy(LaxInterfaces::SomeData *refobj=NULL);
	virtual const char *whattype() { return "LBleedProxy"; }
};


} //namespace Laidout

#endif


