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
// Copyright (C) 2005-2007 by Tom Lechner
//

#include <lax/transformmath.h>
#include "groupinterface.h"
#include "../project.h"
#include "../viewwindow.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;
using namespace Laxkit;

//----------------------------- GroupInterface -----------------------

/*! \class GroupInterface
 * \brief Interface for selecting multiple things, grouping, and ungrouping.
 */



GroupInterface::GroupInterface(int nid,Laxkit::Displayer *ndp)
	: ObjectInterface(nid,ndp)
{
}

GroupInterface::~GroupInterface()
{
	DBG cerr <<"---- in GroupInterface destructor"<<endl;
}

anInterface *GroupInterface::duplicate(anInterface *dup)
{
	GroupInterface *g;
	if (dup==NULL) g=new GroupInterface(id,dp);
	else {g=dynamic_cast<GroupInterface *>(dup);
		if (g==NULL) return NULL;
	}
	return ObjectInterface::duplicate(g);
}

int GroupInterface::draws(const char *atype)
{
	//if (!strcmp(atype,"Group") || !strcmp(atype,"MysteryData")) return 1;
	if (!strcmp(atype,"Group")) return 1;
	return 0;
}

/*! Return 0 if newdata accepted, 1 if accepted.
 *
 * \todo *** UseThis() should optionally use object contexts, not just an anObject.
 */
int GroupInterface::UseThis(anObject *newdata,unsigned int)
{
	if (!newdata) return 0;
	SomeData *d=dynamic_cast<SomeData *>(newdata);
	if (!d) return 0;

	FreeSelection();
	VObjContext oc;
	oc.obj=d; 
	d->inc_count();
	((LaidoutViewport *)viewport)->locateObject(d,oc.context);
	AddToSelection(&oc);
	needtodraw=1;
	return 1;
}

int GroupInterface::LBDown(int x, int y,unsigned int state, int count,const Laxkit::LaxMouse *mouse)
{
	int c=ObjectInterface::LBDown(x,y,state,count,mouse);
	if (count==2 && selection.n==1 && strcmp(selection.e[0]->whattype(),"Group")) {
		if (viewport) viewport->ChangeObject(selection.e[0],NULL);
	}
	return c;
}
	
