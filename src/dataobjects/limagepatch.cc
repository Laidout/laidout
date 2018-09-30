//
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include "limagepatch.h"
#include "../laidout.h"
#include "datafactory.h"
#include "../language.h"
#include "../stylemanager.h"
#include "../calculator/shortcuttodef.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;
using namespace Laxkit;


namespace Laidout {



//------------------------------- LImagePatchData ---------------------------------------
/*! \class LImagePatchData 
 * \brief Subclassing LaxInterfaces::ImagePatchData
 */


LImagePatchData::LImagePatchData(LaxInterfaces::SomeData *refobj)
{
}

LImagePatchData::~LImagePatchData()
{
}

void LImagePatchData::FindBBox()
{
	ImagePatchData::FindBBox();
}

/*! Provide final pointin() definition.
 */
int LImagePatchData::pointin(flatpoint pp,int pin)
{
	return PatchData::pointin(pp,pin);
}

void LImagePatchData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		ImagePatchData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	ImagePatchData::dump_out(f,indent+2,what,context);
}

void LImagePatchData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			ImagePatchData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) ImagePatchData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LImagePatchData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LImagePatchData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject("ImagePatchData"));
	ImagePatchData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}


//------Value functions:

Value *LImagePatchData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("ImagePatchData"));
	ImagePatchData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

ObjectDef *LImagePatchData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ImagePatchData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"ImagePatchData",
            _("ImagePatchData"),
            _("An image mesh distortion"),
            "class",
            NULL,NULL);

//	sd->pushFunction("FlipColors",_("Flip Colors"),_("Flip the order of colors"), NULL,
//					 NULL);
//	sd->pushVariable("p1", _("p1"), _("The starting point"), NULL,0);

	return sd;
}

Value *LImagePatchData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "p1")) {
//		return new DoubleValue(p1);
//	}


	return NULL;
}

int LImagePatchData::assign(FieldExtPlace *ext,Value *v)
{
//	if (ext && ext->n()==1) {
//		const char *str=ext->e(0);
//		int isnum;
//		double d;
//		if (str) {
//			if (!strcmp(str,"p1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p1=d;
//				FindBBox();
//				return 1;
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
int LImagePatchData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
	AffineValue v(m());
	int status=v.Evaluate(func,len,context,parameters,settings,value_ret,log);
	if (status==0) {
		m(v.m());
		return 0;
	}

	return status;
}



//------------------------------- LColorPatchData ---------------------------------------
/*! \class LColorPatchData 
 * \brief Subclassing LaxInterfaces::ColorPatchData
 */


LColorPatchData::LColorPatchData(LaxInterfaces::SomeData *refobj)
{
}

LColorPatchData::~LColorPatchData()
{
}

/*! Provide final pointin() definition.
 */
int LColorPatchData::pointin(flatpoint pp,int pin)
{
	return PatchData::pointin(pp,pin);
}

void LColorPatchData::FindBBox()
{
	ColorPatchData::FindBBox();
}

void LColorPatchData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		ColorPatchData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	ColorPatchData::dump_out(f,indent+2,what,context);
}

void LColorPatchData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			ColorPatchData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) ColorPatchData::dump_in_atts(att,flag,context);
	FindBBox();
}

LaxInterfaces::SomeData *LColorPatchData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LColorPatchData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject("ColorPatchData"));
	ColorPatchData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}


//------Value functions:

Value *LColorPatchData::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("ColorPatchData"));
	ColorPatchData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

ObjectDef *LColorPatchData::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ColorPatchData");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"ColorPatchData",
            _("ColorPatchData"),
            _("A color mesh gradient"),
            "class",
            NULL,NULL);

//	sd->pushFunction("FlipColors",_("Flip Colors"),_("Flip the order of colors"), NULL,
//					 NULL);
//	sd->pushVariable("p1", _("p1"), _("The starting point"), NULL,0);

	return sd;
}

Value *LColorPatchData::dereference(const char *extstring, int len)
{
//	if (extequal(extstring,len, "p1")) {
//		return new DoubleValue(p1);
//	}


	return NULL;
}

int LColorPatchData::assign(FieldExtPlace *ext,Value *v)
{
//	if (ext && ext->n()==1) {
//		const char *str=ext->e(0);
//		int isnum;
//		double d;
//		if (str) {
//			if (!strcmp(str,"p1")) {
//				d=getNumberValue(v, &isnum);
//				if (!isnum) return 0;
//				p1=d;
//				FindBBox();
//				return 1;
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
int LColorPatchData::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
	return -1;
}


