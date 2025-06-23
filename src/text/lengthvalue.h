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
// Copyright (C) 2025-present by Tom Lechner
//
#ifndef _LO_LENGTHVALUE_H
#define _LO_LENGTHVALUE_H


#include <lax/cssutils.h>
#include <lax/units.h>

#include "../calculator/values.h"


namespace Laidout {

//----------------------------------- LengthValue -----------------------------------

class LengthValue : public Value
{
  public:
	double value;
	double v_cached; // cached context dependent computed absolute value. This will be set from
					 // outside LengthValue, such as when computing a StreamCache.

	Laxkit::CSSName type;

	static Laxkit::UnitManager unit_manager;
	Laxkit::Unit units;

	LengthValue(); 
	LengthValue(double val, Laxkit::Unit unit, Laxkit::CSSName len_type);
	LengthValue(double val, Laxkit::Unit unit);
	LengthValue(const char *val, int len=-1, const char **endptr=nullptr);

	int ParseUnits(const char *str, int len);

	//Value overrides:
	virtual Value *duplicate();
 	virtual ObjectDef *makeObjectDef(); //calling code responsible for ref

	static LengthValue *Parse(const char *val, int len = -1, const char **endptr = nullptr);
};

} // namespace Laidout

#endif