//! Return 1 if change, else 0.
int GroupInterface::ToggleGroup()
{
	DBG cerr <<"*******GroupInterface.ToggleGroup"<<endl;

	if (selection.n==0) {
		viewport->postmessage("No objects selected.");
		return 0;
	}
	 // selected objects are of form spread.pagelocation.layer.index...
	 // or limbo.index...
	 // Whether grouping or ungrouping, it is necessary to find either the
	 // layer object or the limbo object:
	
	 //*** this requires more thought to incorporate other "object zones".
	 //now there is limbo, and doc-page-spread, but there should also be
	 //paper-objects, stickies(a type of bookmark)
	 //***also must store contexts when adding objects to a selection....
	int error=0;
	Group *base=NULL;
	FieldPlace place;
	if (!((LaidoutViewport *)viewport)->locateObject(selection.e[0],place)) {
		viewport->postmessage("Ugly internal error finding a selected object! Fire the programmer.");
		return 0;
	} 

	DBG place.out("toggle this group: ");

	 // find the base group which contains the group to ungroup, or which contains the
	 // first selected object to group with others..
	if (place.e(0)==0) { // is limbo
		base=((LaidoutViewport *)viewport)->limbo;
	} else if (place.e(0)==1) {
		 // is doc pages spread, need the page->layers containing the selection
		Page *p=NULL;
		if (place.e(1)>=0) { // if there is a valid spread page...
			Spread *s=((LaidoutViewport *)viewport)->spread;
			if (s) p=dynamic_cast<Page *>(s->object_e(place.e(1))); //spread->object_e returns Page
		}
		 // place now has layer.index.index...
		if (p) base=&p->layers;
	}
	if (!base) {
		viewport->postmessage("Ugly internal error finding a selected object! Fire the programmer.");
		return 0;
	}
	 // now base is either limbo or the Group of page layers, and place is the full place of selection[0]
	
	if (selection.n==1) {
		 // a single Group is selected, ungroup its objects...
		 // or there is a single object that is not a Group, should have option to
		 // force a group of one object maybe..
		if (strcmp(selection.e[0]->whattype(),"Group")) {
			viewport->postmessage("Cannot group single objects like that.");
			return 1;
		}
		if (place.e(0)==0) { // base is limbo
			place.pop(0);
			error=base->UnGroup(place.n(),place.list());
		} else if (place.e(0)==1) {
			 // base is page->layers containing the selection
			place.pop(0); //remove spread index
			place.pop(0); //remove pagelocation index
			 // place now has layer.index.index...
			error=base->UnGroup(place.n(),place.list());
		} else error=1;

		viewport->postmessage(error?"Ungroup failed.":"Ungrouped.");
		
		cout <<"*** must revamp selection after ungroup to have all the subobjs selected!!"<<endl;
		FreeSelection();
		return error?0:1;
		
		//---------
		//in the future, depending on how linked data objects/object containers are, might then be
		//possible to simply do:
		//((Group *)selection.e[0])->UnGroup();
		//which would ungroup its elements into its parent object
	}
	
	 // to be here, there are multiple objects selected, so group them
	
	 // check that all selected objects are same level, 
	 // then make new Group object and fill in selection,
	 // updating LaidoutViewport and Document
	 //
	 // *** note that this is rather inefficient.. object selecting must be
	 // reprogrammed to remember the context, rather than searching for it 
	 // everytime as here..
	FieldPlace place1;
	Laxkit::NumStack<int> list;
	LaidoutViewport *vp=((LaidoutViewport *)viewport);
	list.push(place.pop()); //remove top index from place, which was selection[0]
	for (int c=1; c<selection.n; c++) {
		vp->locateObject(selection.e[c],place1);
		list.pushnodup(place1.pop());
		if (!(place1==place)) {
			viewport->postmessage("Items must all be at same level to group.");
			return 0;
		}
	}
	
	 //now place holds the place of the parent object, and base is limbo or spread->page->layers
	if (place.e(0)==0) place.pop(0); // remove spread index
	else if (place.e(0)==1) { 
		place.pop(0); // remove spread index and pagelocation index
		place.pop(0); 
	} else {
		viewport->postmessage("Containing object must be limbo or a spread.");
		return 0;
	}
	for ( ; place.n(); place.pop()) {
		base=dynamic_cast<Group *>(base->e(place.e(0)));
		if (!base || strcmp(base->whattype(),"Group")) {
			viewport->postmessage("Containing object must be a group to group subobjects.");
			return 0;
		}
	}
	error=base->GroupObjs(list.n,list.e);
	viewport->postmessage(error?"Group failed.":"Grouped.");
	
	cout <<"*** revamp selection after group"<<endl;
	FreeSelection();

	return 1;
}

/*! Returns the number of new things added to the selection.
 */
int GroupInterface::GrabSelection(unsigned int state)
{
	if (!data) return 1;

	DoubleBBox bbox;
	bbox.addtobounds(transform_point(data->m(),data->minx,data->miny));
	bbox.addtobounds(transform_point(data->m(),data->maxx,data->maxy));
	
	DBG cerr <<"grab from: "<<bbox.minx<<','<<bbox.miny<<endl;
	DBG cerr <<"grab to:   "<<bbox.maxx<<','<<bbox.maxy<<endl;
	
	int n;
	VObjContext **objs;
	n=viewport->FindObjects(&bbox,0,0,NULL,(ObjectContext ***)(&objs));

	DBG if (n && !objs) cerr <<"*******ERROR! says found objects, but no objects returned."<<endl;
	DBG else {
	DBG 	cerr <<"find in box: "<<data->minx<<","<<data->miny<<" -> "<<data->maxx<<","<<data->maxy<<endl;
	DBG 	cerr <<"Found objects: "<<n<<endl;
	DBG }

	 //add
	for (int c=0; c<n; c++) {
		DBG cerr << "  ";
		DBG if (objs[c]) objs[c]->context.out("");

		AddToSelection(objs[c]);
	}
	if (n==0) {
		deletedata();
		needtodraw=1;
	}
	
	return n;
}




