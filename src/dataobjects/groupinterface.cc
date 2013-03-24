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
// Copyright (C) 2005-2007,2009-2013 by Tom Lechner
//

#include <lax/transformmath.h>
#include <lax/interfaces/rectpointdefs.h>
#include <lax/colors.h>
#include "dataobjects/lsomedataref.h"
#include "groupinterface.h"
#include "../interfaces/aligninterface.h"
#include "../interfaces/nupinterface.h"
#include "printermarks.h"
#include "../project.h"
#include "../viewwindow.h"
#include "../language.h"
#include "../stylemanager.h"
#include "../calculator/shortcuttodef.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;
using namespace Laxkit;


namespace Laidout {


//----------------------------- GroupInterface -----------------------

/*! \class GroupInterface
 * \brief Interface for selecting multiple things, grouping, and ungrouping.
 */


GroupInterface::GroupInterface(int nid,Laxkit::Displayer *ndp)
	: ObjectInterface(nid,ndp)
{
	popupcontrols=0;
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

void GroupInterface::TransformSelection(const double *N, int s, int e) 
{
	for (int c=0; c<selection.n; c++) {
		DBG cerr<<"-------ObjectInterfaceTransformSelection on "; ((VObjContext *)selection.e[c])->context.out(":");
	}
	ObjectInterface::TransformSelection(N,s,e);
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

#define GROUP_RegistrationMark 8
#define GROUP_GrayBars         9 
#define GROUP_CutMarks         10 

Laxkit::MenuInfo *GroupInterface::ContextMenu(int x,int y,int deviceid)
{
	rx=x,ry=y;

	MenuInfo *menu=NULL;
	LaidoutViewport *lvp=dynamic_cast<LaidoutViewport*>(viewport);
	if (lvp->papergroup) {
		menu=new MenuInfo(_("Group Interface"));
		menu->AddItem(_("Add Registration Mark"),GROUP_RegistrationMark);
		menu->AddItem(_("Add Gray Bars"),GROUP_GrayBars);
		//menu->AddItem(_("Add Cut Marks"),PAPERM_CutMarks);
		//menu->AddSep();
	}

	return menu;
}

int GroupInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item

		LaidoutViewport *lvp=dynamic_cast<LaidoutViewport*>(viewport);
		if (!lvp->papergroup) return 0;

		if (i==GROUP_RegistrationMark) {
			if (!lvp->papergroup) return 0;
			DrawableObject *obj= RegistrationMark(18,1);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			lvp->papergroup->objs.push(obj);
			needtodraw=1;
			return 0;

		} else if (i==GROUP_GrayBars) {
			if (!lvp->papergroup) return 0;
			DrawableObject *obj= BWColorBars(18,LAX_COLOR_GRAY);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			lvp->papergroup->objs.push(obj);
			needtodraw=1;
			return 0;
		}
	}
	return 1;
}

enum GroupControlTypes
{
	GROUP_Link=RP_MAX,
	GROUP_Parent_Link,
	GROUP_Constraints,
	GROUP_Zone,
	GROUP_Chains,
	GROUP_Jump_To_Link,
	GROUP_Sever_Link,
	GROUP_MAX
};

const char *GroupInterface::hoverMessage(int p)
{
	if (p==GROUP_Link) return _("Clone options");
	if (p==GROUP_Parent_Link) return _("Parent alignment options");
	return RectInterface::hoverMessage(p);
}

/*! Called by RectInterface::scan(), for easier adaption to custom object controls.
 */
int GroupInterface::AlternateScan(flatpoint sp, flatpoint p, double xmag,double ymag, double onepix)
{
	//must scan for:
	//  link ball
	//  parent link
	//  constraints
	//  zone selector
	//  chains

	 //check for Somedataref indicator
	if (selection.n==1 && !strcmp(selection.e[0]->obj->whattype(),"SomeDataRef")) {
		double dist=onepix*maxtouchlen*maxtouchlen/4;
		flatpoint pp=flatpoint((somedata->minx+somedata->maxx)/2, somedata->miny-maxtouchlen/ymag);
		DBG cerr <<"...alt scan: "<<p.x<<','<<p.y<< norm2(p-pp)<<endl;
		if (norm2(p-pp)<dist) return GROUP_Link;

		double th=dp->textheight();
		double w=dp->textextent(_("Original"),-1,NULL,NULL)*1.5;
		
		if (popupcontrols==GROUP_Link) {
			 //p.x,p.y, w/2,th*1.5
			pp=dp->realtoscreen(transform_point(somedata->m(),pp));
			pp.y+=2*th;
			pp.x-=w/2;
			if (pp.y+th>dp->Maxy) pp.y=dp->Maxy-th;
			if (sp.y>pp.y-th*1.5 && sp.y<pp.y+th*1.5) {
				if (sp.x>pp.x-w/2 && sp.x<pp.x+w/2) return GROUP_Jump_To_Link;
				if (sp.x>pp.x+w/2 && sp.x<pp.x+w*3/2) return GROUP_Sever_Link;
			}
		}
	}


	return RP_None;
}

int GroupInterface::LBDown(int x, int y,unsigned int state, int count,const Laxkit::LaxMouse *mouse)
{
	DBG cerr <<"GroupInterface::LBDown..."<<endl;
	int c=ObjectInterface::LBDown(x,y,state,count,mouse);

	int curpoint;
    buttondown.getextrainfo(mouse->id,LEFTBUTTON,&curpoint);
	if (curpoint==GROUP_Link) {
		popupcontrols=GROUP_Link;
        PostMessage(" ");
		return 0;
	}

	if (count==2 && selection.n==1 && strcmp(selection.e[0]->obj->whattype(),"Group")) {
		 //double click to switch to more specific tool
		if (viewport) viewport->ChangeObject(selection.e[0],1);
		buttondown.clear();
	}
	return c;
}
	
int GroupInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	//int curpoint;
    //buttondown.getextrainfo(d->id,LEFTBUTTON,&curpoint);
	if (popupcontrols==GROUP_Link) {
		buttondown.up(d->id,LEFTBUTTON);

		if (hover==GROUP_Sever_Link) {
			LaidoutViewport *vp=((LaidoutViewport *)viewport);
			LSomeDataRef *ref=dynamic_cast<LSomeDataRef*>(selection.e[0]->obj);
			SomeData *obj=ref->thedata->duplicate(NULL);
			obj->m(ref->m());
			obj->FindBBox();
			selection.e[0]->SetObject(obj);
			vp->DeleteObject();
			vp->NewData(obj,NULL);

		} else if (hover==GROUP_Jump_To_Link) {
			LaidoutViewport *vp=((LaidoutViewport *)viewport);

			LSomeDataRef *ref=dynamic_cast<LSomeDataRef*>(selection.e[0]->obj);
			SomeData *obj=ref->thedata;
			VObjContext context;
			context.SetObject(obj);
			if (vp->locateObject(obj,context.context)) {
				 //update viewport to view that object, it was found in current spread
				vp->ChangeObject(&context,0);
				FreeSelection();
				AddToSelection(&context);
			} else {
				 //object was not found in current spread, so search in all limbos and docs
				int found=laidout->project->FindObject(obj,context.context);
				if (found && context.context.e(0)==0) {
					 //object was found in a limbo
				} else if (found) {
					 //object was found in a document page
					 //place 0 is doc, 1 is page number
					vp->SelectPage(context.context.e(1));
				} else found=0;

				if (found && vp->locateObject(obj,context.context)) {
					FreeSelection();
					AddToSelection(&context);
				} else {
					PostMessage(_("Can't jump to object"));
				}
			}

		}

		popupcontrols=0;
		hover=0;
		needtodraw=1;
		return 0;
	}

