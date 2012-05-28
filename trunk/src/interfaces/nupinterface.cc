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
	rows=3;
	cols=3;
	rowcenter=colcenter=50;
	flowtype=NUP_Grid;
	name=NULL;

	scale=1;

	minx=0; maxx=100;
	miny=0; maxy=100;
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

	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	overoverlay=-1;
	tempdir=-1;
	temparrowdir=-1;
	active=0;

	nupinfo=new NUpInfo;
	nupinfo->uioffset=flatpoint(150,150);
	createControls();
}

NUpInterface::NUpInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	tempdir=0;

	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);
	overoverlay=-1;
	tempdir=-1;
	temparrowdir=-1;
	active=0;

	nupinfo=new NUpInfo;
	nupinfo->uioffset=flatpoint(150,150);
	createControls();
}

NUpInterface::~NUpInterface()
{
	DBG cerr <<"NUpInterface destructor.."<<endl;

	if (nupinfo) nupinfo->dec_count();

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

//! Define an arrow from to to, with tail width w. Puts 7 points in p. p must be allocated for at least that many.
/*! Arrow height will be 3*w.
 * Returns p.
 */
flatpoint *arrow_coordinates(flatpoint *p, flatpoint from, flatpoint to, double w)
{
	flatpoint x,y;
	x=to-from;
	double d=norm(x);
	x/=d;
	y=transpose(x);

	double a=w*2;

	p[0]=from +     0*x + -w/2*y;
	p[1]=from + (d-a)*x + -w/2*y;
	p[2]=from + (d-a)*x +   -2*w*y;
	p[3]=from +     d*x +    0*y;
	p[4]=from + (d-a)*x +    2*w*y;
	p[5]=from + (d-a)*x +  w/2*y;
	p[6]=from +     0*x +  w/2*y;

	return p;
}

const char *NUpInterface::controlTooltip(int action)
{
	for (int c=0; c<controls.n; c++) 
		if (controls.e[c]->action==action) return controls.e[c]->tip;
	return " ";
}

void NUpInterface::createControls()
{
	controls.push(new ActionArea(NUP_Major_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),0,1,color_arrow,0));
	controls.push(new ActionArea(NUP_Minor_Arrow        , AREA_Handle, NULL, _("Drag to adjust"),0,1,color_arrow,0));
	controls.push(new ActionArea(NUP_Major_Number       , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));
	controls.push(new ActionArea(NUP_Minor_Number       , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));

	controls.push(new ActionArea(NUP_Ok                 , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));
	controls.push(new ActionArea(NUP_Type               , AREA_Handle, NULL, _("Wheel to change"),0,1,color_num,0));

	major      =controls.e[0];
	minor      =controls.e[1];
	majornum   =controls.e[2];
	minornum   =controls.e[3];
	okcontrol  =controls.e[4];
	typecontrol=controls.e[5];
}

