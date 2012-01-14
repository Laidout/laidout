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
#include "nupinterface.h"
//#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>


#include <lax/refptrstack.cc>

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


#define NUP_Major_Arrow  1
#define NUP_Minor_Arrow  2
#define NUP_Major_Number 3
#define NUP_Minor_Number 4
#define NUP_Major_Tip    5
#define NUP_Minor_Tip    6
#define NUP_Type         7
#define NUP_Ok           8

#define NUP_Grid         100
#define NUP_Sized_Grid   101
#define NUP_Flowed       102
#define NUP_Random       103
#define NUP_Unclump      104
#define NUP_Unoverlap    105


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
	flowtype=0;
	name=NULL;

	transform_identity(final_grid_offset);
	transform_identity(ui_offset);
}

NUpInfo::~NUpInfo()
{
	if (name) delete[] name;
}



//------------------------------------- NUpInterface --------------------------------------
	
/*! \class NUpInterface 
 * \brief Interface to define and modify n-up style arrangements. See also NUpInfo.
 */


NUpInterface::NUpInterface(int nid,Displayer *ndp)
	: anInterface(nid,ndp) 
{
	tempdir=0;

	showdecs=0;
	firsttime=1;
	//doc=NULL;
	//UseThisDocument(ndoc);
}

NUpInterface::NUpInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	tempdir=0;

	showdecs=0;
	firsttime=1;
	//doc=NULL;
	//UseThisDocument(ndoc);
}

NUpInterface::~NUpInterface()
{
	DBG cerr <<"NUpInterface destructor.."<<endl;

	//if (doc) doc->dec_count();
}

flatpoint *rect_coordinates(flatpoint *p, double x, double y,double w,double h)
{
	p[0]=flatpoint(x,y);
	p[1]=flatpoint(x+w,y);
	p[2]=flatpoint(x+w,y+h);
	p[3]=flatpoint(x,y+h);
	return p;
}

//! Get coordinates for straight arrows of various lengths, pointing left, right, up or down.
/*! Total height is h, height of tail is t, arrow head width is a.
 * Coordinates start with one point on tail, and end on the other.
 * Returns a new array with 7 points.
 */
flatpoint *arrow_coordinates(int dir, flatpoint *p, double x, double y,double w,double h, double a, double t)
{
	if (!p) p=new flatpoint[7];

	double tt;
	if (dir==LAX_TBLR || dir==LAX_TBRL || dir==LAX_BTLR || dir==LAX_BTRL) { tt=w; w=h; h=tt; }
	
	p[0]=flatpoint(x+0,   y+h/2+t/2);
	p[1]=flatpoint(x+w-a, y+h/2+t/2);
	p[2]=flatpoint(x+w-a, y+h/2+h/2);
	p[3]=flatpoint(x+w,   y+h/2+0);
	p[4]=flatpoint(x+w-a, y+h/2+-h/2);
	p[5]=flatpoint(x+w-a, y+h/2+-t/2);
	p[6]=flatpoint(x+0,   y+h/2+-t/2);

	if (dir==LAX_RLTB || dir==LAX_RLBT || dir==LAX_TBLR || dir==LAX_TBRL) {
		 //reverse direction of arrow
		for (int c=0; c<7; c++) { p[c].x=w-p[c].x; }
	}

	 //position and flip as necessary
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		for (int c=0; c<7; c++) { p[c].x+=x; p[c].y+=y; }
	} else {
		for (int c=0; c<7; c++) { tt=p[c].x; p[c].x=y; p[c].y=tt; p[c].x+=x; p[c].y+=y; }
	}

	return p;
}

////! Define an arrow from to to, with tail with w. Puts 7 points in p. p must be allocated for at least that many.
//flatpoint *arrow_coordinates2(flatpoint *p, flatpoint from, flatpoint to, double w)
//{
//
//	***
//	return p;
//}

void NUpInterface::createControls()
{
	controls.push(new ActionArea(NUP_Major_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),1,1,color_arrows,0));
	controls.push(new ActionArea(NUP_Minor_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),1,1,color_arrows,0));
	controls.push(new ActionArea(NUP_Major_Number       , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));
	controls.push(new ActionArea(NUP_Minor_Number       , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));

	controls.push(new ActionArea(NUP_Ok                 , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));
	controls.push(new ActionArea(NUP_Type               , AREA_Handle, NULL, _("Wheel to change"),1,1,color_num,0));

	major      =controls.e[0];
	minor      =controls.e[1];
	majornum   =controls.e[2];
	minornum   =controls.e[3];
	okcontrol  =controls.e[4];
	typecontrol=controls.e[5];
}

