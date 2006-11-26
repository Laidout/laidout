//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********** spreadeditor.cc *************/

#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include "spreadeditor.h"
#include "laidout.h"
#include "drawdata.h"
#include "document.h"
#include <lax/lists.cc>
#include <X11/cursorfont.h>
#include "headwindow.h"
#include "helpwindow.h"
#include "viewwindow.h"

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



////----------------------- SpreadView --------------------------------------
// *** class to hold a view that can be saved and loaded independent of a
// SpreadEditor or SpreadInterface
////class SpreadView
////{
//// public:
////	PtrStack<LittleSpread> littlespreads;
////};

//----------------------- LittleSpread --------------------------------------

/*! \class LittleSpread
 * \brief Data class for use in SpreadEditor
 */
/*! \var int LittleSpread::what
 * 0 == main thread\n
 * 1 == thread of pages\n
 * 2 == thread of spreads
 */


//! Return if the point is in the littlespread.
/*! pin==1 means return if point is within the spread's path.
 * pin==2 means return the index listed in the spread->pagestack element
 * whose outline contains pp, plus 1. That plus 1 enables 0 to
 * still be used as meaning "not in".
 */
int LittleSpread::pointin(flatpoint pp,int pin)
{
	if (spread) {
		int c;
		double i[6];
		transform_invert(i,m());
		pp=transform_point(i,pp); //makes point in littlespread coords
		c=spread->path->pointin(pp,1);
		if (pin==1 || c==0) return c;
		for (c=0; c<spread->pagestack.n; c++) {
			if (spread->pagestack.e[c]->outline->pointin(pp,1)) 
				return spread->pagestack.e[c]->index+1;
		}
	}
	return 0;
}

//! Transfers pointer, does not dupicate s or check it out.
LittleSpread::LittleSpread(Spread *s, LittleSpread *prv)
{
	spread=s;
	connection=NULL;
	prev=prv;
	next=NULL;
	if (prev) {
		prev->next=this;
		mapConnection();
	}
}

//! Destructor, deletes connection and spread;
LittleSpread::~LittleSpread()
{
	if (connection) delete connection;
	if (spread) delete spread;
}

void LittleSpread::FindBBox()
{
	maxx=minx-1;
	if (!spread) return;
	addtobounds(spread->path->m(),spread->path);
}

//! Either create or adjust the connection line to the previous spread.
void LittleSpread::mapConnection()
{
	if (!prev) {
		if (connection) delete connection; 
		connection=NULL;
		return;
	}
	
	if (connection) { delete connection; }
	connection=new PathsData;
	flatpoint min,max;
	double i[6];
	transform_mult(i,m(),spread->path->m());
	min=transform_point(i,      spread->minimum);
	transform_mult(i,prev->m(),prev->spread->path->m());
	max=transform_point(i,prev->spread->maximum);
	connection->append(min);
	connection->append(min+(max-min)*.33333333, POINT_TOPREV, 1);
	connection->append(min+(max-min)*.66666666, POINT_TONEXT, 1);
	connection->append(max);
	
	//*** could just preserve any modifications made, except for first and last point??
	//    but rot+scale so min-prev->max is still x axis??
}


//----------------------- SpreadInterface --------------------------------------

/*! \class SpreadInterface
 * \brief Interface for the spread editor.
 *
 * Fancy page order rearranger. Show all the available page layout spreads, and allows
 * rearranging the pages by dragging either pages or whole spreads. Also uses a kind of
 * page limbo, where temporarily unplaced pages hover.
 *
 * Left button selecting and moving usually acts on individual pages, and the middle
 * button acts on whole spreads.
 *
 * \todo *** There is much work to be done here!! I have many more features planned,
 * Do it NOW, since this is perhaps the most unique feature of Laidout!
 * 
 * \todo *** could allow page ranges in limbo, too.. just have the connecting lines be
 * a different color.
 * 
 * Ultimately, this should be ported to Inkscape and Scribus, ESPECially Inkscape, when the
 * SVG spec allows multiple pages which makes it fair game for Inkscape, that is..
 */
/*! \var int SpreadInterface::curpage
 * \brief Index that can be used to pagelabel stack..
 */
/*! \var int SpreadInterface::arrangetype
 * \brief How arranging should be done.
 *
 * -1 means auto row or column depending on whether the window is wider or taller.
 */
/*! \var int *SpreadInterface::temppagemap
 * \brief How the document's pages have been rearranged.
 *
 * temppagemap elements say what doc->page index is temporarily in littlespread index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 */



 // values for arrangestate
 // arrangetype has to be within the min to max.
#define ArrangeNeedsArranging -1
#define ArrangeTempRow         1
#define ArrangeTempColumn      2

#define  ArrangetypeMin        3
#define ArrangeAutoAlways      3
#define ArrangeAutoTillMod     4
#define Arrange1Row            5
#define Arrange1Column         6
#define ArrangeGrid            7
#define ArrangeCustom          8
#define  ArrangetypeMax        8

static const char *arrangetypestring(int a)
{
	if (a==ArrangeAutoAlways) return "Auto arrange on each window size change";
	else if (a==ArrangeAutoTillMod) return "Auto arrange until you move a spread";
	else if (a==Arrange1Row) return "Arrange in one row";
	else if (a==Arrange1Column) return "Arrange in one column";
	else if (a==ArrangeGrid) return "Arrange in a grid based on screen proportions";
	else if (a==ArrangeCustom) return "You will do all arranging";
	return "? error: bad arrange value";
}

