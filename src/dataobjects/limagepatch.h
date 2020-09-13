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
#ifndef LIMAGEPATCH_H
#define LIMAGEPATCH_H

#include "drawableobject.h"
#include <lax/interfaces/imagepatchinterface.h>
#include <lax/interfaces/colorpatchinterface.h>


namespace Laidout {



//------------------------------- LImagePatchData ---------------------------------------
class LImagePatchData : public DrawableObject,
						public LaxInterfaces::ImagePatchData
{
  public:
	LImagePatchData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LImagePatchData();
	virtual const char *whattype() { return "ImagePatchData"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual int pointin(flatpoint pp,int pin=1);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);

	 //from Value:
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};


//------------------------------- LColorPatchData ---------------------------------------
class LColorPatchData : public DrawableObject,
						public LaxInterfaces::ColorPatchData
{
  public:
	LColorPatchData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LColorPatchData();
	virtual const char *whattype() { return "ColorPatchData"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);

	 //from Value:
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};




//------------------------------- LImagePatchInterface ---------------------------------------
class LImagePatchInterface : public LaxInterfaces::ImagePatchInterface,
							 public Value
{
 public:
	LImagePatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "ImagePatchInterface"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k);

	//from value
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
};


//------------------------------- LColorPatchInterface ---------------------------------------
class LColorPatchInterface : public LaxInterfaces::ColorPatchInterface,
							 public Value
{
 public:
	LColorPatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "ColorPatchInterface"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k);

	//from value
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
};


} //namespace Laidout

#endif

