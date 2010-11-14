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
// Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/menubutton.h>
#include <lax/popupmenu.h>
#include <lax/mouseshapes.h>
#include <lax/lineinput.h>

#include <lax/lists.cc>

#include "language.h"
#include "spreadeditor.h"
#include "laidout.h"
#include "drawdata.h"
#include "document.h"
#include "headwindow.h"
#include "helpwindow.h"
#include "viewwindow.h"

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



//----------------------- SpreadInterface --------------------------------------

/*! \class SpreadInterface
 * \brief Interface for the spread editor.
 *
 * Fancy page order rearranger. Show all the available page layout spreads, and allows
 * rearranging the pages by dragging either pages or whole spreads. Also uses a kind of
 * page limbo, where temporarily unplaced pages hover. The spreads used come from
 * Imposition::LittleSpreads().
 *
 * Left button selecting and moving usually acts on individual pages, and the middle
 * button acts on whole spreads.
 *
 * \todo when there are threads, apply button should be grayed
 * \todo *** for doc pages not represented in active nets, there should be special warning indicator.
 * \todo Ultimately, this should be ported to Inkscape and Scribus (devs willing), ESPECially Inkscape, when the
 *   SVG spec allows multiple pages which makes it fair game for Inkscape, that is..
 */

/*! \var int SpreadInterface::curpage
 * \brief Index that can be used to pagelabel stack..
 */



//! Initialize everything and call GetSpreads().
/*! Incs count of docum.
 */
SpreadInterface::SpreadInterface(Laxkit::Displayer *ndp,Project *proj,Document *docum)
	: InterfaceWithDp(0,ndp),
	  curspreads(0)
{
	firsttime=1;
	project=proj;
	doc=docum;
	if (doc) doc->inc_count();

	reversebuttons=0;
	curspread=NULL;
	curpage=-1;

	maxmarkertype=8;
	dragpage=-1;
	drawthumbnails=1;

	view=NULL;
	GetSpreads();
}

/*! Dec count of doc. */
SpreadInterface::~SpreadInterface()
{
	 //for debugging:
	DBG cerr<<"SpreadInterface destructor"<<endl;

	if (view) view->dec_count();
	if (doc) doc->dec_count();
}

/*! Doesn't do anything..
 */
void SpreadInterface::Clear(LaxInterfaces::SomeData *)
{
}

/*! Something like:
 * <pre>
 *  document blah
 *  view
 *    spread
 *      index 1 #the document page index of a page in a spread
 *      matrix 1 0 0 1 0 0
 *    spread
 *      index 3
 *      matrix 1 0 0 1 0 0
 * </pre>
 *
 * If what==-1, dump out a pseudocode mockup of file format.
 * 
 * \todo **** when modified but not applied, will save index values
 *   from modified.. needs to be a check somewhere to ask whether to
 *   apply changes before saving..
 *   if (checkPendingChanges(thing)) thing->applyChanges();
 * \todo xbounds and ybounds!!
 */
void SpreadInterface::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sdocument blah      #the name of the document currently viewed\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0 #view area transform\n",spc);
		fprintf(f,"%sview viewname      #the name of the view belonging to the document\n",spc);
		fprintf(f,"\n%s  #If the view is a temporary view, then its format is as follows\n",spc);
		fprintf(f,"%sview\n",spc);
		if (view) {
			view->dump_out(f,indent+2,-1,NULL);
		} else {
			SpreadView v;
			v.dump_out(f,indent+2,-1,NULL);
		}
		
		return;
	}
	
	fprintf(f,"%sdocument %s\n",spc,doc->saveas);

	const double *m=dp->Getctm();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);

	transform_copy(view->matrix,dp->Getctm());
	if (view->doc_id && view->Name()) fprintf(f,"%sview %s\n",spc,view->Name());
	else {
		fprintf(f,"%sview\n",spc);
		view->dump_out(f,indent+2,what,context);
	}
}

/*! Note that 'index' is currently ignored.
 */
void SpreadInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	const char *viewname=NULL;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"document")) {
			Document *newdoc=laidout->findDocument(value);
			if (newdoc) {
				if (doc) doc->dec_count();
				doc=newdoc;
				if (doc) doc->inc_count();
			}

		} else if (!strcmp(name,"view")) {
			if (view) { view->dec_count(); view=NULL; }
			if (isblank(value)) {
				 //no view name given, so assume is temporary view
				view=new SpreadView(_("new view"));
				view->dump_in_atts(att->attributes.e[c], flag,context);
			} else viewname=value;

		} else if (!strcmp(name,"drawthumbnails")) {
			drawthumbnails=BooleanAttribute(value);

		} else if (!strcmp(name,"matrix")) {
			double matrix[6];
			int n=DoubleListAttribute(value,matrix,6);
			if (n!=6) transform_identity(matrix);
			dp->NewTransform(matrix);

		}
	}
	
	if (!view && viewname && doc) {
		for (int v=0; v<doc->spreadviews.n; v++) {
			if (doc->spreadviews.e[v]->Name() && !strcmp(doc->spreadviews.e[v]->Name(),viewname)) {
				view=doc->spreadviews.e[v];
				view->inc_count();
			}
		}
	}

	if (view) view->Update(doc);
	needtodraw=1;
}

/*! \todo *** this is a bit of a hack to force refiguring of page thumbnails
 *    need more thorough mechanism to keep track of last mod times..
 */
int SpreadInterface::InterfaceOn()
{
	if (doc) for (int c=0; c<doc->pages.n; c++) doc->pages.e[c]->modtime=times(NULL);
	return 0;
}

