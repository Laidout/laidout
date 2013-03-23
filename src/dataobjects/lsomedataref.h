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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef LSOMEDATAREF_H
#define LSOMEDATAREF_H

#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/somedataref.h>
#include "drawableobject.h"

//------------------------------- LSomeDataRef ---------------------------------------


namespace Laidout {


class LSomeDataRef : public DrawableObject,
				     public LaxInterfaces::SomeDataRef,
				     //public FunctionEvaluator,
				     public Value
{
  public:
	LSomeDataRef(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LSomeDataRef();
	virtual const char *whattype() { return "SomeDataRef"; }
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);


	virtual int type() { return VALUE_Fields; }
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
//	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log);
};




} //namespace Laidout

#endif


