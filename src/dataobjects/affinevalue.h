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
#ifndef AFFINEVALUE_H
#define AFFINEVALUE_H

#include <lax/transformmath.h>

#include "../calculator/values.h"


namespace Laidout {

//------------------------------------ AffineValue ------------------------------------------------
ObjectDef *makeAffineObjectDef();
class AffineValue : virtual public Value, virtual public Laxkit::Affine, virtual public FunctionEvaluator
{
  public:
	static int TypeNumber();

	AffineValue();
	AffineValue(const double *m);
	virtual int type();
    virtual const char *whattype() { return "AffineValue"; }
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
