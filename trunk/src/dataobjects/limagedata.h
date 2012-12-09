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
#ifndef LIMAGEDATA_H
#define LIMAGEDATA_H

#include <lax/interfaces/imageinterface.h>
#include "drawableobject.h"

//------------------------------- LImageData ---------------------------------------


namespace Laidout {


class LImageData : public DrawableObject, public LaxInterfaces::ImageData
{
  public:
	//ImageImportFilter *importer;
	//char *sourcefile;
	//RefPtrStack<TextObject> sourcetext;

	LImageData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LImageData();
	virtual const char *whattype() { return "ImageData"; }
	virtual void FindBBox();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup=NULL);
};



} //namespace Laidout

#endif


