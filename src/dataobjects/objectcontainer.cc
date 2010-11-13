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
// Copyright (C) 2004-2007 by Tom Lechner
//



#include "objectcontainer.h"

#include <cstdlib>
#include <iostream>
using namespace std;
#define DBG 

//------------------------------ ObjectContainer ----------------------------------

/*! \class ObjectContainer
 * \brief Class to simplify object tree searching.
 *
 * Used by Group, LaidoutViewport, Spread, and others to be a generic object 
 * container.
 *
 * Three functions are built in: getanObject(), contains(), and nextObject(). Derived
 * classes must define their own n() and object_e().
 *
 * \todo *** use this class for a generic Lobject element? ImageData, GradientData, etc
 *   would all have a subclass with a Lobject component.
 * \todo *** do something with parent...
 */
/*! \fn int ObjectContainer::n()
 * \brief  Return the number of objects that can be returned.
 */
/*! \fn Laxkit::anObject *ObjectContainer::object_e(int i)
 * \brief  Return pointer to object with index i.
 */


//! Step to next obect in group tree, and modify place to point to that next obj.
/*! Please note that this function calls itself and also calls somesubobject->nextObject().
 *
 * Return Next_Success       on successful stepping. Place points to that next step.\n
 * Return Next_AtFirstObj    if place==first up to and including curlevel after stepping.\n
 * Return Next_DoneWithLevel if we've stepped through all objs in this level.\n
 * Return Next_NoSubObjects  if there were no subojects to step among.\n
 * Return Next_Error         for error.\n
 *
 * If d!=NULL, then make *d point to what place refers to. Except for when returning Next_Success,
 * d always will be pointing to NULL.
 * Note that place is always modified.
 *
 * Steps through from highest index to lowest. This is because the highest
 * index is usually seen first on screen, and stepping down goes to the ones
 * beneath. If this searches on a first-related level, then searching goes down
 * to 0, then starts at max, goes down to first.
 *
 * If place points to an invalid position, Next_Error is returned, and place is truncated
 * to where the error occurs.
 *
 * This container must correspond to index curlevel in place. For instance, if place is a list such as
 * {obj0.spread=1, obj1.spage=4, obj2.layer=0, obj3.obj=10, obj4.obj=15}, and *this is alleged to be 
 * object 3(==index 0 of the parent object), then curlevel must be 3, and the next link
 * in the chain is this->e(10).
 *
 * Note that nextObject will not include *this. Parent containers must select this object
 * depending perhaps on what this function returns.
 * 
 * \todo *** should probably modify this to either increment or decrement.
 * \todo *** should have flags: 1. search only at place's level, 
 * 								2. select only leaves
 * 								3. (other search criteria? maybe pass in check function?)
 * \todo *** this is a really convoluted function and should be replaced... (working on it, see below)
 */
