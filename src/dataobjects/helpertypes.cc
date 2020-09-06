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


#include "../language.h"
#include "../core/stylemanager.h"
#include "helpertypes.h"


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



//---------------------- LLineStyle ---------------------------------------

Value *LLineStyle::newLLineStyle()
{
	return new LLineStyle();
}

LLineStyle::LLineStyle(LineStyle *style)
{
	linestyle = style;
	if (style) style->inc_count();
	Value::Id();
}

LLineStyle::~LLineStyle()
{
	if (linestyle) linestyle->dec_count();
}

const char *LLineStyle::whattype()
{
	if (linestyle) return linestyle->whattype();
	return "LineStyle";
}

void LLineStyle::Set(LaxInterfaces::LineStyle *style, bool absorb)
{
	if (style == linestyle) {
		if (absorb) style->dec_count();
	} else {
		if (linestyle) linestyle->dec_count();
		linestyle = style;
		if (linestyle && !absorb) linestyle->inc_count();
	}
}

void LLineStyle::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	if (linestyle) linestyle->dump_out(f,indent,what,context);
}

void LLineStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (linestyle) linestyle->dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *LLineStyle::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (linestyle) return linestyle->dump_out_atts(att,what,context);
	return att;
}

Value *LLineStyle::duplicate()
{
	LineStyle *style = nullptr;
	if (linestyle) style = new LineStyle(*linestyle);
	LLineStyle *dup = new LLineStyle(style);
	if (style) style->dec_count();

	return dup;
}

ObjectDef *LLineStyle::makeObjectDef()
{
    ObjectDef *sd=stylemanager.FindDef("LineStyle");
    if (sd) {
        sd->inc_count();
        return sd;
    }

    sd = new ObjectDef(nullptr,
            "LineStyle",
            _("Line Style"),
            _("Line properties"),
            "class",
            NULL,NULL, //range, default value
			nullptr, 0, //fields, flags
			LLineStyle::newLLineStyle
			);

    sd->pushVariable("width", _("Width"), _("Default line width"), "real",0,   new DoubleValue(1),1);
    sd->pushVariable("color", _("Color"),  _("Color"),      "Color",0,  NULL,0);

    sd->pushVariable("dashes", _("Dashes"),  _("Array of numbers specifying dash pattern"), "Array of real",0,  NULL,0);
    sd->pushVariable("dash_offset", _("Dash offset"), _("Start line dashes with this offset"), "real",0, new DoubleValue(0),1);

    sd->pushVariable("miterlimit", _("Miter limit"), _("Miter limit as a multiple of line width"), "real",0,  new DoubleValue(100),1);

	sd->pushEnum("CapStyle",_("Cap style"),_("How to round endpoints"), true, "round",nullptr,nullptr,
			  "default",    _("Default"),    _("Use default cap"),
			  "butt",       _("Butt"),       _("Just stop the line"),
			  "round",      _("Round"),      _("Round endpoints"),
			  //"peak",       _("Peak"),       _(""),
			  "square",     _("Square"),     _("Like butt, but stick out from the endpoint"),
			  "projecting", _("Projecting"), _("Try to continue stroke edges until they connect"),
			  "zero_width", _("Zero width"), _("Insert implied 0 width node at endpoints"),
			  //"custom",     _("Custom"),     _("Use custom shape for the cap"),
			  nullptr);

	ObjectDef *capstyledef = sd->last();
    sd->pushVariable("capstyle",    _("Line cap style"), _("How end points terminate"), "CapStyle",0, new EnumValue(capstyledef, "butt"),1);
    sd->pushVariable("endcapstyle", _("End cap style"), _("End caps, if different than capstyle"), "CapStyle",0, new EnumValue(capstyledef, "default"),1);

	sd->pushEnum("widthtype",_("Width type"),_("How width is used"), false, "normal",nullptr,nullptr,
			  "normal",    _("Normal"),    _("Width is in local space"),
			  "screen",    _("Screen"),    _("Width is in screen space"),
			  //"container", _("Container"), _("Width is in context specific top container space"),
			  nullptr);

	sd->pushEnum("joinstyle",_("Join style"),_("How corners are rendered"), false, "round",nullptr,nullptr,
			  "round",       _("Round"),       _("Round"),
			  "miter",       _("Miter"),       _("Extended hard corners"),
			  "bevel",       _("Bevel"),       _("Chopped corners"),
			  "extrapolate", _("Extrapolate"), _("Join based on curvature"),
			  nullptr);

    ObjectDef *function = stylemanager.FindDef("CombineFunction");
    if (!function) {
		
	    function = new ObjectDef(nullptr,
            "CombineFunction",
            _("Combine function"),
            _("How pixels are combined in rendering"),
            "enum",
            NULL,NULL);

		char laxop[50];
		for (int c = 0; c < LAXOP_MAX; c++) {
			function->pushEnumValue(LaxopToString(c, laxop, 50, nullptr), nullptr, nullptr, c);
		}
		stylemanager.AddObjectDef(function,1);
    }

    sd->pushVariable("func", _("Function"), _("How to render line"), "CombineFunction",0, new EnumValue(function, "over"),1);

//    sd->pushFunction("ChoosePreset",_("Choose preset"),_("Set properties based on a preset resource"),
//                     NULL,
//                     "name",NULL,_("Name of the preset"),"string", NULL, NULL,
//                     NULL);


	stylemanager.AddObjectDef(sd,0);
    return sd;
}