void NUpInterface::remapControls(int tempdir)
{
	double x,y;
	double ww4, hh4;
	//double major_w=100;
	//double minor_w=75;
	flatpoint *p;

	double totalh=nupinfo->maxy-nupinfo->miny;
	double totalw=nupinfo->maxx-nupinfo->minx;
	ww4=totalw/4;
	hh4=totalh/4;

	int dir=(tempdir>=0?tempdir:nupinfo->direction);
	double majthick=ww4/4;
	double minthick=ww4/8;

	 //define major arrow
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //major arrow is horizontal, adjust position downward if at bottom
		p=major->Points(NULL,7,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB) y=hh4/2; else y=hh4*3.5;
		if (dir==LAX_LRTB || dir==LAX_LRBT) 
			 arrow_coordinates(p, flatpoint(ww4,y), flatpoint(4*ww4,y), majthick);
		else arrow_coordinates(p, flatpoint(3*ww4,y), flatpoint(0,y), majthick);
		major->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_LRBT) rect_coordinates(p, 0,y-hh4/2,ww4,hh4);
		else rect_coordinates(p, 3*ww4,y-hh4/2,ww4,hh4);
		majornum->FindBBox();

	} else {
		 //major arrow is vertical
		y=0;
		p=major->Points(NULL,7,0);
		if (dir==LAX_TBRL || dir==LAX_BTRL) x=ww4*3.5; else x=ww4/2;
		if (dir==LAX_TBRL || dir==LAX_TBLR)
			 arrow_coordinates(p, flatpoint(x,hh4), flatpoint(x,4*hh4), majthick);
		else arrow_coordinates(p, flatpoint(x,3*hh4), flatpoint(x,0), majthick);
		major->FindBBox();

		p=majornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_BTRL) rect_coordinates(p, x-ww4/2,y+3*hh4, ww4,hh4);
		else rect_coordinates(p, x-ww4/2,0,ww4,hh4);
		majornum->FindBBox();
	}

	 //define minor arrow
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //minor arrow is vertical, adjust position downward if at bottom
		p=minor->Points(NULL,7,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB)
			 arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(2*ww4,4*hh4), minthick);
		else arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(2*ww4,0), minthick);
		minor->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_LRTB || dir==LAX_RLTB) rect_coordinates(p, 1.5*ww4,hh4, ww4,hh4);
		else rect_coordinates(p, 1.5*ww4,2*hh4, ww4,hh4);
		minornum->FindBBox();

	} else {
		 //minor arrow is horizontal
		p=minor->Points(NULL,7,0);
		if (dir==LAX_TBRL || dir==LAX_BTRL) 
			 arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(0,2*hh4), minthick);
		else arrow_coordinates(p, flatpoint(2*ww4,2*hh4), flatpoint(4*ww4,2*hh4), minthick);
		minor->FindBBox();

		p=minornum->Points(NULL,4,0);
		if (dir==LAX_BTLR || dir==LAX_TBLR) rect_coordinates(p, ww4,1.5*hh4,ww4,hh4);
		else rect_coordinates(p, 2*ww4,1.5*hh4, ww4,hh4);
		minornum->FindBBox();
	}

	 //type of flow
	p=typecontrol->Points(NULL,4,0);
	rect_coordinates(p, ww4*4/3,4*hh4, ww4*4/3,hh4*4/3);
	typecontrol->FindBBox();
	if (!(nup_style&NUP_Has_Type)) typecontrol->hidden=1;

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

//! Return and optionally set a flowtype status message.
const char *NUpInterface::flowtypeMessage(int set)
{
	const char *mes=NULL;
	int i=nupinfo->flowtype;
	if (i==NUP_Grid)            mes=_("Grid");
	else if (i==NUP_Sized_Grid) mes=_("Sized grid");
	else if (i==NUP_Flowed)     mes=_("Flowed");
	else if (i==NUP_Random)     mes=_("Randomize");
	else if (i==NUP_Unclump)    mes=_("Unclump");
	else if (i==NUP_Unoverlap)  mes=_("Unoverlap");
	if (set && mes) viewport->postmessage(mes);
	return mes;
}

const char *dirname(int dir)
{
	if (dir==LAX_LRTB) return _("Left to right, top to bottom");
	if (dir==LAX_LRBT) return _("Left to right, bottom to top");
	if (dir==LAX_RLTB) return _("Right to left, top to bottom");
	if (dir==LAX_RLBT) return _("Right to left, bottom to top");
	if (dir==LAX_TBLR) return _("Top to bottom, left to right");
	if (dir==LAX_BTLR) return _("Bottom to top, left to right");
	if (dir==LAX_TBRL) return _("Top to bottom, right to left");
	if (dir==LAX_BTRL) return _("Bottom to top, right to left");
	return NULL;
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *NUpInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("N-up Interface"));

	menu->AddItem(dirname(LAX_LRTB),LAX_LRTB,LAX_ISTOGGLE|(nupinfo->direction==LAX_LRTB?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_LRBT),LAX_LRBT,LAX_ISTOGGLE|(nupinfo->direction==LAX_LRBT?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_RLTB),LAX_RLTB,LAX_ISTOGGLE|(nupinfo->direction==LAX_RLTB?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_RLBT),LAX_RLBT,LAX_ISTOGGLE|(nupinfo->direction==LAX_RLBT?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_TBLR),LAX_TBLR,LAX_ISTOGGLE|(nupinfo->direction==LAX_TBLR?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_BTLR),LAX_BTLR,LAX_ISTOGGLE|(nupinfo->direction==LAX_BTLR?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_TBRL),LAX_TBRL,LAX_ISTOGGLE|(nupinfo->direction==LAX_TBRL?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_BTRL),LAX_BTRL,LAX_ISTOGGLE|(nupinfo->direction==LAX_BTRL?LAX_CHECKED:0)|LAX_OFF,1);

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

int NUpInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
		remapControls();
	}

	DBG cerr <<"NUpInterface::Refresh()..."<<endl;

	dp->DrawScreen();

	int x=nupinfo->uioffset.x;
	int y=nupinfo->uioffset.y;
	int dir=nupinfo->direction;
	int majorn, minorn;
	unsigned int arrowcolor=rgbcolor(128,128,128);

	if (tempdir>=0) {
		arrowcolor=rgbcolor(0,200,0);
		dir=tempdir;
		remapControls(tempdir);
	}

	flatpoint p;
	char buffer[30];

	 //draw ui outline
	dp->NewFG(rgbcolor(128,128,128));
	dp->DrawScreen();
	//--square:
	//dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->miny));
	//dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy));
	//dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->maxy),flatpoint(x+nupinfo->minx,y+nupinfo->maxy));
	//dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->maxy),flatpoint(x+nupinfo->minx,y+nupinfo->miny));
	 //--rect:
	double width =nupinfo->maxx-nupinfo->minx;
	double height=nupinfo->maxy-nupinfo->miny;
	dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->miny));
	dp->drawline(flatpoint(x+nupinfo->maxx,y+nupinfo->miny),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy+height/4));
	dp->drawline(flatpoint(x+nupinfo->minx,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->minx,y+nupinfo->miny));
	dp->drawline(flatpoint(x+nupinfo->maxx-width/3,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->maxx,y+nupinfo->maxy+height/4));
	dp->drawline(flatpoint(x+nupinfo->minx+width/3,y+nupinfo->maxy+height/4),flatpoint(x+nupinfo->minx,y+nupinfo->maxy+height/4));
	 //with activator button:
	if (active) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
	dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);
	flatpoint cc=flatpoint(x+nupinfo->maxx-width/2,y+nupinfo->maxy+height/4);
	dp->drawellipse(cc.x,cc.y,
					width/6,width/6,
					0,2*M_PI,
					0);
	dp->NewFG(rgbcolor(128,128,128));
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	if (nupinfo->flowtype==NUP_Grid)            sprintf(buffer,"#");
	else if (nupinfo->flowtype==NUP_Sized_Grid) sprintf(buffer,"+");
	else if (nupinfo->flowtype==NUP_Flowed)     sprintf(buffer,"=");
	else if (nupinfo->flowtype==NUP_Random)     sprintf(buffer,"?");
	else if (nupinfo->flowtype==NUP_Unclump)    sprintf(buffer,"o O");
	else if (nupinfo->flowtype==NUP_Unoverlap)  sprintf(buffer,"oO");
	else sprintf(buffer," ");
	dp->textout(cc.x,cc.y, buffer,-1,LAX_CENTER);


	 //draw major arrow number
	p=flatpoint(x+(majornum->minx+majornum->maxx)/2,y+(majornum->miny+majornum->maxy)/2);
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		majorn=nupinfo->rows;
		minorn=nupinfo->cols;
	} else {
		majorn=nupinfo->cols;
		minorn=nupinfo->rows;
	}

	if (majorn>=1) sprintf(buffer,"%d",majorn);
	else if (majorn==0) sprintf(buffer,"n");
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw minor arrow number
	p=flatpoint(x+(minornum->minx+minornum->maxx)/2,y+(minornum->miny+minornum->maxy)/2);
	if (minorn>=1) sprintf(buffer,"%d",minorn);
	else if (minorn==0) sprintf(buffer,"n");
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw arrows
	drawHandle(major,arrowcolor,nupinfo->uioffset);
	drawHandle(minor,arrowcolor,nupinfo->uioffset);

	 //draw ok
	//if (nup_style&NUP_Has_Ok) drawHandle(okcontrol,okcontrol->color,nupinfo->uioffset);

	 //draw type
	//if (nup_style&NUP_Has_Type) drawHandle(typecontrol,typecontrol->color,nupinfo->uioffset);
	


	dp->DrawReal();

	if (tempdir>=0) {
		remapControls(nupinfo->direction);
	}

	return 1;
}

void NUpInterface::drawHandle(ActionArea *area, unsigned int color, flatpoint offset)
{
	if (area->real) dp->DrawReal(); else dp->DrawScreen();
	dp->NewFG(color);

	flatpoint p[10];
	memcpy(p,area->outline,7*sizeof(flatpoint));
	for (int c=0; c<area->npoints; c++) p[c]+=nupinfo->uioffset;

	dp->drawlines(p,area->npoints,1,area->action==overoverlay);
}

