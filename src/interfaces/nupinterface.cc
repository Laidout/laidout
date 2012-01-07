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
// Copyright (C) 2011 by Tom Lechner
//


#include "../language.h"
#include "pagerangeinterface.h"
#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


#define PAD 5
#define fudge 5.0



//  ^     3--->   ^          5
//  |       5     |3-->  <--3|
//  5       |     |          |
//3---->    v     5          v
//
//Moveable box around this ^^ adjuster
//Scaling box for preview of adjustment
//
//int direction; //lrtb, lrbt, rltb, rlbt, ...
//int rows, cols;
//DoubleBBox final_grid_bounds;
//double final_grid_offset[6];
//
//double ui_offset[6];



//------------------------------------- NUpInfo --------------------------------------
/*! \class NUpInfo
 * \brief Info about how to arrange in an n-up style.
 *
 * Set flow direction, number of rows and columns.
 *
 * See also NUpInterface.
 */


NUpInfo::NUpInfo()
{
	direction=LAX_LRTB;
	rows=cols=1;
	rowcenter=colcenter=50;
	cellstyle=0;

	transform_identity(final_grid_offset);
	transform_identity(ui_offset);
}

NUpInfo::~NUpInfo()
{
}



//------------------------------------- NUpInterface --------------------------------------
	
/*! \class NUpInterface 
 * \brief Interface to define and modify n-up style arrangements. See also NUpInfo.
 */


NUpInterface::NUpInterface(int nid,Displayer *ndp,Document *ndoc)
	: anInterface(nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;

	showdecs=0;
	firsttime=1;
	doc=NULL;
	UseThisDocument(ndoc);

	SetupBoxes();
}

NUpInterface::NUpInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;

	showdecs=0;
	firsttime=1;
	doc=NULL;

	SetupBoxes();
}

NUpInterface::~NUpInterface()
{
	DBG cerr <<"NUpInterface destructor.."<<endl;

	if (doc) doc->dec_count();
}

//! Get coordinates for straight arrows of various lengths, pointing left, right, up or down.
/*! Total height is h, height of tail is t, arrow head width is a.
 * Coordinates start with one point on tail, and end on the other.
 * Returns a new array with 7 points.
 */
flatpoint arrow_coordinates(int dir, flatpoint *p, double x, double y,double w,double h, double a, double t)
{
	if (!p) p=new flatpoint[7];

	p[0]=flatpoint(x+0,   y+h/2+t/2);
	p[1]=flatpoint(x+w-a, y+h/2+t/2);
	p[2]=flatpoint(x+w-a, y+h/2+h/2);
	p[3]=flatpoint(x+w,   y+h/2+0);
	p[4]=flatpoint(x+w-a, y+h/2+-h/2);
	p[5]=flatpoint(x+w-a, y+h/2+-t/2);
	p[6]=flatpoint(x+0,   y+h/2+-t/2);

	return p;
}

void NUpInterface::createControls()
{
	controls.push(new ActionArea(NUP_Major_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),1,1,color_arrows,0));
	controls.push(new ActionArea(NUP_Minor_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),1,1,color_arrows,0));
	controls.push(new ActionArea(NUP_Major_Number       , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));
	controls.push(new ActionArea(NUP_Minor_Number       , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));

	controls.push(new ActionArea(NUP_Ok                 , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));
	controls.push(new ActionArea(NUP_Type               , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));
}

void NUpInterface::setArrow(int type, flatpoint *p)
{
	if (dir==LAX_LRTB) {
		major=1;
		minor=
	}
}

void NUpInterface::remapControls()
{ ***

	double ww, hh;
	double ww4, hh4;
	double major_w=100;
	double minor_w=75;

	ActionArea *major=control(NUP_Major_Arrow);
	ActionArea *minor=control(NUP_Minor_Arrow);

	if (dir=LAX_LRTB || dir==LAX_LRBT) {
		 //major arrow is left to right
		if (dir==LAX_LRTB) y=totalh*3/4; else y=0;
		p=area->Points(NULL,7,0);
		arrow_coordinates(dir,p, ww4, y+ww4*3/4, hh4/4, ww4,hh4/3);
		area->FindBBox();

		---------------------
	} else if (dir=LAX_RLTB || dir==LAX_RLBT) {
		 //major arrow is right to left
		p=area->Points(NULL,7,0);
		arrow_coordinates(dir,p, 0,0,major_w,major_w/5, major_w/5,major_w/5/3);
		area->FindBBox();

	} else if (dir=LAX_LRTB || dir==LAX_LRBT) {
		 //major arrow is left to right
		p=area->Points(NULL,7,0);
		arrow_coordinates(dir,p, 0,0,major_w,major_w/5, major_w/5,major_w/5/3);
		area->FindBBox();

	} else if (dir=LAX_RLTB || dir==LAX_RLBT) {
		 //major arrow is right to left
		p=area->Points(NULL,7,0);
		arrow_coordinates(dir,p, 0,0,major_w,major_w/5, major_w/5,major_w/5/3);
		area->FindBBox();
	}
}

const char *NUpInterface::Name()
{ return _("N-up"); }


