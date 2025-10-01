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
// Copyright (C) 2021 by Tom Lechner
//
#ifndef LELLIPSEDATA_H
#define LELLIPSEDATA_H

#include <lax/interfaces/ellipseinterface.h>
#include "drawableobject.h"



namespace Laidout {


//------------------------------- LEllipseData ---------------------------------------
class LEllipseData : public DrawableObject,
				   public LaxInterfaces::EllipseData
{
  public:
	//ImageImportFilter *importer;
	//char *sourcefile;
	//RefPtrStack<TextObject> sourcetext;

	LEllipseData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LEllipseData();
	virtual const char *whattype() { return "EllipseData"; }
	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual int pointin(Laxkit::flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual LaxInterfaces::SomeData *duplicateData(LaxInterfaces::SomeData *dup);

	virtual LaxInterfaces::SomeData *EquivalentObject();
	virtual Value *duplicateValue();
	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};



} //namespace Laidout

#endif


