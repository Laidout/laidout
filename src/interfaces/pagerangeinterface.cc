//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2011-2017 by Tom Lechner
//


#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/inputdialog.h>

#include "../language.h"
#include "pagerangeinterface.h"
#include "../ui/viewwindow.h"

//template implementation:
#include <lax/lists.cc>


#include <iostream>
using namespace std;
#define DBG 
using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



#define PAD 5
#define FUDGE 5.0




//------------------------------------- PageRangeInterface --------------------------------------
	
/*! \class PageRangeInterface 
 * \brief Interface to define and modify page label ranges.
 *
 * This works on PageRange objects kept by Document objects.
 */


#define PART_None          0
#define PART_Label         1
#define PART_DocPageStart  2
#define PART_LabelStart    3
#define PART_Position      4
#define PART_Index         5


PageRangeInterface::PageRangeInterface(int nid,Displayer *ndp,Document *ndoc)
	: anInterface(nid,ndp) 
{
	panelwidth=1;
	panelheight=1;

	currange=0;
	hover_part=-1;
	hover_range=-1;
	hover_position=-1;
	hover_index=-1;
	temp_range_l=NULL;
	temp_range_r=NULL;

	interface_type=INTERFACE_Overlay;
	showdecs=0;
	firsttime=1;
	doc=NULL;
	UseThisDocument(ndoc);

	defaultbg=rgbcolor(255,255,255);
	defaultfg=rgbcolor(0,0,0);
}

PageRangeInterface::PageRangeInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	panelwidth=1;
	panelheight=1;

	currange=0;
	hover_part=-1;
	hover_range=-1;
	hover_position=-1;
	hover_index=-1;
	temp_range_l=NULL;
	temp_range_r=NULL;

	interface_type=INTERFACE_Overlay;
	showdecs=0;
	firsttime=1;
	doc=NULL;

	defaultbg=rgbcolor(255,255,255);
	defaultfg=rgbcolor(0,0,0);
}

PageRangeInterface::~PageRangeInterface()
{
	DBG cerr <<"PageRangeInterface destructor.."<<endl;

	if (doc) doc->dec_count();
}

const char *PageRangeInterface::Name()
{ return _("Page Range Organizer"); }

//! Return something like "1,2,3,..." or "iv,iii,ii,..."
/*! If range<0, then use currange. If first<0, use default start for that range.
 */
char *PageRangeInterface::LabelPreview(int range,int first,int labeltype)
{
	if (!doc->pageranges.n) InstallDefaultRange();
	if (range>=0 && range<doc->pageranges.n) {
		if (!doc->pageranges.n) InstallDefaultRange();
		return LabelPreview(doc->pageranges.e[range],first,labeltype);
	}
	return newstr("?");
}

//! Return something like "1,2,3,..." or "iv,iii,ii,..."
/*! If range==NULL, then use currange. If first<0, use default start for that range.
 */
char *PageRangeInterface::LabelPreview(PageRange *rangeobj,int first,int labeltype)
{
	char *str=NULL;
	int f,l; //first, length of range

	if (first<0) f=rangeobj->first; 
	else f=first;
	l=rangeobj->end-rangeobj->start+1;

	if (rangeobj->labeltype==Numbers_None) {
		str=newstr(_("(no numbers)"));
	} else {
		str=rangeobj->GetLabel(rangeobj->start,f,labeltype);
		if (l>1) {
			appendstr(str,",");
			char *strr=rangeobj->GetLabel(rangeobj->start+1,f,labeltype);
			appendstr(str,strr);
			delete[] strr;
			if (l>2) {
				appendstr(str,",");
				char *strr=rangeobj->GetLabel(rangeobj->start+2,f,labeltype);
				appendstr(str,strr);
				delete[] strr;
			}
			appendstr(str,",...");
		}
	}

	return str;
}


