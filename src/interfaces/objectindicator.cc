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
// Copyright (C) 2012 by Tom Lechner
//


#include "../language.h"
#include "objectindicator.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/lineedit.h>

//template implementation:
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


ObjectIndicator::ObjectIndicator(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	firsttime      = 1;
	showdecs       = 0;
	color_arrow    = rgbcolor(60, 60, 60);
	color_num      = rgbcolor(0, 0, 0);
	interface_type = INTERFACE_Overlay;
	context        = NULL;
	hover_object   = NULL;
	last_hover     = -1;
	font           = NULL;
}

ObjectIndicator::ObjectIndicator(int nid,Displayer *ndp)
	: ObjectIndicator(nullptr, nid,ndp) 
{}

ObjectIndicator::~ObjectIndicator()
{
	if (font) font->dec_count();
	DBG cerr <<"ObjectIndicator destructor.."<<endl;
}


const char *ObjectIndicator::Name()
{ return _("Object Indicator"); }



/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *ObjectIndicator::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	//MenuInfo *menu=new MenuInfo(_("N-up Interface"));
	//return menu;
	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int ObjectIndicator::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"renameobj")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!hover_object || isblank(s->str)) return 0;

		hover_object->Id(s->str);
		hover_object=NULL;
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

double ObjectIndicator::MaxWidth()
{
	ObjectContainer *objc=dynamic_cast<ObjectContainer*>(viewport);
	if (!objc) return 0;

	double w=0, ww;
	char scratch[10];
	const char *str;

	for (int c=0; objc && c<context->context.n(); c++) {
		str = objc->object_e_name(context->context.e(c));
		ww = 0;

		if (!str) {
			sprintf(scratch,"%d",context->context.e(c));
			str=scratch;
		} else {
			sprintf(scratch,"(%d) ",context->context.e(c));
			ww = dp->textextent(scratch,-1, NULL,NULL);
		}
		ww += dp->textextent(str,-1, NULL,NULL);

		if (ww > w) w = ww;

		objc=dynamic_cast<ObjectContainer*>(objc->object_e(context->context.e(c)));
	}

	return w;
}

/*! Return 0 for could not determine, either null selection, or no elements in selection. 
 * Return 1 for all elements at same level.
 * Return 2 for elements at mixed levels.
 *
 * Updates oc, but does not set oc->obj.
 */
