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
// Copyright (C) 2013 by Tom Lechner
//

#include "lsomedataref.h"
#include "datafactory.h"
#include "../core/stylemanager.h"
#include "../language.h"

#include <lax/misc.h>


namespace Laidout {


//------------------------------- LSomeDataRef ---------------------------------------
/*! \class LSomeDataRef
 * \brief Redefined LaxInterfaces::SomeDataRef (clones) to subclass DrawableObject.
 */

LSomeDataRef::LSomeDataRef(LaxInterfaces::SomeData *refobj)
  : SomeDataRef(refobj)
{
}

LSomeDataRef::~LSomeDataRef()
{
}

const char *LSomeDataRef::Id()
{
    if (!nameid) {
        if (object_idstr) makestr(nameid,object_idstr);
        else {
            nameid=Laxkit::make_id("Clone");
            makestr(object_idstr,nameid);
        }
    }
    return nameid;
}

	
void LSomeDataRef::FindBBox()
{
	SomeDataRef::FindBBox();
}

void LSomeDataRef::ComputeAABB(const double *transform, DoubleBBox &box)
{
	SomeDataRef::ComputeAABB(transform, box);
	DrawableObject::ComputeAABB(transform, box);
}

/*! Provide final pointin() definition.
 */
int LSomeDataRef::pointin(flatpoint pp,int pin)
{
	return SomeDataRef::pointin(pp,pin);
}


void LSomeDataRef::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		SomeDataRef::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	SomeDataRef::dump_out(f,indent+2,what,context);
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LSomeDataRef::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			SomeDataRef::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) SomeDataRef::dump_in_atts(att,flag,context);
}

Value *LSomeDataRef::duplicate()
{
	SomeData *dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("SomeDataRef"));
	SomeDataRef::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dynamic_cast<Value*>(dup);
}

LaxInterfaces::SomeData *LSomeDataRef::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LSomeDataRef*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=dynamic_cast<SomeData*>(LaxInterfaces::somedatafactory()->NewObject("SomeDataRef"));
	SomeDataRef::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

ObjectDef *LSomeDataRef::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("SomeDataRef");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	ObjectDef *affinedef=stylemanager.FindDef("Affine");
	sd=new ObjectDef(affinedef,
			"SomeDataRef",
            _("SomeDataRef"),
            _("A linked clone of an object"),
            "class",
            NULL,NULL);

	sd->pushVariable("object", _("Object"), _("The cloned object"), "any",0, NULL,0);

	return sd;
}

Value *LSomeDataRef::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "object")) {
		if (thedata) return new ObjectValue(thedata);
		return NULL;
	}

	return NULL;
}

int LSomeDataRef::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
		const char *str=ext->e(0);
		if (str) {
			if (!strcmp(str,"object")) {
				if (dynamic_cast<DrawableObject*>(v)) {
					Set(dynamic_cast<SomeData*>(v), 0);
					return 1;
				} if (dynamic_cast<ObjectValue*>(v) && dynamic_cast<DrawableObject*>(dynamic_cast<ObjectValue*>(v)->object)) {
					Set(dynamic_cast<SomeData*>(dynamic_cast<ObjectValue*>(v)->object), 0);
					return 1;
				}
				return 1;

			//} else if (!strcmp(str,"width")) { <-- these are read only
			//} else if (!strcmp(str,"height")) {
			}
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

//int LSomeDataRef::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
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
//
//	return -1;
//}



//------------------------------- LBleedProxy ---------------------------------------
/*! \class LBleedProxy
 * A type of LSomeDataRef to hold references to objects that bleed across pages.
 * Ideally, this enables easy selection of bleed objects on other pages and still use
 * mostly the same code to change.
 */

LBleedProxy::LBleedProxy(LaxInterfaces::SomeData *refobj)
  : LSomeDataRef(refobj)
{
}


} //namespace Laidout

