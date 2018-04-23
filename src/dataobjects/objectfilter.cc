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
#include "../interfaces/nodes.h"
#include "drawableobject.h"
#include "lpathsdata.h"
#include "lperspectiveinterface.h"
#include "../language.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/anxapp.h>

//template implementation:
#include <lax/refptrstack.cc>


using namespace LaxInterfaces;


namespace Laidout {

//------------------------ ObjectFilterNode ------------------------

/*! \class ObjectFilterNode
 * Class for a component of an ObjectFilter that has an interface usable in main viewport.
 *
 * Derived classes need to act like a passthrough when IsMuted() in their Update() function.
 */

ObjectFilterNode::ObjectFilterNode()
{
	muted = 0;
}

ObjectFilterNode::~ObjectFilterNode()
{
}

/*! Defualt just return muted.
 */
int ObjectFilterNode::IsMuted()
{
	return muted;
}

/*! Change muted state. If change, derived classes MUST implement the passthrough.
 */
int ObjectFilterNode::Mute(bool yes)
{
	int oldmuted = muted;
	muted = yes;
	if (muted != oldmuted) Update();
	return muted;
}



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


ObjectFilter::ObjectFilter(anObject *nparent, int make_in_outs)
{
	parent = nparent;

	NodeColors *cols = new NodeColors;
	cols->Font(anXApp::app->defaultlaxfont, false);
	InstallColors(cols, 1);

	if (make_in_outs) {

		 //set up barebones filter nodes
		DrawableObject *dobj = dynamic_cast<DrawableObject*>(parent);
		if (dobj) {
			NodeProperty *in  = AddGroupInput ("in",  NULL, NULL);
			NodeProperty *out = AddGroupOutput("out", NULL, NULL);

			in->SetFlag(NodeProperty::PROPF_Label_From_Data, 1);
			in->topropproxy->SetFlag(NodeProperty::PROPF_Label_From_Data, 1);
			in->SetData(dobj, 0);
			out->Label(_("Out"));

			Connect(in->topropproxy, out->frompropproxy);

			NodeBase *from = in ->topropproxy  ->owner;
			NodeBase *to   = out->frompropproxy->owner;

			from->Wrap();
			to  ->Wrap();

			to->x = from->x + from->width*2;

		}
	}
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
	NodeProperty *prop = output->FindProperty("out");
	if (prop) return dynamic_cast<DrawableObject*>(prop->GetData());
	return NULL;
}

/*! Call FindInterfaceNodes(NULL) to find all. Recursively finds any nested ObjectFilters.
 */
int ObjectFilter::FindInterfaceNodes(NodeGroup *group)
{
//   ------brute force search of all nodes, maybe not what we want
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

/*! Get all object filter nodes that contribute to start_here.
 * Return number of filters found.
 */
int ObjectFilter::FindInterfaceNodes(Laxkit::RefPtrStack<ObjectFilterNode> &filternodes, NodeProperty *start_here)
{
	if (start_here == NULL) {
		 //starting from final out of filter
		start_here = FindProperty("out");
		if (!start_here) return 0;
		if (start_here->frompropproxy) start_here = start_here->frompropproxy;
	}

	int n=0;

	 //check owner node
	if (dynamic_cast<ObjectFilterNode*>(start_here->owner)) filternodes.pushnodup(dynamic_cast<ObjectFilterNode*>(start_here->owner));


	 //traverse backward
	if (start_here->connections.n) {
		NodeBase *from = start_here->connections.e[0]->from;

		for (int c=0; c<from->properties.n; c++) {
			NodeProperty *prop = from->properties.e[c];
			if (prop->frompropproxy) prop = prop->frompropproxy;
			if (!prop->IsInput()) continue;

			n += FindInterfaceNodes(filternodes, prop);
		}
	}



	return n;
}


LaxFiles::Attribute *ObjectFilter::dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context)
{
	NodeProperty *in = FindProperty("in");
	in->SetData(NULL,0);
	Attribute *attt = NodeGroup::dump_out_atts(att, what, context);
	in->SetData(dynamic_cast<DrawableObject*>(parent), 0);
	return attt;
}


//------------------------ ObjectFilterNode ------------------------

Laxkit::anObject *newPerspectiveNode(int p, Laxkit::anObject *ref)
{
    return new PerspectiveNode();
}

int RegisterFilterNodes(Laxkit::ObjectFactory *factory)
{
     //--- PerspectiveNode
    factory->DefineNewObject(getUniqueNumber(), "PerspectiveFilter",    newPerspectiveNode,  NULL, 0);


	return 0;
}
 

//----------------------------- ObjectFilterInfo ---------------------------------

/*! \class ObjectFilterInfo
 * Class to pass on to interfaces from ObjectFilterInterface for viewport editing.
 */


ObjectFilterInfo::ObjectFilterInfo(LaxInterfaces::ObjectContext *noc, DrawableObject *nfobj, ObjectFilterNode *nnode)
{
	oc = (noc ? noc->duplicate() : NULL);

	filtered_object = nfobj;
	if (filtered_object) filtered_object->inc_count();
	node = nnode;
	if (node) node->inc_count();
}

ObjectFilterInfo::~ObjectFilterInfo()
{
	if (oc) delete oc;
	if (filtered_object) filtered_object->dec_count();
	if (node) node->dec_count();
}



} //namespace Laidout