//! Use another of the current document's views.
/*! If i is not in doc->spreadviews, then nothing is done and 1 is returned. Else 0 is returned.
 */
int SpreadInterface::SwitchView(int i)
{
	if (i<0 || i>=doc->spreadviews.n) return 1;

	if (view==doc->spreadviews.e[i]) return 0;

	view->dec_count();
	view=doc->spreadviews.e[i];
	view->inc_count();
	view->Update(doc);

	SpreadEditor *se=dynamic_cast<SpreadEditor*>(viewport->win_parent);
	if (se) {
		LineEdit *edit=(LineEdit*)se->findChildWindowByName("name");
		edit->SetText(view->Name());
	}
	needtodraw=1;
	return 0;
}

/*! Return 0 for success, nonzero error.
 */
int SpreadInterface::UseThisDoc(Document *ndoc)
{
	if (ndoc==doc) return 0;

	if (doc) doc->dec_count();
	doc=ndoc;
	if (doc) doc->inc_count();
	if (view) { view->dec_count(); view=NULL; }

	GetSpreads();
	transform_copy(view->matrix,dp->Getctm());//matrix not normally updated otherwise
	ArrangeSpreads(ArrangeGrid); //do auto arrange at start
	Center();
	needtodraw=1;
	return 0;
}

//! Check to make sure spreads containing pages in range [startpage,endpage] are correct.
/*! This currently recomputes all spreads.
 * 
 * If endpage==-1, then doc->pages.n-1 is assumed.
 *
 * \todo ignores startpage and endpage, always checks whole layout for validation
 */
void SpreadInterface::CheckSpreads(int startpage,int endpage)
{
	//if (view) needtodraw=view->Update(doc,startpage,endpage);
	if (view) needtodraw=view->Update(doc);

	//init doc page markers... maybe this should be done elsewhere?
	if (!doc) return;
	for (int c=0; c<doc->pages.n; c++)
		if (doc->pages.e[c]->labeltype==-1) doc->pages.e[c]->labeltype=c%maxmarkertype;
}

//! Create all the LittleSpread objects and default page labels.
/*! This is typically geared only for page spreads, not paper spreads.
 *
 * It is assumed that the developers have adhered to the guideline that
 * Imposition::GetLittleSpread() will return spreads such that the page counts
 * that map to them make sense with next and previous little spreads.
 */
void SpreadInterface::GetSpreads()
{
	curspread=NULL;
	curpage=-1;
	curpages.flush();
	curspreads.flush();

	if (!doc || !doc->imposition || !doc->pages.n) {
		if (view) { view->dec_count(); view=NULL; }
		needtodraw=1;
		return;
	}
	
	if (!view && doc->spreadviews.n) {
		view=doc->spreadviews.e[0];
		view->inc_count();
	} else if (!view) view=new SpreadView(_("new view"));

	view->Update(doc);
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
	if (!view || firsttime) return; // need real window info before arranging...
	
	view->ArrangeSpreads(dp,how);

	if (dp) {
		view->FindBBox();
		double X=view->minx,
			   Y=view->miny,
			   W=view->maxx-view->minx,
			   H=view->maxy-view->miny;
		dp->SetSpace(X-W,X+W+W,Y-H,Y+H+H);
		dp->Center(X,X+W,Y,Y+H);
	}
}

