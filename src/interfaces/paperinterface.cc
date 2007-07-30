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


#include "../language.h"
#include "paperinterface.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


//------------------------------------- PaperInterface --------------------------------------
	
/*! \class PaperInterface 
 * \brief Interface to arrange an arbitrary spread to print on one or more sheets of paper.
 *
 * This can be used to position a single paper over real space, or set to allow any
 * number of papers. Also can be used to set rectangles for crop, bleed, and trim.
 */
/*! \var PaperBoxData *PaperInterface::paperboxdata
 * \brief Temporary pointer to aid viewport refreshing of PaperBoxData objects.
 */

PaperInterface::PaperInterface(int nid,Displayer *ndp)
	: InterfaceWithDp(nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	curbox=maybebox=NULL;
	papergroup=NULL;
	paperboxdata=NULL;
	showdecs=0;
	doc=NULL;

	papergroup=new PaperGroup;//****

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
}

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	curbox=maybebox=NULL;
	papergroup=NULL;
	paperboxdata=NULL;
	showdecs=0;
	doc=NULL;

	papergroup=new PaperGroup;//****

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
}

PaperInterface::~PaperInterface()
{
	DBG cerr <<"PaperInterface destructor.."<<endl;

	if (maybebox) maybebox->dec_count();
	if (papergroup) papergroup->dec_count();
	if (doc) doc->dec_count();
}

/*! incs count of doc.
 *
 * Return 0 for success, nonzero for fail.
 */
int PaperInterface::UseThisDocument(Document *ndoc)
{
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();
	return 0;
}

/*! PaperGroup or PaperBoxData.
 */
int PaperInterface::draws(const char *atype)
{
	return !strcmp(atype,"PaperBoxData") || !strcmp(atype,"PaperGroup");
}