	return ObjectInterface::LBUp(x,y,state,d);
}

int GroupInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (popupcontrols==GROUP_Link) {
		int newhover=scan(x,y);
		if (newhover!=hover) {
			hover=newhover;
			needtodraw|=2;
            const char *mes=hoverMessage(hover);
            PostMessage(mes?mes:" ");
		}
		return 0;
	}

	return ObjectInterface::MouseMove(x,y,state,d);
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
	if (!((LaidoutViewport *)viewport)->locateObject(selection.e[0]->obj,place)) {
		viewport->postmessage("Ugly internal error finding a selected object! Fire the programmer.");
		return 0;
	} 

	DBG place.out("toggle group point: ");

	 // find the base group which contains the group to ungroup, or which contains the
	 // first selected object to group with others..
	ObjectContainer *objc=((LaidoutViewport *)viewport);
	for (int i=0; i<place.n()-1; i++) {
		if (!objc) break;
		objc=dynamic_cast<ObjectContainer*>(objc->object_e(place.e(i)));
	}
	if (dynamic_cast<Page*>(objc)) base=&(dynamic_cast<Page*>(objc)->layers);
	else base=dynamic_cast<Group*>(objc);
	if (!base) {
		viewport->postmessage("Ugly internal error finding a selected object! Fire the programmer.");
		return 0;
	}

	 // now base is either limbo or the Group of page layers, and place is the full place of selection[0]
	
	if (selection.n==1) {
		 // a single Group is selected, ungroup its objects...
		 // or there is a single object that is not a Group, should have option to
		 // force a group of one object maybe..
		if (strcmp(selection.e[0]->obj->whattype(),"Group")) {
			viewport->postmessage("Cannot group single objects like that.");
			return 1;
		}

		DBG cerr <<"*** must revamp selection after ungroup to have all the subobjs selected!!"<<endl;
		int numgrouped=base->n();
		error=base->UnGroup(place.e(place.n()-1));

		FreeSelection();
		VObjContext element;
		if (error==0) {
			int which=place.e(place.n());
			for (int c=which; c<which+numgrouped; c++) {
				element.clear();
				element.obj=base->e(c); element.obj->inc_count();
				for (int c2=0; c2<place.n(); c2++) {
					element.push(place.e(c2));
				}
				AddToSelection(&element);
			}
		}

		viewport->postmessage(error?"Ungroup failed.":"Ungrouped.");
		
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
		vp->locateObject(selection.e[c]->obj,place1);
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
	error=base->GroupObjs(list.n,list.e);
	viewport->postmessage(error?"Group failed.":"Grouped.");
	
	cout <<"*** need to revamp selection after group"<<endl;
	FreeSelection();

	return 1;
}

