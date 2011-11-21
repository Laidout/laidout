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
// Copyright (C) 2004-2010 by Tom Lechner
//


#include "laidout.h"
#include "group.h"
#include "drawdata.h"
#include <lax/strmanip.h>
#include <lax/transformmath.h>

#include <lax/refptrstack.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

//--------------------- Group ------------------------------

/*! \class Group
 * \brief Basically a group of objects
 * 
 * You must not access objs straight. Always go through the n(), e(), etc.
 * This is so the local in the stack can be utilized to either delete or
 * increment and decrement objects.
 */
/*! \fn int Group::findindex(LaxInterfaces::SomeData *d)
 * \brief Just does: return objs.findindex(d);
 */
/*! \fn void Group::swap(int i1,int i2)
 * \brief Swap items i1 and i2
 */
/*! \fn const char *Group::whattype()
 * \brief Returns 'Group'.
 */
/*! \var char Group::selectable
 * \brief Whether when cycling through objects the group itself can be selected.
 *
 * For instance, the Page::layers is a Group, but it cannot be modified, or
 * deleted, so it is not selectable.
 */


Group::Group()
{
	blendmode=0;
	locked=0;
	visible=prints=selectable=1; 
}
	
//! Destructor, calls flush() to checkin any objects.
Group::~Group() 
{
	flush();
}

//! Append all the bboxes of the objects.
void Group::FindBBox()
{
	maxx=minx-1; maxy=miny-1;
	if (!objs.n) return;
	for (int c=0; c<objs.n; c++) {
		addtobounds(objs.e[c]->m(),objs.e[c]);
	}
}

//! Check the point against all objs.
/*! \todo *** this is broken! ignores the obj transform
 */
int Group::pointin(flatpoint pp,int pin)
{ 
	if (!objs.n) return 0;
	if (!selectable) return 0;
	flatpoint p(((pp-origin())*xaxis())/(xaxis()*xaxis()), 
		        ((pp-origin())*yaxis())/(yaxis()*yaxis()));
	for (int c=0; c<objs.n; c++) {
		if (objs.e[c]->pointin(p,pin)) return 1;
	}
	return 0;
}

//! Push obj onto the stack. (new objects only!)
/*! 
 * No previous existence
 * check is done here. For that, use pushnodup().
 */
int Group::push(LaxInterfaces::SomeData *obj)
{
	if (!obj) return -1;
	return objs.push(obj);
}

//! Push obj onto the stack only if it is not already there.
/*! If the item is already on the stack, then its count is not
 * incremented.
 */
int Group::pushnodup(LaxInterfaces::SomeData *obj)
{
	if (!obj) return -1;
	int c=objs.pushnodup(obj,-1);
	return c;
}

//! Pop d, but do not decrement its count.
/*! Returns 1 for item popped, 0 for not.
 */
int Group::popp(LaxInterfaces::SomeData *d)
{
	return objs.popp(d);
}

//! Return the popped item. Does not change its count.
LaxInterfaces::SomeData *Group::pop(int which)
{
	return objs.pop(which);
}

//! Remove item with index i. Return 1 for item removed, 0 for not.
int Group::remove(int i)
{
	return objs.remove(i);
}

//! Pops item i1, then pushes it so that it is in position i2. 
/*! Return 1 for slide happened, else 0.
 *
 * Does not tinker with the object's count.
 */
int Group::slide(int i1,int i2)
{
	if (i1<0 || i1>=objs.n || i2<0 || i2>=objs.n) return 0;
	SomeData *obj;
	objs.pop(obj,i1); //does nothing to count 
	objs.push(obj,-1,i2); //incs count
	obj->dec_count(); //remove the additional count
	return 1;
}

//! Decrements all objects in objs vie objs.flush().
void Group::flush()
{
	objs.flush();
}

//! Return object with index i in stack.
/*! Note that the object's count is not changed. If the calling code wants to hang
 * on to the object they should quickly inc_count the object.
 */
LaxInterfaces::SomeData *Group::e(int i)
{
	if (i<0 || i>=objs.n) return NULL;
	return objs.e[i];
}