//! Initialize everything and call GetSpreads().
SpreadInterface::SpreadInterface(Laxkit::Displayer *ndp,Project *proj,Document *docum)
	: InterfaceWithDp(0,ndp)
{
	firsttime=1;
	project=proj;
	doc=docum;
	reversebuttons=0;
	temppagemap=NULL;
	temppagen=0;
	curspread=NULL;
	curpage=-1;
	dragpage=-1;
	drawthumbnails=1;
	centerlabels=0;
	arrangetype=ArrangeAutoTillMod;
	arrangestate=ArrangeNeedsArranging;

	GetSpreads();
	
	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask|Button2Mask;
}

//! Empty destructor.
SpreadInterface::~SpreadInterface()
{
	 //for debugging:
	DBG cout<<"SpreadInterface destructor\n spreads flush"<<endl;
	spreads.flush();
}


/*! Something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  spread
 *    index 1
 *    matrix 1 0 0 1 0 0
 *  spread
 *    index 3
 *    matrix 1 0 0 1 0 0
 * </pre>
 *
 * \todo **** when modified but not applied, will save index values
 *   from modified.. needs to be a check somewhere to ask whether to
 *   apply changes before saving..
 *   if (checkPendingChanges(thing)) thing->applyChanges();
 */
void SpreadInterface::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	fprintf(f,"%sdocument %s\n",spc,doc->saveas);

	fprintf(f,"%scenterlabels %d\n",spc,centerlabels);
	fprintf(f,"%sdrawthumbnails %d\n",spc,drawthumbnails);
	fprintf(f,"%sarrangetype %d\n",spc,arrangetype);
	
	const double *m=dp->Getctm();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
	for (int c=0; c<spreads.n; c++) {
		fprintf(f,"%sspread\n",spc);
		fprintf(f,"%s  index %d\n",spc,spreads.e[c]->spread->pagestack.e[0]->index);
		m=spreads.e[c]->m();
		fprintf(f,"%s  matrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
	}
}

/*! Note that 'index' is currently ignored.
 */
void SpreadInterface::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	char *name,*value;
	double m[6];
	int n;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			n=DoubleListAttribute(value,m,6);
			if (n==6) viewport->dp->NewTransform(m);
		} else if (!strcmp(name,"document")) {
			doc=laidout->findDocument(value);
		} else if (!strcmp(name,"centerlabels")) {
			IntAttribute(value,&centerlabels);
		} else if (!strcmp(name,"drawthumbnails")) {
			drawthumbnails=BooleanAttribute(value);
		} else if (!strcmp(name,"arrangetype")) {
			IntAttribute(value,&arrangetype);
		}
	}
	
	GetSpreads();
	int s=0,c,c2;
	
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"spread")) {
			for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"matrix")) {
					n=DoubleListAttribute(value,m,6);
					if (n==6) transform_copy(spreads.e[s]->m(),m);
				}
			}
			s++;
			if (s==spreads.n) break;
		}
	}
}

/*! \todo *** this is a bit of a hack to force refiguring of page thumbnails
 *    need more thorough mechanism to keep track of last mod times..
 */
int SpreadInterface::InterfaceOn()
{
	for (int c=0; c<doc->pages.n; c++) doc->pages.e[c]->modtime=times(NULL);
	return 0;
}

//! Return the reverse map from temppagemap[].
/*!
 * temppagemap elements say what doc->page index is temporarily in
 * littlespread->spread->pagestack->index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 * 
 * So reversemap(4) would return 1, reversemap(1) returns 4, etc.
 *
 * Return -1 on not found.
 */
int SpreadInterface::reversemap(int i)
{
	for (int c=0; c<temppagen; c++) 
		if (temppagemap[c]==i) return c;
	return -1;
}

//! Check to make sure spreads containing pages in range [startpage,endpage] are correct.
/*! This currently recomputes all spreads.
 * 
 * If endpage==-1, then doc->pages.n-1 is assumed.
 *
 * \todo ***this is very messy...
 */
void SpreadInterface::CheckSpreads(int startpage,int endpage)
{
	if (!doc) return;
	if (endpage<0) endpage=doc->pages.n-1;

	LittleSpread *newspread;
	double x=0,y=0;
	NumStack<int> pgs;
	Spread *s;
	int c=0,c2;
	
	 // correct temppagemap
	if (doc->pages.n<=temppagen) {
		temppagen=doc->pages.n;
	} else {
		int *tmp=new int[doc->pages.n];
		memcpy(tmp,temppagemap,sizeof(int)*temppagen);
		for (c=temppagen; c<doc->pages.n; c++) {
			tmp[c]=c;
			if (doc->pages.e[c]->labeltype==-1) doc->pages.e[c]->labeltype=c%8;
		}
		delete[] temppagemap;
		temppagemap=tmp;
		temppagen=doc->pages.n;
	}
			
	 // for simplicity, map the spreads to normal
	for (c=0; c<spreads.n; c++) {
		s=spreads.e[c]->spread;
		for (c2=0; c2<s->pagestack.n; c2++) {
			s->pagestack.e[c2]->index=reversemap(s->pagestack.e[c2]->index);
		}
	}
	 // redo the spreads
	for (c=0; c<doc->docstyle->imposition->NumSpreads(); c++) {
		s=doc->docstyle->imposition->GetLittleSpread(c);
		
		 // try to preserve previous spread placement
		if (c<spreads.n) {
			x=spreads.e[c]->m(4);
			y=spreads.e[c]->m(5);		
		} else if (c>=0) {
			x+=(spreads.e[c-1]->maxx-spreads.e[c-1]->minx)*1.2;
		}

		 // set up and install replacement spread
		newspread=new LittleSpread(s,c>0?spreads.e[c-1]:NULL); //takes pointer, not dups or checkout
		newspread->m(4,x);
		newspread->m(5,y);
		newspread->FindBBox();
		if (c<spreads.n) {
			delete spreads.e[c];
			spreads.e[c]=newspread;
		} else {
			spreads.push(newspread);
		}
		if (c>0) spreads.e[c]->mapConnection();

		 // find next page
	}
	while (c!=spreads.n) spreads.remove(c);

	 // map the pages back
	for (c=0; c<spreads.n; c++) {
		s=spreads.e[c]->spread;
		for (c2=0; c2<s->pagestack.n; c2++) {
			s->pagestack.e[c2]->index=temppagemap[c];
			s->pagestack.e[c2]->page=doc->pages.e[temppagemap[c]];
		}
	}
	needtodraw=1;
}

