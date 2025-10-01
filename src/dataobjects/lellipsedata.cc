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
// Copyright (C) 2021 by Tom Lechner
//

#include "lellipsedata.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include <lax/interfaces/pathinterface.h>

#include <lax/debug.h>

using namespace Laxkit;


namespace Laidout {


//------------------------------- LEllipseData ---------------------------------------
/*! \class LEllipseData
 * \brief Redefined LaxInterfaces::EllipseData.
 */

LEllipseData::LEllipseData(LaxInterfaces::SomeData *refobj)
{
	//importer=NULL;
	//sourcefile=NULL;
}

LEllipseData::~LEllipseData()
{
	//if (importer) importer->dec_count();
	//if (sourcefile) delete[] sourcefile;
}

	
void LEllipseData::FindBBox()
{
	EllipseData::FindBBox();
}

void LEllipseData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	EllipseData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LEllipseData::pointin(flatpoint pp,int pin)
{
	return EllipseData::pointin(pp,pin);
}

void LEllipseData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Laxkit::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

Laxkit::Attribute *LEllipseData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	Laxkit::Attribute *att2 = att->pushSubAtt("config");
	EllipseData::dump_out_atts(att2, what,context);
	return att;
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LEllipseData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			EllipseData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) EllipseData::dump_in_atts(att,flag,context);
}

Value *LEllipseData::duplicateValue()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("EllipseData"));
	EllipseData::duplicateData(dup);
	DrawableObject::duplicateData(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LEllipseData::duplicateData(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LEllipseData*>(dup)) return nullptr; //wrong type for reference object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("EllipseData"));
	EllipseData::duplicateData(dup);
	DrawableObject::duplicateData(dup);
	return dup;
}

Value *NewLEllipseData() { return new LEllipseData; }

ObjectDef *LEllipseData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("EllipseData");
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
			"EllipseData",
            _("EllipseData"),
            _("An ellipse"),
            NewLEllipseData,NULL);

	sd->pushVariable("start",  _("Start angle"),  _("in radians"),    "real",0, NULL,0);
	sd->pushVariable("end", _("End angle"), _("in radians"),  "real",0,   NULL,0);
	sd->pushVariable("wedge",_("Wedge"),_("Wedge type: wedge, chord, open"), "string",0,   NULL,0);
	sd->pushVariable("inner_r",_("Inner Radius"),_("As a fraction of normal radius."), "real",0,   NULL,0);
	sd->pushVariable("a",_("a"),_("X radius"), "real",0,   NULL,0);
	sd->pushVariable("b",_("b"),_("Y radius"), "real",0,   NULL,0);
	sd->pushVariable("center",_("Center"),nullptr, "Vector2",0,   NULL,0);
	sd->pushVariable("x",_("X axis"),nullptr, "Vector2",0,   NULL,0);
	sd->pushVariable("y",_("Y axis"),nullptr, "Vector2",0,   NULL,0);

	stylemanager.AddObjectDef(sd, 0);
	return sd;
}

Value *LEllipseData::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "start")) return new DoubleValue(start);
	if (extequal(extstring,len, "end")) return new DoubleValue(end);
 	if (extequal(extstring,len, "inner_r")) return new DoubleValue(inner_r);
 	if (extequal(extstring,len, "a")) return new DoubleValue(a);
 	if (extequal(extstring,len, "b")) return new DoubleValue(b);

 	if (extequal(extstring,len, "wedge")) return new StringValue(
 				wedge_type == ELLIPSE_Open ? "open" : 
 				(wedge_type == ELLIPSE_Chord ? "chord" : "wedge")
			);

 	if (extequal(extstring,len, "center")) return new FlatvectorValue(center);
 	if (extequal(extstring,len, "x")) return new FlatvectorValue(x);
 	if (extequal(extstring,len, "y")) return new FlatvectorValue(y);


	return DrawableObject::dereference(extstring, len);
}

int LEllipseData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
			if (!strcmp(str,"wedge")) {
				StringValue *sv = dynamic_cast<StringValue*>(v);
				if (!sv) return -1;

				if (strEquals(sv->str, "wedge", true)) wedge_type = ELLIPSE_Wedge;
				else if (strEquals(sv->str, "chord", true)) wedge_type = ELLIPSE_Chord;
				else if (strEquals(sv->str, "open", true)) wedge_type = ELLIPSE_Open;
				else return -1;
				return 0;

			} else if (!strcmp(str,"center")) {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(v);
				if (!fv) return -1;
				center = fv->v;
				return 0;

			} else if (!strcmp(str,"x")) {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(v);
				if (!fv) return -1;
				x = fv->v;
				return 0;

			} else if (!strcmp(str,"y")) {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(v);
				if (!fv) return -1;
				y = fv->v;
				return 0;

			} else {
				double d = 0;
				if (!isNumberType(v, &d)) return -1;

				if (!strcmp(str,"inner_r")) inner_r = d;
				else if (!strcmp(str,"a")) a = d;
				else if (!strcmp(str,"b")) b = d;
				else if (!strcmp(str,"start")) start = d;
				else if (!strcmp(str,"end")) end = d;
			}
		}
	}

	return DrawableObject::assign(ext,v);
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int LEllipseData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
}

LaxInterfaces::SomeData *LEllipseData::EquivalentObject()
{
	flatpoint pts[24];
	int n = GetPath(3, pts);
	LaxInterfaces::PathsData *pathsdata = new LaxInterfaces::PathsData();
	pathsdata->append(pts, n);
	pathsdata->InstallLineStyle(linestyle);
	pathsdata->InstallFillStyle(fillstyle);
	pathsdata->m(m());
	return pathsdata;
}

} //namespace Laidout

