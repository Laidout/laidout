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
#ifndef IMAGEVALUE_H
#define IMAGEVALUE_H

#include <lax/laximages.h>

#include "../calculator/values.h"


namespace Laidout {


//------------------------------------ ImageValue ------------------------------------------------
ObjectDef *makeImageValueDef();
class ImageValue : virtual public Value, virtual public FunctionEvaluator
{
  public:
	static int TypeNumber();
	Laxkit::LaxImage *image;

	ImageValue(Laxkit::LaxImage *img = nullptr, bool absorb = false);
	virtual ~ImageValue();
	virtual int type();
    virtual const char *whattype() { return "ImageValue"; }
	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicateValue();
	virtual Value *dereference(int index);
	//virtual int assign(FieldExtPlace *ext,Value *v);
	virtual void SetImage(Laxkit::LaxImage *newimage);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log);
};


} //namespace Laidout

#endif


