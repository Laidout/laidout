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
// Copyright (C) 2010-2012 by Tom Lechner
//


#include <lax/transformmath.h>
#include "drawableobject.h"
#include "../laidout.h"
#include "../drawdata.h"

#include <lax/refptrstack.cc>

#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


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
class ObjectFilter : virtual public Laxkit::anObject
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
};

ObjectFilter::ObjectFilter()
{
	filtername=NULL;
}

ObjectFilter::~ObjectFilter()
{
	if (filtername) delete[] filtername;
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


DrawObjectChain::DrawObjectChain(int id)
{
	chain_id=id;
	object=NULL;
	next=prev=NULL;
}

DrawObjectChain::~DrawObjectChain()
{
	if (next) next->prev=prev;
	if (prev) prev->next=next;
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


DrawableObject::DrawableObject(LaxInterfaces::SomeData *refobj)
{
	clip=NULL;
	wrap_path=inset_path=NULL;
	autowrap=autoinset=0;
	locks=0;
	id=NULL;
}

/*! Will detach this object from any object chain. It is assumed that other objects in
 * the chain are referenced elsewhere, so the other objects in the chain are NOT deleted here.
 */
DrawableObject::~DrawableObject()
{
	if (id) delete[] id;

	if (clip) clip->dec_count();
	if (wrap_path) wrap_path->dec_count();
	if (inset_path) inset_path->dec_count();

	if (chains.n) chains.flush();
}

LaxInterfaces::SomeData *DrawableObject::duplicate(LaxInterfaces::SomeData *dup)
{
	DrawableObject *d=dynamic_cast<DrawableObject*>(dup);
	if (dup && !d) return NULL; //not a drawableobject!
	if (!dup) {
		dup=newObject("Group");
		d=dynamic_cast<DrawableObject*>(dup);
	}

	//assign new id
	//parent??
	//clip
	//wrap
	//inset
	//ignore chains
	//meta? and tags
	//iohints

	 //filters
	d->alpha=alpha;
	d->blur=blur;

	 //kids
	SomeData *obj;
	DrawableObject *dobj;
	for (int c=0; c<kids.n; c++) {
		obj=kids.e[c]->duplicate();
		dobj=dynamic_cast<DrawableObject*>(obj);
		d->push(obj);
		dobj->parent=d;
		obj->dec_count();
	}

	return dup;
}

//! Dump out iohints and metadata, if any.
void DrawableObject::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sid nameofobject\n",spc);
		fprintf(f,"%siohints ...       #object level i/o leftovers from importing\n",spc);
		fprintf(f,"%smetadata ...      #object level metadata\n",spc);
		fprintf(f,"%stags tag1 \"tag 2\" #list of string tags\n",spc);
		fprintf(f,"%sfilters           #list of filters\n",spc);
		fprintf(f,"%s  ...\n",spc);
		fprintf(f,"%skids          #child object list\n",spc);
		fprintf(f,"%s  object ImageData #...or any other drawable object\n",spc);
		fprintf(f,"%s    ...\n",spc);
		return;
	}

	if (!isblank(id)) fprintf(f,"%sid %s\n",spc,id);

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

	if (NumberOfTags()) {
		char *str=GetAllTags();
		fprintf(f,"%stags %s\n",spc, str);
		delete[] str;
	}

//	if (filters.n) {
//		fprintf(f,"%sfilters\n",spc);
//		for (int c=0; c<filters.n; c++) {
//			fprintf(f,"%s  filter\n",spc);
//			filters.e[c]->dump_out(f,indent+4,what,context);
//		}
//	}

	if (kids.n) {
		fprintf(f,"%skids\n",spc);
		dump_out_group(f,indent+2,what,context);
	}
}

