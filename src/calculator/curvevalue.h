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
// Copyright (C) 2018 by Tom Lechner
//
#ifndef CURVEVALUE_H
#define CURVEVALUE_H


#include <lax/curveinfo.h>
#include "../stylemanager.h"
#include "values.h"

namespace Laidout { 


//------------------------ CurveValue ------------------------




class CurveValue : virtual public Value, virtual public Laxkit::CurveInfo, virtual public FunctionEvaluator
{
	static int curve_value_type;

  public:
	CurveValue();
	virtual ~CurveValue();
	virtual const char *whattype() { return "CurveValue"; }
	virtual int type();
	virtual void dump_in_atts(LaxFiles::Attribute*, int, LaxFiles::DumpContext*);
	virtual ObjectDef *makeObjectDef();
    virtual int getValueStr(char *buffer,int len);
    virtual Value *duplicate();
    virtual Value *dereference(int index);
    //virtual int assign(FieldExtPlace *ext,Value *v);
    virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
                         Value **value_ret, Laxkit::ErrorLog *log);
};



} //namespace Laidout


#endif