//! Create all the LittleSpread objects and default page labels.
/*! This is typically geared only for page spreads, not paper spreads.
 *
 * PaperLayouts are non-continuous, but PageLayouts should be continuous.
 * The spread editor currently only does PageLayouts.
 */
void SpreadInterface::GetSpreads()
{
	spreads.flush();
	curspread=NULL;
	curpage=-1;

	if (!doc || !doc->docstyle || !doc->docstyle->imposition || !doc->pages.n) return;
	
	 // define the default page labels
	int c;
	if (temppagemap) delete[] temppagemap;
	temppagemap=new int[doc->pages.n];
	temppagen=doc->pages.n;
	for (c=0; c<doc->pages.n; c++) {
		temppagemap[c]=c;
		if (doc->pages.e[c]->labeltype==-1) doc->pages.e[c]->labeltype=c%8;
	}
	
	Spread *s;
	LittleSpread *ls;
	for (int c=0; c<doc->docstyle->imposition->NumSpreads(); c++) {
		s=doc->docstyle->imposition->GetLittleSpread(c);
		ls=new LittleSpread(s,(spreads.n?spreads.e[spreads.n-1]:NULL)); //takes pointer, not dups or checkout
		ls->FindBBox();
		spreads.push(ls);
	}
}

// // values for arrangestate
//#define ArrangeNeedsArranging -1
//#define ArrangeTempRow         1
//#define ArrangeTempColumn      2
//#define ArrangeAutoAlways      3
//#define ArrangeAutoTillMod     4
//#define Arrange1Row            5
//#define Arrange1Column         6
//#define ArrangeGrid            7
//#define ArrangeCustom          8

//! Arrange the spreads in some sort of order.
/*! Default is have 1 page take up about an inch of screen space.
 * The units stay in spread units, but the viewport is scaled so that a respectable
 * number of spreads fit on screen.
 *
 * how==-1 means use default arrangetype. 
 * Otherwise, 0==auto, 1==1 row, 2==1 column, 3=grid by proportion of curwindow
 *
 * This is called on shift-'a' keypress, a firsttime refresh, and curwindow resize.
 * 
 * \todo the auto grid arranging could be brighter
 */
void SpreadInterface::ArrangeSpreads(int how)//how==-1
{ 
	if (firsttime) return; // need real window info before arranging...
	
	if (arrangetype==arrangestate) return;
	if (arrangetype==ArrangeCustom) {
		if (arrangestate!=ArrangeNeedsArranging) { arrangestate=ArrangeCustom; return; }
	}

	 // figure out how we should be laying out spreads
	 // return if already arranged according to arrangetype
	int towhat=arrangetype;
	double winw,winh;
	winw=dp->Maxx-dp->Minx;
	winh=dp->Maxy-dp->Miny;
	if (towhat==ArrangeAutoAlways || towhat==ArrangeAutoTillMod) {
		 // could be row or column
		if (winw>winh) {
			if (arrangestate==ArrangeTempRow || arrangestate==Arrange1Row) {
				arrangestate=ArrangeTempRow;
				return;
			}
			towhat=Arrange1Row;
			arrangestate=ArrangeTempRow;
		} else {
			if (arrangestate==ArrangeTempColumn || arrangestate==Arrange1Column) {
				arrangestate=ArrangeTempColumn;
				return;
			}
			towhat=Arrange1Column;
			arrangestate=ArrangeTempColumn;
		}
	} else if (towhat==ArrangeCustom) {
		towhat=ArrangeGrid;
		arrangestate=ArrangeCustom;
	} else arrangestate=arrangetype;
	
	 // find the bounding box for all spreads
	DoubleBBox bbox;
	double gap;
	int perrow;
	for (int c=0; c<spreads.n; c++) {
		bbox.addtobounds(spreads.e[c]);
	}
	gap=(bbox.maxx-bbox.minx)*.2;
	
	 // figure out how many spreads per row
	if (towhat==ArrangeGrid) {
		double r=sqrt(spreads.n*winh/winw*(bbox.maxx-bbox.minx+gap)/(bbox.maxy-bbox.miny+gap));
		perrow=(int)(spreads.n/r);
		if (perrow==0) perrow=1;
		//perrow=int(sqrt((double)spreads.n)+1); //put within a square
	} else if (towhat==Arrange1Column) perrow=1;
	else   if (towhat==Arrange1Row) perrow=1000000;
	
	
	double x,y,w,h,X,Y,W,H;
	x=y=w=h=X=Y=W=H=0;

	 // Find how many pixels make 1 inch, basescaling=inches/pixel
	double scaling,basescaling=double(XDisplayWidthMM(app->dpy,0))/25.4/XDisplayWidth(app->dpy,0);
	
	 // Position the LittleSpreads...
	DoubleBBox bb;
	double rh=0,rw=0;

	for (int c=0; c<spreads.n; c++) {
		w=spreads.e[c]->maxx-spreads.e[c]->minx;
		h=spreads.e[c]->maxy-spreads.e[c]->miny;
		if (c==0) { scaling=1./(basescaling*w); }
		spreads.e[c]->origin(flatpoint(x-spreads.e[c]->minx,y-spreads.e[c]->miny));
		x+=w+gap;
		rw+=w+gap;
		if (h>rh) rh=h;
		if (c==spreads.n-1 || (c+1)%perrow==0) { // start a new row.
			H+=rh;
			x=0;
			y-=rh+gap;
			if (rw>W) W=rw; 
			rw=rh=0;
		}
	}
	if (W==0) W=rw;
	if (H==0) H=rh;
	
	 //sync up the connecting lines
	for (int c=1; c<spreads.n; c++) {
		spreads.e[c]->mapConnection();
	}
	
	X-=W*.1;
	W+=.2*W;
	Y-=H*.1;
	H+=.2*H;
//	if (dp) {
//		dp->SetSpace(X-W,X+W+W,Y-H,Y+H+H);
//		dp->Center(X,X+W,Y,Y+H);
//	}
}