Value *LLineStyle::dereference(const char *extstring, int len)
{
	if (!linestyle) linestyle = new LineStyle();

	if (isName(extstring,len, "width")) {
		return new DoubleValue(linestyle->width);
	}

	if (isName(extstring,len, "color")) {
		return new ColorValue(linestyle->color);
	}

	if (isName(extstring,len, "dashes")) {
		SetValue *set = new SetValue();
		for (int c=0; c<linestyle->numdashes; c++) {
			set->Push(new DoubleValue(linestyle->dashes[c]), 1);
		}
		return set;
	}

	if (isName(extstring,len, "dash_offset")) {
		
		return new DoubleValue(linestyle->dash_offset);
	}

	if (isName(extstring,len, "miterlimit")) {
		return new DoubleValue(linestyle->miterlimit);
	}

	if (isName(extstring,len, "capstyle") || isName(extstring,len, "endcapstyle")) {
		int cap = linestyle->capstyle;
		if (*extstring == 'e' && linestyle->endcapstyle >= 0) cap = linestyle->endcapstyle;

		ObjectDef *capdef = objectdef->FindDef("CapStyle");
		switch (cap) {
			case LAXCAP_Butt:       return new EnumValue(capdef, "butt");
			case LAXCAP_Round:      return new EnumValue(capdef, "round");
			case LAXCAP_Peak:       return new EnumValue(capdef, "peak");
			case LAXCAP_Square:     return new EnumValue(capdef, "square");
			case LAXCAP_Projecting: return new EnumValue(capdef, "projecting");
			case LAXCAP_Zero_Width: return new EnumValue(capdef, "zero_width");
			case LAXCAP_Custom:     return new EnumValue(capdef, "custom");
		}
		return new EnumValue(capdef, "round");
	}

	if (isName(extstring,len, "widthtype")) {
		ObjectDef *def = objectdef->FindDef("widthtype");
		switch (linestyle->widthtype) {
			case 0: return new EnumValue(def, "normal");
			case 1: return new EnumValue(def, "screen");
			case 2: return new EnumValue(def, "container");
		}
		return new EnumValue(def, "normal");
	}

	if (isName(extstring,len, "joinstyle")) {
		ObjectDef *def = objectdef->FindDef("joinstyle");
		switch (linestyle->joinstyle) {
			case LAXJOIN_Miter: return new EnumValue(def, "miter");
			case LAXJOIN_Round: return new EnumValue(def, "round");
			case LAXJOIN_Bevel: return new EnumValue(def, "bevel");
			case LAXJOIN_Extrapolate: return new EnumValue(def, "extrapolate");
		}
		return new EnumValue(def, "round");
	}

	if (isName(extstring,len, "func")) {
		ObjectDef *def = stylemanager.FindDef("CombineFunction");
		return new EnumValue(def, linestyle->function);
	}

	return nullptr;
}

/*! return 1 for success, 0 for total failure like wrong type, 2 for success but other contents changed too, -1 for unknown extension
 */