//menu ids
#define RANGE_Custom_Base   10000
#define RANGE_Reverse       10001
#define RANGE_Delete        10002
#define RANGE_NumberType    10003

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PageRangeInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	 //no menu when not right click in box...
	int range=-1, index=-1, part=0;
	int pos=-1;
	int s=scan(x,y, &pos, &range, &part, &index);
	if (!s) return menu;
	currange=range;

	if (!menu) menu=new MenuInfo(_("Paper Interface"));
	else menu->AddSep(_("Paper"));

	menu->AddItem(_("Custom base..."),RANGE_Custom_Base);
	menu->AddItem(_("Reverse"),RANGE_Reverse);
	menu->AddSep();
	//if (doc && doc->pageranges.n>1) {
	//	menu->AddItem(_("Delete range"),RANGE_Delete);
	//	menu->AddSep();
	//}

	char *str=NULL;
	for (int c=0; c<Numbers_MAX; c++) {
		//if (c==Numbers_Default) menu->AddItem(_("(default)"),c);
		if (c==Numbers_Default) ;
		else if (c==Numbers_None) menu->AddItem(_("(none)"),c);
		else {
			 //add first, first++, first+++, ...  in number format
			str=LabelPreview(currange,-1,c);
			menu->AddItem(str, RANGE_NumberType, c);
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
			if (!doc || hover_range<0 || hover_range>=doc->pageranges.n) return 0;

			// ***** THIS IS TEMPORARY!! should be edit in place
			InputDialog *i=new InputDialog(NULL,_("New page label base"),_("New page label base"),ANXWIN_CENTER,
									 0,0,0,0,0,
									 NULL,object_id,"newbase",
									 doc->pageranges.e[hover_range]->labelbase, _("Label:"),
									 _("Ok"),BUTTON_OK,
									 _("Cancel"),BUTTON_CANCEL);
			app->rundialog(i);
			return 0;

		} else if (i==RANGE_Delete) {
			// *** imp
			return 0;

		} else if (i==RANGE_Reverse) {
			if (!doc || currange<0) return 0;
			PageRange *r=NULL;
			if (doc->pageranges.n) r=doc->pageranges.e[0];
			if (!r) doc->ApplyPageRange(NULL,Numbers_Arabic,"#",0,-1,doc->pages.n,1);
			else {
				if (r->decreasing) 
					doc->ApplyPageRange(r->name,r->labeltype,r->labelbase,r->start,r->end,r->first-(r->end-r->start),0);
				else
					doc->ApplyPageRange(r->name,r->labeltype,r->labelbase,r->start,r->end,r->first+(r->end-r->start),1);
			}
			needtodraw=1;
			return 0;

		} else if (i==RANGE_NumberType) {
			if (!doc || hover_range<0 || hover_range>=doc->pageranges.n) return 0;
			int num=s->info4;
			doc->pageranges.e[currange]->labeltype=num;
			doc->UpdateLabels(currange);
			needtodraw=1;
			return 0;
		}

		return 0;

	} else if (!strcmp(mes,"newbase")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 0;

		if (!doc || hover_range<0 || hover_range>=doc->pageranges.n) return 0;
		makestr(doc->pageranges.e[hover_range]->labelbase,s->str);
		doc->UpdateLabels(hover_range);
		needtodraw=1;

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

	needtodraw=1;

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

	panelheight=50;
	panelwidth=dp->Maxx-dp->Minx-2*PAD;
	//if (!doc) UseThisDocument(doc);

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
	//panelheight=50;
	//panelwidth=dp->Maxx-dp->Minx-2*PAD;
	//offset=-flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
}

int PageRangeInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;
	if (!doc) return 0;

	if (firsttime) {
		firsttime=0;
		panelheight=50;
		panelwidth=dp->Maxx-dp->Minx-2*PAD;
		offset=-flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
	}

	DBG cerr <<"PageRangeInterface::Refresh()..."<<doc->pageranges.n<<endl;

	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);

	double h = panelheight;
	flatpoint o;
	o -= offset;
	o.y -= h;

	 //draw blocks
	for (int c=0; c<(doc->pageranges.n?doc->pageranges.n:1); c++) {
		if (doc->pageranges.n) drawBox(doc->pageranges.e[c], (hover_range==c ? 1 : 0));
		else {
			PageRange r(NULL,"#",Numbers_Arabic,0,doc->pages.n-1,1,0);
			drawBox(&r,(hover_range==c?1:0));
		}
	}

	 //draw green position bar
	if (hover_part == PART_Position) {
		double pos=0;
		if (buttondown.isdown(0,LEFTBUTTON)) {
			if (temp_range_l) { drawBox(temp_range_l,0); pos=temp_range_l->end  /(double)doc->pages.n; }
			if (temp_range_r) { drawBox(temp_range_r,0); pos=temp_range_r->start/(double)doc->pages.n; }
		} else {
			if (doc->pageranges.n) {
				if (hover_position<doc->pageranges.n)
					pos = doc->pageranges.e[hover_position]->start/(double)doc->pages.n;
				else pos = 1;
			} else if (hover_position) pos = 1;
		}

		dp->NewFG(0.,1.,0.);
		o.x=-offset.x + panelwidth*pos;
		o.y=-offset.y-h;
		dp->drawrectangle(o.x-FUDGE/2,o.y, FUDGE,h, 1);
	}

	if (hover_part == PART_Index) {
		dp->NewFG(1.,0.,0.);
		o.x=-offset.x + panelwidth*((double)hover_index/doc->pages.n);
		o.y=-offset.y-h;
		dp->drawrectangle(o.x-FUDGE/2,o.y, FUDGE,h, 1);
	}


	dp->DrawReal();

	return 1;
}

//! Draw a whole box for r, outline, text, and all.
/*! <pre>
 *    0:4             4:1           6:1              doc.n-1
 *   |iv,iii,ii...---|1,2,3...-----|A-1 ... A-10--|
 *  ^pos
 * 
 *   --placement on screen from:
 *  double panelwidth,panelheight;
 *  flatpoint offset;
 *  </pre>
 */
void PageRangeInterface::drawBox(PageRange *r, int includehover)
{
	if (!r) return;
	double w, h = panelheight;
	flatpoint o(-offset.x, -offset.y-h);
	//o -= offset;
	//o.y -= h;

	 //draw blocks
	char *str=NULL;
	char sstr[30];
	int s,e,f, th=dp->textheight();

	w = panelwidth*(double)(r->end-r->start+1)/doc->pages.n;
	if (w == 0 || r->end<r->start) return;
	o.x += panelwidth*(double)r->start/doc->pages.n;

	DBG cerr<<"PageRange interface drawing rect "<<o.x<<','<<o.y<<" "<<w<<"x"<<h<<"  offset:"<<offset.x<<','<<offset.y<<endl;

	 //draw background color
	dp->NewFG(&r->color);
	dp->drawrectangle(o.x,o.y, w,h, 1);
	
	 //draw hover indicator for background
	if (includehover) {
		dp->NewFG(coloravg(defaultfg, defaultbg, .9));

		if (hover_part==PART_LabelStart) {
			dp->drawrectangle(o.x+w/2,o.y, w/2,h/2, 1);
		} else if (hover_part==PART_DocPageStart) {
			dp->drawrectangle(o.x,o.y, w/2,h/2, 1);
		} else if (hover_part==PART_Label) {
			dp->drawrectangle(o.x,o.y+h/2, w,h/2, 1);
		}
	}

	 //draw black outline around area
	//dp->NewFG(0.,0.,0.);
	dp->NewFG(coloravg(defaultfg, defaultbg, .5));
	dp->drawrectangle(o.x,o.y, w,h, 0);
	//dp->drawline(o.x,o.y, o.x+w,o.y+h);

	dp->NewFG(defaultfg);
	str = LabelPreview(r,-1,Numbers_Default);
	dp->textout(o.x,o.y+h*3/4, str,-1, LAX_LEFT|LAX_VCENTER);
	delete[] str;

	s = r->start;
	e = r->end;
	f = r->first;

	 //draw range numbers
	 //start - end 
	if (s-e==0) sprintf(sstr,"%d",s); else sprintf(sstr,"%d-%d",s,e);
	dp->textout(o.x+w/2-th/2,o.y+h/4, sstr,-1, LAX_RIGHT|LAX_VCENTER);
	sprintf(sstr,":");
	 //first - (first+start-end)
	dp->textout(o.x+w/2,o.y+h/4, sstr,-1, LAX_HCENTER|LAX_VCENTER);
	if (s-e==0) sprintf(sstr,"%d",f); else sprintf(sstr,"%d-%d",f,f+e-s);
	dp->textout(o.x+w/2+th/2,o.y+h/4, sstr,-1, LAX_LEFT|LAX_VCENTER);
}