//! Return a new PaperInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PaperInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PaperInterface(id,NULL);
	else if (!dynamic_cast<PaperInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int PaperInterface::InterfaceOn()
{
	DBG cerr <<"paperinterfaceOn()"<<endl;
	showdecs=1;
	needtodraw=1;
	return 0;
}

int PaperInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	if (maybebox) { maybebox->dec_count(); maybebox=NULL; }
	if (curbox) { curbox->dec_count(); curbox=NULL; }
	needtodraw=1;
	DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

void PaperInterface::Clear(SomeData *d)
{
}

	
//----This interface, like Spread Editor, keeps everything to itself, so doesn't need:
//int PaperInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask=0)
//{
//	return 0;
//}
//
//int PaperInterface::DrawData(Laxkit::anObject *ndata,
//			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0)
//{***
//}

/*! This will be called by viewports, and the papers will be displayed opaque with
 * drop shadow.
 */
int PaperInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
{
	PaperBoxData *data=dynamic_cast<PaperBoxData *>(tdata);
	if (!data) return 1;
	int td=showdecs,ntd=needtodraw;
	BoxTypes tdrawwhat=drawwhat;
	drawwhat=AllBoxes;
	showdecs=2;
	needtodraw=1;
	Displayer *olddp=dp;
	dp=tdp;
	DrawPaper(data,~0,1,5);
	dp=olddp;
	drawwhat=tdrawwhat;
	needtodraw=ntd;
	showdecs=td;
	paperboxdata=NULL;
	return 1;
}

/*! If fill!=0, then fill the media box with the paper color.
 * If shadow!=0, then put a black drop shadow under a filled media box,
 * at an offset pixel length shadow.
 */
void PaperInterface::DrawPaper(PaperBoxData *data,int what,char fill,int shadow)
{
	if (!data) return;
	int w=1;
	if (data==curbox || curboxes.findindex(data)>=0) w=2;
	XSetLineAttributes(dp->GetDpy(),dp->GetGC(),w,LineSolid,CapButt,JoinMiter);
	//dp->PushAndNewTransform(data->m());

	PaperBox *box=data->box;
	flatpoint p[4];
	if ((what&MediaBox) && (box->which&MediaBox)) {
		p[0]=flatpoint(box->media.minx,box->media.miny);
		p[1]=flatpoint(box->media.minx,box->media.maxy);
		p[2]=flatpoint(box->media.maxx,box->media.maxy);
		p[3]=flatpoint(box->media.maxx,box->media.miny);

		XSetFillStyle(dp->GetDpy(),dp->GetGC(),FillSolid);
		XSetFillRule(dp->GetDpy(),dp->GetGC(),WindingRule);

		 //draw black shadow
		//dp->NewFG(255,0,0);
		//dp->NewBG(~0);
		if (shadow) {
			dp->NewFG(0,0,0);
			dp->PushAxes();
			dp->ShiftScreen(shadow,shadow);
			dp->drawlines(1,1,1,4,p);
			dp->PopAxes();
		}
		
		 //draw white fill or plain outline
		if (fill||shadow) {
			dp->NewFG(0,0,255);
			dp->NewBG(~0);
			dp->drawlines(1,1,2,4,p);
		} else {
			dp->NewFG(0,0,255);
			dp->drawlines(1,1,0,4,p);
		}
	}
	if ((what&ArtBox) && (box->which&ArtBox)) {
		p[0]=dp->realtoscreen(box->art.minx,box->art.miny);
		p[1]=dp->realtoscreen(box->art.minx,box->art.maxy);
		p[2]=dp->realtoscreen(box->art.maxx,box->art.maxy);
		p[3]=dp->realtoscreen(box->art.maxx,box->art.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&TrimBox) && (box->which&TrimBox)) {
		p[0]=dp->realtoscreen(box->trim.minx,box->trim.miny);
		p[1]=dp->realtoscreen(box->trim.minx,box->trim.maxy);
		p[2]=dp->realtoscreen(box->trim.maxx,box->trim.maxy);
		p[3]=dp->realtoscreen(box->trim.maxx,box->trim.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&PrintableBox) && (box->which&PrintableBox)) {
		p[0]=dp->realtoscreen(box->printable.minx,box->printable.miny);
		p[1]=dp->realtoscreen(box->printable.minx,box->printable.maxy);
		p[2]=dp->realtoscreen(box->printable.maxx,box->printable.maxy);
		p[3]=dp->realtoscreen(box->printable.maxx,box->printable.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&BleedBox) && (box->which&BleedBox)) {
		p[0]=dp->realtoscreen(box->bleed.minx,box->bleed.miny);
		p[1]=dp->realtoscreen(box->bleed.minx,box->bleed.maxy);
		p[2]=dp->realtoscreen(box->bleed.maxx,box->bleed.maxy);
		p[3]=dp->realtoscreen(box->bleed.maxx,box->bleed.miny);
		dp->drawlines(1,0,0,4,p);
	}
	//dp->PopAxes(); //spread axes
}

void PaperInterface::DrawGroup(PaperGroup *group,int shadow)
{
	 //draw shadow under whole group
	if (shadow) {
		for (int c=0; c<group->papers.n; c++) {
			dp->PushAndNewTransform(group->papers.e[c]->m());
			DrawPaper(group->papers.e[c],MediaBox, 1,5);
			dp->PopAxes(); 
		}
	}
	for (int c=0; c<group->papers.n; c++) {
		dp->PushAndNewTransform(group->papers.e[c]->m());
		DrawPaper(group->papers.e[c],drawwhat, 1,0);
		dp->PopAxes(); 
	}
}

/*! \todo draw arrow to indicate paper up direction
 */
int PaperInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	PaperBox *box;
	flatpoint p[4];
	if (maybebox) {
		box=maybebox->box;
		dp->PushAndNewTransform(maybebox->m());
		XSetLineAttributes(dp->GetDpy(),dp->GetGC(),0,LineDoubleDash,CapButt,JoinMiter);
		p[0]=dp->realtoscreen(box->media.minx,box->media.miny);
		p[1]=dp->realtoscreen(box->media.minx,box->media.maxy);
		p[2]=dp->realtoscreen(box->media.maxx,box->media.maxy);
		p[3]=dp->realtoscreen(box->media.maxx,box->media.miny);
		dp->drawlines(1,0,0,4,p);
		dp->PopAxes(); 
	}

	if (!papergroup || !papergroup->papers.n) return 0;

	DrawGroup(papergroup,1);

	return 1;
}

//! Return the papergroup->papers element index underneath x,y, or -1.
int PaperInterface::scan(int x,int y)
{
	if (!papergroup->papers.n) return -1;
	flatpoint fp=dp->screentoreal(x,y);

	//if (curbox && curbox->pointin(fp)) return papergroup->papers.findindex(curbox);

	for (int c=papergroup->papers.n-1; c>=0; c--) {
		if (papergroup->papers.e[c]->pointin(fp)) return c;
	}
	return -1;
}

//! Make a maybebox, if one does not exist.
void PaperInterface::createMaybebox(flatpoint p)
{
	if (maybebox) return;

	 // find a paper size, not so easy!
	PaperBox *box=NULL;
	int del=0;
	if (curbox) box=curbox->box;
	else if (papergroup->papers.n) box=papergroup->papers.e[0]->box;
	else if (doc) {
		box=new PaperBox((PaperStyle *)doc->docstyle->imposition->paper->paperstyle->duplicate());
		box->paperstyle->dec_count();
		del=1;
	} else {
		box=new PaperBox((PaperStyle *)laidout->papersizes.e[0]->duplicate());
		box->paperstyle->dec_count();
		del=1;
	}

	maybebox=new PaperBoxData(box); //incs count of box
	maybebox->origin(p-flatpoint((maybebox->maxx-maybebox->minx)/2,(maybebox->maxy-maybebox->miny)/2));
	if (del) box->dec_count();
}