int CommonAnscestor(Selection *selection, VObjContext *oc)
{
	oc->clear();
	if (!selection) return 0;
	if (selection->n() == 0) return 0;

	VObjContext *ooc;
	int i, level=0, done=0;

	while(1) {
		i=-1;
		for (int c=0; c<selection->n(); c++) {
			ooc = dynamic_cast<VObjContext*>(selection->e(c));
			if (ooc->context.n() == level) { done=1; break; }

			if (c==0) i = ooc->context.e(level);
			else {
				if (i != ooc->context.e(level)) { done=1; break; }
			}
		}

		if (done) break;
		oc->push(i);
		level++;
	}

	i = dynamic_cast<VObjContext*>(selection->e(0))->context.n();
	for (int c=1; c<selection->n(); c++) {
		if (dynamic_cast<VObjContext*>(selection->e(c))->context.n() != i) return 2;
	}
	return 1;
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
	dp->NewBG(1.,1.,1.);

	//char buffer[30];

	 //draw ui outline
	dp->DrawScreen();

	if (!font) { font=laidout->defaultlaxfont; font->inc_count(); }
	dp->font(font);
	double th = dp->textheight();

	flatpoint origin(0,viewport->win_h);
	if (last_hover >= 0) {
		 //blank out rect around it
		dp->NewFG(1.,1.,1.,.9);
		double w = MaxWidth() + th;
		double h = context->context.n()*th + th;
		if (!context->obj) h += th;
		dp->drawRoundedRect(origin.x-th/2,origin.y+th/2-h,
				w, h,
				th/2,false, th/2,false, 1, 15);

	}

	dp->NewFG(.5,.5,.5);

	 //draw object place description
	int x=0;
	int y=viewport->win_h;
	ObjectContainer *objc = dynamic_cast<ObjectContainer*>(viewport);
	char scratch[10];
	const char *str;
	dp->NewFG(coloravg(viewport->win_themestyle->fg, viewport->win_themestyle->bg));

	for (int c=0; objc && c<context->context.n(); c++) {
		if (!objc) break;

		if (c == last_hover) dp->NewFG(.2,.2,.2);

		 // textout: (index) element name
		x = 0;
		str = objc->object_e_name(context->context.e(c));
		if (!str) {
			//no label, use index number
			sprintf(scratch,"%d",context->context.e(c));
			str = scratch;
		} else {
			//label exists, write out index number before actual label
			sprintf(scratch,"(%d) ",context->context.e(c));
			x += dp->textout_halo(1, 0,y, scratch,-1, LAX_BOTTOM|LAX_LEFT);

			anObject *obj = objc->object_e(context->context.e(c));
			if (dynamic_cast<Page*>(obj) != NULL && strcmp(str, "page")) {
				sprintf(scratch, "page %s", dynamic_cast<Page*>(obj)->label);
				str = scratch;
			}
		}


		dp->textout_halo(1, x,y, str,-1, LAX_BOTTOM|LAX_LEFT);
		y-=dp->textheight();

		if (c == last_hover) dp->NewFG(.5,.5,.5);

		objc = dynamic_cast<ObjectContainer*>(objc->object_e(context->context.e(c)));
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
	if (!context) context=&dynamic_cast<LaidoutViewport*>(viewport)->curobj;

	if (!font) { font=laidout->defaultlaxfont; font->inc_count(); }
	double th=font->textheight();

	if (x<0 || x>MaxWidth()) return -1;

	int i=(dp->Maxy-y)/th;
	if (i>=context->context.n()) i=-1;
	return i;
}

int ObjectIndicator::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any(0,LEFTBUTTON)) return 1; //only allow one button at a time

	int i=scan(x,y);
	if (i<0) return 1;
	
	DBG cerr <<" -------------lbdown indicator: "<<i<<endl;
	buttondown.down(d->id,LEFTBUTTON, x,y, i);
	return 0;
}

int ObjectIndicator::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int i=-1;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &i);

	if (!dragged) {
		 //get context.e(i) and change its name
		ObjectContainer *objc=dynamic_cast<ObjectContainer*>(viewport);

		for (int c=0; c<i && c<context->context.n(); c++) {
			if (!objc) break;
			objc=dynamic_cast<ObjectContainer*>(objc->object_e(context->context.e(c)));
		}

		anObject *o=objc->object_e(context->context.e(i));
		DrawableObject *d=dynamic_cast<DrawableObject*>(o);

		if (d && d->parent && d->Selectable()) {
			if (i == context->context.n()-1 && context->obj) {
				 //rename top object
				//const char *str=objc->object_e_name(context->context.e(i));
				const char *str=d->Id();
				if (!font) { font=laidout->defaultlaxfont; font->inc_count(); }
				double th=font->textheight(); 

				LineEdit *le= new LineEdit(viewport,"rename",_("Rename object"),
											ANXWIN_OUT_CLICK_DESTROYS|LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE,
											2*th,dp->Maxy-(i+3)*th, 2*dp->textextent(str,-1,NULL,NULL),1.2*th, 4,
											   NULL,object_id,"renameobj",
											   str);
				hover_object=d;
				le->padx=le->pady=th*.1;
				le->SetCurpos(-1);
				app->addwindow(le);

			} else {
				 //select this object
				while (context->context.n()-1 != i) context->pop(); 
				context->SetObject(d);
				viewport->ChangeObject(context, false, true);
				needtodraw=1;
			}
		}
	}

	return 0;
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
	int i=scan(x,y);
	if (i!=last_hover) needtodraw=1;
	last_hover=i;

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

