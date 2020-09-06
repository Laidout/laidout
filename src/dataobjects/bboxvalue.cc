//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2018 by Tom Lechner
//


#include "bboxvalue.h"
#include "../core/stylemanager.h"
#include "../language.h"


using namespace Laxkit;

namespace Laidout {


//--------------------------------------- BBoxValue ---------------------------------------

/* \class BBoxValue
 * \brief Adds scripting functions for a Laxkit::DoubleBBox object.
 */

BBoxValue::BBoxValue()
{}

BBoxValue::BBoxValue(const DoubleBBox &box)
  : DoubleBBox(box)
{}

BBoxValue::BBoxValue(double mix,double max,double miy,double may)
  : DoubleBBox(mix,max,miy,may)
{}

Value *BBoxValue::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "minx")) return new DoubleValue(minx);
	if (extequal(extstring,len, "maxx")) return new DoubleValue(maxx);
	if (extequal(extstring,len, "miny")) return new DoubleValue(miny);
	if (extequal(extstring,len, "maxy")) return new DoubleValue(maxy);

	if (extequal(extstring,len, "x"   )) return new DoubleValue(minx);
	if (extequal(extstring,len, "y"   )) return new DoubleValue(miny);
	if (extequal(extstring,len, "width")) return new DoubleValue(maxx-minx);
	if (extequal(extstring,len, "height")) return new DoubleValue(maxy-miny);
	return NULL;
}

int BBoxValue::assign(FieldExtPlace *ext,Value *v)
{
	if (ext->n()!=1) return -1;
	const char *str=ext->e(0);

	int isnum=0;
    double d=getNumberValue(v,&isnum);
    if (!isnum) return 0;

	if (!strcmp(str,"minx")) minx=d;
	else if (!strcmp(str,"maxx")) maxx=d;
	else if (!strcmp(str,"miny")) miny=d;
	else if (!strcmp(str,"maxy")) maxy=d;
	else if (!strcmp(str,"x")) { maxx+=d-minx; minx=d; }
	else if (!strcmp(str,"y")) { maxy+=d-miny; miny=d; }
	else if (!strcmp(str,"width")) maxx=minx+d;
	else if (!strcmp(str,"height")) maxy=miny+d;
	else return -1;

	return 1;
}

//! Return something like [[.5,.5], [1,0]], which is [min,max] for [x,y].
int BBoxValue::getValueStr(char *buffer,int len)
{
    int needed=4*30;
    if (!buffer || len<needed) return needed;

	sprintf(buffer,"[[%.10g,%.10g],[%.10g,%.10g]]",minx,maxx,miny,maxy);
    modified=0;
    return 0;
}

Value *BBoxValue::duplicate()
{
	BBoxValue *dup=new BBoxValue(minx,maxx,miny,maxy);
	return dup;
}

ObjectDef *BBoxValue::makeObjectDef()
{
	objectdef=stylemanager.FindDef("BBox");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef=makeBBoxObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

//! Constructor for BBoxValue objects.
int NewBBoxObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	BBoxValue *v=new BBoxValue();
	*value_ret=v;

	if (!parameters || !parameters->n()) return 0;

	int err=0;
	double d=parameters->findIntOrDouble("maxx",-1,&err);
	if (err==0) v->maxx=d;

	d=parameters->findIntOrDouble("minx",-1,&err);
	if (err==0) v->minx=d;

	d=parameters->findIntOrDouble("maxy",-1,&err);
	if (err==0) v->maxy=d;

	d=parameters->findIntOrDouble("miny",-1,&err);
	if (err==0) v->miny=d;

	d=parameters->findIntOrDouble("y",-1,&err);
	if (err==0) v->miny=d;

	d=parameters->findIntOrDouble("x",-1,&err);
	if (err==0) v->minx=d;

	d=parameters->findIntOrDouble("width",-1,&err);
	if (err==0) v->maxx=v->minx+d;

	d=parameters->findIntOrDouble("height",-1,&err);
	if (err==0) v->maxy=v->miny+d;

	return 0;
}