//! Draw the little spreads, connection lines, page labels, whatever else...
int SpreadInterface::Refresh()
{
	if (!needtodraw) return 1;
	
	if (firsttime) {
		firsttime=0;
		ArrangeSpreads();
		if (view) view->FindBBox();
		Center(1);
		//((ViewportWindow *)curwindow)->syncrulers();
	}

	needtodraw=0;

	dp->Updates(0);	
	
	dp->NewBG(200,200,200);
	dp->ClearWindow();
	if (!view) { dp->Updates(1); return 1; }

	int c,c2;
	
	 // Draw the connecting lines
	 // draw the connection lines in front of the spreads..
	 //*** this needs little bubble at back of arrows, and little arrow at front
	for (c=0; c<view->spreads.n; c++) {
		if (view->spreads.e[c]->prev) {
			::DrawData(dp,view->spreads.e[c]->connection,NULL,NULL);
		}
	}
	 

	 // Draw the spreads
	SomeData *outline;
	int pg,x,y;
	flatpoint p;
	
	FillStyle fs(0xffff,0xffff,0xffff,0xffff, WindingRule,FillSolid,GXcopy);
	FillStyle efs(0xdddd,0xdddd,0xdddd,0xffff, WindingRule,FillSolid,GXcopy);
	LineStyle ls(0,0,0xffff,0xffff, 4,CapButt,JoinMiter,~0,GXcopy);
	//Page *page;
	ImageData *thumb=NULL;
	dp->ClearClip();

	for (c=0; c<view->spreads.n; c++) {
		dp->PushAndNewTransform(view->spreads.e[c]->m());
		
		 //draw the spread's path
		::DrawData(dp,view->spreads.e[c]->spread->path,NULL,&fs);

		for (c2=0; c2<view->spreads.e[c]->spread->pagestack.n; c2++) {
			 // draw thumbnails
			pg=view->spreads.e[c]->spread->pagestack.e[c2]->index;
			thumb=NULL;

			if (pg<0) {
				 //no page index means this place is an empty page, for a page in a thread
				::DrawData(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,NULL,&efs);

			} else if (drawthumbnails) {
				if (pg>=0 && pg<doc->pages.n) {
					if (pg>=0 && pg<doc->pages.n) thumb=doc->pages.e[pg]->Thumbnail();
				}
				if (thumb) {
					DBG cerr <<"drawing thumbnail for "<<pg<<endl;
					dp->PushAndNewTransform(view->spreads.e[c]->spread->pagestack.e[c2]->outline->m());
					
					 // always setup clipping region to be the page
					dp->PushClip(1);
					SetClipFromPaths(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,dp->m());
					
					::DrawData(dp,thumb,NULL,NULL);
					
					 //remove clipping region
					dp->PopClip();

					dp->PopAxes();
					thumb=NULL;
				}
			}
		}	
		
		 // draw path again over whatever was drawn, but don't fill...
		::DrawData(dp,view->spreads.e[c]->spread->path,NULL,NULL);

		 // draw currently selected pages with thick line
		for (int cc=0; cc<curpages.n; cc++) {
			c2=view->spreads.e[c]->spread->PagestackIndex(curpages.e[cc]);
			if (c2>=0) {
				outline=view->spreads.e[c]->spread->pagestack.e[c2]->outline;

				unsigned long oldfg=dp->FG();
				dp->NewFG(0,0,255);
				dp->LineAttributes(3,LineSolid,CapButt,JoinMiter);
				::DrawData(dp,outline, &ls,NULL);
				dp->NewFG(oldfg);
			}
		}

		 // draw page labels
		for (c2=0; c2<view->spreads.e[c]->spread->pagestack.n; c2++) {
			pg=view->spreads.e[c]->spread->pagestack.e[c2]->index;
			if (pg>=0 && pg<doc->pages.n) {
				outline=view->spreads.e[c]->spread->pagestack.e[c2]->outline;
				dp->PushAndNewTransform(outline->m());

				int centerlabels=view->centerlabels;
				DBG cerr <<"centerlabels:"<<centerlabels<<endl;

				if (centerlabels==LAX_CENTER)
					p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->miny+outline->maxy)/2)); //center
				else if (centerlabels==LAX_BOTTOM)
					p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->miny)));//bottom
				else if (centerlabels==LAX_LEFT)
					p=dp->realtoscreen(flatpoint((outline->minx),(outline->miny+outline->maxy)/2));//left
				else if (centerlabels==LAX_TOP)
					p=dp->realtoscreen(flatpoint((outline->minx+outline->maxx)/2,(outline->maxy)));//top
				else //LAX_RIGHT
					p=dp->realtoscreen(flatpoint((outline->maxx),(outline->miny+outline->maxy)/2));//right
				x=(int)p.x; // figure out where bottom tip of bbox is
				y=(int)p.y;
				drawLabel(x,y,doc->pages.e[pg], pg==view->temppagemap[pg]);

				dp->PopAxes();
			}
		}

		dp->PopAxes(); //spread axes
	}

	 //draw selection rectangle, if any
	if (buttondown.isdown(0,LEFTBUTTON) && dragpage<0) {
		dp->NewFG(0,0,255);
		dp->DrawScreen();
		dp->drawline(  lbdown.x,   lbdown.y,   lbdown.x, lastmove.y);
		dp->drawline(  lbdown.x, lastmove.y, lastmove.x, lastmove.y);
		dp->drawline(lastmove.x, lastmove.y, lastmove.x,   lbdown.y);
		dp->drawline(lastmove.x,   lbdown.y,   lbdown.x,   lbdown.y);
		dp->DrawReal();
	}
	
	dp->Updates(1);	
	return 1;
}

#define SE_First            1000
#define SE_PageLabels       1000
#define SE_InsertPage       1001
#define SE_InsertDummyPage  1002
#define SE_DetachPages      1003
#define SE_DeletePages      1004
#define SE_ExportPages      1005
#define SE_NewView          1006
#define SE_SaveView         1007
#define SE_DeleteView       1008
#define SE_RenameView       1009

Laxkit::MenuInfo *SpreadInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Spread Editor"));

//	//menu->AddItem(_("Page Labels..."),SE_PageLabels);
//	menu->AddItem(_("Insert Page"),SE_InsertPage);
//	menu->AddItem(_("Insert Dummy Page"),SE_InsertDummyPage);
//
//	if (curpages.n) {
//		menu->AddSep(_("Current pages"));
//		menu->AddItem(_("Detach"),SE_DetachPages);//create a limbo page string
//		menu->AddItem(curpages.n>1?_("Delete Pages..."):_("Delete Page"),SE_DeletePages);
//		menu->AddItem(_("Export to new document..."),SE_ExportPages);
//	}
//
//	menu->AddSep();
	menu->AddItem(_("New view"),SE_NewView);
	if (!view->doc_id) menu->AddItem(_("Save view"),SE_SaveView);
	else {
		menu->AddItem(_("Delete current view"),SE_DeleteView);
		//menu->AddItem(_("Rename current view"),SE_RenameView);
	}

	 //new ones not explicitly added to document will be saved only if there are active windows using it
	//if (doc && doc->spreadviews.n>1) menu->AddItem(_("Delete current arrangement"), 202);

	 //add spread arrangement list
	if (doc->spreadviews.n) {
		menu->AddSep("Views");
		for (int c=0; c<doc->spreadviews.n; c++) {
			menu->AddItem(doc->spreadviews.e[c]->Name(), c, 
						  LAX_ISTOGGLE | (doc->spreadviews.e[c]==view?LAX_CHECKED:0) );
		}
	}

	return menu;
}