//! Draw the little spreads, connection lines, page labels, whatever else...
/*! \todo *** should have option to bring connecting lines forward, or put them
 * behind the spreads.
 */
int SpreadInterface::Refresh()
{
	if (!needtodraw) return 1;
	
	if (firsttime) {
		firsttime=0;
		ArrangeSpreads();
		Center(1);
		//((ViewportWindow *)curwindow)->syncrulers();
	}
	dp->Updates(0);	
	
	dp->NewBG(200,200,200);
	dp->ClearWindow();
	int c,c2;
	
	 // Draw the connecting lines
	 // draw the connection lines in front of the spreads..
	 //*** this needs little bubble at back of arrows, and little arrow at front
	for (c=0; c<spreads.n; c++) {
		if (spreads.e[c]->prev) {
			::DrawData(dp,spreads.e[c]->connection,NULL,NULL);
		}
	}
	 
	 // Draw the spreads
	SomeData *outline;
	int pg,x,y;
	flatpoint p;
	
	FillStyle fs(0xffff,0xffff,0xffff,0xffff, WindingRule,FillSolid,GXcopy);
	//Page *page;
	ImageData *thumb=NULL;
	dp->clearclip();
	for (c=0; c<spreads.n; c++) {
		dp->PushAndNewTransform(spreads.e[c]->m());
		
		 //draw the spread's path
		::DrawData(dp,spreads.e[c]->spread->path,NULL,&fs);

		Region region;
		for (c2=0; c2<spreads.e[c]->spread->pagestack.n; c2++) {
			 // draw thumbnails
			pg=spreads.e[c]->spread->pagestack.e[c2]->index;
			if (drawthumbnails) {
				if (pg>=0 && pg<doc->pages.n) {
					pg=temppagemap[pg];
					if (pg>=0 && pg<doc->pages.n) thumb=doc->pages.e[pg]->Thumbnail();
				}
				if (thumb) {
					DBG cout <<"drawing thumbnail for "<<pg<<endl;
					dp->PushAndNewTransform(spreads.e[c]->spread->pagestack.e[c2]->outline->m());
					
					 // always setup clipping region to be the page
					region=GetRegionFromPaths(spreads.e[c]->spread->pagestack.e[c2]->outline,dp->m());
					//DBG ::DrawData(dp,spreads.e[c]->spread->pagestack.e[c2]->outline,NULL,NULL);
					if (!XEmptyRegion(region)) dp->clip(region,3); 
					
					::DrawData(dp,thumb,NULL,NULL);
					
					 //remove clipping region
					dp->clearclip();

					dp->PopAxes();
					thumb=NULL;
				}
			}
		}	
		
		 // draw path again over whatever was drawn, but don't fill...
		::DrawData(dp,spreads.e[c]->spread->path,NULL,NULL);

		 // draw page labels
		for (c2=0; c2<spreads.e[c]->spread->pagestack.n; c2++) {
			pg=spreads.e[c]->spread->pagestack.e[c2]->index;
			if (pg>=0 && pg<doc->pages.n) {
				outline=spreads.e[c]->spread->pagestack.e[c2]->outline;
				dp->PushAndNewTransform(outline->m());
				if (centerlabels==0) 
					p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->miny+outline->maxy)/2));
				else if (centerlabels==1) p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->miny)));
				else if (centerlabels==2) p=dp->realtoscreen(flatpoint((outline->minx),(outline->miny+outline->maxy)/2));
				else if (centerlabels==3) p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->maxy)));
				else  p=dp->realtoscreen(flatpoint((outline->maxx),(outline->miny+outline->maxy)/2));
				x=(int)p.x; // figure out where bottom tip of bbox is
				y=(int)p.y;
				drawLabel(x,y,doc->pages.e[temppagemap[pg]]);
				dp->PopAxes();
			}
		}

		dp->PopAxes(); //spread axes
	}
	
	dp->Updates(1);	
	needtodraw=0;
	return 1;
}

//! Draw the label.
/*! 
 * 0 plain\n
 * 1 white circle\n
 * 2 gray circle\n
 * 3 dark gray circle\n
 * 4 black circle\n
 * 5 square\n
 * 6 gray square\n
 * 7 dark gray square\n
 * 8 black square\n
 */