int LLineStyle::assign(Value *v, const char *extstring)
{
	if (!linestyle) linestyle = new LineStyle();

	if (!strcmp(extstring, "width")) {
		return setNumberValue(&linestyle->width, v);
	}

	if (!strcmp(extstring, "color")) {
		ColorValue *vv = dynamic_cast<ColorValue*>(v);
		if (linestyle->color2) { linestyle->color2->dec_count(); linestyle->color2 = nullptr; }
		//***
		linestyle->color.rgbf(vv->color.Red(), vv->color.Green(), vv->color.Blue(), vv->color.Alpha());
	}

	if (!strcmp(extstring, "dashes")) {
		SetValue *set = dynamic_cast<SetValue*>(v);
		if (!set) return 0;
		double d;
		NumStack<double> nums;
		for (int c=0; c<set->n(); c++) {
			if (!setNumberValue(&d, set->e(c))) return 0;
			nums.push(d);
		}
		if (linestyle->dashes) delete[] linestyle->dashes;
		linestyle->dashes = nums.extractArray(&linestyle->numdashes);
		return 1;
	}

	if (!strcmp(extstring, "dash_offset")) {
		return setNumberValue(&linestyle->dash_offset, v);
	}

	if (!strcmp(extstring, "miterlimit")) {
		return setNumberValue(&linestyle->miterlimit, v);
	}

	if (strEquals(extstring, "capstyle") || strEquals(extstring, "endcapstyle")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "CapStyle")) return 0;

		int cap = -1;
		switch (e->value) {
			case 0: cap = -1; break;
			case 1: cap = LAXCAP_Butt; break;
			case 2: cap = LAXCAP_Round; break;
			//case : cap = LAXCAP_Peak; break;
			case 3: cap = LAXCAP_Square; break;
			case 4: cap = LAXCAP_Projecting; break;
			case 5: cap = LAXCAP_Zero_Width; break;
			//case : cap = LAXCAP_Custom; break;
			default: return 0;
		}

		if (*extstring == 'e') linestyle->endcapstyle = cap;
		else linestyle->capstyle = (cap == -1 ? LAXCAP_Round : cap);
		return 1;
	}

	if (strEquals(extstring, "widthtype")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "widthtype")) return 0;

		switch (e->value) {
			case 0: linestyle->widthtype = 0; break;
			case 1: linestyle->widthtype = 1; break;
			case 2: linestyle->widthtype = 2; break;
			default: return 0;
		}

		return 1;
	}

	if (strEquals(extstring, "joinstyle")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "joinstyle")) return 0;

		switch (e->value) {
			case 0: linestyle->joinstyle = LAXJOIN_Miter      ; break;
			case 1: linestyle->joinstyle = LAXJOIN_Round      ; break;
			case 2: linestyle->joinstyle = LAXJOIN_Bevel      ; break;
			case 3: linestyle->joinstyle = LAXJOIN_Extrapolate; break;
			default: return 0;
		}

		return 1;
	}

	if (strEquals(extstring, "func")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "CombineFunction")) return 0;

		if (e->value < 0 || e->value >= LAXJOIN_MAX) return 0;
		linestyle->function = e->value;

		return 1;
	}

	return -1;
}


//---------------------- LFillStyle ---------------------------------------


Value *LFillStyle::newLFillStyle()
{
	return new LFillStyle();
}

LFillStyle::LFillStyle(FillStyle *style)
{
	fillstyle = style;
	if (style) style->inc_count();
	Value::Id();
}

LFillStyle::~LFillStyle()
{
	if (fillstyle) fillstyle->dec_count();
}

const char *LFillStyle::whattype()
{
	if (fillstyle) return fillstyle->whattype();
	return "FillStyle";
}

void LFillStyle::Set(LaxInterfaces::FillStyle *style, bool absorb)
{
	if (style == fillstyle) {
		if (absorb) style->dec_count();
	} else {
		if (fillstyle) fillstyle->dec_count();
		fillstyle = style;
		if (fillstyle && !absorb) fillstyle->inc_count();
	}
}

void LFillStyle::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	if (fillstyle) fillstyle->dump_out(f,indent,what,context);
}

void LFillStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (fillstyle) fillstyle->FillStyle::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *LFillStyle::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!fillstyle) return att;
	return fillstyle->dump_out_atts(att,what,context);
}

Value *LFillStyle::duplicate()
{
	FillStyle *style = nullptr;
	if (fillstyle) style = new FillStyle(*fillstyle);
	LFillStyle *dup = new LFillStyle(style);
	if (style) style->dec_count();

	return dup;
}