//! Respond to context menu event.
int SpreadInterface::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"viewportmenu")) return 1;

	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
	if (!s) return 1;

	int i=s->info2;
	
	if (i>=0 && i<doc->spreadviews.n) {
		 //select new view
		SwitchView(i);
		return 0;

	} else if (i==SE_PageLabels) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_InsertPage) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_InsertDummyPage) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_DetachPages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_DeletePages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_ExportPages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SE_NewView) {
		view->dec_count();
		view=new SpreadView("new view");
		view->doc_id=doc->object_id;
		doc->spreadviews.push(view);
		GetSpreads();
		ArrangeSpreads();

		SpreadEditor *se=dynamic_cast<SpreadEditor*>(viewport->win_parent);
		if (se) {
			LineEdit *edit=(LineEdit*)se->findChildWindowByName("name");
			edit->SetText(view->Name());
		}
		needtodraw=1;
		return 0;

	} else if (i==SE_SaveView) {
		if (view->doc_id!=0) return 0;
		doc->spreadviews.push(view);
		view->doc_id=doc->object_id;
		return 0;

	} else if (i==SE_DeleteView) {
		int ii=doc->spreadviews.findindex(view);
		if (ii>=0) {
			doc->spreadviews.remove(ii);
		}
		view->dec_count();
		view=NULL;
		GetSpreads();
		ArrangeSpreads();
		needtodraw=1;
		return 0;

	} else if (i==SE_RenameView) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;
	}

	return 0;
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
void SpreadInterface::drawLabel(int x,int y,Page *page, int outlinestatus)
{
	 // *** if (plabel->labeltype==circle, filledcircle, etc...) ...
	 // *** write text
	int w,h;
	getextent(page->label,-1,&w,&h);
	w/=2;
	h/=2;
	w+=h;
	h+=h;
	unsigned long fcolor=~0;//fill color
	unsigned long color=0; //text color
	unsigned long outlinecolor=0;
	if (outlinestatus==0) outlinecolor=rgbcolor(255,0,0);

	int t=THING_None;
	switch (page->labeltype) {
		case 0: fcolor=rgbcolor(255,255,255);
				t=THING_Circle;
				break;
		case 1: fcolor=rgbcolor(175,175,175);
				t=THING_Circle;
				break;
		case 2: fcolor=rgbcolor(100,100,100);
				color=~0;
				t=THING_Circle;
				break;
		case 3: fcolor=rgbcolor(0,0,0);
				color=~0;
				t=THING_Circle;
				break;
		case 4: fcolor=rgbcolor(255,255,255);
				t=THING_Square;
				break;
		case 5: fcolor=rgbcolor(175,175,175);
				t=THING_Square;
				break;
		case 6: fcolor=rgbcolor(100,100,100);
				color=~0;
				t=THING_Square;
				break;
		case 7: fcolor=rgbcolor(0,0,0);
				color=~0;
				t=THING_Square;
				break;
		default: fcolor=rgbcolor(255,255,255); //default provided just in case
				t=THING_Circle;
				break;
	}
	
	draw_thing(dp->GetXw(),x,y,w,h,t, outlinecolor,fcolor, (outlinestatus==0?4:1));
	foreground_color(color);
	textout(dp->GetXw(),page->label,-1,x,y,LAX_CENTER);
}

//! Relays left button event to rLBDown or rMBDown depending on reversebuttons.
int SpreadInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ if (reversebuttons) return rMBDown(x,y,state,count,d); else return rLBDown(x,y,state,count,d); }

//! Relays left button event to rLBUp or rMBUp depending on reversebuttons.
int SpreadInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ if (reversebuttons) return rMBUp(x,y,state,d); else return rLBUp(x,y,state,d); }

//! Relays middle button event to rLBDown or rMBDown depending on reversebuttons.
int SpreadInterface::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ if (reversebuttons) return rLBDown(x,y,state,count,d); else return rMBDown(x,y,state,count,d); }

//! Relays middle button event to rLBUp or rMBDown depending on reversebuttons.
int SpreadInterface::MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ if (reversebuttons) return rLBUp(x,y,state,d); else return rMBUp(x,y,state,d); }

//! Selects pages.
/*! If down on nothing, then start a selection rectangle. 
 *
 * Otherwise figure
 * out which page button is down on. If shift, add to selection, if control, toggle selected
 * state of it.
 */
int SpreadInterface::rLBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any()) return 1;
	buttondown.down(d->id,LEFTBUTTON, x,y,state);

	int psi=-1,page=-1,thread=-1;
	LittleSpread *spread=findSpread(x,y,&psi,&thread);
	if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }

	DBG cerr <<"SpreadInterface lbdown found page "<<page<<" in thread "<<thread<<endl;
	if (page<0) {
		// start a selection rectangle. note: could be click down inside an empty page
		lbdown=lastmove=flatpoint(x,y);
		dragpage=-1;
		return 0;
	}
	
	needtodraw=1;
	dragpage=page;
	int i=curpages.pushnodup(page);
	curspreads.pushnodup(spread,0);
	if (i<0 && !(state&ShiftMask)) { //item wasn't already selected
		curspreads.flush();
		curpages.flush();
		curpages.push(page);
		curspreads.push(spread,0);
		curpage=page;
	}

	int shape=0;
	if ((state&LAX_STATE_MASK)==0 || curpages.n>1) shape=LAX_MOUSE_To_E;
	else shape=LAX_MOUSE_Exchange;
	const_cast<LaxMouse*>(d)->setMouseShape(curwindow,shape);

	return 0;
}