//! Read in iohints and metadata, if any.
void DrawableObject::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	char *name,*value;
	int foundconfig=0;
	if (!strcmp(whattype(),"Group")) foundconfig=-1;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			makestr(id,value);

		} else if (!strcmp(name,"iohints")) {
			if (iohints.attributes.n) iohints.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				iohints.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(name,"metadata")) {
			if (metadata.attributes.n) metadata.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				metadata.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		//} else if (!strcmp(name,"filters")) {

		} else if (!strcmp(name,"tags")) {
			InsertTags(value,0);

		} else if (!strcmp(name,"kids")) {
			dump_in_group_atts(att->attributes.e[c], flag,context);

		} else if (foundconfig==0 && !strcmp(name,"config")) {
			foundconfig=1;

		}
	}

	 //is old school group
	if (foundconfig==-1) dump_in_group_atts(att, flag,context);
}

//! Push obj onto the stack. (new objects only!)
/*! 
 * No previous existence
 * check is done here. For that, use pushnodup().
 */
int DrawableObject::push(LaxInterfaces::SomeData *obj)
{
	if (!obj) return -1;
	return kids.push(obj);
}

//! Push obj onto the stack only if it is not already there.
/*! If the item is already on the stack, then its count is not
 * incremented.
 */
int DrawableObject::pushnodup(LaxInterfaces::SomeData *obj)
{
	if (!obj) return -1;
	int c=kids.pushnodup(obj,-1);
	return c;
}

//! Pop d, but do not decrement its count.
/*! Returns 1 for item popped, 0 for not.
 */
int DrawableObject::popp(LaxInterfaces::SomeData *d)
{
	return kids.popp(d);
}

//! Return the popped item. Does not change its count.
LaxInterfaces::SomeData *DrawableObject::pop(int which)
{
	return kids.pop(which);
}

//! Remove item with index i. Return 1 for item removed, 0 for not.
int DrawableObject::remove(int i)
{
	return kids.remove(i);
}

//! Pops item i1, then pushes it so that it is in position i2. 
/*! Return 1 for slide happened, else 0.
 *
 * Does not tinker with the object's count.
 */
int DrawableObject::slide(int i1,int i2)
{
	if (i1<0 || i1>=kids.n || i2<0 || i2>=kids.n) return 0;
	LaxInterfaces::SomeData *obj;
	kids.pop(obj,i1); //does nothing to count 
	kids.push(obj,-1,i2); //incs count
	obj->dec_count(); //remove the additional count
	return 1;
}

//! Decrements all objects in kids vie kids.flush().
void DrawableObject::flush()
{
	kids.flush();
}


//! Append all the bboxes of the objects.
void DrawableObject::FindBBox()
{
	maxx=minx-1; maxy=miny-1;
	if (!kids.n) return;
	for (int c=0; c<kids.n; c++) {
		addtobounds(kids.e[c]->m(),kids.e[c]);
	}
}

//! Check the point against all objs.
/*! \todo *** this is broken! ignores the obj transform
 */
int DrawableObject::pointin(flatpoint pp,int pin)
{ 
	if (!kids.n) return 0;
	if (!selectable) return 0;
	flatpoint p(((pp-origin())*xaxis())/(xaxis()*xaxis()), 
		        ((pp-origin())*yaxis())/(yaxis()*yaxis()));
	for (int c=0; c<kids.n; c++) {
		if (kids.e[c]->pointin(p,pin)) return 1;
	}
	return 0;
}


//! Normally return kids.n, but return 0 if the object has locked kids.
int DrawableObject::n()
{
	//if (SomeData::flags&(SOMEDATA_LOCK_KIDS|SOMEDATA_LOCK_CONTENTS)) return 0;
	return kids.n;
}

//! Return object with index i in stack.
/*! Note that the object's count is not changed. If the calling code wants to hang
 * on to the object they should quickly inc_count the object.
 */
LaxInterfaces::SomeData *DrawableObject::e(int i)
{
	if (i<0 || i>=kids.n) return NULL;
	return kids.e[i];
}

Laxkit::anObject *DrawableObject::object_e(int i)
{
	if (i>=0 && i<kids.n) return kids.e[i];
	return NULL;
}

