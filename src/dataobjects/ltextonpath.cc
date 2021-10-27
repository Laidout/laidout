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
// Copyright (C) 2016 by Tom Lechner
//

#include "ltextonpath.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"
#include "../ui/viewwindow.h"


using namespace Laxkit;


namespace Laidout {


//------------------------------- LTextOnPath ---------------------------------------
/*! \class LTextOnPath
 * \brief Redefined LaxInterfaces::TextOnPath.
 */

LTextOnPath::LTextOnPath(LaxInterfaces::SomeData *refobj)
{
	//importer=NULL;
	//sourcefile=NULL;
}

LTextOnPath::~LTextOnPath()
{
	//if (importer) importer->dec_count();
	//if (sourcefile) delete[] sourcefile;
}

	
void LTextOnPath::FindBBox()
{
	TextOnPath::FindBBox();
}

void LTextOnPath::ComputeAABB(const double *transform, DoubleBBox &box)
{
	TextOnPath::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LTextOnPath::pointin(flatpoint pp,int pin)
{
	return TextOnPath::pointin(pp,pin);
}

bool LTextOnPath::IsLinkedToParent()
{
	if (!pathcontext) return false;
	VObjContext *oc = dynamic_cast<VObjContext*>(pathcontext);
	if (!oc) return false;
	return oc->context.n() == 1 && oc->context.e(0) == -1;
}

void LTextOnPath::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	LaxFiles::Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
	// char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	// DrawableObject::dump_out(f,indent,what,context);
	// fprintf(f,"%sconfig\n",spc);
	// TextOnPath::dump_out(f,indent+2,what,context);
}

LaxFiles::Attribute *LTextOnPath::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	LaxFiles::Attribute *att2 = att->pushSubAtt("config");
	TextOnPath::dump_out_atts(att2, what,context);
	return att;
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LTextOnPath::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			TextOnPath::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) TextOnPath::dump_in_atts(att,flag,context);
}

Value *LTextOnPath::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("TextOnPath"));
	TextOnPath::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LTextOnPath::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LTextOnPath*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("TextOnPath"));
	TextOnPath::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

Value *NewLTextOnPath() { return new LTextOnPath; }

ObjectDef *LTextOnPath::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("TextOnPath");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *gdef = stylemanager.FindDef("Group");
	if (!gdef) {
		Group g;
		gdef = g.GetObjectDef();
	}
	sd=new ObjectDef(gdef,
			"TextOnPath",
            _("TextOnPath"),
            _("Text on a path"),
            NewLTextOnPath,NULL);
	stylemanager.AddObjectDef(sd, 0);

	sd->pushVariable("text",  _("Text"),  _("Text"),    "string",0, NULL,0);
	//sd->pushVariable("width", _("Width"), _("Pixel width"),  "real",0,   NULL,0);
	//sd->pushVariable("height",_("Height"),_("Pixel height"), "real",0,   NULL,0);

	return sd;
}

Value *LTextOnPath::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "text")) {
		return new StringValue(text+start, end-start);
	}

//	if (extequal(extstring,len, "width")) {
//		return new DoubleValue(maxx);
//	}
//
//	if (extequal(extstring,len, "height")) {
//		return new DoubleValue(maxy);
//	}


	return DrawableObject::dereference(extstring, len);
}

int LTextOnPath::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
			if (!strcmp(str,"text")) {
				Text(str);
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
int LTextOnPath::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
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

	return DrawableObject::Evaluate(func, len, context, parameters, settings, value_ret, log);
}


} //namespace Laidout

