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
// Copyright (C) 2025 by Tom Lechner
//

#include "lroundedrectdata.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include <lax/interfaces/pathinterface.h>

#include <lax/debug.h>


using namespace Laxkit;


namespace Laidout {


//------------------------------- LRoundedRectData ---------------------------------------
/*! \class LRoundedRectData
 * \brief Redefined LaxInterfaces::RoundedRectData.
 */

LRoundedRectData::LRoundedRectData(LaxInterfaces::SomeData *refobj)
{}

LRoundedRectData::~LRoundedRectData()
{}

	
void LRoundedRectData::FindBBox()
{
	RoundedRectData::FindBBox();
}

void LRoundedRectData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	RoundedRectData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LRoundedRectData::pointin(flatpoint pp,int pin)
{
	return RoundedRectData::pointin(pp,pin);
}

void LRoundedRectData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Laxkit::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

Laxkit::Attribute *LRoundedRectData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	Laxkit::Attribute *att2 = att->pushSubAtt("config");
	RoundedRectData::dump_out_atts(att2, what,context);
	return att;
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LRoundedRectData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			RoundedRectData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) RoundedRectData::dump_in_atts(att,flag,context);
}

Value *LRoundedRectData::duplicateValue()
{
	SomeData *dup = dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("RoundedRectData"));
	RoundedRectData::duplicateData(dup);
	DrawableObject::duplicateData(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LRoundedRectData::duplicateData(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LRoundedRectData*>(dup)) return nullptr; //wrong type for reference object!
	if (!dup) dup = dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("RoundedRectData"));
	RoundedRectData::duplicateData(dup);
	DrawableObject::duplicateData(dup);
	return dup;
}

Value *NewLRoundedRectData() { return new LRoundedRectData; }

ObjectDef *LRoundedRectData::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("RoundedRectData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *gdef = stylemanager.FindDef("Group");
	if (!gdef) {
		Group g;
		gdef = g.GetObjectDef();
	}
	sd = new ObjectDef(gdef,
			"RoundedRectData",
            _("RoundedRectData"),
            _("An rounded rectangle"),
            NewLRoundedRectData,nullptr);

	sd->pushVariable("is_square",_("Edit as square"),_("Hint to make width and height the same during editing"), "boolean",0,   nullptr,0);
	sd->pushVariable("width", _("Width"), _("Full base width, ignoring custom corner additions"),  "real",0,   nullptr,0);
	sd->pushVariable("height",_("Height"),_("Full base height, ignoring custom corner additions"), "real",0,   nullptr,0);

	sd->pushVariable("x_align",_("Horizontal alignment"),_("0 full left, 50 centered, 100 full right"), "real",0,   nullptr,0);
	sd->pushVariable("y_align",_("Vertial alignment"),   _("0 full bottom, 50 centered, 100 full top"), "real",0,   nullptr,0);
	sd->pushVariable("round",_("Corner roundedness"),_("Array of 8 corner radius values, clockwise from upper left."), "Array of real",0,   nullptr,0);
	// int round_style; // 0 = ellipsoidal, 1 = squircle, 2 = custom bevel
	// enum RoundType { Symmetric=0, Asymmetric=1, Independent=2 };
	// int round_type; // symmetric for all, different x/y but same for all, different for all
	// linestyle
	// fillstyle

	stylemanager.AddObjectDef(sd, 0);
	return sd;
}

Value *LRoundedRectData::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "is_square")) { return new BooleanValue(is_square); }
	if (extequal(extstring,len, "width")) { return new DoubleValue(width); }
	if (extequal(extstring,len, "height")) { return new DoubleValue(height); }
	if (extequal(extstring,len, "x_align")) { return new DoubleValue(x_align); }
	if (extequal(extstring,len, "y_align")) { return new DoubleValue(y_align); }
	// int round_style; // 0 = ellipsoidal, 1 = squircle, 2 = custom bevel
	// enum RoundType { Symmetric=0, Asymmetric=1, Independent=2 };
	// int round_type; // symmetric for all, different x/y but same for all, different for all
	// linestyle
	// fillstyle
	
	return DrawableObject::dereference(extstring, len);
}

int LRoundedRectData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n() == 1) {
		const char *str = ext->e(0);
		if (str) {
			double d = 0;
			if (isNumberType(v, &d)) {
				if (!strcmp(str,"width")) { if (d >= 0) width = d; }
				else if (!strcmp(str,"height")) { if (d >= 0) height = d; }
				else if (!strcmp(str,"x_align")) x_align = d;
				else if (!strcmp(str,"y_align")) y_align = d;
				else if (!strcmp(str,"is_square")) is_square = (d != 0);
				else return -1;
				return 0;
			}
		}
	}

	return DrawableObject::assign(ext,v);
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int LRoundedRectData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
}

LaxInterfaces::SomeData *LRoundedRectData::EquivalentObject()
{
	flatpoint pts[24]; //todo: subject to change when custom corners implemented
	int n = GetPath(pts);
	LaxInterfaces::PathsData *pathsdata = new LaxInterfaces::PathsData();
	pathsdata->append(pts, n);
	pathsdata->InstallLineStyle(linestyle);
	pathsdata->InstallFillStyle(fillstyle);
	pathsdata->m(m());
	return pathsdata;
}


} //namespace Laidout