void SpreadInterface::drawLabel(int x,int y,Page *page)
{
	 // *** if (plabel->labeltype==circle, filledcircle, etc...) ...
	 // *** write text
	int w,h;
	getextent(page->label,&w,&h);
	w/=2;
	h/=2;
	w+=h;
	h+=h;
	unsigned long fcolor=~0,color=0;
	int t=-1;
	switch (page->labeltype) {
		case 0: fcolor=app->rgbcolor(255,255,255);
				t=0;
				break;
		case 1: fcolor=app->rgbcolor(175,175,175);
				t=0;
				break;
		case 2: fcolor=app->rgbcolor(100,100,100);
				color=~0;
				t=0;
				break;
		case 3: fcolor=app->rgbcolor(0,0,0);
				color=~0;
				t=0;
				break;
		case 4: fcolor=app->rgbcolor(255,255,255);
				t=1;
				break;
		case 5: fcolor=app->rgbcolor(175,175,175);
				t=1;
				break;
		case 6: fcolor=app->rgbcolor(100,100,100);
				color=~0;
				t=1;
				break;
		case 7: fcolor=app->rgbcolor(0,0,0);
				color=~0;
				t=1;
				break;
	}
	drawthing(dp->GetWindow(),app->gc(),x,y,w,h,t,0,fcolor);
	XSetForeground(app->dpy,app->gc(),color);
	textout(dp->GetWindow(),page->label,-1,x,y,LAX_CENTER);
}

//! Relays left button event to rLBDown or rMBDown depending on reversebuttons.
int SpreadInterface::LBDown(int x,int y,unsigned int state,int count)
{ if (reversebuttons) return rMBDown(x,y,state,count); else return rLBDown(x,y,state,count); }

//! Relays left button event to rLBUp or rMBUp depending on reversebuttons.
int SpreadInterface::LBUp(int x,int y,unsigned int state)
{ if (reversebuttons) return rMBUp(x,y,state); else return rLBUp(x,y,state); }

//! Relays middle button event to rLBDown or rMBDown depending on reversebuttons.
int SpreadInterface::MBDown(int x,int y,unsigned int state,int count)
{ if (reversebuttons) return rLBDown(x,y,state,count); else return rMBDown(x,y,state,count); }

//! Relays middle button event to rLBUp or rMBDown depending on reversebuttons.
int SpreadInterface::MBUp(int x,int y,unsigned int state)
{ if (reversebuttons) return rLBUp(x,y,state); else return rMBUp(x,y,state); }

//! Selects pages.
/*! If down on nothing, then start a selection rectangle. 
 *
 * Otherwise figure
 * out which page button is down on. If shift, add to selection, if control, toggle selected
 * state of it.
 */
int SpreadInterface::rLBDown(int x,int y,unsigned int state,int count)
{
	if (buttondown && buttondown!=LEFTBUTTON) return 1;
	buttondown|=LEFTBUTTON;

	int pg,spr;
	spr=findSpread(x,y,&pg);
	DBG cout <<"SpreadInterface lbdown found page "<<pg<<" in spread "<<spr<<endl;
	if (spr>=0) curspread=spreads.e[spr];
	if (pg<0) {
		//*** start a selection rectangle
		dragpage=-1;
		return 0;
	}
	
	dragpage=pg;
	Cursor cursor;
	//cursor=XCreateFontCursor(app->dpy, XC_top_left_corner);
	
	if ((state&LAX_STATE_MASK)==0) cursor=XCreateFontCursor(app->dpy, XC_right_side);
	else cursor=XCreateFontCursor(app->dpy, XC_exchange);
	XDefineCursor(app->dpy, curwindow->window, cursor);
	XFreeCursor(app->dpy, cursor);

	return 0;
}

//! Drops pages.
/*! *** needs clearer, configurable implementation, but right now,
 * plain-up slides pages over, shift-up swaps pages
 *
 * Mouse up outside window into a ViewWindow shifts view to that page.
 * 
 * \todo Idea for use: have a SpreadEditor be behind a ViewWindow pane.
 * bring the spreadeditor forward, double click on a page which causes
 * the view to come forward with that page selected..
 */
int SpreadInterface::rLBUp(int x,int y,unsigned int state)
{
	if (!(buttondown&LEFTBUTTON)) return 1;
	XUndefineCursor(app->dpy, curwindow->window);
	buttondown&=~LEFTBUTTON;
	if (dragpage<0) return 0;
	
	int pg;
	DBG int spr=
	findSpread(x,y,&pg);
	DBG cout <<"SpreadInterface lbup found page "<<pg<<" in spread "<<spr<<endl;
	if (pg<0) {
		 // do not drop page anywhere
		 //*** in the future this will be something like pop page into limbo?
		if (x<0 || x>curwindow->win_w || y<0 || y>curwindow->win_h) {
			 // mouse up outside window so search for a ViewWindow to shift view for
			anXWindow *win=NULL;
			int mx,my;
			mouseposition(&mx,&my,NULL,&win);
			if (win && !strcmp(win->whattype(),"LaidoutViewport")) {
				LaidoutViewport *vp=dynamic_cast<LaidoutViewport *>(win);
				vp->UseThisDoc(doc);
				vp->SelectPage(dragpage);
				
				DBG cout <<" ~~~~~~~~~~~~drop page "<<dragpage<<" to win type: "<<win->whattype()<<endl;
			}
		}
		dragpage=-1;
		return 0;
	}
	
	 // swap dragpage and pg
	DBG cout <<"*** swap "<<dragpage<<" to position "<<pg<<endl;
	if ((state&LAX_STATE_MASK)==0) SlidePages(dragpage,pg);
	else if ((state&LAX_STATE_MASK)==ShiftMask) SwapPages(dragpage,pg);

	dragpage=-1;
	
	return 0;
}

/*! *** this function is temporary! the full SpreadInterface will be much more robust.
 * this is provided here just to get things off the ground....
 *
 * Move page at previouspos to newpos, sliding the intervening pages toward the
 * newly open slot..
 *
 * \todo *** later, this will optionally push the replaced page into limbo
 */
void SpreadInterface::SlidePages(int previouspos, int newpos)
{
	if (previouspos==newpos) return;
	if (previouspos<newpos) { 
		 // 01234 -> 12304 (move 0 to 3 position)
		for (int c=previouspos; c<newpos; c++) SwapPages(c,c+1);
	} else {
		 // 01234 -> 30124 (move 3 to 0 position)
		for (int c=previouspos; c>newpos; c--) SwapPages(c,c-1);
	}
}

