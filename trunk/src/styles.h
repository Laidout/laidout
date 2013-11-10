//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef STYLES_H
#define STYLES_H

#include <lax/anobject.h>
#include <lax/refptrstack.h>
#include <cstdio>
#include <lax/dump.h>
#include <lax/errorlog.h>

#include "fieldplace.h"
#include "calculator/values.h"




namespace Laidout {


//------------------------------------- Style -------------------------------------------

#define STYLE_READONLY
#define STYLE_NO_EDIT

class Style : virtual public Laxkit::anObject, 
			  virtual public LaxFiles::DumpUtility
{
 protected:
	char *stylename; // note this is not a variable name, but it is an instance, 
					 // it would be in a list of styles, like "Bold Body", and the
					 // ObjectDef name/Name might be charstyle/"Character Style"
 public:
	ObjectDef *styledef;
	Style *basedon;
//	FieldMask fieldmask; // is mask of which values are defined in this Style, and would
//						 // preempt fields from basedon
	Style();
	Style(ObjectDef *sdef,Style *bsdon,const char *nstn);
	virtual ~Style();
	virtual Style *duplicate(Style *s=NULL)=0;
	virtual const char *Stylename() { return stylename; }
	virtual int Stylename(const char *nname);

	virtual ObjectDef *makeObjectDef() = 0;
	virtual ObjectDef *GetObjectDef();
	virtual int getNumFields();
	virtual  ObjectDef *FieldInfo(int i);
	virtual const char *FieldName(int i);

	 // these return a mask of what changes when you set the specified value.
	 // set must ask the styledef if it can really set that field, 
	 // 	the ObjectDef returns a mask of what else changes?????***
	 // set must create the field in *this if it does not exist
	 // get should indicate whether the found value is in *this or a basedon
	//maybe:
//	virtual void *dereference(const char *extstr,int copy) { return NULL; }//***
//	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret) { return 1; } //***=0
//	virtual int set(const char *ext,Value *val,FieldMask *mask_ret) { return 1; } //***=0
	
};

class EnumStyle : public Style
{
 public:
	Laxkit::NumStack<int> ids;
	Laxkit::PtrStack<char> names;

	EnumStyle();
	virtual ObjectDef *makeObjectDef();
	virtual Style *duplicate(Style *s=NULL);
	virtual int add(const char *nname,int nid=-1);
	virtual const char *name(int Id);
	virtual int id(const char *Name);
	virtual int num() { return names.n; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context) {}
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context) {}
};






} // namespace Laidout

#endif

