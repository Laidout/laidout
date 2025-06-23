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
#include "affinevalue.h"


using namespace Laxkit;


namespace Laidout {


//--------------------------------------- AffineValue ---------------------------------------

/* \class AffineValue
 * \brief Adds scripting functions for a Laxkit::Affine object.
 */

AffineValue::AffineValue()
{}

AffineValue::AffineValue(const double *m)
  : Affine(m)
{}

int AffineValue::TypeNumber()
{
	static int v = VALUE_MaxBuiltIn + getUniqueNumber();
	return v;
}

int AffineValue::type()
{
	return TypeNumber();
}

Value *AffineValue::dereference(int index)
{
	if (index<0 || index>=6) return NULL;
	return new DoubleValue(m(index));
}

int AffineValue::getValueStr(char *buffer,int len)
{
    int needed=6*30;
    if (!buffer || len<needed) return needed;

	sprintf(buffer,"[%.10g,%.10g,%.10g,%.10g,%.10g,%.10g]",m((int)0),m(1),m(2),m(3),m(4),m(5));
    modified=0;
    return 0;
}

Value *AffineValue::duplicate()
{
	AffineValue *dup=new AffineValue(m());
	return dup;
}

ObjectDef *AffineValue::makeObjectDef()
{
	objectdef = stylemanager.FindDef("Affine");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef = makeAffineObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int AffineValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (len==6 && !strncmp(function,"rotate",6)) {
		int err=0;
		double angle;
		flatpoint p;
		try {
			angle=pp->findIntOrDouble("angle",-1,&err);
			if (err) throw _("Missing angle.");
			err=0;

			FlatvectorValue *fpv=dynamic_cast<FlatvectorValue*>(pp->find("point"));
			if (fpv) p=fpv->v;

			Rotate(angle,p);

		} catch (const char *str) {
			if (log) log->AddMessage(str,ERROR_Fail);
			err=1;
		}
		 
		if (value_ret) *value_ret=NULL;
		return err;

//	} else if (len==8 && !strncmp(function,"array3x3",8)) {
//		ArrayValue *v=new ArrayValue;
//		v->push(new ArrayValue(m(0), m(1), 0));
//		v->push(new ArrayValue(m(2), m(3), 0));
//		v->push(new ArrayValue(m(4), m(5), 1));
//		return v;

	} else if (len==9 && !strncmp(function,"translate",9)) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		origin(origin()+p);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==11 && !strncmp(function,"scalerotate",11)) {
		int err=0;
		flatpoint p1,p2,p3;
		p1=pp->findFlatvector("p1",-1,&err);
		p2=pp->findFlatvector("p2",-1,&err);
		p3=pp->findFlatvector("p3",-1,&err);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		RotateScale(p1,p2,p3);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==11 && !strncmp(function,"anchorshear",11)) {
		int err=0;
		flatpoint p1,p2,p3,p4;
		p1=pp->findFlatvector("p1",-1,&err);
		p2=pp->findFlatvector("p2",-1,&err);
		p3=pp->findFlatvector("p3",-1,&err);
		p4=pp->findFlatvector("p4",-1,&err);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		AnchorShear(p1,p2,p3,p4);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==4 &&  !strncmp(function,"flip",4)) {
		int err1=0,err2=0;
		flatpoint p1,p2;
		p1=pp->findFlatvector("p1",-1,&err1);
		p2=pp->findFlatvector("p2",-1,&err2);
		if (err1!=0 && err2!=0) p2=flatpoint(1,0);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		Flip(p1,p2);
		if (value_ret) *value_ret=NULL;
		return 0;


	} else if (len==12 && !strncmp(function,"settransform",12)) {
		int err=0;
		double mm[6];
		mm[0]=pp->findDouble("a",-1,&err);
		mm[1]=pp->findDouble("b",-1,&err);
		mm[2]=pp->findDouble("c",-1,&err);
		mm[3]=pp->findDouble("d",-1,&err);
		mm[4]=pp->findDouble("x",-1,&err);
		mm[5]=pp->findDouble("y",-1,&err);

		if (is_degenerate_transform(mm)) {
			if (log) log->AddMessage(_("Bad matrix!"),ERROR_Fail);
			return 1;
		}

		m(mm);
		if (value_ret) *value_ret=NULL;
		return 0;
	}

	return 1;
}

//! Contructor for AffineValue objects.
int NewAffineObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	Value *v=new AffineValue();
	*value_ret=v;

	if (!parameters || !parameters->n()) return 0;

//	Value *matrix=parameters->find("matrix");
//	if (matrix) {
//		SetValue *set=dynamic_cast<SetValue*>(matrix);
//		if (set && set->GetNumFields()==6) {
//		}
//	}

	return 0;
}

Value *NewAffineValue() { return new AffineValue(); }

//! Create (if necessary) a new ObjectDef with Affine characteristics.
ObjectDef *makeAffineObjectDef()
{
	ObjectDef *sd = stylemanager.FindDef("Affine");
	if (sd) {
		sd->inc_count();
		return sd;
	}

	sd=new ObjectDef(NULL,"Affine",
			_("Affine"),
			_("Affine transform defined by 6 real numbers."),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NewAffineValue, NewAffineObject);


	sd->pushFunction("translate", _("Translate"), _("Move by a certain amount"),
					 NULL, //evaluator
					 "x",_("X"),_("The amount to move in the x direction"),"number", NULL,NULL,
					 "y",_("Y"),_("The amount to move in the y direction"),"number", NULL,NULL,
					 NULL);

	sd->pushFunction("rotate", _("Rotate"), _("Rotate the object, optionally around a point"),
					 NULL, //evaluator
					 "angle",_("Angle"),_("The angle to rotate"),"number", NULL,NULL,
					 "point",_("Point"),_("The point around which to rotate. Default is the origin."),"flatvector", NULL,"(0,0)",
					 NULL);


	sd->pushFunction("scalerotate", _("ScaleRotate"), _("Rotate and scale the object, keeping one point fixed"),
					 NULL,
					 "p1",_("P1"),_("A constant point"),       "flatvector", NULL,NULL,
					 "p2",_("P2"),_("The point to move."),     "flatvector", NULL,NULL,
					 "p3",_("P3"),_("The new position of p2."),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("anchorshear", _("Anchor Shear"), _("Transform so that p1 and p2 stay fixed, but p3 is shifted to newp3."),
					 NULL,
					 "p1",_("P1"),_("A constant point"),       "flatvector", NULL,NULL,
					 "p2",_("P2"),_("Another constant point"), "flatvector", NULL,NULL,
					 "p3",_("P3"),_("The point to move."),     "flatvector", NULL,NULL,
					 "p4",_("P4"),_("The new position of p3."),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("flip", _("Flip"), _("Flip around an axis defined by two points."),
					 NULL,
					 "p1",_("P1"),_("A constant point"),      "flatvector", NULL,NULL,
					 "p2",_("P2"),_("Another constant point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("settransform", _("Set Transform"), _("Set the object's affine transform, with a set of 6 real numbers: a,b,c,d,x,y."),
					 NULL,
					 "a",_("A"),_("A"),"real", NULL,NULL,
					 "b",_("B"),_("B"),"real", NULL,NULL,
					 "c",_("C"),_("C"),"real", NULL,NULL,
					 "d",_("D"),_("D"),"real", NULL,NULL,
					 "x",_("X"),_("X"),"real", NULL,NULL,
					 "y",_("Y"),_("Y"),"real", NULL,NULL,
					 NULL);

	stylemanager.AddObjectDef(sd, 0);
	return sd;
}



} // namespace Laidout