const char *DrawableObject::object_e_name(int i)
{
	return id;
}

const double *DrawableObject::object_transform(int i)
{
	if (i<0 || i>=kids.n) return NULL;
	return kids.e[i]->m();
}

/*! Recognizes locked, visible, prints, then tries to parse elements...
 * Discards all else.
 * The kids should have been flushed before coming here.
 */
void DrawableObject::dump_in_group_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	int nlocked=-1, nvisible=-1, nprints=-1;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"locked")) {
			locked=BooleanAttribute(value);
		} else if (!strcmp(name,"visible")) {
			visible=BooleanAttribute(value);
		} else if (!strcmp(name,"prints")) {
			prints=BooleanAttribute(value);
		} else if (!strcmp(name,"matrix")) {
			double mm[6];
			if (DoubleListAttribute(value,mm,6)==6) m(mm);
		} else if (!strcmp(name,"object")) {
			int n;
			char **strs=splitspace(value,&n);
			if (strs) {
				// could use the number as some sort of object id?
				// currently out put was like: "object 2 ImageData"
				//***strs[0]==that id
				SomeData *data=newObject(n>1?strs[1]:(n==1?strs[0]:NULL));//objs have 1 count
				if (data) {
					data->dump_in_atts(att->attributes[c],flag,context);
					push(data);
					DBG if (!dynamic_cast<DrawableObject*>(data)) cerr <<" --- WARNING! newObject returned a non-DrawableObject"<<endl;
					data->dec_count();
				}
				deletestrs(strs,n);
			} else {
				DBG cerr <<"*** readin blank object for Group..."<<endl;
			}
		} else { 
			DBG cerr <<"Group dump_in:*** unknown attribute!!"<<endl;
		}
	}
	if (nlocked>0)  locked=nlocked;
	if (nvisible>0) visible=nvisible;
	if (nprints>0)  prints=nprints;

	FindBBox();
}

//! Write out the objects.
/*! If what==-1, dump out pseudocode of file format for a group.
 *
 * \todo automate object management, necessary here for what==-1
 */
void DrawableObject::dump_out_group(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sid      #the name of a group. There can be no whitespace in the id\n",spc);
		fprintf(f,"%slocked  #indicates that this group cannot be modified\n",spc);
		fprintf(f,"%svisible #no indicates that this group cannot be seen on screen nor printed out\n",spc);
		fprintf(f,"%sprints  #no indicates that this group can be seen on screen, but cannot be printed\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #affine transform to apply to the whole group\n",spc);
		fprintf(f,"\n%s#Groups contain any number of drawable objects. Here are all the possible such\n",spc);
		fprintf(f,"%s#objects currently installed:\n",spc);
		fprintf(f,"\n%sobject 1 Group\n%s  #...a subgroup...\n",spc,spc);
		SomeData *obj;
		
		//*** hack until auto obj. type insertion done
		const char *objecttypelist[]={
				"Group",
				"ImageData",
				"ImagePatchData",
				"PathsData",
				"GradientData",
				"ColorPatchData",
				"EpsData",
				"MysteryData",
				NULL
			};

		for (int c=0; objecttypelist[c]; c++) {
			if (!strcmp(objecttypelist[c],"Group")) continue;
			fprintf(f,"\n%sobject %s\n",spc,objecttypelist[c]);
			obj=newObject(objecttypelist[c]);
			obj->dump_out(f,indent+2,-1,NULL);
			delete obj;
		}
		return;
	}
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	if (id) fprintf(f,"%sid %s\n",spc,id);
	if (locked) fprintf(f,"%slocked\n",spc);
	if (visible) fprintf(f,"%svisible\n",spc);
	if (prints) fprintf(f,"%sprints\n",spc);
	for (int c=0; c<kids.n; c++) {
		fprintf(f,"%sobject %d %s\n",spc,c,kids.e[c]->whattype());
		kids.e[c]->dump_out(f,indent+2,0,context);
	}
}

