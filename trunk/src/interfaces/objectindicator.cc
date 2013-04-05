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
// Copyright (C) 2012 by Tom Lechner
//


#include "../language.h"
#include "objectindicator.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>


#include <lax/refptrstack.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



#define PAD 5
#define fudge 5.0






//------------------------------------- ObjectIndicator --------------------------------------
	
/*! \class ObjectIndicator 
 * \brief Interface to build nets out of various shapes. See also ShapeInfo.
 */


ObjectIndicator::ObjectIndicator(int nid,Displayer *ndp)
	: anInterface(nid,ndp) 
{
	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	interface_type=INTERFACE_Overlay;
	context=NULL;
}

ObjectIndicator::ObjectIndicator(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	interface_type=INTERFACE_Overlay;
	context=NULL;
}

ObjectIndicator::~ObjectIndicator()
{
	DBG cerr <<"ObjectIndicator destructor.."<<endl;

	//if (context) delete context;
	//if (doc) doc->dec_count();
}


const char *ObjectIndicator::Name()
{ return _("Object Indicator"); }



/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *ObjectIndicator::ContextMenu(int x,int y,int deviceid)
{
	//MenuInfo *menu=new MenuInfo(_("N-up Interface"));
	//return menu;
	return NULL;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int ObjectIndicator::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		//int i =s->info2; //id of menu item
		int ii=s->info4; //extra id, 1 for direction
		if (ii==0) {

		} else {
			//if (i==NUP_Grid) {
			//} else if (i==NUP_Sized_Grid) {
			//} else if (i==NUP_Flowed) {
			//} else if (i==NUP_Random) {
			//} else if (i==NUP_Unclump) {
			//} else if (i==NUP_Unoverlap) {
			//}
			return 0;

		}

		return 0;
	}
	return 1;
}


/*! Will say it cannot draw anything.
 */
int ObjectIndicator::draws(const char *atype)
{ return 0; }


//! Return a new ObjectIndicator if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *ObjectIndicator::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new ObjectIndicator(id,NULL);
	else if (!dynamic_cast<ObjectIndicator *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int ObjectIndicator::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int ObjectIndicator::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void ObjectIndicator::Clear(SomeData *d)
{
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int ObjectIndicator::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
	}
	if (!context) context=&dynamic_cast<LaidoutViewport*>(viewport)->curobj;
	if (!context) return 0;


	DBG cerr <<"ObjectIndicator::Refresh()..."<<endl;

	dp->DrawScreen();

	//char buffer[30];

	 //draw ui outline
	dp->NewFG(rgbcolor(128,128,128));
	dp->DrawScreen();



	 //draw object place description
	int x=0;
	int y=viewport->win_h;
	ObjectContainer *objc=dynamic_cast<ObjectContainer*>(viewport);
	char scratch[10];
	const char *str;
	dp->NewFG(coloravg(viewport->win_colors->fg, viewport->win_colors->bg));
	for (int c=0; c<context->context.n(); c++) {
		if (!objc) break;

		x=0;
		str=objc->object_e_name(context->context.e(c));
		if (!str) {
			sprintf(scratch,"%d",context->context.e(c));
			str=scratch;
		} else {
			sprintf(scratch,"(%d) ",context->context.e(c));
			x+=dp->textout(0,y, scratch,-1, LAX_BOTTOM|LAX_LEFT);
		}

		dp->textout(x,y, str,-1, LAX_BOTTOM|LAX_LEFT);
		y-=dp->textheight();

		objc=dynamic_cast<ObjectContainer*>(objc->object_e(context->context.e(c)));
	}
	if (!context->obj) {
		//curobj is just a context, not actually selected
		dp->textout(x,y, "?",1, LAX_BOTTOM|LAX_LEFT);
	}




	dp->DrawReal();

	return 1;
}

int ObjectIndicator::scan(int x,int y)
{
	//scan for being inside a shape
	//scan for being on an edge of a shape
	return 0;
}

int ObjectIndicator::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
//	if (buttondown.any(0,LEFTBUTTON)) return 1; //only allow one button at a time
//
//	int action=scan(x,y);
//	buttondown.down(d->id,LEFTBUTTON, x,y);
//
//	return 0;
}

int ObjectIndicator::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ //***
	//if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	//int dragged=buttondown.up(d->id,LEFTBUTTON);


//	if (!dragged) {
//		if ((state&LAX_STATE_MASK)==ControlMask) {
//			// *** edit base
//		} else {
//			// *** edit first
//		}
//	}

	return 1;
}

int ObjectIndicator::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
}

int ObjectIndicator::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
}


int ObjectIndicator::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ 
	//int over=scan(x,y);

	return 1;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int ObjectIndicator::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch==LAX_Esc) {

	} else if (ch=='o') {
	}
	return 1;
}



} // namespace Laidout

