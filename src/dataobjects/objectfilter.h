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



#include <lax/refptrstack.h>
#include "../interfaces/nodeinterface.h"


namespace Laidout {

//----------------------------- ObjectFilter ---------------------------------

class ObjectFilter : public NodeGroup
{
  public:
	anObject *parent; //assume parent owns *this

	//RefPtrStack<NodeBase> interfacenodes;

	//char *filtername;
	//Laxkit::RefPtrStack<Laxkit::anObject> dependencies; //other resources, not filters in filter tree

	ObjectFilter(Laxkit::anObject *nparent);
	virtual ~ObjectFilter();
	virtual anObject *ObjectOwner();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	virtual Laxkit::anObject *FinalObject();
	virtual int FindInterfaceNodes(NodeGroup *group);
};


} //namespace Laidout

#endif