//! Find d somewhere within this (it can be kids also). Searches in kids too.
/*! if n!=NULL, then increment n each time findobj is called. So say an object
 * is in group->group->group->kids, then n gets incremented 3 times. If object
 * is in this group, then do not increment n at all.
 *
 * Return the object if it is found, otherwise NULL.
 */
LaxInterfaces::SomeData *DrawableObject::findobj(LaxInterfaces::SomeData *d,int *n)
{
	if (d==this) return d;
	int c;
	for (c=0; c<kids.n; c++) {
		if (kids.e[c]==d) return d;
	}
	SomeData *s;
	DrawableObject *g;
	for (c=0; c<kids.n; c++) {
		s=kids.e[c];
		g=dynamic_cast<DrawableObject*>(s);
		if (!g) continue;
		if (n) (*n)++;
		if (g->findobj(d,n)) {
			return d;
		}
		if (n) (*n)--;
	}
	return NULL;
}

//! Take all the elements in the list which, and put them in a new group at the first index.
/*! If any of which are not in kids, then nothing is changed. If ne<=0 then the which list
 * is assumed to be terminated by a -1.
 *
 * Return 0 for success, or nonzero error.
 */
int DrawableObject::GroupObjs(int ne, int *which)
{
	if (ne<0) {
		ne=0;
		while (which[ne]>=0) ne++;
	}
	
	 // first verify that all in which are in kids
	int c;
	for (c=0; c<ne; c++) if (which[c]<0 || which[c]>=n()) return 1;
	
	DrawableObject *g=new DrawableObject;
	int where,w[ne];
	memcpy(w,which,ne*sizeof(int));
	where=w[0];
	LaxInterfaces::SomeData *d;
	while (ne) {
		d=pop(w[ne-1]); //doesn't change count
		g->push(d); //incs count
		d->dec_count();  //remove extra count
		ne--;
		for (int c2=0; c2<ne; c2++)
			if (w[c2]>w[ne]) w[c2]--;
	}
	g->FindBBox();
	kids.push(g,-1,where); //incs g
	g->dec_count(); //remove initial count
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

//! If element which is a Group, then make its elements direct elements of this, and remove the group.
/*! Return 0 for success, or nonzero error.
 */
int DrawableObject::UnGroup(int which)
{
	if (which<0 || which>=n()) return 1;
	if (strcmp(object_e(which)->whattype(),"Group")) return 1;

	DrawableObject *g=dynamic_cast<DrawableObject*>(kids.pop(which)); //assumes "Group" is a DrawableObject here. does not change count of g
	if (!g) return 1;
	
	SomeData *d;
	double mm[6];
	while (g->n()) {
		d=g->pop(0); //count stays same on d
		transform_mult(mm,g->m(),d->m());
		d->m(mm);
		kids.push(d,-1,which++); //incs d
		d->dec_count(); //remove extra count
	}
	g->dec_count(); //dec count of now empty group
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

//! Ungroup some descendent of this Group.
/*! which is list of indices of subgroup. So say which=={1,3,6},
 * then ungroup the element this->1->3->6, which is a great
 * grandchild of this. All intervening elements must be Group objects
 * as must be the final object. Then the other UnGroup() is called.
 *
 * If n<=0, then which is assumed to be a -1 terminated list.
 *
 *  Return 0 for success, or nonzero error.
 */
int DrawableObject::UnGroup(int n,const int *which)
{
	if (n<=0) {
		n=0;
		while (which[n]!=-1) n++;
	}
	if (n==0) return 1;
	if (*which<0 || *which>=kids.n) return 2;

	SomeData *d=kids.e[*which];
	DrawableObject *g=dynamic_cast<DrawableObject*>(d);
	if (!g) return 3;

	if (n>1) return g->UnGroup(n-1,which+1);
	
	double mm[6];
	while (d=g->pop(0), d) {
		transform_mult(mm,d->m(),g->m());
		d->m(mm);
		kids.push(d,-1,*which+1);
		d->dec_count();
	}
	remove(*which);
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}


