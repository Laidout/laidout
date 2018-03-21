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
// Copyright (C) 2015 by Tom Lechner
//

#include "lcaptiondata.h"
#include "datafactory.h"
#include "../stylemanager.h"
#include "../language.h"


using namespace Laxkit;


namespace Laidout {


//------------------------------- LCaptionData ---------------------------------------
/*! \class LCaptionData
 * \brief Redefined LaxInterfaces::CaptionData.
 */

LCaptionData::LCaptionData(LaxInterfaces::SomeData *refobj)
{
	//importer=NULL;
	//sourcefile=NULL;
}

LCaptionData::~LCaptionData()
{
	//if (importer) importer->dec_count();
	//if (sourcefile) delete[] sourcefile;
}

	
void LCaptionData::FindBBox()
{
	CaptionData::FindBBox();
}

/*! Provide final pointin() definition.
 */
int LCaptionData::pointin(flatpoint pp,int pin)
{
	return CaptionData::pointin(pp,pin);
}


void LCaptionData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		CaptionData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	CaptionData::dump_out(f,indent+2,what,context);
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LCaptionData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			CaptionData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) CaptionData::dump_in_atts(att,flag,context);
}

Value *LCaptionData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("CaptionData"));
	CaptionData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LCaptionData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LCaptionData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("CaptionData"));
	CaptionData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

ObjectDef *LCaptionData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("CaptionData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"CaptionData",
            _("CaptionData"),
            _("A snippet of text"),
            "class",
            NULL,NULL);

	//sd->pushVariable("file",  _("File"),  _("File name"),    "string",0, NULL,0);
	//sd->pushVariable("width", _("Width"), _("Pixel width"),  "real",0,   NULL,0);
	//sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);

	return sd;
}

Value *LCaptionData::dereference(const char *extstring, int len)
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

int LCaptionData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
//			if (!strcmp(str,"file")) {
//				LoadImage(str,NULL);
//				return 0;

			//} else if (!strcmp(str,"width")) { <-- these are read only
			//} else if (!strcmp(str,"height")) {
//			}
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
int LCaptionData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
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
//		LoadImage(s,NULL);
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


} //namespace Laidout