//! Swaps pagelabel and the littlespread->spread->pagestack[??]->page.
/*! previouspos and newpos are both page indices at the document level, not
 * the spread or little spread level. Changes are temporary here. Permanent
 * changes to the document are made in ApplyChanges()
 */
void SpreadInterface::SwapPages(int previouspos, int newpos)
{
	if (previouspos<0 || previouspos>=doc->pages.n ||
	    newpos<0 || newpos>=doc->pages.n) return;
	
	 // swap index map
	int t=temppagemap[previouspos];
	temppagemap[previouspos]=temppagemap[newpos];
	temppagemap[newpos]=t;
	
	 // swap the littlespread->spread->pagestack->page
	int oc,oc2,nc,nc2;
	for (oc=0; oc<spreads.n; oc++) {
		for (oc2=0; oc2<spreads.e[oc]->spread->pagestack.n; oc2++) {
			if (spreads.e[oc]->spread->pagestack.e[oc2]->index==previouspos) break;
		}
		if (oc2!=spreads.e[oc]->spread->pagestack.n) break;
	}
	if (oc==spreads.n) return;
	for (nc=0; nc<spreads.n; nc++) {
		for (nc2=0; nc2<spreads.e[nc]->spread->pagestack.n; nc2++) {
			if (spreads.e[nc]->spread->pagestack.e[nc2]->index==previouspos) break;
		}
		if (nc2!=spreads.e[nc]->spread->pagestack.n) break;
	}
	if (nc==spreads.n) return;

	 //*** this is a little shoddy... needs more checking, swap which attributes??
	Page *tpage=spreads.e[oc]->spread->pagestack.e[oc2]->page;
	spreads.e[oc]->spread->pagestack.e[oc2]->page=spreads.e[nc]->spread->pagestack.e[nc2]->page;
	spreads.e[nc]->spread->pagestack.e[nc2]->page=tpage;
	t=spreads.e[oc]->spread->pagestack.e[oc2]->index;
	spreads.e[oc]->spread->pagestack.e[oc2]->index=spreads.e[nc]->spread->pagestack.e[nc2]->index;
	spreads.e[nc]->spread->pagestack.e[nc2]->index=t;

	needtodraw=1;
}

//! Apply however pages have been rearranged to the document.
/*! */
void SpreadInterface::ApplyChanges()
{
	DBG cout<<"ApplyChanges:"<<endl;

	//****

	int n;
	char *newlocal,*oldlocal;
	Page **newpages,**oldpages=doc->pages.extractArrays(&oldlocal,&n);
	
	newpages=new Page*[doc->pages.n];
	newlocal=new char[doc->pages.n];
	
	int pg;
	for (int c=0; c<n; c++) {
		DBG cout <<" --move page "<<pg<<" to page "<<c<<endl;
		pg=temppagemap[c];
		newpages[c]=oldpages[pg];
		newlocal[c]=oldlocal[pg];

		temppagemap[c]=c;
	}
	doc->pages.insertArrays(newpages,newlocal,n);
	delete[] oldlocal;
	delete[] oldpages;
	needtodraw=1;

	doc->SyncPages(0,-1);
	laidout->notifyDocTreeChanged(curwindow->win_parent,TreePagesMoved,0,-1);
}

//! Reset whatever tentative changes to page order have been made since last apply.
/*! temppagemap elements say what page index is temporarily in spot index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 * So, reseting is merely a matter of calling SwapPages() until temppagemap[index]==index.
 */
void SpreadInterface::Reset()
{
	int c,c2;
	for (c=0; c<doc->pages.n; c++) {
		if (temppagemap[c]==c) continue;
		for (c2=c+1; c2<doc->pages.n; c2++)
			if (temppagemap[c2]==c) break;
		if (c2==doc->pages.n) continue;
		SwapPages(c,c2);
	}
}

//! Selects spreads.
int SpreadInterface::rMBDown(int x,int y,unsigned int state,int count)
{
	if (buttondown && buttondown!=MIDDLEBUTTON) return 1;
	int pg,p=findSpread(x,y,&pg);
	curspread=NULL;
	curpage=-1;
	if (p<0) return 1;
	buttondown=MIDDLEBUTTON;
	mx=x; my=y;
	curspread=spreads.e[p];
	curpage=pg;
	return 0;
}

//! Drops spreads.
int SpreadInterface::rMBUp(int x,int y,unsigned int state)
{
	if (buttondown!=MIDDLEBUTTON) return 1;
	buttondown=0;
	return 0;
}

//! Drag spreads or pages
int SpreadInterface::MouseMove(int x,int y,unsigned int state)
{
	if (!buttondown) return 1;
	if (!curspread) return 1;

	if (buttondown==MIDDLEBUTTON) {
		curspread->origin(curspread->origin()+screentoreal(x,y)-screentoreal(mx,my));
		curspread->mapConnection();
		if (curspread->next) curspread->next->mapConnection();
		arrangetype=arrangestate=ArrangeCustom;
		mx=x; my=y;
		needtodraw=1;
	} else if (buttondown==LEFTBUTTON) {
	} else return 1;

	return 0;
}

//! Find the spread under screen point (x,y), return index or -1.
/*! If page!=NULL, then also find the index of 
 */
int SpreadInterface::findSpread(int x,int y, int *page)
{
	flatpoint p=screentoreal(x,y);
	int pg=-1,c;
	for (c=spreads.n-1; c>=0; c--) {
		if (page) pg=spreads.e[c]->pointin(p,2);
		else pg=spreads.e[c]->pointin(p,1);
		if (pg) break;
	}
	if (c!=-1) {
		if (page) *page=pg-1; // find page number too.*** what index is a dummy page??
		return c;
	} 
	if (page) *page=-1;
	return -1;
}

