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


#include <lax/language.h>
#include <lax/misc.h>
#include "curvevalue.h"


//template implementation
#include <lax/lists.cc>


using namespace Laxkit;


namespace Laidout {


/*! \class CurveValue
 * Value container for CurveInfo objects.
 */

int NewCurveObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
    Value *v = new CurveValue();
    *value_ret = v;

    if (!parameters || !parameters->n()) return 0;

//  Value *minx = parameters->find("minx");
//  if (minx) {
//		***
//  }

    return 0;
}

CurveValue::CurveValue()
{
}

CurveValue::~CurveValue()
{
}

int CurveValue::curve_value_type = VALUE_MaxBuiltIn + getUniqueNumber();

int CurveValue::type()
{ return curve_value_type; }


ObjectDef *CurveValue::makeObjectDef()
{
    objectdef = stylemanager.FindDef("CurveValue");

    if (objectdef) {
		objectdef->inc_count();
		return objectdef;
	}

	objectdef = new ObjectDef(NULL,"CurveValue",
			_("Curve"),
			_("A curve f(x)"),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NULL, NewCurveObject);


	objectdef->pushFunction("f", _("f"), _("Get value for x"),
					 NULL, //evaluator
					 "x",_("X"),_("The x coordinate"),"number", NULL,NULL,
					 NULL);

	ObjectDef *def = new ObjectDef("CurveType", _("Curve type"), NULL,NULL,"enum", 0);
	objectdef->push(def, 1);

	 //1 argument
	def->pushEnumValue("Linear"     ,_("Linear"),     _("Linear"),     0 );
	def->pushEnumValue("Autosmooth" ,_("Autosmooth"), _("Autosmooth"), 1 );
	def->pushEnumValue("Bezier"     ,_("Bezier"),     _("Bezier"),     2 );

	stylemanager.AddObjectDef(objectdef,0);

    return objectdef;

}

int CurveValue::getValueStr(char *buffer,int len)
{
	// ***
	return Value::getValueStr(buffer,len);
}

Value *CurveValue::duplicate()
{
	CurveValue *v = new CurveValue();

	v->SetTitle(title);
	v->SetXBounds(xmin,xmax, xlabel,false);
	v->SetYBounds(ymin,ymax, ylabel,false);
	v->curvetype = curvetype;
	v->wrap = wrap;
	v->SetDataRaw(points.e, points.n);

	return v;
}

Value *CurveValue::dereference(int index)
{
	//double xmin, xmax;
    //double ymin, ymax;
    //char *xlabel, *ylabel;
    //char *title;
    //CurveTypes curvetype;
    //bool wrap;
    //NumStack<flatpoint> points;

	return NULL;
}

int CurveValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
                         Value **value_ret, Laxkit::ErrorLog *log)
{
	if (isName(func,len, "f")) {
		int err=0;
		double x = pp->findIntOrDouble("x",-1, &err);
		*value_ret = new DoubleValue(f(x));
		return 0;
	}

	return 1;
}

void CurveValue::dump_in_atts(LaxFiles::Attribute*att, int what, LaxFiles::DumpContext *context)
{
	Laxkit::CurveInfo::dump_in_atts(att, what, context);
}



} //namespace Laidout

