//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************** objectcontainer.cc **********************/


#include "objectcontainer.h"

#include <iostream>
using namespace std;

//------------------------------ ObjectContainer ----------------------------------

/*! \class ObjectContainer
 * \brief Class to simplify object tree searching.
 *
 * Used by Group, LaidoutViewport, Spread, and others to be a generic object 
 * container.
 *
 * Three functions are built in: getanObject(), contains(), and nextObject(). Derived
 * classes must define their own n() and object_e().
 */
/*! \fn int ObjectContainer::n()
 * \brief  Return the number of objects that can be returned.
 */
/*! \fn Laxkit::anObject *ObjectContainer::object_e(int i)
 * \brief  Return pointer to object with index i.
 */
//class ObjectContainer : public Laxkit::anObject
//{
// public:
//	virtual int contains(Laxkit::anObject *d,FieldPlace &place);
//	virtual Laxkit::anObject *getanObject(FieldPlace &place,int offset=0);
//	virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, Laxkit::anObject **d=NULL);
//	virtual int n()=0;
//	virtual Laxkit::anObject *object_e(int i)=0;
//};



//! Step to next obect in group tree, and modify place to point to that next obj.
/*!
 * Return 1 on successful stepping. Place points to that next step.\n
 * Return 0 if place==first up to and including curlevel.\n
 * Return -2 if we've stepped through all objs in this level.\n
 * Return -1 for error.\n
 * If d!=NULL, then make *d point to what place refers to. Except for when returning 1,
 * d always will be pointing to NULL.
 * Note that place is always modified.
 *
 * Steps through from highest index to lowest. This is because the highest
 * index is usually seen first on screen, and stepping down goes to the ones
 * beneath. If this searches on a first-related level, then searching goes down
 * to 0, then starts at max, goes down to first.
 *
 * If place points to an invalid position, -1 is returned, and place is truncated
 * to where the error occurs.
 *
 * This container must correspond to index curlevel in place. For instance, if place is a list such as
 * {obj0.spread=1, obj1.spage=4, obj2.layer=0, obj3.obj=10, obj4.obj=15}, and *this is alleged to be 
 * object 3(==index 0 of the parent object), then curlevel must be 3, and the next link
 * in the chain is this->e(10).
 *
 * Note that nextObject will not include *this. (***make sure of this!)
 * 
 * \todo *** should probably modify this to either increment or decrement.
 */
