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
// Copyright (C) 2014 by Tom Lechner
//
#ifndef OBJECTTREE_H
#define OBJECTTREE_H


#include <lax/treeselector.h>
#include "dataobjects/objectcontainer.h"


namespace Laidout {


class ObjectTree : public Laxkit::TreeSelector
{
  public:
	ObjectTree(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *mes);
	//ObjectTree(Value *value);
	virtual ~ObjectTree();
	virtual const char *whattype() { return "ObjectTree"; }

	virtual void UseContainer(ObjectContainer *container);
};


} //namespace Laidout


#endif