//! Return a number in range [0..1] corresponding to page positions [0..doc->pages.n]
double PageRangeInterface::position(int pagenumber)
{
	if (!doc) return 0;
	return pagenumber/(double)doc->pages.n;
}

//! Return various info about the mouse position.
/*! If the mouse is very close to a dividing line of a page range, return the index of
 * that page range to the right in position, or -1. If over the very right most edge, then this
 * will be doc->pageranges.n.
 *
 * range is which page range chunk the mouse is in, or -1 for none.
 * 
 * part is which section of the page range chunk the mouse is over.
 * part is 1 for label preview, 2 for doc page index start, 3 for label start index, 0 for none.
 *
 * If index!=NULL, then return also the document page index that the position corresponds to.
 *
 * If any of position, range, part, index are found, return 1, else return 0.
 *
 * The actual return value of the function is 1 if there was something meaningful at x,y, else 
 */
int PageRangeInterface::scan(int x,int y, int *position, int *range, int *part, int *index)
{
	if (!doc) return 0;

	flatpoint fp(x,y);
	fp+=offset;
	fp.x/=panelwidth; 
	fp.y/=-panelheight; //now fp in range 0..1
	double fudge=FUDGE/panelwidth;
	DBG cerr <<"x,y="<<x<<","<<y<<"  pos="<<fp.x<<","<<fp.y<<"  offset="<<offset.x<<","<<offset.y<<endl;

	if (fp.y<0 || fp.y>1) return 0;

	int r=-1;   //which pagerange
	int pos=-1; //which page index
	double pp=0, ppd=1./doc->pages.n;
	if (!doc->pageranges.n) {
		 //if document has no defined ranges, it has 1 implied range
		if (fp.x>=0 && fp.x<=1) r=0;
		if (fp.x>-fudge && fp.x<fudge) pos=0;
		else if (fp.x>1-fudge && fp.x<1+fudge) pos=1;
	} else {
		for (int c=0; c<doc->pageranges.n; c++) {
			pp=(double)doc->pageranges.e[c]->start/doc->pages.n;
			ppd=(double)(doc->pageranges.e[c]->end+1)/doc->pages.n;

			if (c==0) {
				 //first loop, check for 0 position. checks range endpoints below
				if (fp.x>=0 && fp.x<=ppd) r=0;
				if (fp.x>-fudge && fp.x<fudge) pos=0;
			}
			if (fp.x>=pp && fp.x<=ppd) r=c;
			if (fp.x>=ppd-fudge && fp.x<=ppd+fudge) pos=c+1;
			if (r>=0 && pos>=0) break;
		}
	}

	if (r>=0 && part) {
		if (doc->pageranges.n) {
			pp=(double)doc->pageranges.e[r]->start/doc->pages.n;
			ppd=(double)(doc->pageranges.e[r]->end+1)/doc->pages.n;
		} else { pp=0; ppd=1; }
		if (fp.y<.5) *part=PART_Label;
		else if (fp.x<(pp+ppd)/2) *part=PART_DocPageStart;
		else *part=PART_LabelStart;
	}

	if (index) { 
		*index=(fp.x+.5/doc->pages.n)*(doc->pages.n);
		if (*index>=doc->pages.n) *index=-1;
	}

	if (range) *range=r;
	if (part && pos>=0) *part=PART_Position;
	if (position) *position=pos;
	
	if (pos>=0 || r>=0 || part>0) return 1;
	return 0;
}

int PageRangeInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (!doc) return 1;
	if (buttondown.isdown(0,LEFTBUTTON)) return 1; //only let one mouse work

	int range=-1, index=-1, part=0;
	int pos=-1;
	int s=scan(x,y, &pos, &range, &part, &index);
	if (!s) return 1;

	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	if (part==PART_Position) {
		if (pos>=0) {
			hover_part=PART_Position;

			if (!doc->pageranges.n) InstallDefaultRange();

			 //set up left side range
			PageRange *r;
			int i;
			if (hover_position>0) { r=doc->pageranges.e[hover_position-1]; i=r->end; }
			else { r=doc->pageranges.e[hover_position]; i=-1; }
			temp_range_l=new PageRange(r->name,r->labelbase,r->labeltype,r->start,i,r->first,r->decreasing);

			 //set up right side range
			if (hover_position<doc->pageranges.n) { r=doc->pageranges.e[hover_position]; i=r->start; }
			else { r=doc->pageranges.e[hover_position-1]; i=doc->pages.n; }
			temp_range_r=new PageRange(r->name,r->labelbase,r->labeltype,i,r->end,
										i==r->start?r->first:r->first+r->end-r->start+1,r->decreasing);
		}
		return 0;
	}

	if (!s) hover_part=0;

	//if (count==2) { // ***
	//}

	return 0;
}

int PageRangeInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!doc) return 1;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	int range=-1, index=-1, part=0;
	scan(x,y, NULL, &range, &part, &index);

	if (hover_part==PART_Position) {
		if (temp_range_l && temp_range_l->end>=temp_range_l->start) 
			doc->ApplyPageRange(temp_range_l->name,
								temp_range_l->labeltype,
								temp_range_l->labelbase,
								temp_range_l->start,
								temp_range_l->end,
								temp_range_l->first,
								temp_range_l->decreasing);
		if (temp_range_r && temp_range_r->end>=temp_range_r->start)
			doc->ApplyPageRange(temp_range_r->name,
								temp_range_r->labeltype,
								temp_range_r->labelbase,
								temp_range_r->start,
								temp_range_r->end,
								temp_range_r->first,
								temp_range_r->decreasing);

		if (temp_range_l) { delete temp_range_l; temp_range_l=NULL; }
		if (temp_range_r) { delete temp_range_r; temp_range_r=NULL; }
		needtodraw=1;
		hover_part=0;
		return 0;
	}

	if (!dragged) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (!doc->pageranges.n) InstallDefaultRange();
			if (range>=0 && range<doc->pageranges.n && index>doc->pageranges.e[range]->start && index<=doc->pageranges.e[range]->end) {
				 //Split range if clicking on an index in a range, but not on range endpoints
				char *name=NULL;
				int f;
				PageRange *r=doc->pageranges.e[range];
				if (r->decreasing) f=r->first-(index-r->start); else f=r->first+(index-r->start);
				if (r->name) { name=newstr(r->name); appendstr(name," (split)"); }
				doc->ApplyPageRange(name,r->labeltype,r->labelbase,index,r->end,f,r->decreasing);
				if (name) delete[] name;

				needtodraw=1;
			}
			return 0;
		}

		if ((state&LAX_STATE_MASK)==ControlMask) {
			// *** edit base
		} else {
			// *** edit first
		}
	}

	return 0;
}

//! If the document has no defined page ranges, install a default one.
/*! Return 0 for success or 1 for error.
 */
int PageRangeInterface::InstallDefaultRange()
{
	if (!doc) return 1;
	doc->ApplyPageRange(NULL,Numbers_Arabic,"#",0,-1,1,0);
	return 0;
}

int PageRangeInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if ((state&LAX_STATE_MASK)==ControlMask) {
		if (!scan(x,y, NULL,NULL,NULL,NULL)) return 1;
		panelwidth*=1.2;
		offset.x=1.2*(x+offset.x)-x;
		//offset.y=1.2*(y+offset.y)-y;
		needtodraw=1;
		return 0;
	}

	if (hover_part==PART_Label) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) {
			doc->pageranges.e[currange]->labeltype++;
			if (doc->pageranges.e[currange]->labeltype>=Numbers_MAX)
				doc->pageranges.e[currange]->labeltype=Numbers_None;
		}
		doc->UpdateLabels(currange);
		needtodraw=1;
		return 0;
	}

	if (currange>=0 && hover_part==PART_LabelStart) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) doc->pageranges.e[currange]->first++;
		doc->UpdateLabels(currange);
		needtodraw=1;
		return 0;
	}

	if (hover_part) return 0;
	return 1;
}

int PageRangeInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if ((state&LAX_STATE_MASK)==ControlMask) {
		if (!scan(x,y, NULL,NULL,NULL,NULL)) return 1;
		panelwidth/=1.2;
		offset.x=1/1.2*(x+offset.x)-x;
		//offset.y=1/1.2*(y+offset.y)-y;
		needtodraw=1;
		return 0;
	}

	if (hover_part==PART_Label) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) {
			if (doc->pageranges.e[currange]->labeltype==Numbers_Default)
				doc->pageranges.e[currange]->labeltype=Numbers_Arabic;
			else doc->pageranges.e[currange]->labeltype--;
			if (doc->pageranges.e[currange]->labeltype==Numbers_Default)
				doc->pageranges.e[currange]->labeltype=Numbers_MAX-1;
		}
		doc->UpdateLabels(currange);
		needtodraw=1;
		return 0;
	}

	if (currange>=0 && hover_part==PART_LabelStart) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) doc->pageranges.e[currange]->first--;
		doc->UpdateLabels(currange);
		needtodraw=1;
		return 0;
	}

	if (hover_part) return 0;
	return 1;
}


int PageRangeInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (!doc) return 0;

	int range=-1, index=-1, part=0;
	int over=-1;
	scan(x,y, &over,&range,&part, &index);

	DBG cerr <<"over pos:"<<over<<"  range: "<<range<<"  part:"<<part<<"  index:"<<index<<endl;

	int lx,ly;

	if (!buttondown.any()) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (index<=0) part=PART_None; else part=PART_Index;

			if (part==PART_Index && hover_index!=index) {
				hover_part=part;
				hover_index=index;
				needtodraw=1;
			}
		}
		if (hover_part!=part || hover_range!=range || hover_position!=over) {
			if (!buttondown.isdown(mouse->id,LEFTBUTTON)) hover_part=part;
			hover_range=range;
			hover_position=over;
			hover_index=index;

			if      (hover_part==PART_None)         PostMessage(NULL);
			else if (hover_part==PART_Index)        PostMessage(_("Click to divide range"));
			else if (hover_part==PART_Label)        PostMessage(_("Wheel to change label type"));
			else if (hover_part==PART_DocPageStart) PostMessage(_("Document page range"));
			else if (hover_part==PART_LabelStart)   PostMessage(_("Wheel to change first page of labels"));
			else if (hover_part==PART_Position)     PostMessage(_("Drag to adjust range"));

			needtodraw=1;
		}

		return 0;
	}

	//so to be here, there is a button down

	if (!hover_part) return 0;

	buttondown.move(mouse->id,x,y, &lx,&ly);
	DBG cerr <<"pr last m:"<<lx<<','<<ly<<endl;

	if (buttondown.isdown(mouse->id,LEFTBUTTON)) {
		if (hover_part==PART_Position) {
			if (index==hover_index) return 0;
			hover_index=index;
			hover_range=range;
			int i;
			if (x>lx) { //moved to the right
				temp_range_l->end++;
				if (temp_range_l->end>=doc->pages.n) temp_range_l->end=doc->pages.n-1;

				temp_range_r->first+=(temp_range_l->end+1)-temp_range_r->start;
				temp_range_r->start=temp_range_l->end+1;
			} else { //moved to the left
				i=temp_range_r->start;
				temp_range_r->start--;
				if (temp_range_r->start<0) temp_range_r->start=0;
				if (i!=temp_range_r->start) temp_range_r->first-=i-temp_range_r->start;

				temp_range_l->end=temp_range_r->start-1;
			}
			needtodraw=1;
			return 0;
		}
	}

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

//! Return the range that contains the page index i.
PageRange *PageRangeInterface::findRangeWith(int i)
{
	if (!doc->pageranges.n) {
		InstallDefaultRange();
		return doc->pageranges.e[0];
	}
	for (int c=0; c<doc->pageranges.n; c++) {
		if (i>=doc->pageranges.e[c]->start && i<=doc->pageranges.e[c]->end) return doc->pageranges.e[c];
	}
	return doc->pageranges.e[0];
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

	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		panelheight = 50;
		panelwidth  = dp->Maxx-dp->Minx-2*PAD;
		offset = -flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
		needtodraw = 1;
		return 0;
	}

	return 1;
}

int PageRangeInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}


} // namespace Laidout