const double *Group::object_transform(int i)
{
	if (i<0 || i>=objs.n) return NULL;
	return objs.e[i]->m();
}

/*! Recognizes locked, visible, prints, then tries to parse elements...
 * Discards all else.
 * The objs should have been flushed before coming here.
 */
void Group::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	locked=visible=prints=0;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"id")) {
			makestr(id,value);
		} else if (!strcmp(name,"locked")) {
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
	FindBBox();
}

//! Write out the objects.
/*! If what==-1, dump out pseudocode of file format for a group.
 *
 * \todo automate object management, necessary here for what==-1
 */
void Group::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
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
		fprintf(f,"\n%sobject Group\n%s  #...a subgroup...\n",spc,spc);
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
	for (int c=0; c<objs.n; c++) {
		fprintf(f,"%sobject %d %s\n",spc,c,objs.e[c]->whattype());
		objs.e[c]->dump_out(f,indent+2,0,context);
	}
}
//! Find d somewhere within this (it can be this also). Searches in subobjects too.
/*! if n!=NULL, then increment n each time findobj is called. So say an object
 * is in group->group->group->objs, then n gets incremented 3 times. If object
 * is in this group, then do not increment n at all.
 *
 * Return the object if it is found, otherwise NULL.
 */
LaxInterfaces::SomeData *Group::findobj(LaxInterfaces::SomeData *d,int *n)
{
	if (d==this) return d;
	int c;
	for (c=0; c<objs.n; c++) {
		if (objs.e[c]==d) return d;
	}
	Group *g;
	for (c=0; c<objs.n; c++) {
		g=dynamic_cast<Group *>(objs.e[c]);
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
/*! If any of which are not in objs, then nothing is changed. If ne<=0 then the which list
 * is assumed to be terminated by a -1.
 *
 * Return 0 for success, or nonzero error.
 */
int Group::GroupObjs(int ne, int *which)
{
	if (ne<0) {
		ne=0;
		while (which[ne]>=0) ne++;
	}
	
	 // first verify that all in which are in objs
	int c;
	for (c=0; c<ne; c++) if (which[c]<0 || which[c]>=n()) return 1;
	
	Group *g=new Group;
	int where,w[ne];
	memcpy(w,which,ne*sizeof(int));
	where=w[0];
	SomeData *d;
	while (ne) {
		d=pop(w[ne-1]); //doesn't change count
		g->push(d); //incs count
		d->dec_count();  //remove extra count
		ne--;
		for (int c2=0; c2<ne; c2++)
			if (w[c2]>w[ne]) w[c2]--;
	}
	g->FindBBox();
	objs.push(g,-1,where); //incs g
	g->dec_count(); //remove initial count
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

//! If element which is a Group, then make its elements direct elements of this, and remove the group.
/*! Return 0 for success, or nonzero error.
 */
int Group::UnGroup(int which)
{
	if (which<0 || which>=n()) return 1;
	if (strcmp(object_e(which)->whattype(),"Group")) return 1;
	Group *g=dynamic_cast<Group *>(objs.pop(which)); //does not change count of g
	if (!g) return 1;
	
	SomeData *d;
	double mm[6];
	while (g->n()) {
		d=g->pop(0); //count stays same on d
		transform_mult(mm,g->m(),d->m());
		d->m(mm);
		objs.push(d,-1,which++); //incs d
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
int Group::UnGroup(int n,const int *which)
{
	if (n<=0) {
		n=0;
		while (which[n]!=-1) n++;
	}
	if (n==0) return 1;
	if (*which<0 || *which>=objs.n) return 2;
	Group *g=dynamic_cast<Group *>(objs.e[*which]);
	if (!g) return 3;
	if (n>1) return g->UnGroup(n-1,which+1);
	
	SomeData *d;
	double mm[6];
	while (d=g->pop(0), d) {
		transform_mult(mm,d->m(),g->m());
		d->m(mm);
		objs.push(d,-1,*which+1);
		d->dec_count();
	}
	remove(*which);
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

