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
//    Copyright (C) 2014 by Tom Lechner
//



#include "pagemarkerinterface.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


//You need this if you use any of the Laxkit stack templates in lax/lists.h
#include <lax/lists.cc>


using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//--------------------------- PageMarkerInterface -------------------------------------

/*! \class PageMarkerInterface
 * \brief Interface to select page status indicator.
 */

enum PageMarkerInterfaceValues {
	PSTATE_None       =0,
	PSTATE_Normal     =1,
	PSTATE_Select     =2,
	PSTATE_SelectFull =3,

	PSTATE_Page       =100,
	PSTATE_NewColor   =101,
	PSTATE_NewShape   =102,

	PSTATE_MAX
};

PageMarkerInterface::PageMarkerInterfaceNode::PageMarkerInterfaceNode(Page *npage, flatpoint npos, double nscaling)
{
	page=npage;
	if (page) page->inc_count();
	origin=npos;
	scaling=nscaling;
}

PageMarkerInterface::PageMarkerInterfaceNode::~PageMarkerInterfaceNode()
{
	if (page) page->dec_count();
}


PageMarkerInterface::PageMarkerInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_type=LaxInterfaces::INTERFACE_Overlay;

	mode=PSTATE_Normal;
	showdecs=1;
	shownumbers=true;
	needtodraw=1;
	hover=PSTATE_None;
	hoveri=-1;
	uiscale=1.5;
	curpage=-1;

	boxw=boxh=0;

	sc=NULL;

	colors.push(new ScreenColor(0.,0.,0.,1.));
	colors.push(new ScreenColor(.33,.33,.33,1.));
	colors.push(new ScreenColor(.66,.66,.66,1.));
	colors.push(new ScreenColor(1.,1.,1.,1.));
	colors.push(new ScreenColor(1.,0.,0.,1.));
	colors.push(new ScreenColor(0.,1.,0.,1.));
	colors.push(new ScreenColor(0.,0.,1.,1.)); 
	colors.push(new ScreenColor(0.,1.,1.,1.));
	colors.push(new ScreenColor(1.,0.,1.,1.));
	colors.push(new ScreenColor(1.,1.,0.,1.));

	shapes.push(MARKER_Circle      );
	shapes.push(MARKER_Square      );
	shapes.push(MARKER_Diamond     );
	shapes.push(MARKER_TriangleDown);
	shapes.push(MARKER_Octagon     );
}

PageMarkerInterface::~PageMarkerInterface()
{
	if (sc) sc->dec_count();
}

