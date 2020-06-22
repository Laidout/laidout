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
// Copyright (C) 2012 by Tom Lechner
//

#include "lgradientdata.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include "../calculator/shortcuttodef.h"


using namespace Laxkit;


namespace Laidout {


//------------------------------- GradientStrip ---------------------------------------

/*! \class GradientValue
 */


GradientValue::GradientValue()
  : GradientStrip(1)
{
	Id();
}

void GradientValue::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	GradientStrip::dump_out(f,indent,what,context);
}

void GradientValue::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	GradientStrip::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *GradientValue::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	return GradientStrip::dump_out_atts(att,what,context);
}

GradientStrip *GradientValue::newGradientStrip()
{
	return new GradientValue();
}

Value *GradientValue::duplicate()
{
	GradientValue *dup = new GradientValue();

	GradientStrip::duplicate(dynamic_cast<anObject*>(this));

	return dup;
}

ObjectDef *GradientValue::makeObjectDef()
{
    ObjectDef *sd=stylemanager.FindDef("GradientStrip");
    if (sd) {
        sd->inc_count();
        return sd;
    }

    sd = new ObjectDef(nullptr,
            "GradientStrip",
            _("Gradient"),
            _("A simple gradient definition"),
            "class",
            NULL,NULL);

//    sd->pushFunction("LoadFile",_("Load File"),_("Load a gradient file"),
//                     NULL,
//                     "file",NULL,_("File name to load"),"string", NULL, NULL,
//                     NULL);
//    sd->pushFunction("SaveFile",_("Save File"),_("Save a gradient file"),
//                     NULL,
//                     "file",NULL,_("File name to load"),"string", NULL, NULL,
//                     NULL);

    //sd->pushVariable("file",  _("File"),  _("File name"),    "string",0, NULL,0);
    //sd->pushVariable("width", _("Width"), _("Pixel width"),  "real",0,   NULL,0);
    //sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);

    sd->pushVariable("colors",  _("Colors"),  _("List of colors"),    "array of GradientColor",0, NULL,0);

    return sd;
}

Value *GradientValue::dereference(const char *extstring, int len)
{
	//***
	return nullptr;
}

/*! Return 1 for success, 2 for success, but other contents changed too, -1 for unknown
 */
int GradientValue::assign(FieldExtPlace *ext,Value *v)
{
	//***
	return -1;
}

//int GradientValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//                         Value **value_ret, Laxkit::ErrorLog *log)
//{
//	***
//}


//------------------------------- LGradientData ---------------------------------------

/*! \class LGradientData 
 * \brief Subclassing LaxInterfaces::GradientData
 */


LGradientData::LGradientData(LaxInterfaces::SomeData *refobj)
{
	Id();
}

LGradientData::~LGradientData()
{
}

void LGradientData::FindBBox()
{
	GradientData::FindBBox();
}

/*! Provide final pointin() definition.
 */
int LGradientData::pointin(flatpoint pp,int pin)
{
	return GradientData::pointin(pp,pin);
}

void LGradientData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		GradientData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	GradientData::dump_out(f,indent+2,what,context);
}

void LGradientData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			GradientData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) GradientData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LGradientData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LGradientData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("GradientData"));
	GradientData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}


//------- GradientData.Value functions:

Value *LGradientData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("GradientData"));
	GradientData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

ObjectDef *LGradientData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("GradientData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"GradientData",
            _("GradientData"),
            _("A gradient"),
            "class",
            NULL,NULL);

	sd->pushFunction("FlipColors",_("Flip Colors"),_("Flip the order of colors"), NULL,
					 NULL);

	sd->pushVariable("p1", _("p1"),         _("The starting point"), "real",0, NULL,0);
	sd->pushVariable("p2", _("p2"),         _("The ending point"),   "real",0, NULL,0);
	sd->pushVariable("r1", _("Distance 1"), _("Radius or distance"), "real",0, NULL,0);
	sd->pushVariable("r2", _("Distance 2"), _("Radius or distance"), "real",0, NULL,0);
	//sd->pushVariable("angle",_("Angle"),_("Angle gradient exists at"), NULL,0);

	//sd->pushVariable("stops",  _("Stops"),  _("The set of color positions"), NULL,0);

	return sd;
}

Value *LGradientData::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "p1")) {
		return new FlatvectorValue(P1());
	}

	if (extequal(extstring,len, "p2")) {
		return new FlatvectorValue(P2());
	}

	if (extequal(extstring,len, "r1")) {
		return new DoubleValue(R1());
	}

	if (extequal(extstring,len, "r2")) {
		return new DoubleValue(R2());
	}

//	if (extequal(extstring,len, "colors")) {
//		return new DoubleValue(maxy);
//	}

	return NULL;
}

int LGradientData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		int isnum;
		double d;
		if (str) {
			if (!strcmp(str,"p1")) {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(v);
				if (!fv) return 0;
				P1(fv->v);
				FindBBox();
				return 1;

			} else if (!strcmp(str,"p2")) {
				FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(v);
				if (!fv) return 0;
				P2(fv->v);
				FindBBox();
				return 1;

			} else if (!strcmp(str,"r1")) {
				d=getNumberValue(v, &isnum);
				if (!isnum) return 0;
				R1(d);
				FindBBox();
				return 1;

			} else if (!strcmp(str,"r2")) {
				d=getNumberValue(v, &isnum);
				if (!isnum) return 0;
				R2(d);
				FindBBox();
				return 1;

			}
		}
	}

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
int LGradientData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
	if (len==10 && !strncmp(func,"FlipColors",10)) {
		//if (parameters && parameters->n()) return -1; *** if pp has this, then huh!

		FlipColors();
		return 0;
	}

	AffineValue v(m());
	int status=v.Evaluate(func,len,context,parameters,settings,value_ret,log);
	if (status==0) {
		m(v.m());
		return 0;
	}

	return status;
}



//------------------------------- LGradientInterface --------------------------------
/*! \class LGradientInterface
 * \brief add on a little custom behavior.
 */


LGradientInterface::LGradientInterface(int nid,Laxkit::Displayer *ndp)
  : GradientInterface(nid,ndp)
{
}


LaxInterfaces::anInterface *LGradientInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=dynamic_cast<anInterface *>(new LGradientInterface(id,NULL));
	else if (!dynamic_cast<LGradientInterface *>(dup)) return NULL;

	return GradientInterface::duplicate(dup);
}

int LGradientInterface::InterfaceOn()
{
	GradientInterface::InterfaceOn();
	showdecs &= ~GradientInterface::ShowColors;
	return 0;
}


//! Returns this, but count is incremented.
Value *LGradientInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LGradientInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("GradientInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"GradientInterface",
            _("Gradient Interface"),
            _("Gradient Interface"),
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
//int LGradientInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
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
int LGradientInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LGradientInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LGradientInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	GradientInterface::dump_out(f,indent,what,context);
}

void LGradientInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	GradientInterface::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *LGradientInterface::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	return att;
}

} //namespace Laidout

