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
#include "papertileinterface.h"
#include <lax/strmanip.h>

using namespace Laxkit;


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
	paperstyle=paper;
	if (paper) paper->inc_count();
}

/*! Decs count of paper.
 */
PaperBox::~PaperBox()
{
	if (paper) paper->dec_count();
}

//------------------------------------- PaperBoxData --------------------------------------

/*! \class PaperBoxData
 * \brief Somedata Wrapper around a paper style, for use in a PaperInterface.
 */

PaperBoxData::PaperBoxData(PaperStyle *paper)
	: PaperBox(paper)
{
	if (paperstyle) {
		minx=mix=0;
		maxx=paperstyle->w();
		maxy=paperstyle->h();
	}
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
}

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
}

PaperInterface::~PaperInterface()
{
	DBG cout <<"PaperInterface destructor.."<<endl;
}


//! Return a new PaperInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! This function does not allow creation of a blank PaperInterface object. If dup==NULL, then
 *  NULL is returned.
 */
anInterface *PaperInterface::duplicate(anInterface *dup)//dup=NULL
{***
	if (dup==NULL) return NULL;
	return anInterface::duplicate(dup);
}


int InterfaceOn()
{***
}

int InterfaceOff()
{***
}

void Clear(SomeData *d)
{***
}

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1
{***
}

int UseThis(Laxkit::anObject *ndata,unsigned int mask=0)
{***
}

int DrawData(Laxkit::anObject *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0)
{***
}

int DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1)
{***
}

int Refresh()
{***
	if (!papers.n) return 0;

	PaperBoxData *box;
	flatpoint p[4];
	int c,c2;
	for (c=0; c<papers.n; c++) {
		box=papers.e[c];
		if (drawwhat&MediaBox) {
			p[0]=dp->realtoscreen(box->media->minx,box->media->miny);
			p[1]=dp->realtoscreen(box->media->minx,box->media->maxy);
			p[2]=dp->realtoscreen(box->media->maxx,box->media->maxy);
			p[3]=dp->realtoscreen(box->media->maxx,box->media->miny);
			dp->drawlines(p,4,1);
		}
		if (drawwhat&ArtBox) {
			p[0]=dp->realtoscreen(box->art->minx,box->art->miny);
			p[1]=dp->realtoscreen(box->art->minx,box->art->maxy);
			p[2]=dp->realtoscreen(box->art->maxx,box->art->maxy);
			p[3]=dp->realtoscreen(box->art->maxx,box->art->miny);
			dp->drawlines(p,4,1);
		}
		if (drawwhat&TrimBox) {
			p[0]=dp->realtoscreen(box->trim->minx,box->trim->miny);
			p[1]=dp->realtoscreen(box->trim->minx,box->trim->maxy);
			p[2]=dp->realtoscreen(box->trim->maxx,box->trim->maxy);
			p[3]=dp->realtoscreen(box->trim->maxx,box->trim->miny);
			dp->drawlines(p,4,1);
		}
		if (drawwhat&PrintableBox) {
			p[0]=dp->realtoscreen(box->printable->minx,box->printable->miny);
			p[1]=dp->realtoscreen(box->printable->minx,box->printable->maxy);
			p[2]=dp->realtoscreen(box->printable->maxx,box->printable->maxy);
			p[3]=dp->realtoscreen(box->printable->maxx,box->printable->miny);
			dp->drawlines(p,4,1);
		}
		if (drawwhat&BleedBox) {
			p[0]=dp->realtoscreen(box->bleed->minx,box->bleed->miny);
			p[1]=dp->realtoscreen(box->bleed->minx,box->bleed->maxy);
			p[2]=dp->realtoscreen(box->bleed->maxx,box->bleed->maxy);
			p[3]=dp->realtoscreen(box->bleed->maxx,box->bleed->miny);
			dp->drawlines(p,4,1);
		}
		if (drawwhat&TrimBox) {
			p[0]=dp->realtoscreen(box->trim->minx,box->trim->miny);
			p[1]=dp->realtoscreen(box->trim->minx,box->trim->maxy);
			p[2]=dp->realtoscreen(box->trim->maxx,box->trim->maxy);
			p[3]=dp->realtoscreen(box->trim->maxx,box->trim->miny);
			dp->drawlines(p,4,1);
		}
	}
}
	
int LBDown(int x,int y,unsigned int state,int count)
{***
}

int MBDown(int x,int y,unsigned int state,int count)
{***
}

int RBDown(int x,int y,unsigned int state,int count)
{***
}

int LBUp(int x,int y,unsigned int state)
{***
}

int MBUp(int x,int y,unsigned int state)
{***
}

int RBUp(int x,int y,unsigned int state)
{***
}

int But4(int x,int y,unsigned int state)
{***
}

int But5(int x,int y,unsigned int state)
{***
}

int MouseMove(int x,int y,unsigned int state)
{***
	if (!buttondown) return 1;


	return 0;
}

int CharInput(unsigned int ch,unsigned int state)
{***
	if (ch==LAX_Left && (state&LAX_STATE_MASK)==0) {
		*** select previous paper
		return 0;
	} else if (ch==LAX_Right && (state&LAX_STATE_MASK)==0) {
		*** select next paper
		return 0;
	}
	return 1;
}

int CharRelease(unsigned int ch,unsigned int state)
{***
}


//} // namespace Laidout