const char *PageMarkerInterface::whatdatatype()
{ 
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *PageMarkerInterface::Name()
{  return _("Page Marker tool"); }


//! Return new PageMarkerInterface.
/*! If dup!=NULL and it cannot be cast to PageMarkerInterface, then return NULL.
 */
anInterface *PageMarkerInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new PageMarkerInterface(NULL,id,NULL);
	else if (!dynamic_cast<PageMarkerInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int PageMarkerInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int PageMarkerInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PageMarkerInterface::Clear(SomeData *d)
{
}

Laxkit::MenuInfo *PageMarkerInterface::ContextMenu(int x,int y,int deviceid)
{
//	if (no menu for x,y) return NULL;
//
//	MenuInfo *menu=new MenuInfo;
//	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, (freehand_style&FREEHAND_Raw_Path)?LAX_CHECKED:0);
//	menu->AddItem(_("Some menu item"), SOME_MENU_VALUE);
//	menu->AddSep(_("Some separator text"));
//	menu->AddItem(_("Et Cetera"), SOME_OTHER_VALUE);
//	return menu;
	
	return NULL;
}

int PageMarkerInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    DBG cerr <<"PageMarkerInterface got message: "<<(mes?mes:"?")<<endl;

// ----: unnecessary? now that doc->pages are refcounted??
//    if (!strcmp(mes,"docTreeChange")) {
//        const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);
//        if (!te || te->changer==this) return 1;
//
//        if (te->changetype==TreeObjectRepositioned ||
//                te->changetype==TreeObjectReorder ||
//                te->changetype==TreeObjectDiffPage ||
//                te->changetype==TreeObjectDeleted ||
//                te->changetype==TreeObjectAdded) {
//            //DBG cerr <<"*** need to make a SpreadEditor:: flag for need to update thumbs"<<endl;
//        } else if (te->changetype==TreePagesAdded ||
//                te->changetype==TreePagesDeleted ||
//                te->changetype==TreePagesMoved) {
//            spreadtool->CheckSpreads(te->start,te->end);
//        } else if (te->changetype==TreeDocGone) {
//            //cout <<" ***need to imp SpreadEditor::DataEvent -> TreeDocGone"<<endl;
//
//        }
//		return 0;
//	}

//    if (!strcmp(mes,"menuevent")) {
//        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
//        int i =s->info2; //id of menu item
//
//        if ( i==SOME_MENU_VALUE) {
//			...
//		}
//
//		return 0; 
//	}

	return 1; //event not absorbed
}


/*! Return a color that should stand out against the given color.
 */
unsigned long standoutcolor(const Laxkit::ScreenColor &color)
{
	ScreenColor col(0,0,0,65535);
	if (color.red<32768) col.red=65535;
	if (color.green<32768) col.green=65535;
	if (color.blue<32768) col.blue=65535;
	return col.Pixel();
}


int PageMarkerInterface::Refresh()
{
	if (needtodraw==0) return 0;
	needtodraw=0;

	if (shapes.n==0 || colors.n==0) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	double th=dp->textheight();
	double pad=th/3;

	if (boxw==0) {
		boxh=colors.n*uiscale*th + 2*pad;
		boxw=shapes.n*uiscale*th + 2*pad;
	}

	 //else draw numbers on pages
	if (shownumbers) {
		dp->DrawScreen();

		flatpoint p;
		DrawThingTypes t=THING_None;
		double w,h;
		dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg,.3));
		for (int c=0; c<pages.n; c++) {
			p=dp->realtoscreen(pages.e[c]->origin);

			if (c==curpage) {
				dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
				dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg,.6));
			} else {
				dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
				dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg,.3));
			}
			dp->NewBG(&pages.e[c]->page->labelcolor);

			dp->textextent(pages.e[c]->page->label,-1,&w,&h);
			w/=2;
			h/=2;
			w+=h/2;
			h+=h/2;

			switch (pages.e[c]->page->labeltype) {
				case MARKER_Circle:       t=THING_Circle;  break;
				case MARKER_Square:       t=THING_Square;  break;
				case MARKER_TriangleDown: t=THING_Triangle_Down;  break;
				case MARKER_Octagon:      t=THING_Octagon; break;
				case MARKER_Diamond:      t=THING_Diamond; break;
				default:   t=THING_Circle;  break;
			}
			//dp->drawFormattedPoints(shape[pages.e[c]->labeltype]);
			dp->drawthing(p.x,p.y,w,h, 2, t);

			dp->NewFG(standoutcolor(pages.e[c]->page->labelcolor));
			//dp->NewFG(curwindow->win_colors->fg);
			dp->textout(p.x,p.y, pages.e[c]->page->label,-1, LAX_CENTER);

		}

		dp->DrawReal();
	}

	if (mode==PSTATE_Select || mode==PSTATE_SelectFull) {
		double shapew=(boxw-2*pad-(mode==PSTATE_Select?0:uiscale/2*th))/shapes.n;
		double shapeh=(boxh-2*pad-(mode==PSTATE_Select?0:uiscale/2*th))/colors.n;
		double dx,dy;

		dp->DrawScreen();

		 //fill background
		dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg,.3));
		dp->NewBG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg,.8));
		dp->drawrectangle(boxoffset.x,boxoffset.y, boxw,boxh, 2);

		dp->NewFG(0,0,0);
		DrawThingTypes t=THING_None;

		for (int c2=0; c2<colors.n; c2++) {
			dp->NewBG(colors.e[c2]);
			dy=boxoffset.y + pad + c2*shapeh;
			for (int c=0; c<shapes.n; c++) {
				switch (shapes.e[c]) {
					case MARKER_Circle:       t=THING_Circle;  break;
					case MARKER_Square:       t=THING_Square;  break;
					case MARKER_TriangleDown: t=THING_Triangle_Down;  break;
					case MARKER_Octagon:      t=THING_Octagon; break;
					case MARKER_Diamond:      t=THING_Diamond; break;
					default:   t=THING_Circle;  break;
				}

				dx=boxoffset.x + pad + c*shapew;

				if (hoveri==c2*shapes.n+c) {
					dp->NewFG(1.,1.,1.);
					dp->drawrectangle(dx,dy, shapew,shapeh, 1);
					dp->NewFG(0,0,0);
				}

				dp->drawthing(dx+shapew/2,dy+shapeh/2, .8*shapew/2,.8*shapeh/2, 2, t);
			}
		}
		dp->DrawReal();
		return 0;
	}


	return 0;
}