int ObjectContainer::nextObject(FieldPlace &place,
								FieldPlace &first, 
								int curlevel, 
								Laxkit::anObject **d)//d=NULL
{
	//debugging:
	cout <<"oc.nextObject, curlevel="<<curlevel<<", n="<<n()<<endl;
	place.out("  place");
	first.out("  first");
	
	
	if (!n() || curlevel<0) {
		if (d) *d=NULL;
		cout <<"end("<<curlevel<<")  oc.nextObject=0"<<endl;
		return 0;
	}
	if (place.n()-1<curlevel) {
		if (d) *d=NULL;
		cout <<"end("<<curlevel<<")  oc.nextObject=-1"<<endl;
		return -1; //error! bad place
	}
	
	 // if curlevel and levels indexed below in place correspond to first, then
	 // a special check must be made when tripping through the objects, so must
	 // figure out if place up to curlevel-1 corresponds to first
	int atstartlevel=0;
	int c=0,i;
	if (first.n()) {
		if (curlevel>0) {
			for (c=curlevel-1; c>=0; c--) if (place.e(c)!=first.e(c)) break;
			if (c<0) atstartlevel=1;
		} else atstartlevel=1;
	} //else don't care about first, any decrementing below 0 terminates the nextObject
		
	 
	if (curlevel==place.n()-1) {
		 // current position does not refer to subobjects
		 // so step through this's objects
		i=place.e(curlevel);
		if (i<0) {
			if (d) *d=NULL;
			cout <<"end("<<curlevel<<")  oc.nextObject=-1"<<endl;
			return -1; // invalid place.e(curlevel)
		}
		//***if (dec) {...} else {inc...}
		i--;
		
		 // so i is the decremented (next) position
		 // If this i refers to an element of first, then we have already wrapped
		 // around, so return 0. Also, if i goes below 0 and we are not at an element
		 // of first, then also we are done stepping through this group (return -2).
		//if (i==-1 && n()==1) {
		//if (i==-1 && !atstartlevel && !(n()-1==first.e(curlevel))) {
		//if (i==-1 && !(n()==1 && atstartlevel && n()-1==first.e(curlevel))) {

		if (i==-1 && !atstartlevel) {
			//*****
			 // all done with this bunch, but not at first.. 
			place.pop();
			if (d) *d=NULL;
			cout <<"end("<<curlevel<<")  oc.nextObject=-2"<<endl;
			return -2;
		}
		if (i<0) i=n()-1; // we are at the level of first, so continue searching from top
		place.e(curlevel,i);
		if (atstartlevel && i==first.e(curlevel)) {
			 // is at first, so assume subobjects have already been stepped through
			place.pop();
			if (d) *d=NULL;
			cout <<"end("<<curlevel<<")  oc.nextObject=0"<<endl;
			return 0;
		} // else is ok to be at this object, so continue.
		
		 // if object_e(i) is group, then setup place with the top object: obj->subobj->subobj->etc.
		anObject *gg;
		ObjectContainer *g; 
		gg=object_e(i);
		g=dynamic_cast<ObjectContainer *>(gg);
		while (g && g->n()) {
			i=g->n()-1;
			place.push(i);
			gg=g->object_e(i);
			g=dynamic_cast<ObjectContainer *>(gg);
		}
		if (d) *d=gg;
		place.out("  next is");
		cout <<"end("<<curlevel<<")  oc.nextObject=1"<<endl;
		return 1; //success!
	}

	 // else if (curlevel!=place.n()-1) then:
	 // place refers to a subobject then try to make the subobject increment place:

	i=place.e(curlevel);
	if (i<0 || i>=n()) { // error! place invalid index
		while (curlevel!=place.n()-1) place.pop();
		if (d) *d=NULL;
		cout <<"end("<<curlevel<<")  oc.nextObject=-1"<<endl;
		return -1;
	}
	ObjectContainer *g=dynamic_cast<ObjectContainer *>(object_e(i));
	if (!g) { //was invalid spec, expected a group
		while (curlevel!=place.n()-1) place.pop();
		if (d) *d=NULL;
		cout <<"end("<<curlevel<<")  oc.nextObject-1"<<endl;
		return -1;
	}
	
	i=g->nextObject(place, first, curlevel+1, d);
	cout <<" curlevel="<<curlevel<<"  "; place.out("  stepped to");//debugging
	if (*d) cout <<"  curplaceobj:"<<(*d)->object_id<<endl; else cout <<"  curplaceobj=NULL"<<endl;
	if (i==-2) { 
		 // was all done with sublevels, so must inc at this level
		 // group function calling itself is perhaps a silly way to do it,
		 // should rework this function so the 'if (curlevel==place.n()-1)' block
		 // comes second?
		cout <<"  i==-2"<<endl;
		i=nextObject(place,first,curlevel,d);
		cout <<"end("<<curlevel<<")  oc.nextObject="<<i<<endl;
		return i;
	}
	if (i==0) { // first reached in sublevel, so start iterating at curlevel..
		cout <<"  i==0"<<endl;
		i=nextObject(place,first,curlevel,d);
		cout <<"end("<<curlevel<<")  oc.nextObject="<<i<<endl;
		return i;
	}
	if (i==1){
		cout <<"  i==1"<<endl;
		cout <<"end("<<curlevel<<")  oc.nextObject=1"<<endl;
		return 1; //successful incrementing
	}
	cout <<"end("<<curlevel<<")  oc.nextObject=-1(final)"<<endl;
	return -1; //must be error to be here, so return, do nothing more
}

//! Retrieve the object at position place.
/*! If offset>0, then check position in place starting at index offset.
 *
 * If there is no object at that position, then return NULL;
 */
Laxkit::anObject *ObjectContainer::getanObject(FieldPlace &place, int offset)
{
	if (place.n()<=offset) return NULL;
	if (place.e(offset)>=n() || place.e(offset)<0) return NULL;
	if (place.n()==offset+1) { // place refers to this container
		return object_e(place.e(offset));
	}
	 // must be a subelement
	ObjectContainer *g=dynamic_cast<ObjectContainer *>(object_e(place.e(offset)));
	if (!g) return NULL;
	return g->getanObject(place,offset+1);
}

//! Return whether d is in here somewhere.
/*! Returns place.n if the object is found, which is how deep down the
 * object is. Otherwise return 0.
 *
 * The position gets pushed onto place starting at place.n.
 */
int ObjectContainer::contains(Laxkit::anObject *d,FieldPlace &place)
{
	int c;
	int m=place.n();
	for (c=0; c<n(); c++) {
		if (d==object_e(c)) {
			place.push(c,m);
			return place.n();
		}
	}
	ObjectContainer *g;
	for (c=0; c<n(); c++) {
		g=dynamic_cast<ObjectContainer *>(object_e(c));
		if (g && g->contains(d,place)) {
			place.push(c,m);
			return place.n();
		}
	}
	return 0;
}

