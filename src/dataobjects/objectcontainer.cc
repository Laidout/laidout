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
// Copyright (C) 2004-2007,2010-2013 by Tom Lechner
//



#include "objectcontainer.h"

#include <cstdlib>
#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//------------------------------ ObjectContainer ----------------------------------

/*! \class ObjectContainer
 * \brief Abstract base class to simplify project wide object tree searching.
 *
 * Used by Group, LaidoutViewport, Spread, and others to be a generic object 
 * container. Access methods should be considered very temporary. They are not quite
 * equivalent to member access in scripting. Containment is queried entirely by index numbers,
 * not named extensions, and how those numbers are sometimes assigned depending on state at the moment,
 * not on a standardized naming system as in scripting.
 *
 * No actual data list is defined here, only access methods.
 * Three functions are built in: getanObject(), contains(), and nextObject(). Derived
 * classes must define their own n() and object_e().
 *
 * \todo *** do something with parent...
 */
/*! \fn int ObjectContainer::n()
 * \brief  Return the number of objects that can be returned.
 */
/*! \fn Laxkit::anObject *ObjectContainer::object_e(int i)
 * \brief  Return pointer to object with index i.
 */
/*! \fn const char *ObjectContainer::object_e(int i)
 * \brief  Return the name of fild with index i.
 */
/*! \var unsigned int ObjectContainer::obj_flags
 * \brief Any permanent flags. Returned by default in object_flags().
 */


ObjectContainer::ObjectContainer()
{
	//parent=NULL;
	obj_flags=0;
	id=NULL;
}

ObjectContainer::~ObjectContainer()
{
	if (id) delete[] id;
}



//! Find the next object from place, modifying place to refer to it.
/*! Also make d point to it if found. This function does not check against any first
 * object. That is a higher level searching function.
 *
 * offset means that *this corresponds to place.e(offset-1). In other words,
 * place.e(offset) is an index referring to a child of *this.
 *
 * Incrementing\n
 * If an object has kids, then step to the first child.
 * If on object has no kids, then step to its next sibling. 
 * If there are no more siblings, then step to next sibling of the parent, or its parent's sibling, etc.
 * 
 * Return Next_Success       on successful stepping. Place points to that next step.\n
 * Return Next_Error         for error.\n
 * 
 * \todo *** should implement   1. search only at offset's level, 
 * 								2. select only leaves (nodes with no children),
 * 								3. (other search criteria? maybe pass in check function?)
 */
int ObjectContainer::nextObject(FieldPlace &place,
								int offset, 
								unsigned int flags,
								Laxkit::anObject **d)//d=NULL
{
	DBG place.out("ObjectContainer::nextObject start: ");

	anObject *anobj=NULL;
	ObjectContainer *oc=NULL;
	int i,nn;
	FieldPlace orig = place;
	

	anobj = getanObject(place,offset); //retrieve the object pointed to by place
	oc = dynamic_cast<ObjectContainer *>(anobj);


	if (flags&Next_Increment) {
		if (oc && !(flags & Next_PlaceLevelOnly)) { //find number of kids to consider
			if ((flags & Next_SkipLockedKids) && (oc->object_flags() & OBJ_IgnoreKids)) nn=0;
			else nn=oc->n();
		} else nn=0;

		if (oc && nn) {
			 //object has kids, return the first kid
			place.push(0);
			if (d) *d=oc->object_e(0);
			DBG place.out("ObjectContainer::nextObject returning: ");
			return Next_Success;
		}

		//else object does not have kids. switch to next sibling

		while (1) {
			if (place.n()==offset) {
				if (d) *d=anobj;
				DBG place.out("ObjectContainer::nextObject returning: ");
				return Next_Success;
			}
			i=place.pop(); //old child index
			anobj=getanObject(place,offset); //retrieve the object pointed to by place
			oc=dynamic_cast<ObjectContainer *>(anobj);
			if (oc) { //find number of kids to consider
				if ((flags&Next_SkipLockedKids) && (oc->object_flags()&OBJ_IgnoreKids)) nn=0;
				else nn=oc->n();
			} else nn=0;

			i++;
			if (i<nn) {
				 //we are able to switch to next sibling
				place.push(i);
				anobj=getanObject(place,offset); //retrieve the object pointed to by place
				if (d) *d=anobj;
				DBG place.out("ObjectContainer::nextObject returning: ");
				return Next_Success;
			}
			
			//no more kids of original object's parent, so try siblings of parent
		}

	} else if (flags&Next_Decrement) {
		if (place.n()>offset) {
			i=place.pop();
			if (i==0) {
				anobj=getanObject(place,offset); //retrieve the object pointed to by place
				if (d) *d=anobj;
				DBG place.out("ObjectContainer::nextObject returning: ");
				return Next_Success;
			}

			 //switch to rightmost leaf of earlier sibling
			place.push(i-1);
			anobj=getanObject(place,offset); //retrieve the object pointed to by place
			oc=dynamic_cast<ObjectContainer *>(anobj);

			//if (oc && !(flags & Next_PlaceLevelOnly)) { //find number of kids to consider
			if (oc) { //find number of kids to consider
				if ((flags&Next_SkipLockedKids) && (oc->object_flags()&OBJ_IgnoreKids)) nn=0;
				else nn=oc->n();
			} else nn=0;

			 //select the bottom most, forward most child
			while (oc && nn) {
				place.push(nn-1);
				anobj=getanObject(place,offset); //retrieve the object pointed to by place
				oc=dynamic_cast<ObjectContainer *>(anobj);
				if (oc) { //find number of kids to consider
					if ((flags&Next_SkipLockedKids) && (oc->object_flags()&OBJ_IgnoreKids)) nn=0;
					else nn=oc->n();
				} else nn=0;
			}

			if (d) *d=anobj;
			DBG place.out("ObjectContainer::nextObject found: ");
			return Next_Success;
		}
		if (oc && place.n()==offset) {
			 //If at root, switch to right most leaf
			if (oc) { //find number of kids to consider
				if ((flags&Next_SkipLockedKids) && (oc->object_flags()&OBJ_IgnoreKids)) nn=0;
				else nn=oc->n();
			} else nn=0;
			while (oc && nn) {
				place.push(nn-1);
				anobj=getanObject(place,offset); //retrieve the object pointed to by place
				oc=dynamic_cast<ObjectContainer *>(anobj);
				if (oc) { //find number of kids to consider
					if ((flags&Next_SkipLockedKids) && (oc->object_flags()&OBJ_IgnoreKids)) nn=0;
					else nn=oc->n();
				} else nn=0;
			}
			if (d) *d=anobj;
			DBG place.out("ObjectContainer::nextObject returning: ");
			return Next_Success;
		}
	}

	if (d) *d=this;
	DBG place.out("ObjectContainer::nextObject found: ");
	return Next_Success;
}

	
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
	if (place.n()<offset) return NULL;
	if (place.n()==offset) return this;
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



} //namespace Laidout

