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

#include "objectfilter.h"
#include "drawableobject.h"

#include <lax/refptrstack.cc>


namespace Laidout {


//------------------------ ObjectFilter ------------------------

/*! \class ObjectFilter
 * Container for filter nodes that transform a DrawableObject.
 * These are meant to be owned by either a DrawableObject or a Resource (via parent variable).
 * If neither of these, it is a free, local object.
 * ObjectFilter is implemented as a NodeGroup, with a few convenience functions
 * specific to operating on parent objects.
 *
 * This could be blur, contrast, saturation, distort, etc. 
 *
 * This could also be a adapted to be a dynamic
 * filter that depends on some resource, such as a global integer resource
 * representing the current frame, that might
 * adjust an object's matrix based on keyframes, for instance.
 *
 */


ObjectFilter::ObjectFilter(anObject *nparent)
{
	parent = nparent;
}

ObjectFilter::~ObjectFilter()
{
}

anObject *ObjectFilter::ObjectOwner()
{
	return parent;
}

NodeBase *ObjectFilter::Duplicate()
{
	return NodeGroup::Duplicate();
	//ObjectFilter *node = new ObjectFilter(NULL);
	//node->DuplicateBase(this);
	//return node;
}

/*! Nothing really to do, as updates should happen automatically as things change.
 */
int ObjectFilter::Update()
{
	return GetStatus();
}

int ObjectFilter::GetStatus()
{
	return NodeBase::GetStatus();
}

Laxkit::anObject *ObjectFilter::FinalObject()
{
	NodeProperty *prop = output->FindProperty("Out");
	return dynamic_cast<DrawableObject*>(prop->GetData());
}

/*! Call FindInterfaceNodes(NULL) to find all. Recursively calls any nested ObjectFilters.
 */
int ObjectFilter::FindInterfaceNodes(NodeGroup *group)
{
//	if (!group) {
//		interfacenodes.flush();
//		group = this;
//	}
//
//	NodeBase *node;
//	InterfaceNode *inode;
//	NodeGroup *gnode;
//
//	for (int c=0; c<group->nodes.n; c++) {
//		node = group->nodes.e[c]
//		inode = dynamic_cast<InterfaceNode*>(node);
//		if (inode) interfacenodes.push(inode);
//
//		gnode = dynamic_cast<NodeBase*>(node);
//		if (gnode) FindInterfaceNodes(gnode);
//	}

	int n=0;
	return n;
}


} //namespace Laidout

