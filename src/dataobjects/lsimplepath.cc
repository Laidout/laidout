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
// Copyright (C) 2016 by Tom Lechner
//

#include "lsimplepath.h"
#include "datafactory.h"
#include "group.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include "../calculator/shortcuttodef.h"
#include "affinevalue.h"

#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {




/*! \class LSimplePathData 
 * \brief Subclassing LaxInterfaces::SimplePathData
 */



LSimplePathData::LSimplePathData(LaxInterfaces::SomeData *refobj)
{
	Id();
}

LSimplePathData::~LSimplePathData()
{
}

void LSimplePathData::FindBBox()
{
	SimplePathData::FindBBox();
}

void LSimplePathData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	SimplePathData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LSimplePathData::pointin(flatpoint pp,int pin)
{
	return SimplePathData::pointin(pp,pin);
}

void LSimplePathData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
	// char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	// DrawableObject::dump_out(f,indent,what,context);
	// fprintf(f,"%sconfig\n",spc);
	// SimplePathData::dump_out(f,indent+2,what,context);
}


Laxkit::Attribute *LSimplePathData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	Laxkit::Attribute *att2 = att->pushSubAtt("config");
	SimplePathData::dump_out_atts(att2, what,context);
	return att;
}


void LSimplePathData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			SimplePathData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) SimplePathData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LSimplePathData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LSimplePathData*>(dup)) return nullptr; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("SimplePathData"));
	SimplePathData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

LaxInterfaces::SomeData *LSimplePathData::EquivalentObject()
{
//	PathsData *paths=dynamic_cast<PathsData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
//	paths->line(width_SimplePath, LAXCAP_Round,LAXJOIN_Round, &color_SimplePath->screen);
//	paths->fill(nullptr);
//	paths->Id("SimplePath");
//
//	***
//
//	return paths;

	return nullptr;
}


//------- SimplePathData.Value functions:

Value *LSimplePathData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("SimplePathData"));
	SimplePathData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

ObjectDef *LSimplePathData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("SimplePathData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"SimplePathData",
            _("SimplePath Data"),
            _("SimplePath triangles and SimplePath regions"),
            "class",
            nullptr,nullptr);

//	sd->pushFunction("FlipColors",_("Flip Colors"),_("Flip the order of colors"), nullptr,
//					 nullptr);
//
//	sd->pushVariable("p1", _("p1"),         _("The starting point"), "real",0, nullptr,0);
//	sd->pushVariable("p2", _("p2"),         _("The ending point"),   "real",0, nullptr,0);
//	sd->pushVariable("r1", _("Distance 1"), _("Radius or distance"), "real",0, nullptr,0);
//	sd->pushVariable("r2", _("Distance 2"), _("Radius or distance"), "real",0, nullptr,0);
	//sd->pushVariable("angle",_("Angle"),_("Angle gradient exists at"), nullptr,0);

	//sd->pushVariable("stops",  _("Stops"),  _("The set of color positions"), nullptr,0);

	return sd;
}

Value *LSimplePathData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "p1")) {
//		return new DoubleValue(p1);
//	}
//
//	if (extequal(extstring,len, "p2")) {
//		return new DoubleValue(p2);
//	}
//
//	if (extequal(extstring,len, "r1")) {
//		return new DoubleValue(r1);
//	}
//
//	if (extequal(extstring,len, "r2")) {
//		return new DoubleValue(r2);
//	}

//	if (extequal(extstring,len, "colors")) {
//		return new DoubleValue(maxy);
//	}

	return nullptr;
}

int LSimplePathData::assign(FieldExtPlace *ext,Value *v)
{
//	if (ext && ext->n()==1) {
//		const char *str=ext->e(0);
//		int isnum;
//		double d;
//		if (str) {
//			if (!strcmp(str,"p1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p1=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"p2")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p2=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"r1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				r1=d;
//				FindBBox();
//				return 1;
//
//			} else if (!strcmp(str,"r2")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				r2=d;
//				FindBBox();
//				return 1;
//
//			}
//		}
//	}

	AffineValue affine(m());
	int status=affine.assign(ext,v);
	if (status==1) {
		m(affine.m());
		return 1;
	}
	return 0;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int LSimplePathData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
//	if (len==10 && !strncmp(func,"FlipColors",10)) {
//		//if (parameters && parameters->n()) return -1; *** if pp has this, then huh!
//
//		FlipColors();
//		return 0;
//	}

	AffineValue v(m());
	int status=v.Evaluate(func,len,context,parameters,settings,value_ret,log);
	if (status==0) {
		m(v.m());
		return 0;
	}

	return status;
}



//------------------------------- LSimplePathInterface --------------------------------
/*! \class LSimplePathInterface
 * \brief add on a little custom behavior.
 */


LSimplePathInterface::LSimplePathInterface(int nid,Laxkit::Displayer *ndp)
  : SimplePathInterface(nullptr, nid,ndp)
{
}


LaxInterfaces::anInterface *LSimplePathInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==nullptr) dup=dynamic_cast<anInterface *>(new LSimplePathInterface(id,nullptr));
	else if (!dynamic_cast<LSimplePathInterface *>(dup)) return nullptr;

	return SimplePathInterface::duplicate(dup);
}


//! Returns this, but count is incremented.
Value *LSimplePathInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LSimplePathInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("SimplePathInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(nullptr,"SimplePathInterface",
            _("SimplePath Interface"),
            _("SimplePath Interface"),
            "class",
            nullptr,nullptr);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}


///*!
// * Return
// *  0 for success, value optionally returned.
// * -1 for no value returned due to incompatible parameters, which aids in function overloading.
// *  1 for parameters ok, but there was somehow an error, so no value returned.
// */
//int LSimplePathInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==nullptr, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int LSimplePathInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LSimplePathInterface::dereference(const char *extstring, int len)
{
	return nullptr;
}

void LSimplePathInterface::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	anInterface::dump_out(f,indent,what,context);
}

void LSimplePathInterface::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	anInterface::dump_in_atts(att,flag,context);
}

Laxkit::Attribute *LSimplePathInterface::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	return anInterface::dump_out_atts(att, what, context);
}

} //namespace Laidout

