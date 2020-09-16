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
// Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>
#include "lengraverfilldata.h"
#include "../core/stylemanager.h"
#include "../language.h"

#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;


namespace Laidout {


//------------------------------- TraceSettingsValue ---------------------------------------

class TraceSettingsValue : public Value
{
  public:
  	LaxInterfaces::EngraverTraceSettings *settings;

 	TraceSettingsValue(LaxInterfaces::EngraverTraceSettings *nsettings);
 	virtual ~TraceSettingsValue();

	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};

TraceSettingsValue::TraceSettingsValue(LaxInterfaces::EngraverTraceSettings *nsettings)
{
	settings = nsettings;
	if (settings) settings->inc_count();
}

TraceSettingsValue::~TraceSettingsValue()
{
	if (settings) settings->dec_count();
}

Value *TraceSettingsValue::duplicate()
{
	TraceSettingsValue *dup = new TraceSettingsValue(settings);
	return dup;
}

ObjectDef *TraceSettingsValue::makeObjectDef()
{
	ObjectDef *sd = stylemanager.FindDef("TraceSettings");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd = new ObjectDef(nullptr,
			"TraceSettings",
            _("Trace Settings"),
            _("Trace settings for engraving lines"),
            "class",
            NULL,NULL);
	stylemanager.AddObjectDef(sd, 0);

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

Value *TraceSettingsValue::dereference(const char *extstring, int len)
{
	return nullptr;
}

/*! Return 1 for success, 2 for success, but other contents changed too, -1 for unknown extension.
 * 0 for total fail, as when v is wrong type.
 */
int TraceSettingsValue::assign(FieldExtPlace *ext,Value *v)
{
	return -1;
}

/*! Return
 *  0 for success, value optionally returned.
 * -1 for no value returned due to incompatible parameters or name not known, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int TraceSettingsValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
	return -1;
}


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

const char *LEngraverFillData::Id()
{ return EngraverFillData::Id(); }
    
const char *LEngraverFillData::Id(const char *newid)
{ return EngraverFillData::Id(newid); }
    

	
void LEngraverFillData::FindBBox()
{
	EngraverFillData::FindBBox();
}

void LEngraverFillData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	EngraverFillData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LEngraverFillData::pointin(flatpoint pp,int pin)
{
	return EngraverFillData::pointin(pp,pin);
}


void LEngraverFillData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	LaxFiles::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
	// char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	// DrawableObject::dump_out(f,indent,what,context);
	// fprintf(f,"%sconfig\n",spc);
	// EngraverFillData::dump_out(f,indent+2,what,context);
}

LaxFiles::Attribute *LEngraverFillData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	LaxFiles::Attribute *att2 = att->pushSubAtt("config");
	EngraverFillData::dump_out_atts(att2, what,context);
	return att;
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

Value *NewLEngraverFillData() { return new LEngraverFillData; }

ObjectDef *LEngraverFillData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("EngraverFillData");
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
			"EngraverFillData",
            _("EngraverFillData"),
            _("An field of engraving lines"),
            NewLEngraverFillData,NULL);
	stylemanager.AddObjectDef(sd, 0);

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

	return DrawableObject::dereference(extstring, len);
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

	return DrawableObject::assign(ext,v);
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

	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
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