void NUpInterface::remapControls()
{
	double ww, hh;
	double ww4, hh4;
	double major_w=100;
	double minor_w=75;

	 //define major arrow
	if (dir=LAX_LRTB || dir==LAX_LRBT || dir=LAX_RLTB || dir==LAX_RLBT) {
		 //major arrow is horizontal, adjust position downward if at bottom
		if (dir==LAX_LRTB || dir==LAX_RLTB) y=totalh*3/4; else y=0;
		p=major->Points(NULL,7,0);
		arrow_coordinates(dir,p, ww4, y+ww4*3/4,  hh4/4, ww4,hh4/3);
		major->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_LRBT) rect_coordinates(p, 0,y,ww4,hh4);
		else rect_coordinates(p, 3*ww4,y,ww4,hh4);
		majornum->FindBBox();

	} else {
		 //major arrow is vertical
		if (dir==LAX_TBRL || dir==LAX_BTRL) x=totalw*3/4; else x=0;
		p=major->Points(NULL,7,0);
		arrow_coordinates(dir,p, x+1.5*ww4, y+ww4*3/4,  hh4/4, ww4,hh4/3);
		area->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_BTRL) rect_coordinates(p, x,0,ww4,hh4);
		else rect_coordinates(p, x,y+3*hh4, ww4,hh4);
		majornum->FindBBox();
	}

	 //define minor arrow
	if (dir=LAX_LRTB || dir==LAX_LRBT || dir=LAX_RLTB || dir==LAX_RLBT) {
		 //minor arrow is vertical, adjust position downward if at bottom
		if (dir==LAX_LRTB || dir==LAX_RLTB) y=0; else y=totalh*2/4;
		p=minor->Points(NULL,7,0);
		arrow_coordinates(dir,p, x+1.5*ww4,y,  ww4,2*hh4, hh4/2,hh4/6);
		major->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB) rect_coordinates(p, 1.5*ww4,hh4, ww4,hh4);
		else rect_coordinates(p, 1.5*ww4,2*hh4, ww4,hh4);
		majornum->FindBBox();

	} else {
		 //minor arrow is horizontal
		if (dir==LAX_TBRL || dir==LAX_BTRL) x=0; else x=totalw*2/4;
		p=minor->Points(NULL,7,0);
		arrow_coordinates(dir,p, x,y+ww4*3/2,  2*ww4,hh4, ww4/2,hh4/6);
		minor->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_TBLR) rect_coordinates(p, ww4,1.5*hh4,ww4,hh4);
		else rect_coordinates(p, 2*ww4,1.5*hh4, ww4,hh4);
		minornum->FindBBox();
	}

	 //type of flow
	p=typecontrol->Points(NULL,4,0);
	rect_coordinates(p, ww4,5*hh4, 2*ww4,hh4);
	typecontrol->FindBBox();

	 //optional ok button
	if (nup_style&NUP_Has_Ok) {
		p=okcontrol->Points(NULL,4,0);
		rect_coordinates(p, ww4,6.5*hh4, 2*ww4,hh4);
		okcontrol->FindBBox();
		okcontrol->hidden=0;
	} else {
		okcontrol->hidden=1;
	}
}

const char *NUpInterface::Name()
{ return _("N-up"); }