/*! Returns the number of new things added to the selection.
 */
int GroupInterface::GrabSelection(unsigned int state)
{
	if (!data) return 1;

	DoubleBBox bbox;
	bbox.addtobounds(data->m(),data);
	
	DBG cerr <<"grab from: "<<bbox.minx<<','<<bbox.miny<<endl;
	DBG cerr <<"grab to:   "<<bbox.maxx<<','<<bbox.maxy<<endl;
	
	int n=0;
	VObjContext **objs=NULL;
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
		delete objs[c];
	}
	if (n==0) {
		deletedata();
		needtodraw=1;
	}
	
	return n;
}

enum GroupInterfaceActions {
	GIA_Align = OIA_MAX,
	GIA_Distribute,
	GIA_Clone,
	GIA_CloneB,
	GIA_Duplicate,
	GIA_DuplicateB,
	GIA_MAX
};

Laxkit::ShortcutHandler *GroupInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=ObjectInterface::GetShortcuts();

	sc->Add(GIA_Align,     'a',0,0,    "Align",       _("Align selected objects"),NULL,0);
	sc->Add(GIA_Distribute,'n',0,0,    "Nup",         _("Distribute selected objects in rows and columns"),NULL,0);
	sc->Add(GIA_Clone,     ' ',ControlMask,0,"Clone", _("Clone objects"),NULL,0);
	sc->Add(GIA_CloneB,    ' ',ControlMask,1,"CloneB",_("Clone objects, if button down"),NULL,0);
	sc->Add(GIA_Duplicate, ' ',0,0,    "Duplicate",   _("Duplicate objects"),NULL,0);
	sc->Add(GIA_DuplicateB,' ',0,1,    "DuplicateB",  _("Duplicate objects, if button down"),NULL,0);
	//...go to path, image, image warp, mesh, gradient tools...?


	// *** initialize nup and align shortcuts.. there should be a better way to do this!!
	NUpInterface nup((int)0);
	nup.GetShortcuts();
	AlignInterface align((int)0);
	align.GetShortcuts();

	return sc;
}