/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *NUpInterface::ContextMenu(int x,int y,int deviceid)
{ ***
	MenuInfo *menu=new MenuInfo(_("N-up Interface"));

	menu->AddItem(_("Left to right, top to bottom"),LAX_LRTB);
	menu->AddItem(_("Left to right, bottom to top"),LAX_LRBT);
	menu->AddItem(_("Right to left, top to bottom"),LAX_RLTB);
	menu->AddItem(_("Right to left, bottom to top"),LAX_RLBT);
	menu->AddItem(_("Top to bottom, left to right"),LAX_TBLR);
	menu->AddItem(_("Bottom to top, left to right"),LAX_BTLR);
	menu->AddItem(_("Top to bottom, right to left"),LAX_TBRL);
	menu->AddItem(_("Bottom to top, right to left"),LAX_BTRL);

	menu->AddSep();

	menu->AddItem(_("Grid"),NUP_Grid);
	menu->AddItem(_("Sized Grid"),NUP_Sized_Grid);
	menu->AddItem(_("Flowed"),NUP_Flowed);
	menu->AddItem(_("Random"),NUP_Random);
	menu->AddItem(_("Unclump"),NUP_Unclump);
	menu->AddItem(_("Unoverlap"),NUP_Unoverlap);

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int NUpInterface::Event(const Laxkit::EventData *e,const char *mes)
{  ***
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item
		if (i==RANGE_Custom_Base) {
			return 0;

		} else if (i==RANGE_Delete) {
			return 0;

		}

		return 0;
	}
	return 1;
}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int NUpInterface::UseThisDocument(Document *ndoc)
{ ***
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();

	positions.flush();
	double total=doc->pageranges.n;
	if (doc->pageranges.n==0) {
		total=1;
		positions.push(0);
	} else {
		for (int c=0; c<doc->pageranges.n; c++) {
			if (doc->pageranges.e[c]->start==0) positions.push(0);
			positions.push(doc->pageranges.e[c]->start/(total-1));
		}
	}
	positions.push(1);

	return 0;
}

//! Use a Document.
int NUpInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{ ***
	Document *d=dynamic_cast<Document *>(ndata);
	if (!d && ndata) return 0; //was a non-null object, but not a document

	UseThisDocument(d);
	needtodraw=1;

	return 1;
}

/*! Will say it cannot draw anything.
 */
int NUpInterface::draws(const char *atype)
{ return 0; }


//! Return a new NUpInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *NUpInterface::duplicate(anInterface *dup)//dup=NULL
{ ***
	if (dup==NULL) dup=new NUpInterface(id,NULL);
	else if (!dynamic_cast<NUpInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int NUpInterface::InterfaceOn()
{ ***
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	yscale=50;
	xscale=dp->Maxx-dp->Minx-2*PAD;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int NUpInterface::InterfaceOff()
{ ***
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void NUpInterface::Clear(SomeData *d)
{ ***
	offset.x=offset.y=0;
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int NUpInterface::Refresh()
{ ***
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
		yscale=50;
		xscale=dp->Maxx-dp->Minx-10;
	}

	DBG cerr <<"NUpInterface::Refresh()..."<<positions.n<<endl;

	dp->DrawScreen();

	double w,h=yscale;
	flatpoint o(dp->Minx+PAD,dp->Maxy-PAD);
	o-=offset;
	o.y-=h;

	 //draw blocks
	char *str=NULL;
	for (int c=0; c<positions.n-1; c++) {
		w=xscale*(positions.e[c+1]-positions.e[c]);

		DBG cerr<<"PageRange interface drawing rect "<<o.x<<','<<o.y<<" "<<w<<"x"<<h<<"  offset:"<<offset.x<<','<<offset.y<<endl;

		if (doc->pageranges.n) dp->NewFG(&doc->pageranges.e[c]->color);
		else dp->NewFG(rgbcolor(255,255,255));
		dp->drawrectangle(o.x,o.y, w,h, 1);

		dp->NewFG((unsigned long)0);
		dp->drawrectangle(o.x,o.y, w,h, 0);

		str=LabelPreview(c,-1,Numbers_Default);
		dp->textout(o.x,o.y+h/2, str,-1, LAX_LEFT|LAX_VCENTER);

		o.x+=w;
	}
	dp->DrawReal();

	return 1;
}

//! Return which position and range mouse is over
int NUpInterface::scan(int x,int y)
{ ***
	flatpoint fp(x,y);
	fp-=offset;
	fp.x/=xscale;
	fp.y/=yscale;

	if (fp.y<0 || fp.y>1) return -1;

	int r=-1, pos=-1;
	for (int c=0; c<positions.n; c++) {
		if (c<positions.n-1 && fp.x>=positions.e[c] && fp.x<=positions.e[c+1]) r=c;
		if (fp.x>=positions.e[c]-fudge && fp.x<=positions.e[c]+fudge) pos=c;
	}

	if (range) *range=r;
	return pos;
}

int NUpInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ ***
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	int r=-1;
	int over=scan(x,y,&r);
	flatpoint fp=dp->screentoreal(x,y);

	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
	}

	if (count==2) { // ***
	}

	return 0;
}

int NUpInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ ***
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (!dragged) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			// *** edit base
		} else {
			// *** edit first
		}
	}

	//***
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

//int NUpInterface::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int NUpInterface::MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int NUpInterface::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int NUpInterface::RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int NUpInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
//int NUpInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);

int NUpInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ ***
	int r=-1;
	int over=scan(x,y,&r);

	DBG cerr <<"over pos,range: "<<over<<","<<r<<endl;

	int lx,ly;

	if (!buttondown.any()) return 0;

	buttondown.move(mouse->id,x,y, &lx,&ly);
	DBG cerr <<"pr last m:"<<lx<<','<<ly<<endl;

	if ((state&LAX_STATE_MASK)==0) {
		offset.x-=x-lx;
		offset.y-=y-ly;
		needtodraw=1;
		return 0;
	}

	 //^ scales
	if ((state&LAX_STATE_MASK)==ControlMask) {
	}

	 //+^ rotates
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
	}

	return 0;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int NUpInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{ ***
	DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch==LAX_Esc) {

	} else if (ch==LAX_Shift) {
	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='r' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='d' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='9' && (state&LAX_STATE_MASK)==0) {
	}
	return 1;
}

int NUpInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{ ***
	return 1;
}


//} // namespace Laidout