////! With spread coordinate p, find corresponding pagestack index in the spread, or -1
///*! p is in coordinates of the spread->path.
// */
//int SpreadInterface::findPageInSpread(Spread *s,flatpoint p)
//{
//	int c;
//	for (c=0; c<s->pagestack.n; c2++) {
//		
//	}
//}

//! Find the page under screen point (x,y), return index or -1.
/*! \todo *** imp me!
 */
int SpreadInterface::findPage(int x,int y)
{//***
	return -1;
}

//! Center all (w==1)...
void SpreadInterface::Center(int w)
{
	DoubleBBox bb;
	for (int c=0; c<spreads.n; c++) {
		bb.addtobounds(spreads.e[c]->m(),spreads.e[c]);
	}
	double ww=bb.maxx-bb.minx,
		   hh=bb.maxy-bb.miny;
	bb.minx-=ww*.05;
	bb.maxx+=ww*.05;
	bb.miny-=hh*.05;
	bb.maxy+=hh*.05;
	dp->Center(bb.minx,bb.maxx,bb.miny,bb.maxy);
}

/*! Switch to slide cursor when shift up.
 */
int SpreadInterface::CharRelease(unsigned int ch,unsigned int state)
{
	if (ch==LAX_Shift && dragpage>=0) {
		Cursor cursor=XCreateFontCursor(app->dpy, XC_right_side);
		XDefineCursor(app->dpy, curwindow->window, cursor);
		XFreeCursor(app->dpy, cursor);
	}
	return 1;
}

//! Key presses.
/*!
 * <pre>
 *   ' '    Center with all little spreads in view
 *   'c'    toggle where the page labels go
 *   'm'    toggle mark of current page
 *   'M'    reverse toggle mark of current page
 *   't'    toggle drawing of thumbnails
 *   'A'    toggle how to arrange the spreads
 *  +'A'    force arranging the spreads using current arrange style
 *   'p'    *** for debugging thumbs
 * </pre>
 *
 * \todo *** space should arrange if auto arrange
 */
int SpreadInterface::CharInput(unsigned int ch,unsigned int state)
{
	if (ch==LAX_Shift && dragpage>=0) {
		Cursor cursor=XCreateFontCursor(app->dpy, XC_exchange);
		XDefineCursor(app->dpy, curwindow->window, cursor);
		XFreeCursor(app->dpy, cursor);
	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		Center(1);
		needtodraw=1;
		return 0;
	} else if (ch=='m' && (state&LAX_STATE_MASK)==0) { // toggle mark
		if (curpage<0) return 0;
		doc->pages.e[temppagemap[curpage]]->labeltype=
				(doc->pages.e[temppagemap[curpage]]->labeltype+1)%8;
		needtodraw=1;
		return 0;
	} else if (ch=='M' && (state&LAX_STATE_MASK)==ShiftMask) { // toggle mark
		if (curpage<0) return 0;
		doc->pages.e[temppagemap[curpage]]->labeltype=
				(doc->pages.e[temppagemap[curpage]]->labeltype+7)%8;
		needtodraw=1;
		return 0;
	} else if (ch=='p') { //*** for debugging thumbnails....
		DBG if (curpage<0) return 0;
		DBG ImageData *thumb=doc->pages.e[curpage]->Thumbnail();
		DBG cout <<"'P' image dump:"<<endl;
		DBG thumb->dump_out(stdout,2,0);
		DBG if (thumb) {
		DBG 	dp->StartDrawing(curwindow);
		DBG 	//double i[6];
		DBG 	//transform_invert(i,thumb->m());
		DBG 	//dp->PushAndNewTransform(i);
		DBG 	::DrawData(dp,thumb,NULL,NULL,DRAW_AXES|DRAW_BOX);
		DBG 	//dp->PopAxes();
		DBG 	dp->EndDrawing();
		DBG }
		DBG return 0;
	} else if (ch=='t' && (state&LAX_STATE_MASK)==0) {
		drawthumbnails=!drawthumbnails;
		needtodraw=1;
		return 0;
	} else if (ch=='c' && (state&LAX_STATE_MASK)==0) {
		centerlabels++;
		if (centerlabels>4) centerlabels=0;
		needtodraw=1;
		return 0;
	} else if (ch=='A' && (state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
		arrangestate=ArrangeNeedsArranging;
		ArrangeSpreads();
		needtodraw=1;
		return 0;
	} else if (ch=='A' && (state&LAX_STATE_MASK)==ShiftMask) {
		arrangetype++;
		if (arrangetype==ArrangetypeMax+1) arrangetype=ArrangetypeMin;

		if (viewport) viewport->postmessage(arrangetypestring(arrangetype));
		else app->postmessage(arrangetypestring(arrangetype));

		ArrangeSpreads();
		needtodraw=1;
		return 0;
	}
	return 1;
}

//----------------------- SpreadEditor --------------------------------------

/*! \class SpreadEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for spread editing.
 *
 * This creates the window with a SpreadInterface.
 * The SpreadInterface is an interface rather than a viewport because someday it might
 * be optionally integrated into the ViewWindow to provide the infinite scroll features
 * found in various other programs.
 */


//! Make the window using project.
SpreadEditor::SpreadEditor(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
						int xx, int yy, int ww, int hh, int brder,
						//Window owner,const char *mes, 
						Project *nproj,Document *ndoc)
	: ViewerWindow(parnt,ntitle,nstyle|ANXWIN_DELETEABLE|VIEWPORT_RIGHT_HANDED|VIEWER_BACK_BUFFER, 
					xx,yy,ww,hh, brder, NULL)//final null is standard ViewportWindow...
{
	project=nproj;
	doc=ndoc;

	if (!project) project=laidout->project;
	if (project) {
		if (doc) { // make sure doc is in proj?
			int c;
			for (c=0; c<project->docs.n; c++) if (doc==project->docs.e[c]) break;
			if (c==project->docs.n) doc=NULL;
		} 
		if (!doc) { // doc=first proj doc
			if (project->docs.n) doc=project->docs.e[0];
		}
	} 
	viewport->win_xatts.background_pixel=app->rgbcolor(200,200,200);
	viewport->win_xattsmask|=CWBackPixel;
	viewport->dp->NewBG(255,255,255);

	needtodraw=1;
	AddTool(new SpreadInterface(viewport->dp,project,doc),1,1); // local, and select it
}

//! Passes off to SpreadInterface::dump_out().
void SpreadEditor::dump_out(FILE *f,int indent,int what)
{
	((SpreadInterface *)curtool)->dump_out(f,indent,0);
}

//! Passes off to SpreadInterface::dump_in_atts().
void SpreadEditor::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	((SpreadInterface *)curtool)->dump_in_atts(att,flag);
}

