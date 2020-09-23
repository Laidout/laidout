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
#include "../nodes/nodeinterface.h"


namespace Laidout {


//------------------------ RegisterFilterNodes ------------------------

int RegisterFilterNodes(Laxkit::ObjectFactory *factory);


//------------------------ ObjectFilterNode ------------------------

class ObjectFilterNode : public NodeBase
{
  public:
    ObjectFilterNode();
    virtual ~ObjectFilterNode();

    virtual LaxInterfaces::anInterface *ObjectFilterInterface() = 0;
    virtual DrawableObject *ObjectFilterData() = 0;
};


//----------------------------- ObjectFilter ---------------------------------

class ObjectFilter : public NodeGroup
{
  public:
	anObject *parent; //assume parent owns *this

	//RefPtrStack<NodeBase> interfacenodes;

	//char *filtername;
	//Laxkit::RefPtrStack<Laxkit::anObject> dependencies; //other resources, not filters in filter tree

	ObjectFilter(Laxkit::anObject *nparent, int make_in_outs);
	virtual ~ObjectFilter();
	virtual anObject *ObjectOwner();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	virtual int SetParent(anObject *newparent);
	virtual Laxkit::anObject *FinalObject();
	virtual int FindInterfaceNodes(NodeGroup *group);
	virtual int FindInterfaceNodes(Laxkit::RefPtrStack<ObjectFilterNode> &filternodes, NodeProperty *start_here=NULL);

    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);
};


//----------------------------- ObjectFilterInfo ---------------------------------

class ObjectFilterInfo : public NodeGroup
{
  public:
	//DrawableObject *object; //object that owns the filter
	DrawableObject *filtered_object;
	LaxInterfaces::ObjectContext *oc;
	ObjectFilterNode *node;	//node home to filtered_object

	ObjectFilterInfo(LaxInterfaces::ObjectContext *noc, DrawableObject *nfobj, ObjectFilterNode *nnode);
	virtual ~ObjectFilterInfo();
};


} //namespace Laidout

#endif

