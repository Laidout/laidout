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

#include <lax/laxutils.h>
#include <lax/language.h>

#include "lengthvalue.h"

#include <lax/debug.h>


using namespace Laxkit;

namespace Laidout {


//----------------------------------- LengthValue -----------------------------------


// internal static unit manager, so as not to clog up global unit manager. do we really need this?
Laxkit::UnitManager LengthValue::unit_manager;


/*! Static constructing function.
 */
LengthValue *LengthValue::Parse(const char *val, int len, const char **endptr)
{
	//TODO: it would be nice to do this with no allocation on error.
	LengthValue *v = new LengthValue(val,len);
	if (v->type == CSS_Error) {
		delete v;
		return nullptr;
	}
	return v;
}

LengthValue::LengthValue()
{
	value    = 0;
	v_cached = 0;
	type     = CSS_Physical;
	units    = 0;
}

LengthValue::LengthValue(double val, Laxkit::Unit unit)
  : LengthValue()
{
	value = val;
	units = unit;
}

LengthValue::LengthValue(double val, Laxkit::Unit unit, Laxkit::CSSName len_type)
{
	value = val;
	units = unit;
	type  = len_type;
}
	
LengthValue::LengthValue(const char *val, int len, const char **end_ptr)
  : LengthValue()
{
	//TODO: properly use len to truncate val if necessary

	if (isblank(val)) return;

	if (len < 0) len = strlen(val);
	char *endptr = nullptr;
	value = strtod(val, &endptr);

	if (endptr != val) {
		while (isspace(*endptr)) endptr++;

		if (*endptr == '%') {
			type = CSS_Percent;
			endptr++;

		} else {
			// parse units
			const char *uptr = endptr;
			while (isalpha(*uptr)) uptr++;
			int _units = ParseUnits(endptr, uptr - endptr);
			if (_units != Laxkit::UNITS_None) {
				units = _units;
				if      (units == Laxkit::UNITS_em)   { type = CSS_em;   }
				else if (units == Laxkit::UNITS_ex)   { type = CSS_ex;   }
				else if (units == Laxkit::UNITS_ch)   { type = CSS_ch;   }
				else if (units == Laxkit::UNITS_vw)   { type = CSS_vw;   }
				else if (units == Laxkit::UNITS_vh)   { type = CSS_vh;   }
				else if (units == Laxkit::UNITS_vmin) { type = CSS_vmin; }
				else if (units == Laxkit::UNITS_vmax) { type = CSS_vmax; }
				else type = CSS_Physical;				
			}
		}
	} else {
		type = CSS_Error;
	}
	if (end_ptr) *end_ptr = endptr;
}


int LengthValue::ParseUnits(const char *str, int len)
{
	if (unit_manager.NumberOfUnits() == 0) {
		Laxkit::CreateDefaultUnits(&unit_manager);

		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_Pixels, .0254/96, 0, _("px"),   _("pixel"), _("pixels"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_em,     1,        0, _("em"),   _("em"),    _("em"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_ex,     1,        0, _("ex"),   _("ex"),    _("ex"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_ch,     1,        0, _("ch"),   _("ch"),    _("ch"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_rem,    1,        0, _("rem"),  _("rem"),   _("rem"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_vw,     1,        0, _("vw"),   _("vw"),    _("vw"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_vh,     1,        0, _("vh"),   _("vh"),    _("vh"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_vmin,   1,        0, _("vmin"), _("vmin"),  _("vmin"));
		unit_manager.AddUnits(Laxkit::UNITS_Length, Laxkit::UNITS_vmax,   1,        0, _("vmax"), _("vmax"),  _("vmax"));
	}

	return unit_manager.UnitId(str, len);
}

Value *LengthValue::duplicate()
{
	LengthValue *dup = new LengthValue(value, units, type);
	return dup;
}

ObjectDef *LengthValue::makeObjectDef()
{
	//calling code responsible for ref
	DBGE("IMPLEMENT ME!!");
	return nullptr;
}

} // namespace Laidout
