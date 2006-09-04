//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef OBJECTCONTAINER_H
#define OBJECTCONTAINER_H


#include <lax/anobject.h>
#include "styles.h"

//------------------------------ ObjectContainer ----------------------------------

class ObjectContainer : virtual public Laxkit::anObject
{
 public:
	virtual int contains(Laxkit::anObject *d,FieldPlace &place);
	virtual Laxkit::anObject *getanObject(FieldPlace &place,int offset=0);
	virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, Laxkit::anObject **d=NULL);
	virtual int n()=0;
	virtual Laxkit::anObject *object_e(int i)=0;
};








#endif

