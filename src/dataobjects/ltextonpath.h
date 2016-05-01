//
// $Id: limagedata.h 1046 2015-06-14 04:56:26Z tomlechner $
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
// Copyright (C) 2016 by Tom Lechner
//
#ifndef LTEXTONPATH_H
#define LTEXTONPATH_H

#include <lax/interfaces/textonpathinterface.h>
#include "drawableobject.h"



namespace Laidout {


//------------------------------- LTextOnPath ---------------------------------------
class LTextOnPath : public DrawableObject,
				     public LaxInterfaces::TextOnPath
{
  public:
	//ImageImportFilter *importer;
	//char *sourcefile;
	//RefPtrStack<TextObject> sourcetext;

	LTextOnPath(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LTextOnPath();
	virtual const char *whattype() { return "TextOnPath"; }
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
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


