//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/****************** group.cc ********************/

#include "laidout.h"
#include "group.h"
#include "drawdata.h"
#include <lax/strmanip.h>
#include <lax/transformmath.h>

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
 * 
 * ***perhaps migrate Group to some kind of GroupData class of Laxkit?
 * groups are perhaps too specialized..
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
	flatpoint p(((pp-origin())*xaxis())/(xaxis()*xaxis()), 
		        ((pp-origin())*yaxis())/(yaxis()*yaxis()));
	for (int c=0; c<objs.n; c++) {
		if (objs.e[c]->pointin(p,pin)) return 1;
	}
	return 0;
}

//! Push obj onto the stack. (new objects only!)
/*! If local==1 then obj is delete'd when remove'd from objs.
 * Any other local value means the obj is managed with its inc_count() and dec_count() functions,
 * and its count is incremented here.
 *
 * No previous existence
 * check is done here. For that, use pushnodup().
 */
int Group::push(LaxInterfaces::SomeData *obj,int local)
{
	if (!obj) return -1;
	if (local==0) obj->inc_count();
	return objs.push(obj,(local==1?1:0));
}

//! Push obj onto the stack only if it is not already there.
/*! If the item is already on the stack, then its count is not
 * incremented, and local is ignored.
 *
 * If local==1 then obj is delete'd when remove'd from this->objs.
 * Any other local value means the obj is managed with its inc_count() and dec_count() functions,
 * and its count is incremented here.
 */
int Group::pushnodup(LaxInterfaces::SomeData *obj,int local)
{
	if (!obj) return -1;
	int c=objs.pushnodup(obj,(local==1?1:0));
	//if was there, then do not increase count, otherwise:
	if (c<0 && local==0) obj->inc_count();
	return c;
}

//! Pop d, but do not decrement its count.
/*! Returns 1 for item popped, 0 for not.
 *
 * Puts the object's local value in local.
 */
int Group::popp(LaxInterfaces::SomeData *d,int *local)
{
	return objs.popp(d,local);
}

//! Return the popped item. Does not change its count.
LaxInterfaces::SomeData *Group::pop(int which,int *local)//local=NULL
{
	return objs.pop(which,local);
}

//! Remove item with index i. Return 1 for item removed, 0 for not.
/*! This will decrement the object's count by 1 if its local value is 0.
 */
int Group::remove(int i)
{
	int local;
	SomeData *obj;
	objs.pop(obj,i,&local);
	if (obj) {
		if (local==0) obj->dec_count();
		else if (local==1) delete obj;
		else if (local==2) delete[] obj;
		return 1;
	}
	return 0;
}

//! Pops item i1, then pushes it so that it is in position i2. 
/*! Return 1 for slide happened, else 0.
 *
 * Does not tinker with the object's count.
 */
int Group::slide(int i1,int i2)
{
	if (i1<0 || i1>=objs.n || i2<0 || i2>=objs.n) return 0;
	int local;
	SomeData *obj;
	objs.pop(obj,i1,&local);
	objs.push(obj,local,i2);
	return 1;
}

//! Decrement any objects that have local==0, then call objs.flush().
void Group::flush()
{
	for (int c=0; c<objs.n; c++) {
		if (objs.islocal[c]==0) objs.e[c]->dec_count();
	}
	objs.flush();
}

//! Return object with index i in stack.
LaxInterfaces::SomeData *Group::e(int i)
{
	if (i<0 || i>=objs.n) return NULL;
	return objs.e[i];
}

/*! Recognizes locked, visible, prints, then tries to parse elements...
 * Discards all else.
 * The objs should have been flushed before coming here.
 */
void Group::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	locked=visible=prints=0;
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
		} else if (!strcmp(name,"object")) {
			int n;
			char **strs=splitspace(value,&n);
			if (strs) {
				// could use the number as some sort of object id?
				// currently out put was like: "object 2 ImageData"
				//***strs[0]==that id
				SomeData *data=newObject(n>1?strs[1]:(n==1?strs[0]:NULL));//objs have 1 count
				if (data) {
					data->dump_in_atts(att->attributes[c],flag);
					push(data,0);
				}
				deletestrs(strs,n);
			} else {
				//DBG cout <<"*** readin blank object for Group..."<<endl;
			}
		} else { 
			//DBG cout <<"Group dump_in:*** unknown attribute!!"<<endl;
		}
	}
	FindBBox();
}

//! Write out the objects.
void Group::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (locked) fprintf(f,"%slocked\n",spc);
	if (visible) fprintf(f,"%svisible\n",spc);
	if (prints) fprintf(f,"%sprints\n",spc);
	for (int c=0; c<objs.n; c++) {
		fprintf(f,"%sobject %d %s\n",spc,c,objs.e[c]->whattype());
		objs.e[c]->dump_out(f,indent+2,0);
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
		if (n) *n++;
		if (g->findobj(d,n)) {
			return d;
		}
		if (n) *n--;
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
	int local;
	while (ne) {
		d=pop(w[ne-1],&local); //doesnt change count
		g->push(d,local); //incs count
		d->dec_count();
		ne--;
		for (int c2=0; c2<ne; c2++)
			if (w[c2]>w[ne]) w[c2]--;
	}
	g->FindBBox();
	objs.push(g,0,where);
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
	Group *g=dynamic_cast<Group *>(objs.pop(which));
	if (!g) return 1;
	
	SomeData *d;
	double mm[6];
	int local;
	for (int c=0; g->n(); c++) {
		d=g->pop(0,&local);
		transform_mult(mm,g->m(),d->m());
		transform_copy(d->m(),mm);
		objs.push(d,local,which++);
	}
	g->dec_count();
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
	int local;
	double mm[6];
	while (d=g->pop(0,&local), d) {
		transform_mult(mm,d->m(),g->m());
		transform_copy(d->m(),mm);
		objs.push(d,local,*which+1);
		d->dec_count();
	}
	remove(*which);
	FindBBox();
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

