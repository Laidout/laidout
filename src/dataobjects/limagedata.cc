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

#include "limagedata.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"


using namespace Laxkit;


namespace Laidout {


//------------------------------- LImageData ---------------------------------------
/*! \class LImageData
 * \brief Redefined LaxInterfaces::ImageData.
 */

LImageData::LImageData(LaxInterfaces::SomeData *refobj)
{
	//importer=NULL;
	//sourcefile=NULL;
}

LImageData::~LImageData()
{
	//if (importer) importer->dec_count();
	//if (sourcefile) delete[] sourcefile;
}

	
void LImageData::FindBBox()
{
	ImageData::FindBBox();
}

void LImageData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	ImageData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LImageData::pointin(flatpoint pp,int pin)
{
	return ImageData::pointin(pp,pin);
}

void LImageData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Laxkit::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
	// char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	// DrawableObject::dump_out(f,indent,what,context);
	// fprintf(f,"%sconfig\n",spc);
	// ImageData::dump_out(f,indent+2,what,context);
}

Laxkit::Attribute *LImageData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	Laxkit::Attribute *att2 = att->pushSubAtt("config");
	ImageData::dump_out_atts(att2, what,context);
	return att;
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LImageData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			ImageData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) ImageData::dump_in_atts(att,flag,context);
}

Value *LImageData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("ImageData"));
	ImageData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LImageData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LImageData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("ImageData"));
	ImageData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

Value *NewLImageData() { return new LImageData; }

ObjectDef *LImageData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ImageData");
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
			"ImageData",
            _("ImageData"),
            _("An image"),
            NewLImageData,NULL);

	sd->pushFunction("LoadFile",_("Load File"),_("Load an image file"),
					 NULL,
			 		 "file",NULL,_("File name to load"),"string", NULL, NULL,
					 NULL);

	sd->pushVariable("file",  _("File"),  _("File name"),    "string",0, NULL,0);
	sd->pushVariable("width", _("Width"), _("Pixel width"),  "real",0,   NULL,0);
	sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);

	stylemanager.AddObjectDef(sd, 0);
	return sd;
}

Value *LImageData::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "file")) {
		return new StringValue(image ? image->filename : "");
	}

	if (extequal(extstring,len, "width")) {
		return new DoubleValue(maxx);
	}

	if (extequal(extstring,len, "height")) {
		return new DoubleValue(maxy);
	}

	if (extequal(extstring,len, "depth")) {
		return new IntValue(8);
	}

	//if (extequal(extstring,len, "channels")) {
	//if (extequal(extstring,len, "importer")) {
	//if (extequal(extstring,len, "ColorSpace")) {
	//if (extequal(extstring,len, "HasAlpha")) {

	return DrawableObject::dereference(extstring, len);
}

int LImageData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
			if (!strcmp(str,"file")) {
				LoadImage(str,NULL);
				return 0;

			//} else if (!strcmp(str,"width")) { <-- these are read only
			//} else if (!strcmp(str,"height")) {
			}
		}
	}

	return DrawableObject::assign(ext,v);
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int LImageData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log)
{
	if (len==8 && !strncmp(func,"LoadFile",8)) {
		if (!parameters) return -1;
		int status=0;
		const char *s=parameters->findString("file",-1,&status);
		if (status!=0) return -1;
		if (isblank(s)) {
			log->AddMessage(_("Cannot load null file"),ERROR_Fail);
			return 1;
		}
		LoadImage(s,NULL);
		return 0;
	}

	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
}


} //namespace Laidout

