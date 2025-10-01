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


#include <lax/misc.h>

#include "../language.h"
#include "../core/stylemanager.h"
#include "pointsetvalue.h"


using namespace Laxkit;


namespace Laidout {


//--------------------------------------- PointSetValue ---------------------------------------

/* \class PointSetValue
 * \brief Adds scripting functions for a Laxkit::PointSet object.
 */

ObjectDef *makePointSetObjectDef();

PointSetValue::PointSetValue()
{}

int PointSetValue::TypeNumber()
{
	static int v = VALUE_MaxBuiltIn + getUniqueNumber();
	return v;
}

int PointSetValue::type()
{
	return TypeNumber();
}

void PointSetValue::dump_out(FILE *f,int indent,int what, Laxkit::DumpContext *context)
{
	PointSet::dump_out(f,indent,what,context);
}

Laxkit::Attribute *PointSetValue::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{
	return PointSet::dump_out_atts(att,what,context);
}

void PointSetValue::dump_in_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context)
{
	PointSet::dump_in_atts(att,what,context);
}

Value *PointSetValue::dereference(int index)
{
	if (index<0 || index >= points.n) return nullptr;
	return new FlatvectorValue(points.e[index]->p);
}

int PointSetValue::getValueStr(char *buffer,int len)
{
    int needed = points.n * 30 * 2;
    if (!buffer || len<needed) return needed;

	Utf8String str = "[", s2;
	for (int c=0; c<points.n; c++) {
		flatpoint p = points.e[c]->p;
		s2.Sprintf("(%.10g,%.10g)%s", p.x, p.y, c == points.n-1 ? "" : ",");
		str.Append(s2);
	}
	str.Append("]");
	strcpy(buffer, str.c_str());
    modified=0;
    return 0;
}

/*! Note, this duplicates point info.
 */
Value *PointSetValue::duplicateValue()
{
	PointSetValue *dup = new PointSetValue();
	dup->CopyFrom(this, 2, 0);
	return dup;
}

ObjectDef *PointSetValue::makeObjectDef()
{
	objectdef = stylemanager.FindDef("PointSet");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef = makePointSetObjectDef();
	}
	return objectdef;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int PointSetValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (isName(function,len, "AddPoint")) {
		int err=0;
		try {
			int i = -1;
			flatpoint p;
			i = pp->findIntOrDouble("angle",-1,&err);
			if (err == 2) throw _("Bad index value.");
			else if (err == 1) i = -1;
			err=0;

			FlatvectorValue *fpv = dynamic_cast<FlatvectorValue*>(pp->find("p"));
			if (fpv) p = fpv->v;
			else throw _("Bad vector!");

			Insert(i, p);

		} catch (const char *str) {
			if (log) log->AddMessage(str,ERROR_Fail);
			err=1;
		}
		 
		if (value_ret) *value_ret = nullptr;
		return err;

	} else if (isName(function,len, "RemovePoint")) {
		int i = -1;
		flatpoint p;
		int err = 0;
		i = pp->findIntOrDouble("angle",-1,&err);
		if (err != 0) throw _("Bad index value.");

		Remove(i);
		if (value_ret) *value_ret = nullptr;
		return 0;

	} else if (isName(function,len, "Flush")) {
		Flush();
		if (value_ret) *value_ret = nullptr;
		return 0;

	// } else if (isName(function,len, "CreateGrid")) {
	// 	if (value_ret) *value_ret = nullptr;
	// 	return 0;

	// } else if (isName(function,len, "CreateHexChunk")) {
	// 	if (value_ret) *value_ret = nullptr;
	// 	return 0;

	// } else if (isName(function,len, "CreateRandomPoints")) {
	// 	if (value_ret) *value_ret = nullptr;
	// 	return 0;

	// } else if (isName(function,len, "CreateRandomRadial")) {
	// 	if (value_ret) *value_ret = nullptr;
	// 	return 0;		
	}

	return 1;
}