/*! Add maybe box if shift is down.
 */
int PaperInterface::LBDown(int x,int y,unsigned int state,int count)
{
	buttondown|=LEFTBUTTON;

	mx=x; my=y;
	flatpoint fp=dp->screentoreal(x,y);
	int over=scan(x,y);
	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
		//add a new box
		if (!maybebox) createMaybebox(dp->screentoreal(x,y));
		papergroup->papers.push(maybebox);
		maybebox=NULL;
		needtodraw=1;
		//return 0; -- do not return, continue to let box be added..
	}

	int b=scan(x,y);
	if (b>=0) {
		if (curbox) curbox->dec_count();
		curbox=papergroup->papers.e[b];
		curbox->inc_count();
		if ((state&LAX_STATE_MASK)!=ShiftMask) curboxes.flush();
		curboxes.pushnodup(curbox,0);
		needtodraw=1;
	}

	return 0;
}

int PaperInterface::LBUp(int x,int y,unsigned int state)
{
	if (!(buttondown&LEFTBUTTON)) return 1;
	buttondown&=~LEFTBUTTON;

	//***
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

//int PaperInterface::MBDown(int x,int y,unsigned int state,int count)
//int PaperInterface::MBUp(int x,int y,unsigned int state)
//int PaperInterface::RBDown(int x,int y,unsigned int state,int count)
//int PaperInterface::RBUp(int x,int y,unsigned int state)
//int PaperInterface::But4(int x,int y,unsigned int state);
//int PaperInterface::But5(int x,int y,unsigned int state);

int PaperInterface::MouseMove(int x,int y,unsigned int state)
{
	int over=scan(x,y);

	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown) {
		if (!(state&ShiftMask)) return 1;
		//*** activate maybebox
		if (over>=0) {
			if (maybebox) { 
				maybebox->dec_count();
				maybebox=NULL;
				needtodraw=1;
			}
			mx=x; my=y;
			return 0;
		}
		if (!maybebox) {
			createMaybebox(dp->screentoreal(x,y));
		} else {
			maybebox->origin(maybebox->origin()
						+dp->screentoreal(x,y)-dp->screentoreal(mx,my));
		}
		mx=x; my=y;
		needtodraw=1;
		return 0;
	}

	if (curboxes.n==0) return 1;

	flatpoint fp=dp->screentoreal(x,y),
			  d=fp-dp->screentoreal(mx,my);

	 //plain or + moves curboxes (or the box given by editwhat)
	if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
		// ***snapto

		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->origin(curboxes.e[c]->origin()+d);
		}
		needtodraw=1;
		mx=x; my=y;
		return 0;
	}

	 //^ scales
	//***

	 //+^ rotates
	//***

	return 0;
}

/*!
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int PaperInterface::CharInput(unsigned int ch,unsigned int state)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	if (ch==LAX_Shift) {
		if (maybebox) return 0;
		int x,y;
		if (mouseposition(viewport,&x,&y,NULL,NULL,NULL)!=0) return 0;
		if (scan(x,y)>=0) return 0;
		mx=x; my=y;
		createMaybebox(flatpoint(dp->screentoreal(x,y)));
		needtodraw=1;
		return 0;
	} else if (ch==LAX_Left && (state&LAX_STATE_MASK)==0) {
		//*** select previous paper
		return 0;
	} else if (ch==LAX_Right && (state&LAX_STATE_MASK)==0) {
		//*** select next paper
		return 0;
	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) {
		if (!curbox || !papergroup) return 0;
		int c=papergroup->papers.findindex(curbox);
		if (c<0) return 0;
		papergroup->papers.remove(c);
		curbox->dec_count(); curbox=NULL;
		needtodraw=1;
		return 0;
	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
		if (!papergroup) return 1;
		needtodraw=1;
		int n=curboxes.n;
		curboxes.flush();
		if (curbox) { curbox->dec_count(); curbox=NULL; }
		if (n) return 0;
		for (int c=0; c<papergroup->papers.n; c++) 
			curboxes.push(papergroup->papers.e[c],0);
		return 0;
	}
	return 1;
}

int PaperInterface::CharRelease(unsigned int ch,unsigned int state)
{//***
	if (ch==LAX_Shift) {
		if (!maybebox) return 1;
		maybebox->dec_count();
		maybebox=NULL;
		needtodraw=1;
		return 0;
	}
	return 1;
}


//} // namespace Laidout