/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *NUpInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("N-up Interface"));

	menu->AddItem(_("Left to right, top to bottom"),LAX_LRTB,LAX_OFF,1);
	menu->AddItem(_("Left to right, bottom to top"),LAX_LRBT,LAX_OFF,1);
	menu->AddItem(_("Right to left, top to bottom"),LAX_RLTB,LAX_OFF,1);
	menu->AddItem(_("Right to left, bottom to top"),LAX_RLBT,LAX_OFF,1);
	menu->AddItem(_("Top to bottom, left to right"),LAX_TBLR,LAX_OFF,1);
	menu->AddItem(_("Bottom to top, left to right"),LAX_BTLR,LAX_OFF,1);
	menu->AddItem(_("Top to bottom, right to left"),LAX_TBRL,LAX_OFF,1);
	menu->AddItem(_("Bottom to top, right to left"),LAX_BTRL,LAX_OFF,1);

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
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item
		int ii=s->info4; //extra id, 1 for direction
		if (ii==0) {
			cerr <<"change direction to "<<i<<endl;
			return 0;

		} else {
			if (i==NUP_Grid) {
			} else if (i==NUP_Sized_Grid) {
			} else if (i==NUP_Flowed) {
			} else if (i==NUP_Random) {
			} else if (i==NUP_Unclump) {
			} else if (i==NUP_Unoverlap) {
			}
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
{
//	if (ndoc==doc) return 0;
//	if (doc) doc->dec_count();
//	doc=ndoc;
//	if (ndoc) ndoc->inc_count();

	return 0;
}

//! Use a Document.
int NUpInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
//	Document *d=dynamic_cast<Document *>(ndata);
//	if (!d && ndata) return 0; //was a non-null object, but not a document
//
//	UseThisDocument(d);
//	needtodraw=1;

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
{
	if (dup==NULL) dup=new NUpInterface(id,NULL);
	else if (!dynamic_cast<NUpInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int NUpInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int NUpInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void NUpInterface::Clear(SomeData *d)
{
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int NUpInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
		yscale=50;
		xscale=dp->Maxx-dp->Minx-10;
	}

	DBG cerr <<"NUpInterface::Refresh()..."<<positions.n<<endl;

	dp->DrawScreen();

	***

	dp->DrawReal();

	return 1;
}

//! Return which position and range mouse is over
int NUpInterface::scan(int x,int y)
{
	flatpoint fp=screentoreal(x,y);
	int action, dir=nupinfo->direction;
	double w,h;
	for (int c=0; c<controls.n; c++) {
		if (controls.e[c]->hidden) continue;

		if (controls.e[c]->category==0) {
			 //normal, single instance handles
			if (!controls.e[c]->real && controls.e[c]->PointIn(x,y)) {
				w=controls.e[c]->maxx-controls.e[c]->minx;
				h=controls.e[c]->maxy-controls.e[c]->miny;
				action=controls.e[c]->action;
				if (action==NUP_Major_Arrow) {
					if ((dir==LAX_LRTB || dir==LAX_LRBT) && x>(controls.e[c]->minx+.75*w))
						action=NUP_Major_Tip;
					else if ((dir==LAX_RLTB || dir==LAX_RLBT) && x<(controls.e[c]->minx+.25*w))
						action=NUP_Major_Tip;
					else if ((dir==LAX_TBLR || dir==LAX_TBRL) && y<(controls.e[c]->miny+.25*h))
						action=NUP_Major_Tip;
					else if ((dir==LAX_BTLR || dir==LAX_BTRL) && y>(controls.e[c]->miny+.75*h))
						action=NUP_Major_Tip;

				} else if (action==NUP_Minor_Arrow) {
					if ((dir==LAX_LRTB || dir==LAX_RLTB) && y>(controls.e[c]->miny+.6*h))
						action=NUP_Minor_Tip;
					else if ((dir==LAX_RLBT || dir==LAX_LRBT) && y<(controls.e[c]->miny+.3*h))
						action=NUP_Minor_Tip;
					else if ((dir==LAX_TBLR || dir==LAX_BTLR) && x>(controls.e[c]->minx+.6*w))
						action=NUP_Minor_Tip;
					else if ((dir==LAX_TBRL || dir==LAX_BTRL) && x<(controls.e[c]->minx+.3*w))
						action=NUP_Minor_Tip;
				}
				return action;
			}
		}
	}

	return 0;
}

int NUpInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ //***
//	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
//	buttondown.down(d->id,LEFTBUTTON,x,y,state);
//
//	int r=-1;
//	int over=scan(x,y,&r);
//	flatpoint fp=dp->screentoreal(x,y);
//
//	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
//	}
//
//	if (count==2) { // ***
//	}

	return 0;
}

int NUpInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ //***
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

//	if (!dragged) {
//		if ((state&LAX_STATE_MASK)==ControlMask) {
//			// *** edit base
//		} else {
//			// *** edit first
//		}
//	}

	return 0;
}

//int NUpInterface::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int NUpInterface::MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int NUpInterface::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int NUpInterface::RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int NUpInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
//int NUpInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);

int NUpInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ //***
	int r=-1;
	int over=scan(x,y,&r);

	DBG cerr <<"over pos,range: "<<over<<","<<r<<endl;

//	int lx,ly;
//
//	if (!buttondown.any()) return 0;
//
//	buttondown.move(mouse->id,x,y, &lx,&ly);
//	DBG cerr <<"pr last m:"<<lx<<','<<ly<<endl;
//
//	if ((state&LAX_STATE_MASK)==0) {
//		offset.x-=x-lx;
//		offset.y-=y-ly;
//		needtodraw=1;
//		return 0;
//	}
//
//	 //^ scales
//	if ((state&LAX_STATE_MASK)==ControlMask) {
//	}
//
//	 //+^ rotates
//	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
//	}

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
{ //***
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
{ //***
	return 1;
}


//} // namespace Laidout

