//
// $Id$
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
// Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>
#include "lengraverfilldata.h"
#include "../stylemanager.h"
#include "../language.h"

#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;


namespace Laidout {


//------------------------------- LEngraverFillData ---------------------------------------
/*! \class LEngraverFillData
 * \brief Redefined LaxInterfaces::EngraverFillData.
 */

LEngraverFillData::LEngraverFillData()
{
}

LEngraverFillData::~LEngraverFillData()
{
}

	
void LEngraverFillData::FindBBox()
{
	EngraverFillData::FindBBox();
}

/*! Provide final pointin() definition.
 */
int LEngraverFillData::pointin(flatpoint pp,int pin)
{
	return EngraverFillData::pointin(pp,pin);
}


void LEngraverFillData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		EngraverFillData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	EngraverFillData::dump_out(f,indent+2,what,context);
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LEngraverFillData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			EngraverFillData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) EngraverFillData::dump_in_atts(att,flag,context);
}

Value *LEngraverFillData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("EngraverFillData"));
	EngraverFillData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LEngraverFillData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LEngraverFillData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("EngraverFillData"));
	EngraverFillData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

ObjectDef *LEngraverFillData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("EngraverFillData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"EngraverFillData",
            _("EngraverFillData"),
            _("An field of engraving lines"),
            "class",
            NULL,NULL);

//	sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);
	
//	sd->pushFunction("LoadFile",_("Load File"),_("Load an image file"),
//					 NULL,
//			 		 "file",NULL,_("File name to load"),"string", NULL, NULL,
//					 NULL);
//
//	sd->pushVariable("file",  _("File"),  _("File name"),    "string",0, NULL,0);
//	sd->pushVariable("width", _("Width"), _("Pixel width"),  "real",0,   NULL,0);
//	sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);

	return sd;
}

Value *LEngraverFillData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "file")) {
//		return new StringValue(image ? image->filename : "");
//	}
//
//	if (extequal(extstring,len, "width")) {
//		return new DoubleValue(maxx);
//	}
//
//	if (extequal(extstring,len, "height")) {
//		return new DoubleValue(maxy);
//	}

	return NULL;
}

int LEngraverFillData::assign(FieldExtPlace *ext,Value *v)
{

//	if (ext && ext->n()==1) {
//		const char *str=ext->e(0);
//		if (str) {
//			if (!strcmp(str,"file")) {
//				LoadEngraverFill(str,NULL);
//				return 0;
//
//			//} else if (!strcmp(str,"width")) { <-- these are read only
//			//} else if (!strcmp(str,"height")) {
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
int LEngraverFillData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
//	if (len==8 && !strncmp(func,"LoadFile",8)) {
//		if (!parameters) return -1;
//		int status=0;
//		const char *s=parameters->findString("file",-1,&status);
//		if (status!=0) return -1;
//		if (isblank(s)) {
//			log->AddMessage(_("Cannot load null file"),ERROR_Fail);
//			return 1;
//		}
//		LoadEngraverFill(s,NULL);
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

LaxInterfaces::SomeData *LEngraverFillData::EquivalentObject()
{
	DrawableObject *group=new DrawableObject;
	LaxInterfaces::PathsData *p;
	
	for (int c=0; c<groups.n; c++) {
		p=MakePathsData(c);
		group->push(p);
		p->dec_count();
	}

	return group;
}


} //namespace Laidout

