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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef LGRADIENTDATA_H
#define LGRADIENTDATA_H

#include <lax/interfaces/gradientinterface.h>
#include "drawableobject.h"



namespace Laidout {



//------------------------------- LGradientData ---------------------------------------

class LGradientData : public DrawableObject,
					  public LaxInterfaces::GradientData
{
  public:
	LGradientData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LGradientData();
	virtual const char *whattype() { return "GradientData"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
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


//------------------------------- LGradientInterface --------------------------------
class LGradientInterface : public LaxInterfaces::GradientInterface,
						   public Value
{
 protected:
 public:
	LGradientInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "GradientInterface"; }
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);

	//from value
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} //namespace Laidout

#endif

