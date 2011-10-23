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
// Copyright (C) 2010-2011 by Tom Lechner
//



//----------------------------- ObjectFilter ---------------------------------
/*! \class ObjectFilter
 * \brief Class that modifies any DrawableObject somehow.
 *
 * This could be blur, contrast, saturation, etc. 
 *
 * This could also be a adapted to be a dynamic
 * filter that depends on some resource, such as a global integer resource
 * representing the current frame, that might
 * adjust an object's matrix based on keyframes, for instance.
 *
 * Every object can have any number of filters applied to it. Filters behave in
 * a manner similar to svg filters. They can specify the input source(s), and output target,
 * which can then be the input of another filter.
 *
 * \todo it would be nice to support all the built in svg filters, and additionally
 *   image warping as a filter.
 */
class ObjectFilter : public Laxkit::anObject
{
 public:
	char *filtername;

	PtrStack<ObjectFilter> inputs;
	ObjectFilter *output;

	RefPtrStack<Laxkit::anObject> dependencies; //other resources, not filters in filter tree
	virtual int RequiresRasterization() = 0; //whether object contents readonly while filter is on
	virtual double *FilterTransform() = 0; //additional affine transform to apply to object's transform
	virtual LaxInterfaces::anInterface *Interface() = 0; //optional editing interface

	ObjectFilter();
	virtual ~ObjectFilter();
}

ObjectFilter::ObjectFilter()
{
	filtername=NULL;
}

ObjectFilter::~ObjectFilter()
{
	if (filtername) delete[] filtername;
}


//----------------------------- DrawableObject ---------------------------------
/*! \class DrawableObject
 * \brief base of all drawable Laidout objects.
 *
 * The Laxkit interfaces get the elements of DrawableObject
 *
 * The object may or may not have its own clip path or mask, or a wraparound
 * or inset path. The inset path is the area it defines in which streams may be
 * laid inside. If these things are not specified, then they are generated 
 * automatically.
 */


DrawableObject::DrawableObject()
{
	clip=NULL;
	wraparound_path=inset_path=NULL;
	autowrap=autoinset=0;
	locks=0;

	next=prev=NULL;
}

/*! Will detach this object from any object chain. It is assumed that other objects in
 * the chain are referenced elsewhere, so the other objects in the chain are NOT deleted here.
 */
DrawableObject::~DrawableObject()
{
	if (clip) clip->dec_count();
	if (wraparound_path) wraparound_path->dec_count();
	if (inset_path) inset_path->dec_count();

	if (next) next->prev=prev;
	if (prev) prev->next=next;
}

//! Dump out iohints and metadata, if any.
void DrawableObject::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%siohints ...   #object level i/o leftovers from importing\n",spc);
		fprintf(f,"%smetadata ...  #object level metadata\n",spc);
		return;
	}

	 // dump notes/meta data
	if (metadata.attributes.n) {
		fprintf(f,"%smetadata\n",spc);
		metadata.dump_out(f,indent+2);
	}
	
	 // dump iohints if any
	if (iohints.attributes.n) {
		fprintf(f,"%siohints\n",spc);
		iohints.dump_out(f,indent+2);
	}
}

//! Read in iohints and metadata, if any.
void DrawableObject::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"iohints")) {
			if (iohints.attributes.n) iohints.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				iohints.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(name,"metadata")) {
			if (metadata.attributes.n) metadata.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				metadata.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);
		}
	}
}

//! Return the number of children of this object.
/*! Any drawable object can have children attached to it. The default is only
 * for the kids to follow the transform of the parent. The parent can
 * optionally clip its children based on GetArea().
 */
int DrawableObject::n()
{
	return subobjects.n;
}

Laxkit::anObject *DrawableObject::object_e(int i)
{
	if (i>=0 && i<subobjects.n) return subobjects.e[c];
	return NULL;
}



//---------------------------------- DrawObjectChain ---------------------------------
/*! \class DrawObjectChain
 * \brief Class to link objects together.
 *
 * Chains can be used in various ways. The most common is linking text frames. Also,
 * they can be used to link imported images into a single chain. Say you import pages of a multipage pdf.
 * A chain can preserve the order of imported pages, and if you reimport later, you can
 * automatically replace that chain with the new one. This can be useful for updating externally defined content.
 *
 * Further food for thought is using chains to define connector networks, but that might be overkill.
 * More food for thought would be using chains for special filter application, or for skeleton manipulation.
 *
 * Please note that DrawableObjects increment and decrement DrawObjectChains, but NOT the other way around.
 * The chain only has a plain pointer to the objects. Thus the objects are really parents, and the chain is a child.
 */


DrawObjectChain::DrawObjectChain()
{
	id=NULL;
}

DrawObjectChain::~DrawObjectChain()
{
	if (id) delete[] id;
}






