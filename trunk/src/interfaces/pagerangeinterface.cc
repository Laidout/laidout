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
// Copyright (C) 2010 by Tom Lechner
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



#define fudge 5.0


// 0               4             6              doc.n-1
// |iv,iii,ii...---|1,2,3...-----|A-1 ... A-10--|
// ^pos
//
// NumStack<double> positions;
// double xscale,yscale;
// flatpoint offset;






//------------------------------------- PageRangeInterface --------------------------------------
	
/*! \class PageRangeInterface 
 * \brief Interface to define and modify page label ranges.
 *
 * This works on PageRange objects kept by Document objects.
 */


PageRangeInterface::PageRangeInterface(int nid,Displayer *ndp,Document *ndoc)
	: InterfaceWithDp(nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;

	showdecs=0;
	doc=NULL;
	UseThisDocument(doc);
}

PageRangeInterface::PageRangeInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;

	showdecs=0;
	doc=NULL;
}

PageRangeInterface::~PageRangeInterface()
{
	DBG cerr <<"PageRangeInterface destructor.."<<endl;

	if (doc) doc->dec_count();
}

const char *PageRangeInterface::Name()
{ return _("Page Range Organizer"); }

//! Return something like "1,2,3,..." or "iv,iii,ii,..."
/*! If range<0, then use currange. If first<0, use default for that range.
 */
char *PageRangeInterface::LabelPreview(int range,int first)
{
	if (range<0) range=currange;

	char *str=NULL;
	int f,l; //first, length of range
	PageRange *rangeobj=NULL;
	char del=0;

	if (doc->pageranges.n) {
		if (range>=doc->pageranges.n) range=doc->pageranges.n-1;
		rangeobj=doc->pageranges.e[range];

		if (first<0) f=rangeobj->start; 
		else f=first;
		l=rangeobj->end-rangeobj->start+1;
	} else {
		rangeobj=new PageRange;
		del=1;

		if (range>0) range=0;
		l=doc->pages.n;
		f=0;
	}

	str=rangeobj->GetLabel(rangeobj->start,f);
	if (l>1) {
		appendstr(str,",");
		char *strr=rangeobj->GetLabel(rangeobj->start+1,f);
		appendstr(str,strr);
		delete[] strr;
		if (l>2) {
			appendstr(str,",");
			char *strr=rangeobj->GetLabel(rangeobj->start+2,f);
			appendstr(str,strr);
			delete[] strr;
		}
		appendstr(str,",...");
	}

	return str;
}

#define RANGE_Custom_Base   10000
#define RANGE_Delete        10001

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PageRangeInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Paper Interface"));

	menu->AddItem(_("Custom base..."),RANGE_Custom_Base);
	menu->AddSep();
	menu->AddItem(_("Delete range"),RANGE_Delete);

	char *str=NULL;
	for (int c=0; c<Numbers_MAX; c++) {
		if (c==Numbers_None) menu->AddItem(_("(none)"),c);
		else {
			 //add first, first++, first+++, ...  in number format
			str=LabelPreview(currange,-1);
			menu->AddItem(str,c);
			delete[] str;
		}
	}

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int PageRangeInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item
		if (i==RANGE_Custom_Base) {
			 //portrait
			return 0;

		} else if (i==RANGE_Delete) {
			 //landscape
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
int PageRangeInterface::UseThisDocument(Document *ndoc)
{
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
int PageRangeInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	Document *d=dynamic_cast<Document *>(ndata);
	if (!d && ndata) return 0; //was a non-null object, but not a document

	UseThisDocument(d);
	needtodraw=1;

	return 1;
}

/*! Will say it cannot draw anything.
 */
int PageRangeInterface::draws(const char *atype)
{ return 0; }


//! Return a new PageRangeInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PageRangeInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PageRangeInterface(id,NULL);
	else if (!dynamic_cast<PageRangeInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int PageRangeInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	yscale=50;
	xscale=dp->Maxx-dp->Minx-10;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int PageRangeInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PageRangeInterface::Clear(SomeData *d)
{
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int PageRangeInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	dp->DrawScreen();

	double w,h=yscale;
	flatpoint o(dp->Minx,dp->Maxy);
	o-=offset;

	 //draw blocks
//	for (int c=0; c<positions.n-1; c++) {
//		w=xscale*(positions.e[c+1]-positions.e[c]);
//
//		dp->NewFG(&doc->pageranges.e[c]->color);
//		dp->drawrectangle(o.x,o.y, w,h, 1);
//
//		dp->NewFG((unsigned long)0);
//		dp->drawrectangle(o.x,o.y, w,h, 0);
//	}
	dp->DrawReal();

	return 1;
}

//! Return which position and range mouse is over
int PageRangeInterface::scan(int x,int y, int *range)
{
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

int PageRangeInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
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

int PageRangeInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
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

//int PageRangeInterface::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int PageRangeInterface::MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int PageRangeInterface::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
//int PageRangeInterface::RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
//int PageRangeInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
//int PageRangeInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);

int PageRangeInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int r=-1;
	int over=scan(x,y,&r);

	DBG cerr <<"over pos,range: "<<over<<","<<r<<endl;

	int lx,ly;
	buttondown.move(mouse->id,x,y, &lx,&ly);

	if (!buttondown.any()) {}

	if ((state&LAX_STATE_MASK)==0) {
		offset.x+=x-lx;
		offset.y+=y-ly;
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
int PageRangeInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
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

int PageRangeInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}


//} // namespace Laidout

