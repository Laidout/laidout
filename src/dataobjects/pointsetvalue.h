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
// Copyright (C) 2020 by Tom Lechner
//
#ifndef POINTSETVALUE_H
#define POINTSETVALUE_H

#include <lax/transformmath.h>
#include <lax/pointset.h>

#include "../calculator/values.h"


namespace Laidout {

//------------------------------------ PointSetValue ------------------------------------------------
ObjectDef *makeAffineObjectDef();
class PointSetValue : virtual public Value, virtual public Laxkit::PointSet, virtual public FunctionEvaluator
{
  public:
	static int TypeNumber();

	PointSetValue();
	virtual int type();
    virtual const char *whattype() { return "PointSetValue"; }

	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicateValue();
	virtual anObject *duplicate() { return duplicateValue(); }
	virtual Value *dereference(int index);
	//virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log);

	// from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what, Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
};


} //namespace Laidout

#endif