ObjectDef *LFillStyle::makeObjectDef()
{
    ObjectDef *sd=stylemanager.FindDef("FillStyle");
    if (sd) {
        sd->inc_count();
        return sd;
    }

    sd = new ObjectDef(nullptr,
            "FillStyle",
            _("Fill Style"),
            _("Fill properties"),
            "class",
            NULL,NULL);

    sd->pushVariable("color", _("Color"),  _("Color"), "Color",0,  NULL,0);

	sd->pushEnum("fillrule",_("Fill rule"),_("Winding rule to determine area to be filled"), false, "normal",nullptr,nullptr,
			  "none",    _("None"),    _("No fill"),
			  "nonzero", _("Nonzero"), _("Fill where winding number is nonzero"),
			  "evenodd",    _("Even Odd"),    _("Fill alternates with even or odd winding number"),
			  nullptr);

	sd->pushEnum("fillstyle",_("Fill style"),_("Source type of fill"), false, "solid",nullptr,nullptr,
			  "solid",       _("Solid"),       _("Solid color"),
			  "pattern",     _("Pattern"),     _("Fill with a pattern"),
			  nullptr);

    ObjectDef *function = stylemanager.FindDef("CombineFunction");
    if (!function) {
		
	    function = new ObjectDef(nullptr,
            "CombineFunction",
            _("Combine function"),
            _("How pixels are combined in rendering"),
            "enum",
            NULL,NULL);

		char laxop[50];
		for (int c = 0; c < LAXOP_MAX; c++) {
			function->pushEnumValue(LaxopToString(c, laxop, 50, nullptr), nullptr, nullptr, c);
		}
		stylemanager.AddObjectDef(function,1);
    }

    sd->pushVariable("func", _("Function"), _("How to render line"), "CombineFunction",0, new EnumValue(function, "over"),1);


	stylemanager.AddObjectDef(sd,0);
    return sd;
}

Value *LFillStyle::dereference(const char *extstring, int len)
{
	if (!fillstyle) fillstyle = new FillStyle();

	if (isName(extstring,len, "color")) {
		return new ColorValue(fillstyle->color);
	}

	if (isName(extstring,len, "fillrule")) {
		ObjectDef *def = objectdef->FindDef("fillrule");
		switch (fillstyle->fillrule) {
			case LAXFILL_None: return new EnumValue(def, "none");
			case LAXFILL_Nonzero: return new EnumValue(def, "nonzero");
			case LAXFILL_EvenOdd: return new EnumValue(def, "evenodd");
		}
		return new EnumValue(def, "none");
	}

	if (isName(extstring,len, "fillstyle")) {
		ObjectDef *def = objectdef->FindDef("fillstyle");
		switch (fillstyle->fillstyle) {
			case LAXFILL_None: return new EnumValue(def, "none");
			case LAXFILL_Solid: return new EnumValue(def, "solid");
			case LAXFILL_Pattern: return new EnumValue(def, "pattern");
		}
		return new EnumValue(def, "solid");
	}

	if (isName(extstring,len, "func")) {
		ObjectDef *def = stylemanager.FindDef("CombineFunction");
		return new EnumValue(def, fillstyle->function);
	}

	return nullptr;
}

/*! return 1 for success, 0 for total failure like wrong type, 2 for success but other contents changed too, -1 for unknown extension
 */
int LFillStyle::assign(Value *v, const char *extstring)
{
	if (!fillstyle) fillstyle = new FillStyle();

	if (!strcmp(extstring, "color")) {
		ColorValue *vv = dynamic_cast<ColorValue*>(v);
		if (fillstyle->color2) { fillstyle->color2->dec_count(); fillstyle->color2 = nullptr; }
		//***
		fillstyle->color.rgbf(vv->color.Red(), vv->color.Green(), vv->color.Blue(), vv->color.Alpha());
	}

	if (strEquals(extstring, "fillrule")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "fillrule")) return 0;

		switch (e->value) {
			case 0: fillstyle->fillrule = LAXFILL_None; break;
			case 1: fillstyle->fillrule = LAXFILL_Nonzero; break;
			case 2: fillstyle->fillrule = LAXFILL_EvenOdd; break;
			default: return 0;
		}
		return 1;
	}

	if (strEquals(extstring, "fillstyle")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "fillstyle")) return 0;

		switch (e->value) {
			case 0: fillstyle->fillstyle = LAXFILL_None; break;
			case 1: fillstyle->fillstyle = LAXFILL_Solid; break;
			case 2: fillstyle->fillstyle = LAXFILL_Pattern; break;
			default: return 0;
		}
		return 1;
	}

	if (strEquals(extstring, "func")) {
		EnumValue *e = dynamic_cast<EnumValue*>(v);
		if (!e) return 0;
		if (strcmp(e->GetObjectDef()->name, "CombineFunction")) return 0;

		if (e->value < 0 || e->value >= LAXJOIN_MAX) return 0;
		fillstyle->function = e->value;

		return 1;
	}

	return -1;
}


} // namespace Laidout