//------------------------------------- LImagePatchInterface -------------------------
/*! \class LImagePatchInterface
 * 
 * Subclass LaxInterfaces::ImagePatchInterface so that a change to recurse affects
 * the pool class as well. 
 *
 * \todo *** this is a big hack... need a better way to control interface data drawing,
 *   if there is a draw in place, then a draw on top, then should be a flag to not draw
 *   the one beneath...
 */

LImagePatchInterface::LImagePatchInterface(int nid,Laxkit::Displayer *ndp)
	: ImagePatchInterface(nid,ndp)
{
	style|=IMGPATCHI_POPUP_INFO;
	drawrendermode=1;
	rendermode=3;
}

anInterface *LImagePatchInterface::duplicate(anInterface *dup)
{
	return ImagePatchInterface::duplicate(new LImagePatchInterface(id,NULL));
}

int LImagePatchInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k)
{
	////DBG cerr <<"*****************in LImagePatchInterface::CharInput"<<endl;
	int r=recurse,m=rendermode;
	int cc=ImagePatchInterface::CharInput(ch,buffer,len,state,k);
	if (recurse!=r || m!=rendermode) {
		for (int c=0; c<laidout->interfacepool.n; c++) {
			if (!strcmp(laidout->interfacepool.e[c]->whattype(),"ImagePatchInterface")) {
				static_cast<ImagePatchInterface *>(laidout->interfacepool.e[c])->recurse=recurse;
				static_cast<ImagePatchInterface *>(laidout->interfacepool.e[c])->rendermode=rendermode;
				break;
			}
		}
	}
	if (cc==1) return 1;
	return cc;
}


//! Returns this, but count is incremented.
Value *LImagePatchInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LImagePatchInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ImagePatchInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"ImagePatchInterface",
            _("Image Patch Interface"),
            _("Image Patch Interface"),
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
//int LImagePatchInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
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
int LImagePatchInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LImagePatchInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LImagePatchInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	ImagePatchInterface::dump_out(f,indent,what,context);
}

void LImagePatchInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	ImagePatchInterface::dump_in_atts(att,flag,context);
}

//------------------------------------- LColorPatchInterface -------------------------
/*! \class LColorPatchInterface
 * 
 * Subclass LaxInterfaces::ColorPatchInterface so that a change to recurse affects
 * the pool class as well. 
 *
 * \todo *** this is a big hack... need a better way to control interface data drawing,
 *   if there is a draw in place, then a draw on top, then should be a flag to not draw
 *   the one beneath...
 */

LColorPatchInterface::LColorPatchInterface(int nid,Laxkit::Displayer *ndp)
	: ColorPatchInterface(nid,ndp)
{
	drawrendermode=2;
	rendermode=2;
}

anInterface *LColorPatchInterface::duplicate(anInterface *dup)
{
	return ColorPatchInterface::duplicate(new LColorPatchInterface(id,NULL));
}

int LColorPatchInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k)
{
	////DBG cerr <<"*****************in LColorPatchInterface::CharInput"<<endl;
	int r=recurse,m=rendermode;
	int cc=ColorPatchInterface::CharInput(ch,buffer,len,state,k);
	if (recurse!=r || m!=rendermode) {
		for (int c=0; c<laidout->interfacepool.n; c++) {
			if (!strcmp(laidout->interfacepool.e[c]->whattype(),"ColorPatchInterface")) {
				static_cast<ColorPatchInterface *>(laidout->interfacepool.e[c])->recurse=recurse;
				static_cast<ColorPatchInterface *>(laidout->interfacepool.e[c])->rendermode=rendermode;
				break;
			}
		}
	}
	if (cc==1) return 1;
	return cc;
}

//! Returns this, but count is incremented.
Value *LColorPatchInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *LColorPatchInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ColorPatchInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"ColorPatchInterface",
            _("Image Patch Interface"),
            _("Image Patch Interface"),
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
//int LColorPatchInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
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
int LColorPatchInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LColorPatchInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LColorPatchInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	ColorPatchInterface::dump_out(f,indent,what,context);
}

void LColorPatchInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	ColorPatchInterface::dump_in_atts(att,flag,context);
}


} //namespace Laidout