void SpreadInterface::clearSelection()
{
	curpage=-1;
	curspread=NULL;
	curpages.flush();
	curspreads.flush();
	dragpage=-1;
	needtodraw=1;
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
int SpreadInterface::rLBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	const_cast<LaxMouse*>(d)->setMouseShape(curwindow,0);
	buttondown.up(d->id,LEFTBUTTON);

	if (dragpage<0) {
		 //dragged out a selection, so select everything inside the box
		if ((state&LAX_STATE_MASK)==0) clearSelection();
		cout <<" **** spread editor must implement select all in box!!!"<<endl;
		needtodraw=1;
		return 0;
	}
	
	int psi,thread,page;
	LittleSpread *spread=findSpread(x,y,&psi,&thread);
	if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }
	DBG cerr <<"SpreadInterface lbup found page "<<psi<<" in thread "<<thread<<endl;

	 //If mouse up outside window, maybe call up that page in a view window
	if (x<0 || x>curwindow->win_w || y<0 || y>curwindow->win_h) {
		 // mouse up outside window so search for a ViewWindow to shift view for
		anXWindow *win=NULL;
		int mx,my;
		mouseposition(d->id,NULL,&mx,&my,NULL,&win);
		if (win && !strcmp(win->whattype(),"LaidoutViewport")) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport *>(win);
			vp->UseThisDoc(doc);
			vp->SelectPage(dragpage);
			
			DBG cerr <<" ~~~~~~~~~~~~drop page "<<dragpage<<" to win type: "<<win->whattype()<<endl;
		}
		dragpage=-1;
		needtodraw=1;
		return 0;
	}
	//***finish imp here!!!
	
	 // swap dragpage and page
	DBG cerr <<"*** drag "<<dragpage<<" to "<<(spread?"swap":"make thread")<<endl;

	if (!spread) {
		//*** transfer all the curpages to a thread
	} else {
		if ((state&LAX_STATE_MASK)==0 || curpages.n>1)
			//SlidePages(dragpage,page,0);
			SlidePages(view->reversemap(dragpage),view->reversemap(page),0);
		else if ((state&LAX_STATE_MASK)==ShiftMask)
			//SwapPages(dragpage,page);
			SwapPages(view->reversemap(dragpage),view->reversemap(page));
	}

	//if (dragged) clearSelection();
	dragpage=-1;
	
	return 0;
}

/*! *** this function is temporary! the full SpreadInterface will be much more robust.
 * this is provided here just to get things off the ground....
 *
 * Move page at previouspos to newpos (indices for the given thread),
 * sliding the intervening pages toward the newly open slot..
 *
 * \todo *** later, this will optionally push the replaced page into limbo
 */
void SpreadInterface::SlidePages(int previouspos, int newpos, int thread)
{
	if (!view) return;

	if (previouspos==newpos) return;
	if (previouspos<newpos) { 
		 // 01234 -> 12304 (move 0 to 3 position)
		for (int c=previouspos; c<newpos; c++) SwapPages(c,c+1);
	} else {
		 // 01234 -> 30124 (move 3 to 0 position)
		for (int c=previouspos; c>newpos; c--) SwapPages(c,c-1);
	}
}

//! Swap the pages corresponding to previouspos and newpos.
/*! previouspos and newpos are both indices corresponding to document
 * pages via (doc page index)=view->map(previouspos) and view->map(newpos).
 * temp:*** they are indices into current thread...
 *
 * Changes are temporary here. Permanent
 * changes to the document are made in ApplyChanges()
 */
void SpreadInterface::SwapPages(int previouspos, int newpos)
{
	if (!view || previouspos<0 || previouspos>=doc->pages.n ||
	    newpos<0 || newpos>=doc->pages.n) return;

	view->SwapPages(previouspos,newpos);
	needtodraw=1;
}

//! Apply however pages have been rearranged to the document.
/*! If there are any threads, then changes will not be applied.
 */
void SpreadInterface::ApplyChanges()
{
	DBG cerr<<"ApplyChanges:"<<endl;

	if (view->threads.n) {
		if (viewport) viewport->postmessage(_("Cannot apply when there are unconnected threads"));
		return;
	}

	view->ApplyChanges();
	laidout->notifyDocTreeChanged(curwindow->win_parent,TreePagesMoved,0,-1);
	needtodraw=1;
}

//! Reset whatever tentative changes to page order have been made since last apply.
/*! temppagemap elements say what page index is temporarily in spot index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 * So, reseting is merely a matter of calling SwapPages() until temppagemap[index]==index.
 */
void SpreadInterface::Reset()
{
	if (!view) return;
	view->Reset();
}

//! Selects spreads.
int SpreadInterface::rMBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (!view || buttondown.any()) return 1;

	int page=-1,psi,thread;
	LittleSpread *spread=findSpread(x,y,&psi,&thread);
	if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }
	if (!spread) {
		clearSelection();
		return 1;
	}

	buttondown.down(d->id,MIDDLEBUTTON, x,y,state);
	mx=x; my=y;

	if ((state&LAX_STATE_MASK)==0) {
		clearSelection();//clear if not shift or control click
		curpages.pushnodup(page);
		curspreads.pushnodup(spread,0);

	} else if ((state&LAX_STATE_MASK)==ShiftMask) {
		 //push all pages between curpage and new curpage
		for (int c=(curpage<page?curpage:page); c<=(curpage>page?curpage:page); c++) {
			curpages.pushnodup(c);
			curspreads.pushnodup(view->SpreadOfPage(c,NULL),0);
		}
	}
	curpage=page;
	curspread=spread;

	needtodraw=1;
	return 0;
}

