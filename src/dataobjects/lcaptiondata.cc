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
#include "../core/stylemanager.h"
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

void LCaptionData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	CaptionData::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
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
	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	CaptionData::dump_out(f,indent+2,what,context);
}

LaxFiles::Attribute *LCaptionData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att = DrawableObject::dump_out_atts(att, what,context);
	LaxFiles::Attribute *att2 = att->pushSubAtt("config");
	CaptionData::dump_out_atts(att2, what,context);
	return att;
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

Value *NewLCaptionData()
{
	LCaptionData *cap = new LCaptionData;
	cap->yaxis(-cap->yaxis()); //premptively reorient for right handed coordinates
	return cap;
}

ObjectDef *LCaptionData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("CaptionData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *groupdef = stylemanager.FindDef("Group");
	if (!groupdef) {
		Group g;
		groupdef = g.GetObjectDef();
	}
	sd = new ObjectDef(groupdef,
			"CaptionData",
            _("CaptionData"),
            _("A snippet of text"),
            NewLCaptionData,NULL);
	stylemanager.AddObjectDef(sd, 0);


	sd->pushVariable("color",    _("Color"),  _("Color fill"),    "Color",0, NULL,0);
	sd->pushVariable("fontsize", _("Font size"), _("Size of font in local units"),  "real",0,   NULL,0);
	sd->pushVariable("text",_("Text"),_("Text"), "string",0,   NULL,0);
	//sd->pushVariable("font",_("Font"),_("The font"), "Font",0,   NULL,0);
	sd->pushVariable("xcentering", _("X Centering"), _("0 is left, 100 is right. Any other number works too."),  "real",0,   NULL,0);
	sd->pushVariable("ycentering", _("Y Centering"), _("0 is top, 100 is bottom. Any other number works too."),  "real",0,   NULL,0);
	sd->pushVariable("linespacing", _("Line Spacing"), _("1 is 100% of font's default line spacing."),  "real",0,   NULL,0);
	

	return sd;
}

Value *LCaptionData::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "color")) {
		return new ColorValue(red, green, blue, CaptionData::alpha);
	}

	if (extequal(extstring,len, "fontsize")) {
		return new DoubleValue(Size());
	}

	if (extequal(extstring,len, "text")) {
		char *text = GetText();
		StringValue *s = new StringValue;
		s->InstallString(text);
		return s;
	}

	if (extequal(extstring,len, "xcentering")) {
		return new DoubleValue(xcentering);
	}

	if (extequal(extstring,len, "ycentering")) {
		return new DoubleValue(ycentering);
	}

	if (extequal(extstring,len, "linespacing")) {
		return new DoubleValue(LineSpacing());
	}

	return DrawableObject::dereference(extstring, len);
}

/*! Return 1 for success, 2 for success, but other contents changed too, -1 for unknown extension.
 * 0 for total fail, as when v is wrong type.
 */
int LCaptionData::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
			if (!strcmp(str,"color")) {
				ColorValue *cv = dynamic_cast<ColorValue*>(v);
				if (!cv) return 0;
				red   = cv->color.Red();
				green = cv->color.Green();
				blue  = cv->color.Blue();
				CaptionData::alpha = cv->color.Alpha();
				touchContents();
				return 1;

			} else if (!strcmp(str,"fontsize")) {
				double d;
				if (!setNumberValue(&d, v)) return 0;
				if (d < 0) return 0;
				Size(d);
				return 1;
			
			} else if (!strcmp(str,"linespacing")) {
				double d;
				if (!setNumberValue(&d, v)) return 0;
				if (d <= 0) return 0;
				LineSpacing(d);
				return 1;

			} else if (!strcmp(str,"xcentering")) {
				double d = 50;
				if (v->type() == VALUE_String) {
					StringValue *sv = dynamic_cast<StringValue*>(v);
					if (!sv || !sv->str) return 0;
					if (!strcasecmp(sv->str, "left")) d = 0;
					else if (!strcasecmp(sv->str, "center")) d = 50;
					else if (!strcasecmp(sv->str, "right")) d = 100;
					else d = strtod(sv->str, nullptr);
				} else {
					if (!setNumberValue(&d, v)) return 0;
				}
				XCenter(d);
				return 1;

			} else if (!strcmp(str,"ycentering")) {
				double d = 50;
				if (v->type() == VALUE_String) {
					StringValue *sv = dynamic_cast<StringValue*>(v);
					if (!sv || !sv->str) return 0;
					if (!strcasecmp(sv->str, "top")) d = 0;
					else if (!strcasecmp(sv->str, "center")) d = 50;
					else if (!strcasecmp(sv->str, "bottom")) d = 100;
					else d = strtod(sv->str, nullptr);
				} else {
					if (!setNumberValue(&d, v)) return 0;
				}
				YCenter(d);
				return 1;

			} else if (!strcmp(str,"text")) {
				StringValue *s = dynamic_cast<StringValue*>(v);
				if (!s) return 0;
				SetText(s->str);
				return 1;
			}
		}
	}

	return DrawableObject::assign(ext, v);
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

	return DrawableObject::Evaluate(func,len, context, parameters, settings, value_ret, log);
}


} //namespace Laidout

