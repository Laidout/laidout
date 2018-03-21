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
// Copyright (C) 2016 by Tom Lechner
//
#ifndef OBJECTFILTER_H
#define OBJECTFILTER_H



#include <lax/interfaces/aninterface.h>
#include <lax/refptrstack.h>


namespace Laidout {

//----------------------------- ObjectFilter ---------------------------------
class ObjectFilter : virtual public Laxkit::anObject
{
 public:
	char *filtername;

	Laxkit::PtrStack<ObjectFilter> inputs;
	Laxkit::PtrStack<ObjectFilter> outputs; 
	Laxkit::RefPtrStack<Laxkit::anObject> dependencies; //other resources, not filters in filter tree

	virtual int RequiresRasterization() = 0; //whether object contents readonly while filter is on
	virtual double *FilterTransform() = 0; //additional affine transform to apply to object's transform
	virtual LaxInterfaces::anInterface *Interface() = 0; //optional editing interface

	ObjectFilter();
	virtual ~ObjectFilter();
};


} //namespace Laidout

#endif