//! Drops spreads.
int SpreadInterface::rMBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,MIDDLEBUTTON)) return 1;

	buttondown.up(d->id,MIDDLEBUTTON);
	return 0;
}

//! Drag spreads or pages
int SpreadInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) return 1;
	if (!view) return 1;

	buttondown.move(d->id,x,y);

	if (buttondown.isdown(d->id,MIDDLEBUTTON) && curspreads.n) {
		 //move around spreads
		LittleSpread *s;
		for (int c=0; c<curspreads.n; c++) {
			s=curspreads.e[c];
			s->origin(s->origin()+screentoreal(x,y)-screentoreal(mx,my));
			s->mapConnection();
			if (s->next) s->next->mapConnection();
		}
		view->arrangetype=view->arrangestate=ArrangeCustom;
		view->FindBBox();
		mx=x; my=y;
		needtodraw=1;

	} else if (buttondown.isdown(d->id,LEFTBUTTON)) {
		if (dragpage<0) {
			 //draw select rectangle
			 //*** need to temp select pages which box touches them...
			needtodraw=1;
			lastmove.x=x;
			lastmove.y=y;
			mx=x; my=y;
		} else {
			//*** if moving back to original area, make cursor be a "cancel" cursor
			//*** for dropping pages to limbo, use XC_sizing

			int page=-1,psi,thread;
			LittleSpread *spread=findSpread(x,y,&psi,&thread);
			if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }
			if (curpages.findindex(page)>=0) {
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Cancel);
			} else if (!spread) {
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Boxes);
			} else {
				if ((state&LAX_STATE_MASK)==0) 
					const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_To_E);
				else 
					const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Exchange);
			}
		}

	} else return 1;

	return 0;
}

//! Find the spread under screen point (x,y), return it, or NULL.
/*! Also find the index of the page clicked down on, and
 * the thread number, where 0 means the main thread, and any other positive number
 * is the index of the thread in threads stack plus 1.
 *
 * Please note that pagestacki and thread are assumed to not be null, as that is where
 * the pagestack index and thread numbers get returned.
 */
LittleSpread *SpreadInterface::findSpread(int x,int y, int *pagestacki, int *thread)
{
	if (!view) return NULL;

	flatpoint p=screentoreal(x,y);
	return view->findSpread(p.x,p.y,pagestacki,thread);
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

//! Center all (w==1)...
void SpreadInterface::Center(int w)
{
	if (!view) { dp->CenterReal(); return; }

	double X=view->minx,
		   Y=view->miny,
		   W=view->maxx-view->minx,
		   H=view->maxy-view->miny;
	dp->SetSpace(X-W,X+W+W,Y-H,Y+H+H);
	dp->Center(X-W*.05,X+W*1.05,Y-H*.05,Y+H*1.05);

	needtodraw=1;
}

//! Change the type of mark for all in curpages.
/*! If newmark==-1, then make all the marks one more than the 1st page in curpages.
 *  If newmark==-2, then make all the marks one less than the 1st page in curpages.
 *  If newmark>=0, then make all the marks that mark.
 *
 * Returns the form of the new mark, or -1 for no marks changed.
 */
int SpreadInterface::ChangeMarks(int newmark)
{
	if (!view || !curpages.n) return -1;

	if (newmark==-1) newmark=(doc->pages.e[view->map(curpages.e[0])]->labeltype+1) % maxmarkertype;
	else if (newmark==-2) newmark=(doc->pages.e[view->map(curpages.e[0])]->labeltype+maxmarkertype-1) % maxmarkertype;

	for (int c=0; c<curpages.n; c++) doc->pages.e[view->map(curpages.e[c])]->labeltype=newmark;

	return newmark;
}

/*! Switch to slide cursor when shift up.
 */
int SpreadInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Shift && dragpage>=0) {
		 //update mouse shape
		int mid=buttondown.whichdown(0);
		if (!mid) return 1;
		if (!buttondown.isdown(mid,LEFTBUTTON)) return 1;

		LaxMouse *mouse=app->devicemanager->findMouse(mid);
		if (!mouse) return 1;
		int x,y;
		buttondown.getinfo(mid,LEFTBUTTON, NULL,NULL, NULL,NULL, &x,&y);

		int page=-1,psi,thread;
		LittleSpread *spread=findSpread(x,y,&psi,&thread);
		if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }
		if (curpages.findindex(page)>=0) {
			const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Cancel);
		} else if (!spread) {
			const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Boxes);
		} else {
			//if ((state&LAX_STATE_MASK)==0) 
				const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_To_E);
			//else 
				//const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Exchange);
		}
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
int SpreadInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Shift && dragpage>=0 && curpages.n==1) {
		const_cast<LaxMouse*>(d->paired_mouse)->setMouseShape(curwindow,LAX_MOUSE_Exchange);
		return 0;

	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		Center(1);
		needtodraw=1;
		return 0;

	} else if (ch=='m' && (state&LAX_STATE_MASK)==0) { // toggle mark
		if (ChangeMarks(-1)>=0) needtodraw=1;
		return 0;

	} else if (ch=='M' && (state&LAX_STATE_MASK)==ShiftMask) { // toggle mark
		if (ChangeMarks(-2)>=0) needtodraw=1;
		return 0;

	} else if (ch=='p') { //*** for debugging thumbnails....
		DBG if (curpage<0) return 0;
		DBG ImageData *thumb=doc->pages.e[curpage]->Thumbnail();
		DBG cerr <<"'P' image dump:"<<endl;
		DBG thumb->dump_out(stderr,2,0,NULL);
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
		DBG cerr <<"-- drawthumbnails: "<<drawthumbnails<<endl;
		needtodraw=1;
		return 0;

	} else if (ch=='c' && (state&LAX_STATE_MASK)==0) {
		if (view->centerlabels==LAX_CENTER) view->centerlabels=LAX_TOP;
		else if (view->centerlabels==LAX_TOP) view->centerlabels=LAX_RIGHT;
		else if (view->centerlabels==LAX_RIGHT) view->centerlabels=LAX_BOTTOM;
		else if (view->centerlabels==LAX_BOTTOM) view->centerlabels=LAX_LEFT;
		else view->centerlabels=LAX_CENTER;
		needtodraw=1;
		return 0;

	} else if (ch=='A' && (state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
		view->arrangestate=ArrangeNeedsArranging;
		ArrangeSpreads();
		Center(1);
		needtodraw=1;
		return 0;

	} else if (ch=='A' && (state&LAX_STATE_MASK)==ShiftMask) {
		view->arrangetype++;
		if (view->arrangetype==ArrangetypeMax+1) view->arrangetype=ArrangetypeMin;

		if (viewport) viewport->postmessage(arrangetypestring(view->arrangetype));
		else app->postmessage(arrangetypestring(view->arrangetype));

		ArrangeSpreads();
		Center(1);
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
/*! Inc count of ndoc.
 */
SpreadEditor::SpreadEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx, int yy, int ww, int hh, int brder,
						//Window owner,const char *mes, 
						Project *nproj,Document *ndoc)
	: ViewerWindow(parnt,nname,ntitle,nstyle|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER, 
					xx,yy,ww,hh, brder, NULL)
{
	project=nproj;
	doc=ndoc;
	if (doc) doc->inc_count();

	if (!project) project=laidout->project;
	if (project) {
		if (doc) { // make sure doc is in proj?
			int c;
			for (c=0; c<project->docs.n; c++) if (doc==project->docs.e[c]->doc) break;
			if (c==project->docs.n) { doc->dec_count(); doc=NULL; }
		} 
		if (!doc) { // doc=first proj doc
			if (project->docs.n) { doc=project->docs.e[0]->doc; doc->inc_count(); }
		}
	} 
	if (!viewport) viewport=new ViewportWindow(this,"spread-editor-viewport","spread-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);
	win_colors->bg=rgbcolor(200,200,200);
	viewport->dp->NewBG(255,255,255);

	needtodraw=1;
	AddTool(new SpreadInterface(viewport->dp,project,doc),1,1); // local, and select it
}

SpreadEditor::~SpreadEditor()
{
	if (doc) doc->dec_count();
}

//! Passes off to SpreadInterface::dump_out().
void SpreadEditor::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	((SpreadInterface *)curtool)->dump_out(f,indent,what,context);
}

//! Passes off to SpreadInterface::dump_in_atts().
void SpreadEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	((SpreadInterface *)curtool)->dump_in_atts(att,flag,context);
}

