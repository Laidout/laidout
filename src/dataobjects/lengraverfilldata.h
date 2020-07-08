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
// Copyright (C) 2012-2013 by Tom Lechner
//
#ifndef LENGRAVERFILLDATA_H
#define LENGRAVERFILLDATA_H

#include <lax/interfaces/engraverfillinterface.h>
#include "drawableobject.h"



namespace Laidout {


//------------------------------- LEngraverFillData ---------------------------------------
class LEngraverFillData : public DrawableObject,
				   public LaxInterfaces::EngraverFillData
{
  public:
	LEngraverFillData();
	virtual ~LEngraverFillData();
	virtual const char *whattype() { return "EngraverFillData"; }
    virtual const char *Id(); 
	virtual const char *Id(const char *newid);

	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);
	virtual LaxInterfaces::SomeData *EquivalentObject();


	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};



} //namespace Laidout

#endif


