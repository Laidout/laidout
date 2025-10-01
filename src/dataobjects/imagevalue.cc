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
#include "imagevalue.h"


using namespace Laxkit;


namespace Laidout {

//--------------------------------------- ImageValue ---------------------------------------

/* \class ImageValue
 * \brief Adds scripting functions for a Laxkit::LaxImage object.
 */

ImageValue::ImageValue(Laxkit::LaxImage *img, bool absorb)
{
	image = img;
	if (image && !absorb) image->inc_count();
}

//ImageValue::ImageValue(int width, int height)
//{
//	image = nullptr;
//}

ImageValue::~ImageValue()
{
	if (image) image->dec_count();
}

/*! Incs count.
 */
void ImageValue::SetImage(LaxImage *newimage)
{
	if (image == newimage) return;
	if (image) image->dec_count();
	image = newimage;
	if (image) image->inc_count();
}

int ImageValue::TypeNumber()
{
	static int v = VALUE_MaxBuiltIn + getUniqueNumber();
	return v;
}

int ImageValue::type()
{
	return TypeNumber();
}

Value *ImageValue::dereference(int index)
{
	if (image == nullptr) return nullptr;
	if (index == 0) return new IntValue(image->w());
	if (index == 1) return new IntValue(image->h());
	if (index == 2) return new StringValue(image->filename);
	return nullptr;
}

int ImageValue::getValueStr(char *buffer,int len)
{
	if (!image) {
		int needed = 7;
		if (!buffer || len<needed) return needed;
		sprintf(buffer,"Image()");
		return 0;
	}

    int needed = 30 + 20 + 20 + (image->filename ? strlen(image->filename) : 0);
    if (!buffer || len<needed) return needed;

	sprintf(buffer,"Image(width=%d, height=%d, filename=\"%s\")", image->w(),image->h(), image->filename ? image->filename : "");
    modified=0;
    return 0;
}

Value *ImageValue::duplicateValue()
{
	ImageValue *dup = new ImageValue();
	return dup;
}

ObjectDef *ImageValue::makeObjectDef()
{
	objectdef = stylemanager.FindDef("Image");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef = makeImageValueDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int ImageValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (len == 5 && !strncmp(function,"width",6)) {
		if (value_ret) *value_ret = new IntValue(image->w());
		return 0;

	} else if (len == 6 && !strncmp(function,"height",6)) {
		if (value_ret) *value_ret = new IntValue(image->h());
		return 0;

	} else if (len == 5 && !strncmp(function,"index",5)) {
		if (value_ret) *value_ret = new IntValue(image->index);
		return 0;

	} else if (len == 8 && !strncmp(function,"filename",8)) {
		if (value_ret) *value_ret = new StringValue(image->filename);
		return 0;
	}

	return 1;
}

//! Contructor for ImageValue objects.
int NewImageObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	Value *v=new ImageValue();
	*value_ret=v;

	if (!parameters || !parameters->n()) return 0;

//	Value *matrix=parameters->find("matrix");
//	if (matrix) {
//		SetValue *set=dynamic_cast<SetValue*>(matrix);
//		if (set && set->GetNumFields()==6) {
//		}
//	}

	return 0;
}

//! Create a new ObjectDef with Image characteristics. Always creates new one, does not search for Image globally.
ObjectDef *makeImageValueDef()
{
	ObjectDef *sd = new ObjectDef(NULL,"Image",
			_("Image"),
			_("Image buffer"),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NULL,NewImageObject);


	sd->pushFunction("width",    _("Width"),    _("Width"), NULL, NULL);
	sd->pushFunction("height",   _("Height"),   _("Height"), NULL, NULL);
	sd->pushFunction("filename", _("Filename"), _("Filename"), NULL, NULL);
	sd->pushFunction("index",    _("Index"),    _("Index, if file contains more than one image"), NULL, NULL);

	return sd;
}



} // namespace Laidout