//! Contructor for PointSetValue objects.
int NewPointSetObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	Value *v = new PointSetValue();
	*value_ret = v;

	if (!parameters || !parameters->n()) return 0;

//	Value *matrix=parameters->find("matrix");
//	if (matrix) {
//		SetValue *set=dynamic_cast<SetValue*>(matrix);
//		if (set && set->GetNumFields()==6) {
//		}
//	}

	return 0;
}

Value *NewPointSetValue() { return new PointSetValue(); }

//! Create (if necessary) a new ObjectDef with Affine characteristics.
ObjectDef *makePointSetObjectDef()
{
	ObjectDef *sd = stylemanager.FindDef("PointSet");
	if (sd) {
		sd->inc_count();
		return sd;
	}

	sd = new ObjectDef(NULL,"PointSet",
			_("Point Set"),
			_("Set of two dimensional points."),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NewPointSetValue, NewPointSetObject);


	sd->pushFunction("Flush", _("Flush"), _("Remove all points."),
					 NULL,
					 NULL);

	sd->pushFunction("AddPoint", _("Add point"), _("Add point with optional index"),
					 NULL,
					 "p",_("p"),_("Point"),"Flatvector", NULL,NULL,
					 "i",_("Index"),_("Index starting at 0"),"int", NULL, "-1",
					 NULL);

	sd->pushFunction("RemovePoint", _("Remove point"), _("Remove i'th point"),
					 NULL,
					 "i",_("Index"),_("Index starting at 0"),"int", NULL, nullptr,
					 NULL);


	// sd->pushFunction("CreateRandomPoints", _("Random points in rect"), nullptr,
	// 				 NULL, //evaluator
	// 				 "num",_("Num"),_("Number of points to make"),"number", NULL,NULL,
	// 				 "seed",_("Seed"),_("Random seed"),"number", NULL,NULL,
	// 				 "x",_("X"),_("Origin x"),"number", NULL,NULL,
	// 				 "y",_("Y"),_("Origin y"),"number", NULL,NULL,
	// 				 "w",_("Width"),_("Width"),"number", NULL,NULL,
	// 				 "h",_("Height"),_("Height"),"number", NULL,NULL,
	// 				 NULL);

	// sd->pushFunction("CreateRandomRadial", _("Random points in circle"), _("Centered at x,y"),
	// 				 NULL, //evaluator
	// 				 "num",   _("Num points"),nullptr,"number", NULL,NULL,
	// 				 "seed",  _("Seed"),      nullptr,"number", NULL,NULL,
	// 				 "x",     _("Center x"),  nullptr,"number", NULL,NULL,
	// 				 "y",     _("Center y"),  nullptr,"number", NULL,NULL,
	// 				 "radius",_("Radius"),    nullptr,"number", NULL,NULL,
	// 				 NULL);


	// sd->pushFunction("CreateGrid", _("Grid of points"), nullptr,
	// 				 NULL,
	// 				 "x",_("X"),_("Origin x"),"number", NULL,NULL,
	// 				 "y",_("Y"),_("Origin y"),"number", NULL,NULL,
	// 				 "w",_("Width"),_("Width"),"number", NULL,NULL,
	// 				 "h",_("Height"),_("Height"),"number", NULL,NULL,
	// 				 "nx",_("Num X"),_("Number of divisions in the x direction"),"number", NULL,NULL,
	// 				 "ny",_("Num Y"),_("Number of divisions in the y direction"),"number", NULL,NULL,
	// 				 NULL);


	// sd->pushFunction("CreateHexChunk", _("Triangular grid in hexagon"), _(""),
	// 				 NULL,
	// 				 "side",_("Edge length"),   _("Containing hexagon sides this long"),"number", NULL,NULL,
	// 				 "num", _("Points on edge"),_("Divide edges by this many"),"number", NULL,NULL,
	// 				 NULL);



	stylemanager.AddObjectDef(sd, 0);
	return sd;
}



} // namespace Laidout