/*! Return 0 for success, nonzero error.
 */
int SpreadEditor::UseThisDoc(Document *ndoc)
{
	if (doc) doc->dec_count();
	doc=ndoc;
	if (doc) doc->inc_count();

	SpreadInterface *s=((SpreadInterface *)curtool);
	s->UseThisDoc(doc);
	return 0;
}

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int SpreadEditor::init()
{
	//AddWin(***)...
	ViewerWindow::init();

	 // *** remove the rulers.... not actually deleting?
	wholelist.remove(0); //first null
	wholelist.remove(0); //x
	wholelist.remove(0); //y
	viewport->UseTheseRulers(NULL,NULL);


	anXWindow *last=NULL;
	Button *tbut;

	AddNull(); // makes the status bar fill whole line

	MenuButton *menub;
	 //add a menu button thingy in corner between rulers
	//**** menu would hold a list of the available documents, plus other control stuff, dialogs, etc..
	//**** mostly same as would be in right-click in viewport.....	
	rulercornerbutton=menub=new MenuButton(this,"rulercornerbutton",NULL,
										 MENUBUTTON_CLICK_CALLS_OWNER,
										 //MENUBUTTON_DOWNARROW|MENUBUTTON_CLICK_CALLS_OWNER,
										 0,0,0,0, 0,
										 last,object_id,"rulercornerbutton",0,
										 NULL,0, //menu, local
										 "v",
										 NULL,laidout->icons.GetIcon("Laidout")
										);
	menub->tooltip(_("Document list"));
	AddWin(menub,1, menub->win_w,0,50,50,0, menub->win_h,0,50,50,0, -1);
	//AddWin(menub,menub->win_w,0,50,50, menub->win_h,0,50,50, -1);//add before status bar

	//wholelist.e[wholelist.n-1]->pw(100);
	//AddNull(); // makes the status bar fill whole line

	SpreadInterface *interf=(SpreadInterface*)tools.e[0];
	LineEdit *linp=new LineEdit(this,"name",NULL, 0, 0,0,0,0,1, NULL,object_id,"newname",
								  interf->view->viewname,0);
	linp->setWinStyle(LINEEDIT_SEND_ANY_CHANGE,1);
	linp->tooltip(_("Name of the current view"));
	AddWin(linp,1, linp->win_w,0,2000,50,0, linp->win_h,0,50,50,0, -1);


	last=tbut=new Button(this,"applybutton",NULL, 0, 0,0,0,0,1, NULL,object_id,"applybutton",0,_("Apply"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"resetbutton",NULL, 0, 0,0,0,0,1, last,object_id,"resetbutton",0,_("Reset"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"updatethumbs",NULL, 0, 0,0,0,0,1, last,object_id,"updatethumbs",0,_("Update Thumbs"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	Sync(1);	
	return 0;
}

/*! Responds to:
 *
 * "resetbutton",
 * "applybutton",
 * "updatethumbs",
 * "docTreeChange".
 */
int SpreadEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"SpreadEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);
		if (!te || te->changer==this) return 1;

		if (te->changetype==TreeObjectRepositioned ||
				te->changetype==TreeObjectReorder ||
				te->changetype==TreeObjectDiffPage ||
				te->changetype==TreeObjectDeleted ||
				te->changetype==TreeObjectAdded) {
			DBG cerr <<"*** need to make a SpreadEditor:: flag for need to update thumbs"<<endl;
		} else if (te->changetype==TreePagesAdded ||
				te->changetype==TreePagesDeleted ||
				te->changetype==TreePagesMoved) {
			((SpreadInterface *)curtool)->CheckSpreads(te->start,te->end);
		} else if (te->changetype==TreeDocGone) {
			cout <<" ***need to imp SpreadEditor::DataEvent -> TreeDocGone"<<endl;
		}
		
		return 0;

	} else if (!strcmp("newname",mes)) {
		SpreadInterface *interf=(SpreadInterface*)tools.e[0];
		if (!interf->view->doc_id) {
			interf->view->doc_id=doc->object_id;
			doc->spreadviews.push(interf->view);
		}
		LineEdit *edit=(LineEdit*)findChildWindowByName("name");
		makestr(interf->view->viewname,edit->GetCText());
		return 0;

	} else if (!strcmp("resetbutton",mes)) {
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

	} else if (!strcmp(mes,"rulercornerbutton")) {
		 //pop up a list of available documents 

		if (!laidout->project->docs.n) return 0;
		MenuInfo *menu;
		menu=new MenuInfo("Viewer");

		 //---add document list, numbers start at 0
		int c,pos;

		menu->AddSep("Documents");
		for (c=0; c<laidout->project->docs.n; c++) {
			pos=menu->AddItem(laidout->project->docs.e[c]->doc->Name(1), 500+c)-1;
			menu->menuitems.e[pos]->state|=LAX_ISTOGGLE;
			if (laidout->project->docs.e[c]->doc==doc) {
				menu->menuitems.e[pos]->state|=LAX_CHECKED;
			}
		}

//		menu->AddSep();
//		menu->AddItem(_("New view"),SE_NewView);
		SpreadInterface *interf=(SpreadInterface*)tools.e[0];
//		if (!interf->view->doc_id) menu->AddItem(_("Save view"),SE_SaveView);
//		else {
//			menu->AddItem(_("Delete current view"),SE_DeleteView);
//		}

		 //new ones not explicitly added to document will be saved only if there are active windows using it
		//if (doc && doc->spreadviews.n>1) menu->AddItem(_("Delete current arrangement"), 202);

		 //add spread arrangement list
		if (doc->spreadviews.n) {
			menu->AddSep("Views");
			for (int c=0; c<doc->spreadviews.n; c++) {
				menu->AddItem(doc->spreadviews.e[c]->Name(), c, 
							  LAX_OFF | LAX_ISTOGGLE | (doc->spreadviews.e[c]==interf->view?LAX_CHECKED:0) );
			}
		}

		 //create the actual popup menu...
		PopupMenu *popup;
		popup=new PopupMenu(NULL,_("Documents"), MENUSEL_CHECK_ON_LEFT|MENUSEL_LEFT,
						0,0,0,0, 1, 
						object_id,"rulercornermenu", 
						0,
						menu,1);
//		popup=new PopupMenu(_("Documents"), MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT,
//						0,0,0,0, 1, 
//						viewport->object_id,"rulercornermenu", 
//						menu,1);
		popup->pad=5;
		popup->Select(0);
		popup->WrapToMouse(None);
		app->rundialog(popup);
		return 0;

	} else if (!strcmp(mes,"rulercornermenu")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);

		SpreadInterface *interf=(SpreadInterface*)tools.e[0];
		int i=s->info2;
		DBG cerr <<"rulercornermenu:"<<i<<endl;

		 //0-999 was document things
		if (i-500>=0 && i-500<laidout->project->docs.n) {
			UseThisDoc(laidout->project->docs.e[i]->doc);
			return 0;

		} else if (i>=0 && i<doc->spreadviews.n) {
			interf->SwitchView(i);
		}
	}
	return 1;
}

int SpreadEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
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
	if (((SpreadInterface *)curtool)->view->arrangetype==ArrangeAutoTillMod ||
			((SpreadInterface *)curtool)->view->arrangetype==ArrangeAutoAlways) 
		((SpreadInterface *)curtool)->ArrangeSpreads();
	return c;
}

//! Trigger an ArrangeSpreads if arrangetype is auto.
int SpreadEditor::Resize(int nw,int nh)
{
	int c=ViewerWindow::Resize(nw,nh);
	if (((SpreadInterface *)curtool)->view->arrangetype==ArrangeAutoTillMod ||
			((SpreadInterface *)curtool)->view->arrangetype==ArrangeAutoAlways) 
		((SpreadInterface *)curtool)->ArrangeSpreads();
	return c;
}