Value *NewBBoxValue() { return new BBoxValue; }

//! Create a new ObjectDef with BBox characteristics. Always creates new one, does not search for BBox globally.
ObjectDef *makeBBoxObjectDef()
{
	ObjectDef *sd = stylemanager.FindDef("BBox");
	if (sd) {
		sd->inc_count();
		return sd;
	}

	sd = new ObjectDef(NULL,"BBox",
			_("BBox"),
			_("Bounding box"),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NewBBoxValue, NewBBoxObject); //newfunc, objectfunc


	 //Contstructor
	sd->pushFunction("BBox", _("Bounding Box"), _("Bounding Box"),
					 NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	sd->push("minx",_("Minx"),_("Minimium x"),"real",NULL,NULL,0,0);
	sd->push("maxx",_("Maxx"),_("Maximium x"),"real",NULL,NULL,0,0);
	sd->push("miny",_("Miny"),_("Minimium y"),"real",NULL,NULL,0,0);
	sd->push("maxy",_("Maxy"),_("Maximium y"),"real",NULL,NULL,0,0);

	sd->pushFunction("ClearBBox", _("Clear bounds"), _("Clear bounds."),
					 NULL, //evaluator
					 NULL);

	sd->pushFunction("IsValid", _("Is Valid"), _("True if bounds are valid, meaning max values are greater than min values."),
					 NULL, //evaluator
					 NULL);


	sd->pushFunction("Add", _("Add To Bounds"), _("Add a point to bounds"),
					 NULL,
					 "x",_("x"),_("An x coordinate"),"real", NULL,NULL,
					 "y",_("y"),_("A y coordinate"), "real""real", NULL,NULL,
					 "p",_("p"),_("A point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("AddBox", _("Add box"), _("Add another bbox to bounds, so that old and new bounds contain both."),
					 NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);


	sd->pushFunction("Contains", _("Contains"), _("True if bounds contain point (inside or right on edge)."),
					 NULL,
					 "x",_("x"),_("An x coordinate"),"real", NULL,NULL,
					 "y",_("y"),_("A y coordinate"), "real", NULL,NULL,
					 "p",_("p"),_("A point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("Intersects", _("Intersects"), _("Return whether a box intersects."),
					NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	sd->pushFunction("Intersection", _("Intersection"), _("Return a new box that is the intersection with current."),
					 NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	return sd;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int BBoxValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (isName(function,len,"ClearBBox")) {
		ClearBBox();		 
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (isName(function,len,"Add")) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		addtobounds(p);
		if (value_ret) *value_ret=NULL;
		return 0;

	} if (isName(function,len,"x")) {
		*value_ret=new DoubleValue(minx);
		return 0;

	} if (isName(function,len,"y")) {
		*value_ret=new DoubleValue(miny);
		return 0;

	} if (isName(function,len,"width")) {
		*value_ret=new DoubleValue(maxx-minx);
		return 0;

	} if (isName(function,len,"height")) {
		*value_ret=new DoubleValue(maxy-miny);
		return 0;

	} if (isName(function,len,"IsValid")) {
		*value_ret=new BooleanValue(validbounds());
		return 0;

	} if (isName(function,len,"Contains")) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		*value_ret=new BooleanValue(boxcontains(p.x,p.y));
		return 0;

	} if (isName(function,len,"AddBox")) {
		DoubleBBox box;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) box=*dynamic_cast<DoubleBBox*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box.minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box.maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box.miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box.maxy=d;
		addtobounds(&box);

	} if (isName(function,len,"Intersects")) {
		DoubleBBox box;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) box=*dynamic_cast<DoubleBBox*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box.minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box.maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box.miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box.maxy=d;
		*value_ret=new BooleanValue(intersect(&box,0));
		return 0;

	} if (isName(function,len,"Intersection")) {
		BBoxValue *box=new BBoxValue;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) *box=*dynamic_cast<BBoxValue*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box->minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box->maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box->miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box->maxy=d;
		box->intersect(this,1);
		*value_ret=box;
		return 0;
	}

	return 1;
}

} //namespace Laidout