int PageMarkerInterface::scan(int x,int y, int &index)
{
	index=-1;
	double th=dp->textheight();
	double pad=th/3;

	if (mode==PSTATE_Select || mode==PSTATE_SelectFull) {
		 //scan selection box
		if (colors.n==0 || shapes.n==0) return PSTATE_None;

		x-=boxoffset.x+pad;
		x=floor(x/((boxw-2*pad)/shapes.n));
		y-=boxoffset.y+pad;
		y=floor(y/((boxh-2*pad)/colors.n));

		if (x<0 || y<0) return PSTATE_None;
		if (mode==PSTATE_SelectFull) {
			if (x==shapes.n && y>=0 && y<colors.n) return PSTATE_NewShape;
			if (y==colors.n && x>=0 && x<shapes.n) return PSTATE_NewColor;
		}
		if (x>=shapes.n || y>=colors.n) return PSTATE_None;
		index=y*shapes.n+x;
		return PSTATE_Select;

	} else {
		 //scan pages
		flatpoint p2;
		flatpoint p(x,y);
		for (int c=0; c<pages.n; c++) {
			p2=dp->realtoscreen(pages.e[c]->origin);
			if (norm(p2-p)<th) {
				index=c;
				return PSTATE_Page;
			}
		}
	}

	return PSTATE_None;
}

//! Start a new freehand line.
int PageMarkerInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int index=-1;
	int what=scan(x,y, index);

	if (what==PSTATE_None) return 1;

	buttondown.down(d->id,LEFTBUTTON,x,y, what,index);

	if (what==PSTATE_Page) { //index should be >=0
		double th=dp->textheight();
		double pad=th/3;
		int xx=0;

		for ( ; xx<shapes.n; xx++) {
			if (pages.e[index]->page->labeltype==shapes.e[xx]) break;
		}
		int yy=nearestcolor(&pages.e[index]->page->labelcolor);
		boxoffset=flatpoint(x,y)-flatpoint(pad+(xx+.5)*uiscale*th, pad+(yy+.5)*uiscale*th);
		DBG cerr <<"   lbdown page marker: "<<xx<<','<<yy<<endl;

		mode=PSTATE_Select;
		int ii;
		what=scan(x,y, ii);
		buttondown.moveinfo(d->id,LEFTBUTTON, index,ii);
	}

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

/*! Diffs color only, not transparency.
 *
 * Returns the distance between space vectors where (x,y,z)==(r,g,b) with
 * r,g,b in range [0..1].
 *
 * For example, if c1 is all red, and c2 is all black, 1 is returned.
 * If c1 is all yellow, and c2 is all black, sqrt(2) is returned.
 * If c1 is white, and c2 is black, sqrt(3) is returned.
 */
double color_distance(ScreenColor *c1, ScreenColor *c2)
{
	spacepoint p1(c1->red/65535.,c1->green/65535.,c1->blue/65535.);
	spacepoint p2(c2->red/65535.,c2->green/65535.,c2->blue/65535.);
	return norm(p1-p2);
}

/*! Return the index in colors of the color nearest to the given color, or -1 if there are no colors.
 */
int PageMarkerInterface::nearestcolor(Laxkit::ScreenColor *color)
{
	if (colors.n==0) return -1;

	int index=-1;
	double d=2, dd;
	for (int i=0; i<colors.n; i++) {
		dd=color_distance(colors.e[i],color);
		if (dd<d) { index=i; d=dd; }
	}
	return index;
}