//! Return a new flow direction, or -1 if not in the panel.
int NUpInterface::vscan(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int dir=nupinfo->direction;
	double w,h;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	if (!(x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy)) return -1;

	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;

	int newdir=-1;
	int oldh=-1;
	if (dir==LAX_LRBT || dir==LAX_LRTB || dir==LAX_TBLR || dir==LAX_BTLR) oldh=NUP_LtoR; else oldh=NUP_RtoL;

	if (xx<1) {
		if (yy<2) newdir=LAX_BTLR;
		if (yy>=2) newdir=LAX_TBLR;
	}
	if (xx>3) {
		if (yy<2) newdir=LAX_BTRL;
		if (yy>=2) newdir=LAX_TBRL;
	}
	if (xx>=1 && xx<=3) {
		if (yy<2)  { if (oldh==NUP_LtoR) newdir=LAX_LRBT; else newdir=LAX_RLBT; }
		if (yy>=2) { if (oldh==NUP_LtoR) newdir=LAX_LRTB; else newdir=LAX_RLTB; }
	}

	return newdir;
}

//! Return a new flow direction, or -1 if not in the panel.
int NUpInterface::hscan(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int dir=nupinfo->direction;
	double w,h;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	if (!(x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy)) return -1;

	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;

	int newdir=-1;
	int oldv=-1;
	if (dir==LAX_LRBT || dir==LAX_RLBT || dir==LAX_BTLR || dir==LAX_BTRL) oldv=NUP_BtoT; else oldv=NUP_TtoB;

	if (yy<1) {
		if (xx<2) newdir=LAX_RLTB;
		if (xx>=2) newdir=LAX_LRTB;
	}
	if (yy>3) {
		if (xx<2) newdir=LAX_RLBT;
		if (xx>=2) newdir=LAX_LRBT;
	}
	if (yy>=1 && yy<=3) {
		if (xx<2)  { if (oldv==NUP_BtoT) newdir=LAX_BTRL; else newdir=LAX_TBRL; }
		if (xx>=2) { if (oldv==NUP_BtoT) newdir=LAX_BTLR; else newdir=LAX_TBLR; }
	}

	return newdir;
}


