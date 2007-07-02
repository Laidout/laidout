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

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


//------------------------------------- PaperBox --------------------------------------

/*! \class PaperBox 
 * \brief Wrapper around a paper style, for use in a PaperInterface.
 */
/*! \var Laxkit::DoubleBBox PaperBox::media
 * \brief Normally, this will be the same as paperstyle.
 */
/*! \var Laxkit::DoubleBBox PaperBox::printable
 * \brief Basically, the area of media that a printer can physically print on.
 */

/*! Incs count of paper.
 */
PaperBox::PaperBox(PaperStyle *paper)
{
	which=0; //a mask of which boxes are defined
	paperstyle=paper;
	if (paper) {
		paper->inc_count();
		which=MediaBox;
		media.minx=media.miny=0;
		media.maxx=paper->w(); //takes into account paper orientation
		media.maxy=paper->h();
	}
}

/*! Decs count of paper.
 */
PaperBox::~PaperBox()
{
	if (paperstyle) paperstyle->dec_count();
}

//------------------------------------- PaperBoxData --------------------------------------

/*! \class PaperBoxData
 * \brief Somedata Wrapper around a paper style, for use in a PaperInterface.
 */

PaperBoxData::PaperBoxData(PaperBox *paper)
{
	box=paper;
	if (box) {
		box->inc_count();
		setbounds(&box->media);
	}
}

PaperBoxData::~PaperBoxData()
{
	if (box) box->dec_count();
}

//------------------------------------- PaperInterface --------------------------------------
	
/*! \class PaperInterface 
 * \brief Interface to arrange an arbitrary spread to print on one or more sheets of paper.
 *
 * This can be used to position a single paper over real space, or set to allow any
 * number of papers. Also can be used to set rectangles for crop, bleed, and trim.
 */


PaperInterface::PaperInterface(int nid,Displayer *ndp)
	: InterfaceWithDp(nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	curbox=maybebox=NULL;
	papergroup=NULL;
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
//
//int PaperInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
//			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1)
//{***
//}

int PaperInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;
	if (!papergroup || !papergroup->papers.n) return 0;

	PaperBox *box;
	flatpoint p[4];
	int c;
	for (c=0; c<papergroup->papers.n; c++) {
		dp->PushAndNewTransform(papergroup->papers.e[c]->m());

		box=papergroup->papers.e[c]->box;
		if ((drawwhat&MediaBox) && (box->which&MediaBox)) {
			p[0]=dp->realtoscreen(box->media.minx,box->media.miny);
			p[1]=dp->realtoscreen(box->media.minx,box->media.maxy);
			p[2]=dp->realtoscreen(box->media.maxx,box->media.maxy);
			p[3]=dp->realtoscreen(box->media.maxx,box->media.miny);
			dp->drawlines(1,0,0,4,p);
		}
		if ((drawwhat&ArtBox) && (box->which&ArtBox)) {
			p[0]=dp->realtoscreen(box->art.minx,box->art.miny);
			p[1]=dp->realtoscreen(box->art.minx,box->art.maxy);
			p[2]=dp->realtoscreen(box->art.maxx,box->art.maxy);
			p[3]=dp->realtoscreen(box->art.maxx,box->art.miny);
			dp->drawlines(1,0,0,4,p);
		}
		if ((drawwhat&TrimBox) && (box->which&TrimBox)) {
			p[0]=dp->realtoscreen(box->trim.minx,box->trim.miny);
			p[1]=dp->realtoscreen(box->trim.minx,box->trim.maxy);
			p[2]=dp->realtoscreen(box->trim.maxx,box->trim.maxy);
			p[3]=dp->realtoscreen(box->trim.maxx,box->trim.miny);
			dp->drawlines(1,0,0,4,p);
		}
		if ((drawwhat&PrintableBox) && (box->which&PrintableBox)) {
			p[0]=dp->realtoscreen(box->printable.minx,box->printable.miny);
			p[1]=dp->realtoscreen(box->printable.minx,box->printable.maxy);
			p[2]=dp->realtoscreen(box->printable.maxx,box->printable.maxy);
			p[3]=dp->realtoscreen(box->printable.maxx,box->printable.miny);
			dp->drawlines(1,0,0,4,p);
		}
		if ((drawwhat&BleedBox) && (box->which&BleedBox)) {
			p[0]=dp->realtoscreen(box->bleed.minx,box->bleed.miny);
			p[1]=dp->realtoscreen(box->bleed.minx,box->bleed.maxy);
			p[2]=dp->realtoscreen(box->bleed.maxx,box->bleed.maxy);
			p[3]=dp->realtoscreen(box->bleed.maxx,box->bleed.miny);
			dp->drawlines(1,0,0,4,p);
		}
		dp->PopAxes(); //spread axes
	}
	return 1;
}

//! Return the papergroup->papers element index underneath x,y, or -1.
int PaperInterface::scan(int x,int y)
{
	if (!papergroup->papers.n) return -1;
	flatpoint fp=dp->screentoreal(x,y);
	if (curbox && curbox->pointin(fp)) return papergroup->papers.findindex(curbox);

	for (int c=0; c<papergroup->papers.n; c++) {
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
		box=new PaperBox((PaperStyle *)doc->docstyle->imposition->paperstyle->duplicate());
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
	if ((state&LAX_STATE_MASK)==ShiftMask) {
		//add a new box
		if (!maybebox) createMaybebox(dp->screentoreal(x,y));
		papergroup->papers.push(maybebox);
		maybebox=NULL;
		needtodraw=1;
		return 0;
	}

	int b=scan(x,y);
	if (b>=0) {
		if (curbox) curbox->dec_count();
		curbox=papergroup->papers.e[b];
		curbox->inc_count();
		curboxes.pushnodup(curbox,0);
	}

	return 0;
}

int PaperInterface::LBUp(int x,int y,unsigned int state)
{
	if (!(buttondown&LEFTBUTTON)) return 1;
	buttondown&=~LEFTBUTTON;

	//***
	if (curbox) { curbox->dec_count(); curbox=NULL; }
	if (curboxes.n) curboxes.flush();

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
	DBG cerr <<"over box: "<<scan(x,y)<<endl;
	if (!buttondown) {
		if (!(state&ShiftMask)) return 1;
		//*** activate maybebox
		if (!maybebox) createMaybebox(dp->screentoreal(x,y));
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
	if (ch==LAX_Left && (state&LAX_STATE_MASK)==0) {
		//*** select previous paper
		return 0;
	} else if (ch==LAX_Right && (state&LAX_STATE_MASK)==0) {
		//*** select next paper
		return 0;
	}
	return 1;
}

int PaperInterface::CharRelease(unsigned int ch,unsigned int state)
{//***
	return 1;
}


//} // namespace Laidout

