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
#include "fontvalue.h"
#include "../laidout.h"

using namespace Laxkit;


namespace Laidout {


//--------------------------------------- FontValue ---------------------------------------

/* \class FontValue
 * \brief Adds scripting functions for a Laxkit::LaxFont object.
 */

/*! Incs count of newobj.
 */
FontValue::FontValue(Laxkit::LaxFont *newobj, int absorb)
{
	font = nullptr;
	Set(newobj, absorb);
}

//FontValue::FontValue(int width, int height)
//{
//	image = nullptr;
//}

FontValue::~FontValue()
{
	if (font) font->dec_count();
}

/*! Incs count.
 */
void FontValue::Set(LaxFont *newobj, int absorb)
{
	if (font != newobj) {
		if (font) font->dec_count();
		font = newobj;
		if (font && !absorb) font->inc_count();
	} else if (font && absorb) font->dec_count();
}

Value *FontValue::dereference(const char *extstring, int len)
{
	if (!font) return nullptr;

	if (isName(extstring,len, "family")) return new StringValue(font->Family());
	if (isName(extstring,len, "style")) return new StringValue(font->Style());
	if (isName(extstring,len, "file")) return new StringValue(font->FontFile());
	if (isName(extstring,len, "size")) return new DoubleValue(font->Msize());
	
	return nullptr;
}

int FontValue::getValueStr(char *buffer,int len)
{
	if (!font) {
		int needed = 7;
		if (!buffer || len<needed) return needed;
		sprintf(buffer, "Font()");
		return 0;
	}

    const char *family = font ? font->Family()   : "";
    const char *style  = font ? font->Style()    : "";
    const char *file   = font ? font->FontFile() : "";
    int needed = 70 + strlen(family) + strlen(style) + strlen(file);
    if (!buffer || len < needed) return needed;

	sprintf(buffer,"Font(family=\"%s\", style=\"%s\", file=\"%s\", size=%f)", family, style, file, font->Msize());
    modified = 0;
    return 0;
}

/*! Note: this deep copies the font.
 */
Value *FontValue::duplicateValue()
{
	FontValue *dup = new FontValue();
	if (font) {
		LaxFont *nfont = font->duplicateFont();
		if (nfont) dup->Set(nfont, 1);
	}
	return dup;
}

Value *FontValue::NewFontValue()
{
	return new FontValue();
}

//! Contructor for FontValue objects.
int NewFontObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	FontValue *v = new FontValue();
	*value_ret = v;

	if (!parameters || !parameters->n()) {
		//default font
		LaxFont *font = laidout->fontmanager->MakeFont("sans", nullptr, 1, -1);
		v->Set(font, 1);
		return 0;
	}

	int err = 0;
	double size = parameters->findIntOrDouble("size", -1, &err);
	if (err) return 1;
	if (size <= 0) return 1;

	const char *family = parameters->findString("family", -1, &err);
	if (err == 2) return 1;

    const char *style = parameters->findString("style", -1, &err);
	if (err == 2) return 1;

    const char *file = parameters->findString("file", -1, &err);
	if (err == 2) return 1;

    LaxFont *newfont = nullptr;
    if (file) {
		newfont = laidout->fontmanager->MakeFontFromFile(file, family, style, size, -1);		
    } else {
		newfont = laidout->fontmanager->MakeFont(family, style, size, -1);
    }
    v->Set(newfont, 1);
    return 0;
}

ObjectDef *FontValue::makeObjectDef()
{
	ObjectDef *def = stylemanager.FindDef("Font");
	if (def) { def->inc_count(); return def; }

	def = new ObjectDef(NULL,"Font",
			_("Font"),
			_("A font"),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NewFontValue, NewFontObject);

	def->pushVariable("family", _("Family"), _("Font family"), "string", 0,nullptr,0);
	def->pushVariable("style",  _("Style"),  _("Font style"),  "string", 0,nullptr,0);
	def->pushVariable("file",   _("File"),   _("The file this font comes from"), "File", 0,nullptr,0);
	def->pushVariable("size",   _("Size"),   _("Font size in user units"), "real", 0, new DoubleValue(1),1);

	def->pushFunction("Layers", _("Layers"), _("Set of layers for this font, or null if only one layer"), NULL, NULL);
	def->pushFunction("Extent", _("Extent"), _("Return the horizontal extent of the text"), NULL, 
		"text", _("Text"), _("String to find extent of"), "string", nullptr, nullptr,
		NULL);
	def->pushFunction("ascent",  _("Ascent"),  _("Font defined height above baseline"), NULL, NULL);
	def->pushFunction("descent", _("Descent"), _("Font defined height below baseline"), NULL, NULL);

	stylemanager.AddObjectDef(def,0);
	return def;
}

/*! Return 0 success, -1 name unknown, 1 for error.
 */
int FontValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (!font) return 1;

	if (isName(function, len, "Layers")) {
		if (pp && pp->n() != 0) return 1;
		if (!font || font->Layers() == 1) {
			*value_ret = new NullValue();
			return 0;
		}
		SetValue *sv = new SetValue();
		LaxFont *f = font;
		while (f) {
			FontValue *nf = new FontValue(f,0);
			sv->Push(nf, 1);
			f = f->nextlayer;
		}
		*value_ret = sv;
		return 0;

	} else if (isName(function, len, "Extent")) {
		if (!pp || pp->n() != 1) return 1;
		StringValue *sv = dynamic_cast<StringValue*>(pp->e(0));
		if (!sv) return 1;
		
		*value_ret = new DoubleValue(font->Extent(sv->str,-1));
		return 0;

	} else if (isName(function, len, "ascent")) {
		if (pp && pp->n() != 0) return 1;
		*value_ret = new DoubleValue(font->ascent());
		return 0;
	
	} else if (isName(function, len, "descent")) {
		if (pp && pp->n() != 0) return 1;
		*value_ret = new DoubleValue(font->descent());
		return 0;
	}

	return -1;
}

/*! Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int FontValue::assign(FieldExtPlace *ext,Value *v)
{
	if (!font) return 0;

	if (ext && ext->n()) return 0;
	const char *str = ext->e(0);
 
	if (!strcmp(str, "size")) {
		double d;
		if (setNumberValue(&d, v) && d > 0) {
			font->Resize(d);
			return 1;
		}
		return 0;
	}

    LaxFont *newfont = nullptr;

    const char *ss = nullptr;
	if (strcmp(str, "family") && strcmp(str, "style") && strcmp(str, "file")) 
		return -1;

	if (v->type() == VALUE_File) {
		FileValue *fv = dynamic_cast<FileValue*>(v);
		if (!fv) return 0;
		ss = fv->filename;
	} else {
		if (v->type() != VALUE_String) return 0;
		StringValue *s = dynamic_cast<StringValue*>(v);
		ss = s->str;
	}
	if (isblank(ss)) return 0;

	if (!strcmp(str, "family")) {
		newfont = laidout->fontmanager->MakeFont(ss, font->Style(), font->Msize(), -1);

	} else if (!strcmp(str, "style")) {
		newfont = laidout->fontmanager->MakeFont(font->Family(), ss, font->Msize(), -1);
	
	} else if (!strcmp(str, "file")) {
		newfont = laidout->fontmanager->MakeFontFromFile(ss, font->Family(), font->Style(), font->Msize(), -1);		
	}

	if (!newfont) return 0;
	Set(newfont, 1);
	return 0;
}


} // namespace Laidout