//! Finish a new freehand line by calling newData with it.
int PageMarkerInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	if (mode==PSTATE_Select) {
		int what, index=-1;
		buttondown.up(d->id,LEFTBUTTON, &what, &index);

		if (mode==PSTATE_Select && index>=0) {
			int color=index/shapes.n;
			int shape=index%shapes.n;

			if (owner) {
				 //send a message
				SimpleColorEventData *data=new SimpleColorEventData(65535,
						colors.e[color]->red, colors.e[color]->green, colors.e[color]->blue, 65535, shapes.e[shape]);
				app->SendMessage(data, owner->object_id, "pagemarker", object_id);
			} else {
				 //change just the page
				pages.e[what]->page->labeltype=shapes.e[shape];
				pages.e[what]->page->labelcolor=*colors.e[color];

			}
		}

		mode=PSTATE_Normal;
		needtodraw=1;
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int PageMarkerInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	int index=-1;
	int what=scan(x,y, index);
	DBG cerr <<"PageMarkerInterface move: "<<x<<','<<y<<"  i:"<<index<<" what:"<<what<<endl;

	if (!buttondown.any()) { 
		if (hover!=what || index!=hoveri) {
			hover=what;
			hoveri=index;
			needtodraw=1;
			if (hover==PSTATE_Page) PostMessage(_("Click to select marker"));
			else if (hover==PSTATE_None) PostMessage("");
		}
		return 1;
	}

	//else deal with mouse dragging...
	int lx, ly;
	buttondown.move(d->id, x,y, &lx,&ly);

	 //if drag outside box and box off screen, move it onscreen...
	if (y>dp->Maxy && boxoffset.y+boxh>dp->Maxy) { boxoffset.y-=y-dp->Maxy; needtodraw=1; }
	else if (y<dp->Miny && boxoffset.y<dp->Miny) { boxoffset.y-=y-dp->Miny; needtodraw=1; }

	if (x>dp->Maxx && boxoffset.x+boxw>dp->Maxx) { boxoffset.x-=x-dp->Maxx; needtodraw=1; }
	else if (x<dp->Minx && boxoffset.x<dp->Minx) { boxoffset.x-=x-dp->Minx; needtodraw=1; }

	what=scan(x,y, index); 
	
	if (mode==PSTATE_Select) {
		if (hover!=what || index!=hoveri) {
			hover=what;
			hoveri=index;
			int ii;
			buttondown.getextrainfo(d->id,LEFTBUTTON, &ii);
			buttondown.moveinfo(d->id,LEFTBUTTON, ii,index);
			needtodraw=1;
			//if (hover==PSTATE_Page) PostMessage(_("Click to select marker"));
			//else if (hover==PSTATE_None) PostMessage("");
		}
	}
	DBG cerr <<endl;

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int PageMarkerInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (mode==PSTATE_Normal) {
		// *** wheel to change marker?
		return 1;
	}

	boxoffset.y-=uiscale*dp->textheight();
	if (boxoffset.y + boxh <=dp->Miny) mode=PSTATE_Normal;
	needtodraw=1;
	return 0;
}

int PageMarkerInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (mode==PSTATE_Normal) return 1;

	boxoffset.y+=uiscale*dp->textheight();
	if (boxoffset.y>dp->Maxy) mode=PSTATE_Normal;
	needtodraw=1;
	return 0;
}





int PageMarkerInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"PageMarkerInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int PageMarkerInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"mode: "<<mode<<"  "<<(int)PSTATE_Select<<" "<<(int)PSTATE_SelectFull<<endl;
	if (ch==LAX_Esc && (mode==PSTATE_Select || mode==PSTATE_SelectFull)) {
		mode=PSTATE_Normal;
		needtodraw=1;
		return 0;
	}

	return 1; //key not dealt with, propagate to next interface
}

//int PageMarkerInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
//{ ***
//	return 1; //key not dealt with
//}

Laxkit::ShortcutHandler *PageMarkerInterface::GetShortcuts()
{
	// *** maybe define actions, but don't bind keys, since we want to hide it a little in Viewports, but not Spreadeditors?
	return NULL;
}

int PageMarkerInterface::PerformAction(int action)
{
	return 1;
}

int PageMarkerInterface::AddPage(Page *page, flatpoint pos, double scaling)
{
	return pages.push(new PageMarkerInterfaceNode(page,pos,scaling));
}

void PageMarkerInterface::ClearPages()
{
	pages.flush();
	needtodraw=1;
	mode=PSTATE_Normal;
}

/*! Make the page the current page, which gets displayed slightly differently.
 * Returns old curpage, an index into pages stack.
 */
int PageMarkerInterface::UpdateCurpage(Page *page)
{
	int old=curpage;
	if (page==NULL) curpage=-1;
	else for (int c=0; c<pages.n; c++) {
		if (page==pages.e[c]->page) { curpage=c; break; }
		if (c==pages.n-1) curpage=-1;
	}
	needtodraw=1;
	return old;
}



} // namespace Laidout