int ObjectContainer::nextObject(FieldPlace &place,
								FieldPlace &first, 
								int curlevel, 
								Laxkit::anObject **d)//d=NULL
{
	//debugging:
	//DBG cerr <<"oc.nextObject, curlevel="<<curlevel<<", n="<<n()<<endl;
	//DBG place.out("  place");
	//DBG first.out("  first");
	
	
	if (!n() || curlevel<0) {
		 //does *this contain no objects or is *this beyond the curlevel? then:
		if (d) *d=NULL;
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=0: no objs or this>curlevel"<<endl;
		return curlevel<0 ? Next_Error : Next_NoSubObjects;
	}
	if (place.n()-1<curlevel) {
		 //place is corrupted, does not have enough elements
		if (d) *d=NULL;
		//DBG cerr <<"corrupt place, not enough elements, end("<<curlevel<<")"<<endl;
		return Next_Error; //error! bad place
	}
	
	 // if curlevel and levels indexed below in place correspond to first, then
	 // a special check must be made when tripping through the objects, so must
	 // figure out if place up to curlevel-1 corresponds to first
	int atstartlevel=0;
	int c=0,i;
	if (first.n()) { //&& !ignore_first
		for (c=curlevel-1; c>=0; c--) if (place.e(c)!=first.e(c)) break;
		if (c<0) atstartlevel=1; //note that curlevel=0 implies atstartlevel=1
	} //else don't care about first, any decrementing below 0 terminates the nextObject
		
	 
	if (curlevel==place.n()-1) {
		 // current position does not refer to subobjects
		 // so step through this's objects
		i=place.e(curlevel);
		if (i<0) {
			if (d) *d=NULL;
			//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=-1"<<endl;
			return Next_Error; // invalid place.e(curlevel)
		}
		//***if (dec) {...} else {inc...}
		i--;
		
		 // so i is the decremented (next) position
		 // If this i refers to an element of first, then we have already wrapped
		 // around, so return Next_AtFirstObj. Also, if i goes below 0 and we are not at an element
		 // of first, then also we are done stepping through this group (return Next_DoneWithLevel).
		//if (i==-1 && n()==1) {
		//if (i==-1 && !atstartlevel && !(n()-1==first.e(curlevel))) {
		//if (i==-1 && !(n()==1 && atstartlevel && n()-1==first.e(curlevel))) {

		if (i==-1 && !atstartlevel) {
			//*****must integrate inc and dec, not just dec
			 // all done with this bunch, but not at first obj.. 
			place.pop();
			if (d) *d=NULL;
			//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=-2"<<endl;
			return Next_DoneWithLevel;
		}
		if (i<0) i=n()-1; // we are at the level of first, so continue searching from top
		place.e(curlevel,i);
		if (atstartlevel && i==first.e(curlevel)) {
			 // is at first, so assume subobjects have already been stepped through
			 // make place have position of parent object
			place.pop();
			if (d) *d=NULL;
			//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=0"<<endl;
			return Next_AtFirstObj;
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
		//DBG place.out("  next is");
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=1"<<endl;
		return Next_Success; 
	}

	 // else if (curlevel!=place.n()-1) then:
	 // place refers to a subobject then try to make the subobject increment place:

	i=place.e(curlevel);
	if (i<0 || i>=n()) { // error! place invalid index
		 // truncate place to this level
		while (curlevel!=place.n()-1) place.pop();
		if (d) *d=NULL;
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=-1"<<endl;
		return Next_Error;
	}
	ObjectContainer *g=dynamic_cast<ObjectContainer *>(object_e(i));
	if (!g) { //was invalid spec, expected an ObjectContainer
		while (curlevel!=place.n()-1) place.pop();
		if (d) *d=NULL;
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject-1"<<endl;
		return Next_Error;
	}
	
	i=g->nextObject(place, first, curlevel+1, d);
	//DBG cerr <<" curlevel="<<curlevel<<"  "; place.out("  stepped to");//debugging
	//DBG if (*d) cerr <<"  curplaceobj:"<<(*d)->object_id<<","<<(*d)->whattype()<<endl;
	//DBG    else cerr <<"  curplaceobj=NULL"<<endl;
	if (i==Next_DoneWithLevel) { 
		 // was all done with sublevels, so must inc at this level
		 // group function calling itself is perhaps a silly way to do it,
		 // should rework this function so the 'if (curlevel==place.n()-1)' block
		 // comes second?
		 //***
		//DBG cerr <<"  i==-2"<<endl;
		i=nextObject(place,first,curlevel,d);
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject="<<i<<endl;
		return i;
	}
	if (i==Next_AtFirstObj) {
		 // first obj reached in sublevel, so start iterating at curlevel,
		 // using *this as the nextObject
		//DBG cerr <<"  i==0"<<endl;
		//i=nextObject(place,first,curlevel,d);
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject="<<i<<endl;
		if (d && curlevel==place.n()-1) *d=this;
		return Next_AtFirstObj;
	}
	if (i==Next_Success){
		//DBG cerr <<"  success"<<endl;
		//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=1"<<endl;
		return Next_Success; //successful incrementing
	}
	//DBG cerr <<"end("<<curlevel<<")  oc.nextObject=-1(final)"<<endl;
	return Next_Error; //must be error to be here, so return, do nothing more
}

//------------------redoing nextObject:

//! Find the next object from place, modifying place to refer to it.
/*! Also make d point to it if found. This function does not check against any first
 * object. That is a higher level searching function.
 *
 * offset means that *this corresponds to place.e(offset-1). In other words,
 * place.e(offset) is an index referring to a child of *this.
 * 
 * If an object has kids, then step to the first (or last if decrementing) child.
 * If on object has no kids, then step to its next sibling. 
 * If there are no siblings, then step to the immediate parent.
 * 
 * Return Next_Success       on successful stepping. Place points to that next step.\n
 * Return Next_Error         for error.\n
 * 
 * \todo *** should implement   1. search only at offset's level, 
 * 								2. select only leaves (nodes with no children),
 * 								3. (other search criteria? maybe pass in check function?)
 */
int ObjectContainer::nextObject2(FieldPlace &place,
								int offset, 
								unsigned int flags,
								Laxkit::anObject **d)//d=NULL
{
	//DBG place.out("ObjectContainer::nextObject2 start: ");

	anObject *anobj=NULL;
	ObjectContainer *oc=NULL;
	
	if (place.n()==offset) {
		anobj=oc=this;
	} else {
		anobj=getanObject(place,offset);
		oc=dynamic_cast<ObjectContainer *>(anobj);
	}
	if (oc && oc->n()) {
		 // if object is a container and has kids, then step to first kid
		if (flags&Next_Increment) {
			place.push(0);
			if (d) *d=oc->object_e(0);
		} else {
			place.push(oc->n()-1);
			if (d) *d=oc->object_e(oc->n()-1);
		}
		//DBG place.out("ObjectContainer::nextObject2 found: ");
		return Next_Success;
	}

	 //object has no kids, so 
	 //try to get object adjacent to place. must retrieve place's parent object
	 //and check for existence of adjacent
	do {
		if (place.n()==offset) break;
		int i=place.pop();
		if (place.n()==offset) {
			anobj=oc=this;
		} else {
			anobj=getanObject(place,offset);
			oc=dynamic_cast<ObjectContainer *>(anobj);
		}
		if (!oc) {
			cout <<"*** error! Parent must be ObjectContext"<<endl; //this is more of an assert, not a DBG
			exit(1);
		}
		
		if (flags&Next_Increment) {
			i++;
		} else {
			i--;
		}
		if (i>=0 && i<oc->n()) {
			if (d) *d=oc->object_e(i);
			place.push(i);
			//DBG place.out("ObjectContainer::nextObject2 found: ");
			return Next_Success;
		}
	} while (1);

	//to be here, place.n()==0, and we are at the top node (*this), so just return this

	if (d) *d=this;
	//DBG place.out("ObjectContainer::nextObject2 found: ");
	return Next_Success;
}

//------------------end redo nextObject

	
//! Retrieve the object at position place.
/*! If offset>0, then check position in place starting at index offset. That is, pretend
 * that *this is at offset-1, so place[offset] is a subobject of *this.
 * 
 * If nn>0, then only go down nn number of subobjects from offset. Otherwise, use
 * up the rest of place.
 *
 * If there is no object at that position, then return NULL;
 *
 * Note that if place.n()==0, then NULL is returned, not <tt>this</tt>.
 */
Laxkit::anObject *ObjectContainer::getanObject(FieldPlace &place, int offset,int nn)//nn=-1
{
//	if (place.n()<=offset) return NULL;
//	if (place.e(offset)>=n() || place.e(offset)<0) return NULL;
//	if (nn<=0) nn=place.n()-offset;
//	if (nn==1) { // place refers to this container
//		return object_e(place.e(offset));
//	}
//	 // must be a subelement
//	ObjectContainer *g=dynamic_cast<ObjectContainer *>(object_e(place.e(offset)));
//	if (!g) return NULL;
//	return g->getanObject(place,offset+1);
//------------non-recursive:
	if (place.n()<=offset) return NULL;
	int i=offset;
	ObjectContainer *g=this;
	if (nn<=0) nn=place.n()-offset;
	while (nn) {
		if (place.e(i)>=g->n() || place.e(i)<0) return NULL;
		if (nn==1) { // place refers to g
			return g->object_e(place.e(i));
		}
		 // must be a subelement
		g=dynamic_cast<ObjectContainer *>(g->object_e(place.e(i)));
		if (!g) return NULL;
		i++;
		nn--;
	}
	return NULL;
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

