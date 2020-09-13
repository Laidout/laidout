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
#ifndef LPATHSDATA_H
#define LPATHSDATA_H

#include <lax/interfaces/pathinterface.h>
#include "drawableobject.h"




namespace Laidout {


//------------------------------- LPathsData ---------------------------------------

class LPathsData : public DrawableObject,
				   public LaxInterfaces::PathsData
{
  public:
	LPathsData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LPathsData();
	virtual const char *whattype() { return "PathsData"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
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


//------------------------------- LPathInterface --------------------------------
class LPathInterface : public LaxInterfaces::PathInterface,
					   public Value
{
 protected:
 public:
	LPathInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "PathInterface"; }
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);

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

