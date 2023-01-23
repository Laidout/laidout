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
#ifndef LCAPTIONDATA_H
#define LCAPTIONDATA_H

#include <lax/interfaces/captioninterface.h>
#include "drawableobject.h"



namespace Laidout {


//------------------------------- LCaptionData ---------------------------------------
class LCaptionData : public DrawableObject,
				     public LaxInterfaces::CaptionData
{
  public:
	//ImageImportFilter *importer;
	//char *sourcefile;
	//RefPtrStack<TextObject> sourcetext;

	LCaptionData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LCaptionData();
	virtual const char *whattype() { return "CaptionData"; }
	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual void touchContents();
	virtual int pointin(Laxkit::flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);


	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};



} //namespace Laidout

#endif