//! Return which position and range mouse is over
int NUpInterface::scan(int x,int y)
{
	//flatpoint fp=screentoreal(x,y);
	int action=NUP_None, dir=nupinfo->direction;
	double w,h;
	w=nupinfo->maxx-nupinfo->minx;
	h=nupinfo->maxy-nupinfo->miny;


	x-=nupinfo->uioffset.x;
	y-=nupinfo->uioffset.y;

	double xx,yy;
	xx=(x-nupinfo->minx)/w*4;
	yy=(y-nupinfo->miny)/h*4;
	cerr <<"  over:"<<xx<<','<<yy<<endl;

	 //check activate button
	if (norm(flatpoint(nupinfo->minx+w/2,nupinfo->maxy+h/4)-flatpoint(x,y))<w/6) return NUP_Activate;


	 //check for position inside the panel
	if (x>=nupinfo->minx && x<=nupinfo->maxx && y>=nupinfo->miny && y<=nupinfo->maxy+h/4) {
		action=NUP_None;

		 //check against major arrow places
		if ((xx<1 && (dir==LAX_BTLR || dir==LAX_TBLR)) || (xx>=3 && (dir==LAX_BTRL || dir==LAX_TBRL))) {
			//major arrow is vertical..

			if (yy<1) {
				if (dir==LAX_BTLR || dir==LAX_BTRL) action=NUP_Major_Tip;
				else action=NUP_Major_Number;
			} else if (yy>=3) {
				if (dir==LAX_BTLR || dir==LAX_BTRL) action=NUP_Major_Number;
				else action=NUP_Major_Tip;
			} else action=NUP_Major_Arrow;

		} else if ((yy<1 && (dir==LAX_LRTB || dir==LAX_RLTB)) || (yy>=3 && (dir==LAX_RLBT || dir==LAX_LRBT))) {
			//major arrow is horizontal..

			if (xx<1) {
				if (dir==LAX_LRTB || dir==LAX_LRBT) action=NUP_Major_Number;
				else action=NUP_Major_Tip;
			} else if (xx>=3) {
				if (dir==LAX_LRTB || dir==LAX_LRBT) action=NUP_Major_Tip;
				else action=NUP_Major_Number;
			} else action=NUP_Major_Arrow;

		 //check against minor arrow places
		} else if (dir==LAX_BTLR || dir==LAX_TBLR) {
			if (yy>=1.5 && yy<2.5) {
				if (xx>=1 && xx<2) action=NUP_Minor_Number;
				else if (xx>=2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_BTRL || dir==LAX_TBRL) {
			if (yy>=1.5 && yy<2.5) {
				if (xx>=2 && xx<3) action=NUP_Minor_Number;
				else if (xx<2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_LRBT || dir==LAX_RLBT) {
			if (xx>=1.5 && xx<2.5) {
				if (yy>=2 && yy<3) action=NUP_Minor_Number;
				else if (yy<2) action=NUP_Minor_Arrow;
			}
		} else if (dir==LAX_LRTB || dir==LAX_RLTB) {
			if (xx>=1.5 && xx<2.5) {
				if (yy>=1 && yy<2) action=NUP_Minor_Number;
				else if (yy>=2) action=NUP_Minor_Arrow;
			}
		}


		if (action==NUP_None) action=NUP_Panel;
		return action;
	}

	x+=nupinfo->uioffset.x;
	y+=nupinfo->uioffset.y;

	 //search for other controls
	for (int c=0; c<controls.n; c++) {
		if (controls.e[c]->hidden) continue;

		if (controls.e[c]->category==0) {
			 //normal, single instance handles
			if (!controls.e[c]->real && controls.e[c]->PointIn(x,y)) {
				return action;
			}
		}
	}

	return NUP_None;
}

int NUpInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any(0,LEFTBUTTON)) return 0; //only allow one button at a time

	int action=scan(x,y);
	if (action!=NUP_None) {
		buttondown.down(d->id,LEFTBUTTON,x,y,action);
		int dir=nupinfo->direction;
		if (action==NUP_Major_Arrow || action==NUP_Major_Tip) {
			if (dir==LAX_LRTB || dir==LAX_LRBT) temparrowdir=NUP_LtoR;
			else if (dir==LAX_RLTB || dir==LAX_RLBT) temparrowdir=NUP_RtoL;
			else if (dir==LAX_TBLR || dir==LAX_TBRL) temparrowdir=NUP_TtoB;
			else if (dir==LAX_BTRL || dir==LAX_BTLR) temparrowdir=NUP_BtoT;
		} else if (action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
			if (dir==LAX_LRTB || dir==LAX_RLTB) temparrowdir=NUP_TtoB;
			else if (dir==LAX_RLBT || dir==LAX_LRBT) temparrowdir=NUP_BtoT;
			else if (dir==LAX_TBLR || dir==LAX_BTLR) temparrowdir=NUP_LtoR;
			else if (dir==LAX_BTRL || dir==LAX_TBRL) temparrowdir=NUP_RtoL;
		}
		if (action!=NUP_Panel) tempdir=dir;
		return 0;
	}

	return 1;
}

int NUpInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int action;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &action);

	if (tempdir>=0 && tempdir!=nupinfo->direction) {
		nupinfo->direction=tempdir;
		remapControls();
		validateInfo();
	}
	tempdir=-1;
	temparrowdir=0;
	needtodraw=1;
	DBG cerr <<"NUpInterface::LBUp() dragged="<<dragged<<endl;

	if (action==NUP_Activate && !dragged) {
		active=!active;
		if (active) Apply();
		needtodraw=1;
		return 0;
	}

	return 0;
}

int NUpInterface::validateInfo()
{
	int dir=nupinfo->direction;

	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		 //horizontal major axis
		if (nupinfo->rows<0) nupinfo->rows=0;
	} else {
		 //vertical major axis
		if (nupinfo->cols<0) nupinfo->cols=0;
	}
	needtodraw=1;
	return 0;
}

int NUpInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int action=scan(x,y);
	int dir=nupinfo->direction;

	if (action!=NUP_None && (state&ControlMask)) {
		nupinfo->maxx*=1.05;
		nupinfo->maxy*=1.05;
		remapControls();
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Activate) {
		nupinfo->flowtype++;
		if (nupinfo->flowtype>=NUP_MAX) nupinfo->flowtype=NUP_Grid;
		flowtypeMessage(1);
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Major_Number || action==NUP_Major_Arrow || action==NUP_Major_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			if (nupinfo->rows<0) nupinfo->rows=0; else nupinfo->rows++;
		} else {
			if (nupinfo->cols<0) nupinfo->cols=0; else nupinfo->cols++;
		}
		needtodraw=1;
		return 0;

	} else if (action==NUP_Minor_Number || action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			if (nupinfo->cols<0) nupinfo->cols=0; else nupinfo->cols++;
		} else {
			if (nupinfo->rows<0) nupinfo->rows=0; else nupinfo->rows++;
		}
		needtodraw=1;
		return 0;
	}

	if (action!=NUP_None) return 0;
	return 1;
}

int NUpInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int action=scan(x,y);
	int dir=nupinfo->direction;

	if (action!=NUP_None && (state&ControlMask)) {
		nupinfo->maxx*=.95;
		nupinfo->maxy*=.95;
		remapControls();
		needtodraw=1;
		return 0;
	}

	if (action==NUP_Activate) {
		nupinfo->flowtype--;
		if (nupinfo->flowtype<=NUP_Noflow) nupinfo->flowtype=NUP_MAX-1;
		flowtypeMessage(1);
		needtodraw=1;
		return 0;
	}


	 //the major axis cannot have infinity
	if (action==NUP_Major_Number || action==NUP_Major_Arrow || action==NUP_Major_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			 //horizontal major axis
			nupinfo->rows--;
			if (nupinfo->rows<0) nupinfo->rows=0;
		} else {
			 //vertical major axis
			nupinfo->cols--;
			if (nupinfo->cols<0) nupinfo->cols=0;
		}
		needtodraw=1;
		return 0;

	} else if (action==NUP_Minor_Number || action==NUP_Minor_Arrow || action==NUP_Minor_Tip) {
		if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
			nupinfo->cols--;
			if (nupinfo->cols<0) nupinfo->cols=-1;
		} else {
			nupinfo->rows--;
			if (nupinfo->rows<0) nupinfo->rows=-1;
		}
		needtodraw=1;
		return 0;
	}

	if (action!=NUP_None) return 0;
	return 1;
}



DBG int lastx,lasty;

int NUpInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ //***

	int over=scan(x,y);

	DBG lastx=x; lasty=y;
	DBG cerr <<"over: "<<over<<endl;

	if (!buttondown.any()) {
		if (overoverlay!=over) {
			overoverlay=over;
			needtodraw=1;
			const char *mes=controlTooltip(over);
			if (mes) viewport->postmessage(mes);
		}
		return 0;
	}

	//so here the button must be down

	overoverlay=NUP_None;

	int oldx,oldy;
	buttondown.move(mouse->id,x,y,&oldx,&oldy);
	flatpoint d=flatpoint(x-oldx,y-oldy);
	
	int action=NUP_None;
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &action);
	if (action==NUP_None) return 0;

	if (action==NUP_Panel || (state&LAX_STATE_MASK)!=0) {
		nupinfo->uioffset+=d;
		needtodraw=1;
		return 0;

	} else if (action!=NUP_None) {
		if (temparrowdir>=0) {
			int d=-1;
			if (temparrowdir==NUP_LtoR || temparrowdir==NUP_RtoL) {
				d=hscan(x,y);
			} else if (temparrowdir==NUP_TtoB || temparrowdir==NUP_BtoT) {
				d=vscan(x,y);
			}
			if (d!=tempdir) {
				needtodraw=1;
				tempdir=d;
			}
			return 0;
		}
	}

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

	} else if (ch=='o') {
		cerr <<"--------------------------";
		DBG int action=scan(lastx,lasty);
		DBG cerr << " x,y:"<<lastx<<','<<lasty<<"  action:"<<action<<endl;
		return 0;

	} else if (ch==LAX_Left) {
		nupinfo->direction++;
		if (nupinfo->direction>7) nupinfo->direction=0;
		remapControls();
		viewport->postmessage(dirname(nupinfo->direction));
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) {
		nupinfo->direction--;
		if (nupinfo->direction<0) nupinfo->direction=7;
		remapControls();
		viewport->postmessage(dirname(nupinfo->direction));
		needtodraw=1;
		return 0;


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

/*! Return 0 for success or 1 for unable to apply for some reason.
 */
int NUpInterface::Apply()
{
	// ***
	return 1;
}

//} // namespace Laidout

