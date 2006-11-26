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

//------------------remove: (now in ObjectContainer
////! Step to next obect in group tree, and modify place to point to that next obj.
///*!
// * On a successful nextObject, 1 is returned, and place points to it.
// * If d!=NULL, then make *d point to what place refers to.
// * If the new place==first after change, then return 0. If we have otherwise already stepped
// * through all elements of this group, then return -2. d in these cases is set to NULL.
// * Note that place is still modified.
// *
// * Steps through from highest index to lowest. This is because the highest
// * index is usually seen first on screen, and stepping down goes to the ones
// * beneath. If this searches on a first-related level, then searching goes down
// * to 0, then starts at max, goes down to first.
// *
// * If place points to an invalid position, -1 is returned, and place is truncated
// * to where the error occurs.
// *
// * This Group must correspond to index curlevel in place. For instance, if place is a list such as
// * {obj0.spread=1, obj1.spage=4, obj2.layer=0, obj3.obj=10, obj4.obj=15}, and *this is alleged to be 
// * object 3(==index 0 of the parent object), then curlevel must be 3, and the next link
// * in the chain is this->e(10).
// *
// */
//int Group::nextObject(FieldPlace &place, FieldPlace &first, int curlevel, LaxInterfaces::SomeData **d)//d=NULL
//{
//	if (!objs.n || curlevel<0) return 0;
//	if (place.n()-1<curlevel) {
//		if (d) *d=NULL;
//		return -1; //error! bad place
//	}
//	
//	 // if curlevel and levels indexed below in place correspond to first, then
//	 // a special check must be made when tripping through the objects, so must
//	 // figure out if place up to curlevel-1 corresponds to first
//	int atstartlevel=0;
//	int c,i;
//	if (first.n()) {
//		for (c=curlevel-1; c>0; c--) if (place.e(c)!=first(c)) break;
//		if (c<0) atstartlevel=1;
//	} //else don't care about first, any decrementing below 0 terminates the nextObject
//		
//	 
//	if (curlevel==place.n()-1) {
//		 // current position does not refer to subobjects
//		 // 
//		i=place.e(curlevel)-1;
//		
//		 // so i is the decremented (next) position
//		 // If this i refers to an element of first, then we have already wrapped
//		 // around, so return 0. Also, if i goes below 0 and we are not at an element
//		 // of first, then also we are done stepping through this group (return -2).
//		if (i==-1 && !(atstartlevel && 0==first.e(curlevel))) {
//			 // all done with this bunch, but not at first.. this extra check
//			 // for first match is because the group might only have 1 object and that 1
//			 // is a first element. That case is dealt with below.
//			place.pop();
//			if (d) *d=NULL;
//			return -2;
//		}
//		if (i==-2) { // error! place.e(curlevel) had invalid spec
//			if (d) *d=NULL;
//			return -1;
//		}
//		if (i<0) i=objs.n()-1; // we are at an element of first, so continue searching from top
//		place.e(curlevel,i);
//		if (atstartlevel && i==first.e(curlevel)) {
//			 // is at first, so assume subobjects have already been stepped through
//			place.pop();
//			return 0;
//		} // else is ok to be at this object, so continue.
//		
//		 // if objs.e(i) is group, then setup place with the top object: obj->subobj->subobj->etc.
//		Group *gg,*g; 
//		gg=objs.e[i];
//		g=dynamic_cast<Group *>(gg);
//		while (g && g->n()) {
//			place.push(i);
//			i=g->n()-1;
//			gg=g->e(i);
//			g=dynamic_cast<Group *>(gg);
//		}
//		if (d) *d=gg;
//		return 1; //success!
//	}
//
//	 // else if (curlevel!=place.n()-1) then:
//	 // place refers to a subobject then try to make the subobject increment place:
//
//	i=place.e(curlevel);
//	if (i<0 || i>=objs.n) { // error! place invalid index
//		while (curlevel!=place.n()-1) place.pop();
//		if (d) *d=NULL;
//		return -1;
//	}
//	Group *g=dynamic_cast<Group *g>(objs.e(i));
//	if (!g) { //was invalid spec, expected a group
//		while (curlevel!=place.n()-1) place.pop();
//		if (d) *d=NULL;
//		return -1;
//	}
//	
//	i=g->nextObject(place, first, curlevel+1);
//	if (i==-2) { 
//		 // was all done with sublevels, so must inc at this level
//		 // group function calling itself is perhaps a silly way to do it,
//		 // should rework this function so the 'if (curlevel==place.n()-1)' block
//		 // comes second?
//		return nextObject(place,first,curlevel);
//	}
//	if (i==0) { // first reached in sublevel, so start iterating at curlevel..
//		return nextObject(place,first,curlevel);
//	}
//	if (i==1) return 1; //successful incrementing
//	return -1; //must be error to be here, so return, do nothing more
//}
//
////! Retrieve the object at position place.
///*! If offset>0, then check position in place starting at index offset.
// *
// * If there is no object at that position, then return NULL;
// */
//LaxInterfaces::SomeData *Group::getObject(FieldPlace &place, int offset)
//{
//	if (place.n()>=offset) return NULL;
//	if (place.e(offset)>=objs.n() || place.e(offset)<0) return NULL;
//	if (place.n()==offset+1) { // place refers to this Group
//		return objs.e(place.e(offset));
//	}
//	 // must be a subelement
//	Group *g=dynamic_cast<Group *>(objs.e(place.e(offset)));
//	if (!g) return NULL;
//	return g->getObject(place,offset+1);
//}
//
////! Return whether d is in Group somewhere.
///*! Returns place.n if the object is found, which is how deep down the
// * object is. Otherwise return 0.
// *
// * The position gets pushed onto place starting at place.n.
// */
//int Group::contains(SomeData *d,FieldPlace &place)
//{
//	int c;
//	int n=place.n;
//	for (c=0; c<objs.n; c++) {
//		if (d==objs.e[c]) {
//			place.push(c,n);
//			return place.n;
//		}
//	}
//	Group *g;
//	for (c=0; c<objs.n; c++) {
//		g=dynamic_cast<Group *>(objs.e[c]);
//		if (g && g->contains(d,place)) {
//			place.push(c,n);
//			return place.n;
//		}
//	}
//	return 0;
//}
//---^^^ in ObjectContainer, remove if tests ok...

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
				DBG cout <<"*** readin blank object for Group..."<<endl;
			}
		} else { 
			DBG cout <<"Group dump_in:*** unknown attribute!!"<<endl;
		}
	}
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
	if (n>1) return UnGroup(n-1,which+1);
	
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