/*! Return 0 for success, nonzero error.
 */
int SpreadEditor::UseThisDoc(Document *ndoc)
{
	if (!doc) return 1;
	doc=ndoc;
	SpreadInterface *s=((SpreadInterface *)curtool);
	s->doc=doc;
	s->GetSpreads();
	s->ArrangeSpreads(-1);
	return 0;
}

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int SpreadEditor::init()
{
	//AddWin(***)...
	ViewerWindow::init();
	 //***remove the rulers
	wholelist.remove(0); //first null
	wholelist.remove(0); //x
	wholelist.remove(0); //y
	
	//XSetWindowBackground(app->dpy,viewport->window,~0);
//	viewport->syncrulers();

	anXWindow *last=NULL;
	TextButton *tbut;

	wholelist.e[wholelist.n-1]->pw(100);
	//AddNull(); // makes the status bar fill whole line

	last=tbut=new TextButton(this,"applybutton",0, 0,0,0,0,1, NULL,window,"applybutton","Apply");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	last=tbut=new TextButton(this,"resetbutton",0, 0,0,0,0,1, last,window,"resetbutton","Reset");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	last=tbut=new TextButton(this,"updatethumbs",0, 0,0,0,0,1, last,window,"updatethumbs","Update Thumbs");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	Sync(1);	
	return 0;
}

/*! Catches "docTreeChange".
 */
int SpreadEditor::DataEvent(Laxkit::EventData *data,const char *mes)
{
	DBG cout <<"SpreadEditor got message: "<<mes<<endl;
	if (!strcmp(mes,"docTreeChange")) {
		TreeChangeEvent *te=dynamic_cast<TreeChangeEvent *>(data);
		if (!te || te->changer==this) return 1;

		if (te->changetype==TreeObjectRepositioned ||
				te->changetype==TreeObjectReorder ||
				te->changetype==TreeObjectDiffPage ||
				te->changetype==TreeObjectDeleted ||
				te->changetype==TreeObjectAdded) {
			DBG cout <<"*** need to make a SpreadEditor:: flag for need to update thumbs"<<endl;
		} else if (te->changetype==TreePagesAdded ||
				te->changetype==TreePagesDeleted ||
				te->changetype==TreePagesMoved) {
			((SpreadInterface *)curtool)->CheckSpreads(te->start,te->end);
		} else if (te->changetype==TreeDocGone) {
			cout <<" ***need to imp SpreadEditor::DataEvent -> TreeDocGone"<<endl;
		}
		
		delete te;
		return 0;
	}
	return 1;
}

/*! Responds to:
 *
 * "resetbutton",
 * "applybutton",
 * "updatethumbs"
 */
int SpreadEditor::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp("resetbutton",mes)) {
		//cout <<"SpreadEditor got resetbutton message"<<endl;
		SpreadInterface *s=dynamic_cast<SpreadInterface *>(curtool);
		if (s) s->Reset();
		return 0;
	} else if (!strcmp("applybutton",mes)) {
		//cout <<"SpreadEditor got applybutton message"<<endl;
		SpreadInterface *s=dynamic_cast<SpreadInterface *>(curtool);
		if (s) s->ApplyChanges();
		return 0;
	} else if (!strcmp("updatethumbs",mes)) {
		 // *** kind of a hack
		for (int c=0; c<doc->pages.n; c++) doc->pages.e[c]->modtime=times(NULL);
		((anXWindow *)viewport)->Needtodraw(1);
		return 0;
	}
	return 1;
}

int SpreadEditor::CharInput(unsigned int ch,unsigned int state)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
		app->addwindow(new HelpWindow());
		return 0;
	}
	return 1;
}

//! Trigger an ArrangeSpreads if arrangetype is auto.
int SpreadEditor::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=ViewerWindow::MoveResize(nx,ny,nw,nh);
	if (((SpreadInterface *)curtool)->arrangetype==ArrangeAutoTillMod ||
			((SpreadInterface *)curtool)->arrangetype==ArrangeAutoAlways) 
		((SpreadInterface *)curtool)->ArrangeSpreads();
	return c;
}

//! Trigger an ArrangeSpreads if arrangetype is auto.
int SpreadEditor::Resize(int nw,int nh)
{
	int c=ViewerWindow::Resize(nw,nh);
	if (((SpreadInterface *)curtool)->arrangetype==ArrangeAutoTillMod ||
			((SpreadInterface *)curtool)->arrangetype==ArrangeAutoAlways) 
		((SpreadInterface *)curtool)->ArrangeSpreads();
	return c;
}

