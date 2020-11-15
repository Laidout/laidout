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
#include "../interfaces/objectfilterinterface.h"
#include "../nodes/nodes.h"
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
}

ObjectFilterNode::~ObjectFilterNode()
{
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


/*! make_in_outs==0 when DrawableObject::dump_in_atts().
 * In that case, we need a stripped down version, and relevant in/out nodes
 * get put in after constructor.
 */
ObjectFilter::ObjectFilter(anObject *nparent, int make_in_outs)
{
	DBG cerr << "ObjectFilter constructor, id: "<<object_id<<", "<<(Id()?Id():"")<<endl;

	parent = nparent;

	NodeColors *cols = new NodeColors;
	cols->Font(anXApp::app->defaultlaxfont, false);
	InstallColors(cols, 1);

	 //set up barebones filter nodes
	if (make_in_outs) {

		NodeProperty *in  = AddGroupInput ("in",  NULL, NULL);
		NodeProperty *out = AddGroupOutput("out", NULL, NULL);

		in->SetFlag(NodeProperty::PROPF_Label_From_Data, 1);
		in->topropproxy->SetFlag(NodeProperty::PROPF_Label_From_Data, 1);
		in->data_is_linked = true; //this is so node->dup doesn't get stuff in endless loop
		out->Label(_("Out"));

		Connect(in->topropproxy, out->frompropproxy);

		NodeBase *from = in ->topropproxy  ->owner;
		NodeBase *to   = out->frompropproxy->owner;

		from->Wrap();
		to  ->Wrap();

		to->x = from->x + from->width*2;

		DrawableObject *dobj = dynamic_cast<DrawableObject*>(parent);
		if (dobj) in->SetData(dobj, 0); //aaa!!! this makes circular ref count!! this is handled by DrawableObject::dec_count
	}
}

ObjectFilter::~ObjectFilter()
{
	DBG cerr << "ObjectFilter destructor, id: "<<object_id<<", "<<(Id()?Id():"")<<endl;
}

anObject *ObjectFilter::ObjectOwner()
{
	return parent;
}

NodeBase *ObjectFilter::Duplicate()
{
	ObjectFilter *newgroup = new ObjectFilter(NULL, 0);
	newgroup->InstallColors(colors, 0);
	DuplicateGroup(newgroup);

//	//set designated output
//	NodeBase *node = FindNodeByType("GroupOutputs", 0);
//	if (node) DesignateOutput(node);
//
//	//set designated output
//	node = FindNodeByType("GroupInputs", 0);
//	if (node) DesignateInput(node);


    return newgroup;
}

int ObjectFilter::SetParent(anObject *newparent)
{
	if (parent == newparent) return 0;
	NodeProperty *in = FindProperty("in");
	in->SetData(dynamic_cast<DrawableObject*>(newparent), 0);
	parent = newparent;
	return 0;
}

/*! Nothing really to do, as updates should happen automatically as things change.
 */
int ObjectFilter::Update()
{
	return NodeBase::Update();
	// return GetStatus();
}

int ObjectFilter::GetStatus()
{
	return NodeBase::GetStatus();
}

Laxkit::anObject *ObjectFilter::FinalObject()
{
	NodeProperty *prop = output->FindProperty("out");
	// clock_t recent = MostRecentIn(nullptr);
	// if (recent > prop->modtime) {
	// 	// filter needs updating
	// }
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
	if (parent) parent->inc_count();
	NodeProperty *in = FindProperty("in");
	in->SetData(NULL,0);
	Attribute *attt = NodeGroup::dump_out_atts(att, what, context);
	in->SetData(dynamic_cast<DrawableObject*>(parent), 0);
	if (parent) parent->dec_count();
	return attt;
}

LaxInterfaces::anInterface *ObjectFilter::AlternateInterface()
{
	return new ObjectFilterInterface(nullptr, -1, nullptr);
}


//------------------------ ObjectFilterNode ------------------------

Laxkit::anObject *newPerspectiveNode(int p, Laxkit::anObject *ref)
{
    return new PerspectiveNode();
}

/*! Register nodes for DrawableObject filters.
 * This is called from SetupDefaultNodeTypes().
 */
int RegisterFilterNodes(Laxkit::ObjectFactory *factory)
{
     //--- PerspectiveNode
    factory->DefineNewObject(getUniqueNumber(), "Filters/PerspectiveFilter",    newPerspectiveNode,  NULL, 0);


	return 0;
}


//----------------------------- ObjectFilterInfo ---------------------------------

/*! \class ObjectFilterInfo
 * Class to pass on to interfaces from ObjectFilterInterface for viewport editing.
 */


ObjectFilterInfo::ObjectFilterInfo(LaxInterfaces::ObjectContext *noc, DrawableObject *nfobj, ObjectFilterNode *nnode, NodeBase *selnode)
{
	oc = (noc ? noc->duplicate() : NULL);

	filtered_object = nfobj;
	if (filtered_object) filtered_object->inc_count();
	node = nnode;
	if (node) node->inc_count();

	selected_node = selnode;
	if (selected_node) selected_node->inc_count();
}

ObjectFilterInfo::~ObjectFilterInfo()
{
	if (oc) delete oc;
	if (filtered_object) filtered_object->dec_count();
	if (node) node->dec_count();
	if (selected_node) selected_node->dec_count();
}



} //namespace Laidout

