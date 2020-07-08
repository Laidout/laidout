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
// Copyright (C) 2013 by Tom Lechner
//

#include "lpathsdata.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include "../calculator/shortcuttodef.h"
#include "objectfilter.h"



namespace Laidout {



//------------------------------- LPathsData ---------------------------------------

/*! \class LPathsData 
 * \brief Subclassing LaxInterfaces::PathsData
 */



LPathsData::LPathsData(LaxInterfaces::SomeData *refobj)
  : LaxInterfaces::PathsData(0)
{
	child_clip_type = CLIP_From_Parent_Area;
}

LPathsData::~LPathsData()
{
}

void LPathsData::FindBBox()
{
	PathsData::FindBBox();
}

void LPathsData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	PathsData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LPathsData::pointin(flatpoint pp,int pin)
{
	return PathsData::pointin(pp,pin);
}


void LPathsData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		PathsData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	PathsData::dump_out(f,indent+2,what,context);
}

void LPathsData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			PathsData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) PathsData::dump_in_atts(att,flag,context);

	if (filter) {
		ObjectFilter *of = dynamic_cast<ObjectFilter*>(filter);
		of->FindProperty("in")->topropproxy->owner->Update();
	}
}

LaxInterfaces::SomeData *LPathsData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LPathsData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
	PathsData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}


//--------Value functions: 


Value *LPathsData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("PathsData"));
	PathsData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

ObjectDef *LPathsData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("PathsData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"PathsData",
            _("PathsData"),
            _("A collection of paths"),
            "class",
            NULL,NULL);

	sd->pushFunction("moveto",_("moveto"),_("Start a new subpath"), NULL,
					"x",_("X"),_("X position"),"number", NULL,NULL,
					"y",_("Y"),_("Y position"),"number", NULL,NULL,
					"p",_("P"),_("Point"),"flatvector", NULL,NULL,
					 NULL);

	sd->pushFunction("lineto",_("lineto"),_("Add a simple straight line to the path"), NULL,
					"x",_("X"),_("X position"),"number", NULL,NULL,
					"y",_("Y"),_("Y position"),"number", NULL,NULL,
					"p",_("P"),_("Point"),"flatvector", NULL,NULL,
					 NULL);

	sd->pushFunction("curveto",_("curveto"),_("Add a bezier segment"), NULL,
					 "c1",_("c1"),_("Control for current point"),"flatvector", NULL,NULL,
                     "c2",_("c2"),_("Control for new point"),"flatvector", NULL,NULL,
                     "p",_("p"),_("Point"),"flatvector", NULL,NULL,
					 NULL);

	sd->pushFunction("appendRect",_("appendRect"),_("Append a rectangle"), NULL,
                     "x",_("x"),_("x"),"real", NULL,NULL,
                     "y",_("y"),_("y"),"real", NULL,NULL,
                     "w",_("w"),_("Width"),"real", NULL,NULL,
                     "h",_("h"),_("Height"),"real", NULL,NULL,
					 NULL);

	sd->pushFunction("close",_("close"),_("Close current path. New points will start a new subpath"), NULL, NULL);

	sd->pushFunction("NumPaths",_("NumPaths"),_("Number of subpaths"), NULL, NULL);

	sd->pushFunction("clear",_("Clear"),_("Clear all paths"), NULL, NULL);

	return sd;
}

Value *LPathsData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "p1")) {
//		return new DoubleValue(p1);
//	}

	return NULL;
}

int LPathsData::assign(FieldExtPlace *ext,Value *v)
{
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
int LPathsData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
	//return -1;
	AffineValue affine(m());
	int status=affine.Evaluate(func,len,context,parameters,settings,value_ret,log);
	if (status==0) {
		m(affine.m());
	}
	return status;
}




//------------------------------- LPathInterface --------------------------------
/*! \class LPathInterface
 * \brief add on a little custom behavior.
 */


LPathInterface::LPathInterface(int nid,Laxkit::Displayer *ndp)
  : PathInterface(nid,ndp)
{
	pathi_style|=LaxInterfaces::PATHI_Render_With_Cache;
}


LaxInterfaces::anInterface *LPathInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=dynamic_cast<anInterface *>(new LPathInterface(id,NULL));
	else if (!dynamic_cast<LPathInterface *>(dup)) return NULL;

	return PathInterface::duplicate(dup);
}


//! Returns this, but count is incremented.
Value *LPathInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LPathInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("PathInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"PathInterface",
            _("Path Interface"),
            _("Path Interface"),
            "class",
            NULL,NULL);

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
//int LPathInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, Laxkit::ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int LPathInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LPathInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LPathInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	PathInterface::dump_out(f,indent,what,context);
}

void LPathInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	PathInterface::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *LPathInterface::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	return att;
}


} //namespace Laidout

