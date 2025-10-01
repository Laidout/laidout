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
// Copyright (C) 2018 by Tom Lechner
//
#ifndef BBOXVALUE_H
#define BBOXVALUE_H


#include "../calculator/values.h"
#include <lax/doublebbox.h>

namespace Laidout {

//------------------------------------ BBoxValue ------------------------------------------------
ObjectDef *makeBBoxObjectDef();
class BBoxValue : virtual public Value, virtual public Laxkit::DoubleBBox, virtual public FunctionEvaluator
{
  public:
	BBoxValue();
	BBoxValue(const Laxkit::DoubleBBox &box);
	BBoxValue(double mix,double max,double miy,double may);
    virtual const char *whattype() { return "BBoxValue"; }
	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicateValue();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log);
};



} //namespace Laidout

#endif

