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
// Copyright (C) 2013 by Tom Lechner
//


#include "../language.h"
#include "anchorinterface.h"
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









//------------------------------------- AnchorInterface --------------------------------------
	
/*! \class AnchorInterface 
 * \brief Interface to build nets out of various shapes. See also AnchorInfo.
 */


enum AnchorPlaces
{
	ANCHOR_Parents      =(1<<0),
	ANCHOR_Other_Objects=(1<<1),
	ANCHOR_Margin_Area  =(1<<2),
	ANCHOR_Page_Area    =(1<<3),
	ANCHOR_Paper        =(1<<4),
	ANCHOR_Guides       =(1<<5),
	ANCHOR_MAX
};


AnchorInterface::AnchorInterface(int nid,Displayer *ndp)
	: AlignInterface(nid,ndp) 
{
	firsttime=1;
	showdecs=0;
	aligni_style|=ALIGNI_Hide_Path;
	showrotation=0;

	aligninfo->snap_align_type  =FALIGN_Align;
	aligninfo->final_layout_type=FALIGN_Align;
}

AnchorInterface::AnchorInterface(anInterface *nowner,int nid,Displayer *ndp)
	: AlignInterface(nowner,nid,ndp) 
{
	firsttime=1;
	showdecs=0;
	aligni_style|=ALIGNI_Hide_Path;
	showrotation=0;

	aligninfo->snap_align_type  =FALIGN_Align;
	aligninfo->final_layout_type=FALIGN_Align;
}

AnchorInterface::~AnchorInterface()
{
	DBG cerr <<"AnchorInterface destructor.."<<endl;

	//if (doc) doc->dec_count();
}


const char *AnchorInterface::Name()
{ return _("Anchor"); }



/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *AnchorInterface::ContextMenu(int x,int y,int deviceid)
{
	//MenuInfo *menu=new MenuInfo(_("Anchor Interface"));

	//menu->AddItem(dirname(LAX_BTRL),LAX_BTRL,LAX_ISTOGGLE|(shapeinfo->direction==LAX_BTRL?LAX_CHECKED:0)|LAX_OFF,1);
	//menu->AddSep();

	//return menu;
	return NULL;
}

/*! Return 0 for success.
 */
int AnchorInterface::AddAnchor(flatpoint p,const char *name)
{
	PointAnchor *a=new PointAnchor(name,PANCHOR_Absolute,p,flatpoint(),0);
	anchors.push(a);
	a->dec_count();
	return 0;
}

/*! Return number of anchors added.
 */
int AnchorInterface::AddAnchors(VObjContext *context)
{ 
	if (!dynamic_cast<DrawableObject*>(context->obj)) return 0;
	double m[6];
	viewport->transformToContext(m,context,0,1);
	DrawableObject *d=dynamic_cast<DrawableObject*>(context->obj);
	flatpoint p,pp;
	for (int c=0; c<d->NumAnchors(); c++) {
		d->GetAnchorI(c,&p,0,NULL);
		pp=transform_point(m,p);
		AddAnchor(pp,NULL);
	}

	return d->NumAnchors();
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int AnchorInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item
		int ii=s->info4; //extra id, 1 for direction
		if (ii==0) {
			cerr <<"change direction to "<<i<<endl;
			return 0;

		} else {
			return 0;

		}

		return 0;
	}
	return 1;
}


/*! Will say it cannot draw anything.
 */
int AnchorInterface::draws(const char *atype)
{ return 0; }


//! Return a new AnchorInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *AnchorInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new AnchorInterface(id,NULL);
	else if (!dynamic_cast<AnchorInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int AnchorInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	aligninfo->center=dp->screentoreal((dp->Minx+dp->Maxx)/2,(dp->Miny+dp->Maxy)/2);

	showdecs=2;
	needtodraw=1;
	return 0;
}

int AnchorInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void AnchorInterface::Clear(SomeData *d)
{
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int AnchorInterface::Refresh()
{
	if (!needtodraw) return 0;

	DBG cerr <<"AnchorInterface::Refresh()..."<<endl;

	AlignInterface::Refresh();



	 //draw ui outline
	dp->NewFG(rgbcolor(128,128,128));
	dp->DrawScreen();
	

	flatpoint p;
	for (int c=0; c<anchors.n; c++) {
		p=dp->realtoscreen(anchors.e[c]->p);

		dp->NewFG(&anchors.e[c]->color);
		dp->drawthing(p.x,p.y, 4,4, 1, THING_Circle);
		dp->NewFG(~0);
		dp->drawthing(p.x,p.y, 5,5, 0, THING_Circle);

		if (anchors.e[c]->name) {
			dp->textout(p.x+5,p.y, anchors.e[c]->name,-1, LAX_LEFT|LAX_VCENTER);
		}
	}


	dp->DrawReal();


	needtodraw=0;
	return 1;
}

int AnchorInterface::scan(int x,int y)
{
	//scan for being inside a shape
	//scan for being on an edge of a shape
	return 0;
}

int AnchorInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any(0,LEFTBUTTON)) return 0; //only allow one button at a time

	//int action=scan(x,y);
	//buttondown.down(d->id,LEFTBUTTON, x,y);

	return AlignInterface::LBDown(x,y,state,count,d);

}

int AnchorInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ //***
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	//int dragged=buttondown.up(d->id,LEFTBUTTON);

	return AlignInterface::LBUp(x,y,state,d);

//	if (!dragged) {
//		if ((state&LAX_STATE_MASK)==ControlMask) {
//			// *** edit base
//		} else {
//			// *** edit first
//		}
//	}

	return 0;
}

int AnchorInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
}

int AnchorInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
}


int AnchorInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ //***

	//int over=scan(x,y);

	return AlignInterface::MouseMove(x,y,state,mouse);
}

int AnchorInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{ //***
	DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	return AlignInterface::CharInput(ch,buffer,len,state,d);


//	if (ch==LAX_Esc) {
//
//	} else if (ch=='o') {
//	}
	return 1;
}



} // namespace Laidout