/*! Check for has selection, and button is down.
 */
int GroupInterface::GetMode()
{
	if (selection.n && buttondown.any(0,LEFTBUTTON)) return 1;
	return 0;
}

int GroupInterface::PerformAction(int action)
{
	if (action==GIA_Align) {
		 //change to align interface with the objects
		if (selection.n<=1) return 0;
		AlignInterface *align=new AlignInterface(NULL,10000,dp);
		align->AddToSelection(selection);
		align->owner=this;
		child=align;
		viewport->Push(align,-1,0);
		viewport->postmessage(_("Align"));
		//viewport->Pop(this);
		FreeSelection();
		return 0;

	} else if (action==GIA_Distribute) {
		 //change to nup interface with the objects
		if (selection.n<=1) return 0;
		NUpInterface *nup=new NUpInterface(NULL,10001,dp);
		nup->AddToSelection(selection);
		nup->owner=this;
		child=nup;
		viewport->Push(nup,-1,0);
		viewport->postmessage(_("Flow objects"));
		FreeSelection();
		return 0;

	} else if (action==GIA_Clone || action==GIA_CloneB) {
		 // duplicate selection as clones
		SomeData *obj;
		LSomeDataRef *lobj;
		for (int c=0; c<selection.n; c++) {
			obj=NULL;
			DBG cerr <<" - Clone "<<selection.e[c]->obj->whattype()<<":"<<selection.e[c]->obj->object_id<<endl;

			lobj=new LSomeDataRef();
			lobj->Set(selection.e[c]->obj,0);
			lobj->parent=dynamic_cast<DrawableObject*>(selection.e[c]->obj)->parent;
			obj=lobj;
			viewport->ChangeContext(selection.e[c]);
			viewport->NewData(obj,NULL);
			obj->dec_count();
			PostMessage(_("Cloned."));
		}
		return 0;

	} else if (action==GIA_Duplicate || action==GIA_DuplicateB) {
		 // duplicate selection
		SomeData *obj;
		for (int c=0; c<selection.n; c++) {
			obj=NULL;
			DBG cerr <<" - Duplicate "<<selection.e[c]->obj->whattype()<<":"<<selection.e[c]->obj->object_id<<endl;

			obj=selection.e[c]->obj->duplicate(NULL);
			obj->FindBBox();

			viewport->ChangeContext(selection.e[c]);
			viewport->NewData(obj,NULL);
			obj->dec_count();
			PostMessage(_("Duplicated."));
		}
		return 0;
	}

	return ObjectInterface::PerformAction(action);
}


int GroupInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<" ****************GroupInterface::CharInput"<<endl;
//	if (ch==' ' && selection.n && buttondown.any(0,LEFTBUTTON)) {
//		SomeData *obj;
//		for (int c=0; c<selection.n; c++) {
//			obj=NULL;
//			int cloned=0;
//			if (state&ControlMask) {
//				 // duplicate selection as clones
//				DBG cerr <<" - Clone "<<selection.e[c]->obj->whattype()<<":"<<selection.e[c]->obj->object_id<<endl;
//
//				cloned=1;
//				LSomeDataRef *lobj=new LSomeDataRef();
//				lobj->Set(selection.e[c]->obj,0);
//				lobj->parent=selection.e[c]->obj->parent;
//				obj=lobj;
//			} else {
//				 //duplicate selection
//				DBG cerr <<" - Duplicate "<<selection.e[c]->obj->whattype()<<":"<<selection.e[c]->obj->object_id<<endl;
//
//				cloned=0;
//				obj=selection.e[c]->obj->duplicate(NULL);
//				obj->FindBBox();
//			}
//			if (!obj) continue;
//			viewport->ChangeContext(selection.e[c]);
//			viewport->NewData(obj,NULL);
//			obj->dec_count();
//			PostMessage(cloned?_("Cloned."):_("Duplicated."));
//		}
//		return 0;
//	}
	return ObjectInterface::CharInput(ch,buffer,len,state,d);
}

int GroupInterface::Refresh()
{
	if (!needtodraw) return 0;


	ObjectInterface::Refresh();

	if (selection.n!=1) return 0;


	 //if we are moving only one object, then put various doodads around it.
	 
	DoubleBBox box;
	GetOuterRect(&box,NULL);


	SomeData *obj=selection.e[0]->obj;
	double m[6];
	if (!strcmp(obj->whattype(),"SomeDataRef")) {
		 //is link
		viewport->transformToContext(m,selection.e[0],0,1);
		flatpoint p =transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->maxy))/2); //center
		flatpoint p2=transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->miny))/2); //edge midpoint

		p=dp->realtoscreen(p);
		p2=dp->realtoscreen(p2);

		flatpoint v=p2-p;
		v.normalize();
		p=p2+v*maxtouchlen;

		dp->DrawScreen();
		dp->NewFG(0.,.7,0.);
		dp->drawpoint(p, maxtouchlen/2,hover==GROUP_Link?1:0);
		//dp->PushAndNewTransform(m);
		//dp->PopAxes();

		if (popupcontrols==GROUP_Link) {
			double th=dp->textheight();
			double w=dp->textextent(_("Original"),-1,NULL,NULL)*1.5;
			p.y+=2*th;
			p.x-=w/2;
			if (p.y+th>dp->Maxy) p.y=dp->Maxy-th;
			dp->NewFG(.8,.8,.8);
			if (hover==GROUP_Jump_To_Link) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			dp->drawellipse(p.x,p.y, w/2,th*1.5, 0,2*M_PI, 2);
			if (hover==GROUP_Sever_Link) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			dp->drawellipse(p.x+w,p.y, w/2,th*1.5, 0,2*M_PI, 2);

			dp->NewFG(0.,0.,0.);
			dp->textout(p.x,p.y, _("Jump to"),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->textout(p.x,p.y+th, _("Original"),-1, LAX_HCENTER|LAX_BOTTOM);
			p.x+=w;
			dp->textout(p.x,p.y, _("Sever"),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->textout(p.x,p.y+th, _("Link"),-1, LAX_HCENTER|LAX_BOTTOM);

		}

		dp->DrawReal();
	}

	 //parent link
	//****
	
	 //object edit zone
	//***
	
	 //transform constraints
	//***

	 //chains
	//***

	return 0;
}

//! Returns this, but count is incremented.
Value *GroupInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *GroupInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ObjectInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"ObjectInterface",
            _("Group Interface"),
            _("Group Interface"),
            VALUE_Class,
            NULL,NULL);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}


///*!
// * Return
// *  0 for success, value optionally returned.
// * -1 for no value returned due to incompatible parameters, which aids in function overloading.
// *  1 for parameters ok, but there was somehow an error, so no value returned.
// */
//int GroupInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int GroupInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *GroupInterface::dereference(const char *extstring, int len)
{
	return NULL;
}


} //namespace Laidout

