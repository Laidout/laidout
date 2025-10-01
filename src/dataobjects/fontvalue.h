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
#ifndef FONTVALUE_H
#define FONTVALUE_H

#include <lax/fontmanager.h>

#include "../calculator/values.h"


namespace Laidout {


//------------------------------------ FontValue ------------------------------------------------

class FontValue : virtual public Value, virtual public FunctionEvaluator
{
  public:
	Laxkit::LaxFont *font;

	FontValue(Laxkit::LaxFont *newobj = nullptr, int absorb = false);
	virtual ~FontValue();
	virtual int type() { return VALUE_Font; }
    virtual const char *whattype() { return "FontValue"; }
	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicateValue();
	// virtual Value *dereference(int index);
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual void Set(Laxkit::LaxFont *newobj, int absorb);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, Laxkit::ErrorLog *log);

	static Value *NewFontValue();
	// static int NewFontObject(ValueHash *context, ValueHash *parameters, Value **value_ret, Laxkit::ErrorLog &log);
};


} //namespace Laidout

#endif


