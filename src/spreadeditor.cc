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
// Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/menubutton.h>
#include <lax/popupmenu.h>
#include <lax/mouseshapes.h>
#include <lax/lineinput.h>
#include <lax/shortcutwindow.h>

#include <lax/lists.cc>

#include "language.h"
#include "spreadeditor.h"
#include "laidout.h"
#include "drawdata.h"
#include "document.h"
#include "headwindow.h"
#include "helpwindow.h"
#include "viewwindow.h"
#include "interfaces/pagerangeinterface.h"
#include "interfaces/pagemarkerinterface.h"
#include "interfaces/paperinterface.h"

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



namespace Laidout {


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
 * curpages contains a list of document page indices.
 *
 * \todo when there are threads, apply button should be grayed
 * \todo *** for doc pages not represented in active nets, there should be special warning indicator.
 * \todo Ultimately, this should be ported to Inkscape and Scribus (devs willing), ESPECially Inkscape, when the
 *   SVG spec allows multiple pages which makes it fair game for Inkscape, that is..
 */

/*! \var int SpreadInterface::curpage
 * \brief Document page index for whatever could be considered the current page, or -1.
 */



//! Initialize everything and call GetSpreads().
/*! Incs count of docum.
 */
SpreadInterface::SpreadInterface(Laxkit::Displayer *ndp,Project *proj,Document *docum)
	: anInterface(0,ndp),
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

	sc=NULL;
}

/*! Dec count of doc. */
SpreadInterface::~SpreadInterface()
{
	 //for debugging:
	//DBG cerr<<"SpreadInterface destructor"<<endl;

	if (view) view->dec_count();
	if (doc) doc->dec_count();
	if (sc) sc->dec_count();
}

const char *SpreadInterface::Name()
{ return _("Spread Tool"); }

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
void SpreadInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
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
void SpreadInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
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
	for (int c=0; c<doc->pages.n; c++) {
		if (doc->pages.e[c]->labeltype==MARKER_Unmarked) {
			doc->pages.e[c]->labeltype=MARKER_Circle;
			doc->pages.e[c]->labelcolor.rgbf(1,1,1);
		}
	}
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


//! Arrange the spreads in some sort of order.
/*! Default is have 1 page take up about an inch of screen space.
 * The units stay in spread units, but the viewport is scaled so that a respectable
 * number of spreads fit on screen.
 *
 * how==-1 means use default arrangetype. 
 * Otherwise, use ArrangeTypes.
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
		if (view->arrangetype!=ArrangeRows) {
			dp->Center(X,X+W,Y,Y+H);
		}
	}

	UpdateMarkers(true);
}

//! Draw the little spreads, connection lines, page labels, whatever else...
int SpreadInterface::Refresh()
{
	if (!needtodraw) return 1;
	//DBG cerr << "SpreadInterface::Refresh()"<<endl;
	
	if (firsttime) {
		firsttime=0;
		ArrangeSpreads();
		if (view) view->FindBBox();
		Center(1);
		//((ViewportWindow *)curwindow)->syncrulers();
	}

	if (pagestorender.n) {
		 //do one page render, to preserve some interactivity when having to render a ton of pages
		if (drawthumbnails) {
			pagestorender.e[0]->Thumbnail();
			pagestorender.remove(0);
			if (pagestorender.n) needtodraw=2;
			else needtodraw=0;
		}
	} else needtodraw=0;

	//DBG cerr <<"pagestorender.n: "<<pagestorender.n<<endl;
	
	if (!view) { return 1; }

	int c,c2;
	
	 // Draw the connecting lines
	 // draw the connection lines in front of the spreads..
	 //*** this needs little bubble at back of arrows, and little arrow at front
	for (c=0; c<view->spreads.n; c++) {
		if (view->spreads.e[c]->prev) {
			Laidout::DrawData(dp,view->spreads.e[c]->connection,NULL,NULL);
		}
	}
	 

	 // Draw the spreads
	SomeData *outline;
	int pg;
	flatpoint p;
	
	FillStyle fs(0xffff,0xffff,0xffff,0xffff, WindingRule,FillSolid,GXcopy); //normal page
	FillStyle hfs(0xdddd,0xffff,0xffff,0xffff, WindingRule,FillSolid,GXcopy); //hidden
	FillStyle efs(0xdddd,0xdddd,0xdddd,0xffff, WindingRule,FillSolid,GXcopy); //empty
	LineStyle ls(0,0,0xffff,0xffff, 4/dp->Getmag(),CapButt,JoinMiter,~0,GXcopy);
	ls.widthtype=0;
	//Page *page;
	ImageData *thumb=NULL;
	dp->ClearClip();

	for (c=0; c<view->spreads.n; c++) {
		dp->PushAndNewTransform(view->spreads.e[c]->m());
		
//		if (view->spreads.e[c]->hidden) {
//			int c2;
//			while (view->spreads.e[c2]->hidden && c2<view->spreads.n) {
//				c2++;
//			}
//			LittleSpread *s=view->spreads.e[c];
//			flatpoint center+=s->transformPoint((flatpoint(s->outline->minx,s->outline->miny)+flatpoint(s->outline->maxx,s->outline->maxy))/2);
//			c=c2-1;
//
//			***draw outline with text saying what pages are in this hidden block
//				use outline of first in block of hidden
//
//			continue;
//		}

		 //draw the spread's path
		Laidout::DrawData(dp,view->spreads.e[c]->spread->path,NULL,view->spreads.e[c]->hidden ? &hfs : &fs);

		for (c2=0; c2<view->spreads.e[c]->spread->pagestack.n(); c2++) {
			 // draw thumbnails
			pg=view->spreads.e[c]->spread->pagestack.e[c2]->index;
			thumb=NULL;

			if (pg<0) {
				 //no page index means this place is an empty page, for a page in a thread
				Laidout::DrawData(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,NULL,&efs);

			} else if (view->spreads.e[c]->hidden) {
				// ***temp way to hide some pages' thumbnails......
				SomeData *outline=view->spreads.e[c]->spread->pagestack.e[c2]->outline;

				dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg));
				flatpoint center=(flatpoint(outline->minx,outline->miny)+flatpoint(outline->maxx,outline->maxy))/2;
				center=outline->transformPoint(center);
				dp->textout(center.x,center.y, "...",3,LAX_CENTER);

			} else if (drawthumbnails) {
				if (pg>=0 && pg<doc->pages.n) {
					if (pg>=0 && pg<doc->pages.n) {
						Page *page=doc->pages.e[pg];
						if (page->thumbmodtime<page->modtime) {
							 //need to regenerate page thumbnail, add to to-render stack
							if (pagestorender.findindex(page)<0) pagestorender.push(page,0);
							needtodraw|=2;
							thumb=NULL;
						} else thumb=doc->pages.e[pg]->Thumbnail();
					}
				}
				if (thumb) {
					//DBG cerr <<"drawing thumbnail for "<<pg<<endl;
					dp->PushAndNewTransform(view->spreads.e[c]->spread->pagestack.e[c2]->outline->m());
					
					 // always setup clipping region to be the page
					dp->PushClip(1);
					//SetClipFromPaths(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,dp->Getctm());
					SetClipFromPaths(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,NULL, true);
					
					//dp->PushAndNewTransform(thumb->m());
					//dp->imageout(thumb->image, thumb->minx,thumb->maxy, thumb->maxx-thumb->minx,thumb->miny-thumb->maxy);
					//dp->PopAxes();
					Laidout::DrawData(dp,thumb,NULL,NULL);
					
					 //remove clipping region
					dp->PopClip();

					dp->PopAxes();
					thumb=NULL;
				}
			}
		}	
		
		 // draw path again over whatever was drawn, but don't fill...
		Laidout::DrawData(dp,view->spreads.e[c]->spread->path,NULL,NULL);

		 // draw currently selected pages with thick line
		for (int cc=0; cc<curpages.n; cc++) {
			c2=view->spreads.e[c]->spread->PagestackIndex(curpages.e[cc]);
			if (c2>=0) {
				outline=view->spreads.e[c]->spread->pagestack.e[c2]->outline;

				unsigned long oldfg=dp->FG();
				dp->NewFG(0,0,255);
				dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
				dp->LineWidthScreen(3);
				Laidout::DrawData(dp,outline, &ls,NULL);
				dp->NewFG(oldfg);
			}
		}

		dp->PopAxes(); //spread axes
	}

	 //draw selection rectangle, if any
	if (buttondown.isdown(0,LEFTBUTTON) && dragpage<0) {
		dp->NewFG(0,0,255);
		dp->DrawScreen();
		dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
		dp->drawline(  lbdown.x,   lbdown.y,   lbdown.x, lastmove.y);
		dp->drawline(  lbdown.x, lastmove.y, lastmove.x, lastmove.y);
		dp->drawline(lastmove.x, lastmove.y, lastmove.x,   lbdown.y);
		dp->drawline(lastmove.x,   lbdown.y,   lbdown.x,   lbdown.y);
		dp->DrawReal();
	}
	
	return 1;
}

Laxkit::MenuInfo *SpreadInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo(_("Spread Editor"));
	else if (menu->n()==0) menu->AddSep(_("Spread Editor"));

//	//menu->AddItem(_("Page Labels..."),SIA_PageLabels);
	menu->AddItem(_("Insert Page After"),SIA_AddPageAfter);
	menu->AddItem(_("Insert Page Before"),SIA_AddPageBefore);
//	menu->AddItem(_("Insert Dummy Page"),SIA_InsertDummyPage);

	if (curpages.n || curpage>=0) {
		menu->AddSep(_("Current pages"));
		menu->AddItem(curpages.n>1?_("Delete Pages"):_("Delete Page"),SIA_DeletePages);
//		menu->AddItem(_("Detach"),SIA_DetachPages);//create a limbo page string
//		menu->AddItem(_("Export to new document..."),SIA_ExportPages);

		if (curpages.n>1) {
			bool ishidden=false;
			for (int c=0; c<curspreads.n; c++) if (curspreads.e[c]->hidden) { ishidden=true; break; }
			if (ishidden) 
				 menu->AddItem(_("Unhide Pages"),SIA_HidePages);
			else menu->AddItem(_("Hide Pages"),SIA_HidePages);
		}
	}

	menu->AddSep();
	menu->AddItem(_("New view"),SIA_NewView);
	if (!view->doc_id) menu->AddItem(_("Save view"),SIA_SaveView);
	else {
		menu->AddItem(_("Delete current view"),SIA_DeleteView);
		//menu->AddItem(_("Rename current view"),SIA_RenameView);
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


	menu->AddSep();
    menu->AddItem(_("Show thumbnails"), SIA_Thumbnails, LAX_ISTOGGLE|(drawthumbnails?LAX_CHECKED:0));

	menu->AddSep(_("Arrange"));
	for (int c=ArrangetypeMin; c<=ArrangetypeMax; c++) {
	    menu->AddItem(arrangetypestring(c), SIA_ArrangeTypeMin+c-ArrangetypeMin, LAX_ISTOGGLE|(view->arrangetype==c?LAX_CHECKED:0));
	}

	return menu;
}

//! Respond to context menu event.
int SpreadInterface::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp("pagemarker",mes)) {
		const SimpleColorEventData *cdata=dynamic_cast<const SimpleColorEventData *>(data);
		if (!cdata) return 1;
		if (curpages.n) {
			for (int c=0; c<curpages.n; c++) {
				if (curpages.e[c]>=0 && curpages.e[c]<=doc->pages.n) {
					doc->pages.e[curpages.e[c]]->labelcolor.rgb(cdata->channels[0],cdata->channels[1],cdata->channels[2]);
					doc->pages.e[curpages.e[c]]->labeltype=cdata->id;
				}
			}
		} else {
			//modding only one, taken care of by PageMarkerInterface
		}
		needtodraw=1;
		return 0;
	}

	if (!strcmp(mes,"viewportmenu") && !strcmp(mes,"menuevent")) return 1;

	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
	if (!s) return 1;

	int i=s->info2;
	
	if (i>=0 && i<doc->spreadviews.n) {
		 //select new view
		SwitchView(i);
		return 0;

	} else if (i==SIA_Thumbnails) {
		PerformAction(SIA_Thumbnails);
		return 0;

	} else if (i==SIA_AddPageAfter) {
		PerformAction(SIA_AddPageAfter);
		return 0;

	} else if (i==SIA_AddPageBefore) {
		PerformAction(SIA_AddPageBefore);
		return 0;

	} else if (i==SIA_DeletePages) {
		PerformAction(SIA_DeletePages);
		return 0;

	} else if (i==SIA_HidePages) {
		PerformAction(SIA_HidePages);
		return 0;

	} else if (i>=SIA_ArrangeTypeMin && i<=SIA_ArrangeTypeMax) {
		view->arrangetype = i-SIA_ArrangeTypeMin + ArrangetypeMin;

		if (viewport) viewport->postmessage(arrangetypestring(view->arrangetype));
		else app->postmessage(arrangetypestring(view->arrangetype));

		ArrangeSpreads();
		//Center(1);
		needtodraw=1;
		return 0;

	} else if (i==SIA_NewView) {
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

	} else if (i==SIA_SaveView) {
		if (view->doc_id!=0) return 0;
		doc->spreadviews.push(view);
		view->doc_id=doc->object_id;
		return 0;

	} else if (i==SIA_DeleteView) {
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

	} else if (i==SIA_PageLabels) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SIA_InsertDummyPage) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SIA_DetachPages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SIA_ExportPages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (i==SIA_RenameView) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;
	}

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

/*! Move page at previouspos to newpos (indices for the given thread),
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
	    newpos<0 || newpos>=doc->pages.n || previouspos==newpos) return;

	view->SwapPages(previouspos,newpos);
	needtodraw=1;
}

//! Apply however pages have been rearranged to the document.
/*! If there are any threads, then changes will not be applied.
 */
void SpreadInterface::ApplyChanges()
{
	//DBG cerr<<"ApplyChanges:"<<endl;
	if (!view) return;

	if (view->threads.n) {
		if (viewport) viewport->postmessage(_("Cannot apply when there are unconnected threads"));
		return;
	}
	if (!view->Modified()) {
		PostMessage(_("Already arranged."));
		return;
	}

	int olddocid=view->doc_id; //preserve unsaved state.. doc_id only supposed to exist in saved views
	if (!view->doc_id) view->doc_id=doc->object_id;
	view->ApplyChanges();
	view->doc_id=olddocid;
	doc->UpdateLabels(-1);

	laidout->notifyDocTreeChanged(curwindow->win_parent,TreePagesMoved,0,-1);
	UpdateMarkers(true);
	needtodraw=1;

	PostMessage(_("Arranged."));
}

//! Reset whatever tentative changes to page order have been made since last apply.
/*! temppagemap elements say what page index is temporarily in spot index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 * So, reseting is merely a matter of calling SwapPages() until temppagemap[index]==index.
 */
void SpreadInterface::Reset()
{
	if (!view) return;
	if (!view->Modified()) {
		PostMessage(_("Nothing needs reseting"));
		return;
	}

	clearSelection();
	view->Reset();
	UpdateMarkers(true);
	needtodraw=1;
	PostMessage(_("Reseted."));
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
	buttondown.down(d->id,LEFTBUTTON, x,y);

	int psi=-1,page=-1,thread=-1;
	LittleSpread *spread=findSpread(x,y,&psi,&thread);
	if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }

	//DBG cerr <<"SpreadInterface lbdown found page "<<page<<" in thread "<<thread<<endl;
	if (page<0) {
		// start a selection rectangle. note: could be click down inside an empty page
		lbdown=lastmove=flatpoint(x,y);
		buttondown.moveinfo(d->id,LEFTBUTTON, SIA_DragRect);
		dragpage=-1;
		return 0;
	}
	
	dragpage=page;
	int i=curpages.findindex(page);

	if (i<0) { //item wasn't already selected, so definitely select something
		if (!(state&ShiftMask)) {
			 //no shift, deselect all, and select that one,
			 //but holding control doesn't flush the others
			if (!(state&ControlMask)) {
				curspreads.flush();
				curpages.flush();
			}
			curpages.pushnodup(page);
			curspreads.pushnodup(spread,0);
			curpage=page;

		} else {
			 //shift click on non-selected, select all between curpage and clicked one
			if (curpage<0) curpage=page;
			for (int c=(curpage<page?curpage:page); c<=(curpage>page?curpage:page); c++) {
				curpages.pushnodup(c);
				curspreads.pushnodup(view->SpreadOfPage(c,NULL,NULL,NULL,0),0);
			}
		}

		buttondown.moveinfo(d->id,LEFTBUTTON, SIA_MaybeMovePages, page);

	} else { //item was selected already
		if (state&ControlMask) {
			 //control click: deselect page, and also spread if no other pages in that spread are selected
			DeselectPage(page,spread);
			buttondown.up(d->id,LEFTBUTTON);
			return 0;
		}
		//else we are probably trying to move selected pages...
		buttondown.moveinfo(d->id,LEFTBUTTON, SIA_MaybeMovePages, page);
	}

	//--wait to change shape until actual movement...
	//int shape=0;
	//if ((state&LAX_STATE_MASK)==0 || curpages.n>1) shape=LAX_MOUSE_To_E;
	//else shape=LAX_MOUSE_Exchange;
	//const_cast<LaxMouse*>(d)->setMouseShape(curwindow,shape);

	needtodraw=1;
	return 0;
}

/*! Removes page from curpages, and checks if spread must be removed also, and removes it if necessary.
 * If spread!=NULL, it is assumed to contain page. If NULL, then it simply
 * looks up with view->SpreadOfPage().
 */
int SpreadInterface::DeselectPage(int page,LittleSpread *spread)
{
	if (page<0) return 1;
	if (curpages.findindex(page)<0) return 2; //page already not selected!

	if (!spread) spread=view->SpreadOfPage(page,NULL,NULL,NULL,0);

	int max=spread->spread->pagestack.n();
	int selected=0;
	curpages.pop(curpages.findindex(page));

	int i;
	for (int c=0; c<max; c++) {
		i=spread->spread->pagestack.e[c]->index;
		if (i<0 || i==page) continue;
		if (curpages.findindex(i)>=0) selected++;
	}
	if (selected==0) {
		curspreads.remove(curspreads.findindex(spread));
	}

	 //update curpage, in case it was page
	if (curpage==page) {
		if (curpages.n>0) {
			curpage=curpages.e[0];
			curspread=view->SpreadOfPage(curpage,NULL,NULL,NULL,0);
		} else {
			curpage=-1;
			curspread=NULL;
		}
	}

	needtodraw=1;
	return 0;
}

//! Drops pages.
/*! Mouse up outside window into a ViewWindow shifts view to that page.
 * 
 * \todo Idea for use: have a SpreadEditor be behind a ViewWindow pane.
 * bring the spreadeditor forward, double click on a page which causes
 * the view to come forward with that page selected..
 */
int SpreadInterface::rLBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	const_cast<LaxMouse*>(d)->setMouseShape(curwindow,0);
	int what=0;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &what);

	if (what==SIA_DragRect) {
		 //dragged out a selection, so select everything inside the box
		if ((state&LAX_STATE_MASK)==0) clearSelection();

		int page;
		DoubleBBox box;
		box.addtobounds(lbdown);
		box.addtobounds(lastmove);
		double m[6];
		for (int c=0; c<view->spreads.n; c++) {
			transform_mult(m, view->spreads.e[c]->m(), dp->Getctm());
			if (box.intersect(m, view->spreads.e[c], true, false)) {
				curspreads.pushnodup(view->spreads.e[c],0);

				for (int c2=0; c2<view->spreads.e[c]->spread->pagestack.n(); c2++) {
					page=view->spreads.e[c]->spread->pagestack.e[c2]->index;
					if (page>=0) curpages.pushnodup(page);
				}
			}
		}

		needtodraw=1;
		return 0;
	}
	
	if (what==SIA_MoveSpreads) return 0;

	 //find destination for dragging...
	int psi,thread,destpage=-1;
	LittleSpread *spread=findSpread(x,y,&psi,&thread);
	if (spread && psi>=0) { destpage=spread->spread->pagestack.e[psi]->index; }
	//DBG cerr <<"SpreadInterface lbup found page "<<psi<<" in thread "<<thread<<endl;


	 //If mouse up outside window, maybe call up that page in a view window
	if (x<0 || x>curwindow->win_w || y<0 || y>curwindow->win_h) {
		 // mouse up outside window so search for a ViewWindow to shift view for
		int mx,my;
		anXWindow *win;
		mouseposition(d->id,NULL,&mx,&my,NULL,&win);
		//anXWindow *win=app->findDropCandidate(curwindow,x,y, NULL,NULL);

		if (win && !strcmp(win->whattype(),"LaidoutViewport")) {
			LaidoutViewport *vp=dynamic_cast<LaidoutViewport *>(win);
			vp->UseThisDoc(doc);
			vp->SelectPage(dragpage);
			
			//DBG cerr <<" ~~~~~~~~~~~~drop page "<<dragpage<<" to win type: "<<win->whattype()<<endl;
		}
		dragpage=-1;
		needtodraw=1;
		return 0;
	}
	
	 // swap dragpage and page
	//DBG cerr <<"*** drag "<<dragpage<<" to "<<(spread?"swap":"make thread")<<endl;

	if (!spread) {
		//*** transfer all the curpages to a thread
		//DBG cerr <<" *** need to implement spread editor drag pages to limbo thread..."<<endl;

	} else {
		if (dragged<5) {
			if ((state&LAX_STATE_MASK)==0 && destpage>=0) {
				 //plain clicked down and up on same page, so deselect all but that one
				clearSelection();
				curpages.push(destpage);
				curspreads.push(spread);
				curpage=destpage;
				curspread=spread;
				needtodraw=1;
			} //else was down and up with little movement, do nothing!

		} else {
			if (curpages.findindex(destpage)<0) { //only change when destination is not already selected
				SortCurpages();

				if ((state&LAX_STATE_MASK)==0) {
					 //slide pages
					int oldi, newi;
					for (int c=0; c<curpages.n; c++) {
						dragpage=curpages.e[c];
						oldi=view->reversemap(dragpage);
						newi=view->reversemap(destpage);
						if (oldi>newi && c>0) newi++;
						SlidePages(oldi,newi,0);
						destpage=dragpage;
					}

				} else if ((state&LAX_STATE_MASK)==ControlMask) {
					 //swap pages... if more than one page, just swap the first and slide the rest after the swapped
					dragpage=curpages.e[0];
					SwapPages(view->reversemap(dragpage),view->reversemap(destpage));
					destpage=dragpage;
					for (int c=1; c<curpages.n; c++) {
						SlidePages(view->reversemap(curpages.e[c]),view->reversemap(destpage), 0);
						destpage=curpages.e[c];
					}
				}

				 //update curspreads to new page order
				curspreads.flush();
				for (int c=0; c<curpages.n; c++) {
					curspreads.pushnodup(view->SpreadOfPage(c,NULL,NULL,NULL,0),0);
				}
				curspread=view->SpreadOfPage(curpage,NULL,NULL,NULL,0);
			}
		}
	}

	//if (dragged) clearSelection();
	dragpage=-1;
	UpdateMarkers(true);
	
	return 0;
}

/*! Make page list in curpages be increasing in order of appearance (not in order in document).
 */
void SpreadInterface::SortCurpages()
{
	 //selection sort because I'm lazy
	int t;
	for (int c=0; c<curpages.n; c++) {
		for (int c2=c+1; c2<curpages.n; c2++) {
			if (view->reversemap(curpages.e[c2]) < view->reversemap(curpages.e[c])) {
				t=curpages.e[c2];
				curpages.e[c2]=curpages.e[c];
				curpages.e[c]=t;
			}
		}
	}
}

/*! Intercept to update curpage to be under x,y, so that context menu applies to that page.
 */
int SpreadInterface::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (curpage<0) {
		 //need to update curpage so that menu actions can affect it
		int psi=-1, thread=-1;
		LittleSpread *spread=findSpread(x,y,&psi,&thread);
		if (spread && psi>=0) { curpage=spread->spread->pagestack.e[psi]->index; }
	}
	return 1;
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

	if ((state&LAX_STATE_MASK)==0) {
		clearSelection();//clear if not shift or control click
		curpages.pushnodup(page);
		curspreads.pushnodup(spread,0);

	} else if ((state&LAX_STATE_MASK)==ShiftMask) {
		 //push all pages between curpage and new curpage
		for (int c=(curpage<page?curpage:page); c<=(curpage>page?curpage:page); c++) {
			curpages.pushnodup(c);
			curspreads.pushnodup(view->SpreadOfPage(c,NULL,NULL,NULL,0),0);
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

	int mx,my;
	buttondown.move(d->id,x,y, &mx,&my);

	int what=0;
	if (buttondown.isdown(d->id,MIDDLEBUTTON)) what=SIA_MoveSpreads;
	else if (buttondown.isdown(d->id,LEFTBUTTON)) {
		buttondown.getextrainfo(d->id,LEFTBUTTON, &what);
		if ((state&LAX_STATE_MASK)==ShiftMask && what!=SIA_MoveSpreads) {
			what=SIA_MoveSpreads;
			buttondown.moveinfo(d->id,LEFTBUTTON, what);
		}
	}

	if (what==SIA_MoveSpreads && curspreads.n) {
		 //move around spread outline locations
		LittleSpread *s;
		for (int c=0; c<curspreads.n; c++) {
			s=curspreads.e[c];
			s->origin(s->origin()+screentoreal(x,y)-screentoreal(mx,my));
			s->mapConnection();
			if (s->next) s->next->mapConnection();
		}
		view->arrangetype=view->arrangestate=ArrangeCustom;
		view->FindBBox();
		UpdateMarkers(false);
		needtodraw=1;
		return 0;

	} else if (what==SIA_DragRect) {
		 //draw select rectangle
		 //*** need to temp select pages which box touches them...
		lastmove.x=x;
		lastmove.y=y;
		needtodraw=1;
		return 0;

	} else if (what==SIA_MaybeMovePages) {
		 //dragging a group of pages to possibly a new location
		 //change mouse shape to relevant indicator 

		if (x<0 || x>curwindow->win_w || y<0 || y>curwindow->win_h) {
			 // mouse outside window so search for a LaidoutViewport to shift view for
			int mx,my;
			anXWindow *win;
			mouseposition(d->id,NULL,&mx,&my,NULL,&win);
			//anXWindow *win=app->findDropCandidate(curwindow,x,y, NULL,NULL);

			//DBG if (win) cerr <<"maybe drop to: "<<win->whattype()<<endl;
			//DBG else cerr <<"couldn't find drop candidate"<<endl;

			if (win && !strcmp(win->whattype(),"LaidoutViewport")) {
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_To_S);
			} else {
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Cancel);
			}
			return 0;
		}

		int page=-1,psi,thread;
		LittleSpread *spread=findSpread(x,y,&psi,&thread);
		if (spread && psi>=0) { page=spread->spread->pagestack.e[psi]->index; }

		if (page>=0 && curpages.findindex(page)>=0) {
			 //we must have a non-selected page to move things to
			const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Cancel);

		} else if (!spread) {
			 //maybe moving to a limbo thread.. IMPLEMENT ME!!
			//const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Boxes);
			const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Cancel);

		} else {
			 //these must sync with LBUp:
			if ((state&LAX_STATE_MASK)==0) 
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_To_E); //shift pages
			else if ((state&LAX_STATE_MASK)==ControlMask)
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Exchange); //swap pages
			else
				const_cast<LaxMouse*>(d)->setMouseShape(curwindow,LAX_MOUSE_Cancel); //do nothing
		}

		return 0;

	}

	return 1;
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

	if (newmark==-1) {
		newmark=(doc->pages.e[view->map(curpages.e[0])]->labeltype) % (MARKER_MAX-1) + 1;
	} else if (newmark==-2) {
		newmark=(doc->pages.e[view->map(curpages.e[0])]->labeltype+MARKER_MAX-3) % (MARKER_MAX-1) + 1;
	}

	for (int c=0; c<curpages.n; c++) doc->pages.e[view->map(curpages.e[c])]->labeltype=newmark;

	return newmark;
}

/*! Called when page positions have changed.
 * If all, then update all markers in view, else just update the ones in curspreads.
 */
void SpreadInterface::UpdateMarkers(bool all)
{
    PageMarkerInterface *pmi=dynamic_cast<PageMarkerInterface*>(viewport->HasInterface("PageMarkerInterface",NULL));
	if (!pmi) return;

	if (all) pmi->ClearPages();

	LittleSpread *lspread;
	Spread *spread;
	SomeData *outline;
	flatpoint p;
	int where=view->centerlabels;
	int linetype=0;

	for (int c=0; c<(all ? view->spreads.n : curspreads.n); c++) {
		if (all) lspread=view->spreads.e[c];
		else lspread=curspreads.e[c];
		spread=lspread->spread;

		PageLocation *pl;
		for (int c=0; c<spread->pagestack.n(); c++) {
			pl=spread->pagestack.e[c];
			if (!pl->page && pl->index>=0) pl->page=doc->pages.e[pl->index];
			if (!pl->page) continue;
			outline=pl->outline;
			if (pl->index==view->temppagemap[pl->index]) linetype=0; else linetype=1;

			if (where==LAX_CENTER)
				p=flatpoint((outline->minx+outline->maxx)/2,(outline->miny+outline->maxy)/2); //center
			else if (where==LAX_BOTTOM)
				p=flatpoint((outline->minx+outline->maxx)/2,(outline->miny));//bottom
			else if (where==LAX_LEFT)
				p=flatpoint((outline->minx),(outline->miny+outline->maxy)/2);//left
			else if (where==LAX_TOP)
				p=flatpoint((outline->minx+outline->maxx)/2,(outline->maxy));//top
			else //LAX_RIGHT
				p=flatpoint((outline->maxx),(outline->miny+outline->maxy)/2);//right

			p=pl->outline->transformPoint(p);
			p=lspread->transformPoint(p);

			if (all) pmi->AddPage(pl->page, p, 1, linetype);
			else {
				pmi->UpdatePage(pl->page, p, 1, linetype);
			}

		}
	}
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
			const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Cancel); //swap
		} else if (!spread) {
			const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Boxes); //float
		} else {
			//if ((state&LAX_STATE_MASK)==0) 
				const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_To_E); //slide
			//else 
				//const_cast<LaxMouse*>(mouse)->setMouseShape(curwindow,LAX_MOUSE_Exchange);
		}
	}
	return 1;
}

Laxkit::ShortcutHandler *SpreadInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("SpreadInterface");
	if (sc) return sc;

	sc=new ShortcutHandler("SpreadInterface");

	sc->Add(SIA_ToggleSelect,   'a',0,0,          _("ToggleSelect"),   _("Select all or none"),NULL,0);
	sc->Add(SIA_SelectSimilar,  'm',ControlMask,0,_("SelectSimilar"),  _("Select all pages with the same kind of markers"),NULL,0);

	sc->Add(SIA_Center,         ' ',0,0,          _("Center"),         _("Center"),NULL,0);
	sc->Add(SIA_LabelPos,       'l',0,0,          _("LabelPos"),       _("Move label position"),NULL,0);
	sc->Add(SIA_ToggleColor,    'c',0,0,          _("ToggleColor"),    _("Toggle color among defined colors"),NULL,0);
	sc->Add(SIA_ToggleColorR,   'C',ShiftMask,0,  _("ToggleColorR"),   _("Toggle color among defined colors"),NULL,0);
	sc->Add(SIA_ToggleMark,     'm',0,0,          _("ToggleMark"),     _("Toggle mark type"),NULL,0);
	sc->Add(SIA_ToggleMarkR,    'M',ShiftMask,0,  _("ToggleMarkR"),    _("Toggle mark type"),NULL,0);
	sc->Add(SIA_Thumbnails,     't',0,0,          _("Thumbs"),         _("Toggle showing thumbnails"),NULL,0);
	sc->Add(SIA_ToggleArrange,  'A',ShiftMask,0,  _("Arrange"),        _("Toggle arrangement type"),NULL,0);
	sc->Add(SIA_RefreshArrange, 'A',ShiftMask|ControlMask,0, _("RefreshArrange"), _("Refresh with current arrangement type"),NULL,0);

	sc->Add(SIA_AddPageAfter,   'p',0,0,          _("AddPage"),         _("Add a page after current"),NULL,0);
	sc->Add(SIA_AddPageBefore,  'P',ShiftMask,0,  _("AddPageBefore"),   _("Add a page before current"),NULL,0);
	sc->Add(SIA_DeletePages,    LAX_Del,0,0,      _("Deletepages"),     _("Delete pages"),NULL,0);
	sc->AddShortcut(LAX_Bksp,0,0, SIA_DeletePages);
	sc->Add(SIA_HidePages,      'h',0,0,          _("Hidepages"),       _("Toggle hiding of pages"),NULL,0);

	manager->AddArea("SpreadInterface",sc);
	return sc;
}

int SpreadInterface::PerformAction(int action)
{
	if (action==SIA_Center) {
		Center(1);
		needtodraw=1;
		return 0;

	} else if (action==VIEWPORT_ResetView || action==VIEWPORT_Default_Zoom) {
		return PerformAction(SIA_Center);

	} else if (action==SIA_ToggleSelect) {
		if (curpages.n>0) {
			 //unselect all
			clearSelection();
			needtodraw=1;
			return 0;
		}

		 //select all
		Spread *spread;
		for (int c=0; c<view->spreads.n; c++) {
			 //foreach page in each spread...
			curspreads.pushnodup(view->spreads.e[c],0);

			spread=view->spreads.e[c]->spread;
			if (!spread) continue;
			for (int c2=0; c2<spread->pagestack.n(); c2++) {
				if (spread->pagestack.e[c2]->index>=0) curpages.pushnodup(spread->pagestack.e[c2]->index);
			}
		}
		needtodraw=1;
		return 0;

	} else if (action==SIA_SelectSimilar) {
		int pp=curpage;
		if (pp<0 && curpages.n) pp=curpages.e[0];
		if (pp<0) return 0;
		Page *page=doc->pages.e[pp], *pg;
		Spread *spread;

		for (int c=0; c<view->spreads.n; c++) {
			spread=view->spreads.e[c]->spread;
			for (int c2=0; c2<spread->pagestack.n(); c2++) {
				if (spread->pagestack.e[c2]->index<0) continue;
				pg=doc->pages.e[spread->pagestack.e[c2]->index];

				if (!pg->labelcolor.equals(page->labelcolor)) continue;
				if ( pg->labeltype!=page->labeltype) continue;

				curpages.pushnodup(spread->pagestack.e[c2]->index);
				curspreads.pushnodup(view->spreads.e[c],0);
			}
		}
		needtodraw=1;
		return 0;

	} else if (action==SIA_LabelPos) {
		if (view->centerlabels==LAX_CENTER) view->centerlabels=LAX_TOP;
		else if (view->centerlabels==LAX_TOP) view->centerlabels=LAX_RIGHT;
		else if (view->centerlabels==LAX_RIGHT) view->centerlabels=LAX_BOTTOM;
		else if (view->centerlabels==LAX_BOTTOM) view->centerlabels=LAX_LEFT;
		else view->centerlabels=LAX_CENTER;
		UpdateMarkers(true);
		needtodraw=1;
		return 0;

	} else if (action==SIA_ToggleColor || action==SIA_ToggleColorR) {
		if (!view || !curpages.n) return -1;

		ViewerWindow *viewer=dynamic_cast<ViewerWindow*>(viewport->win_parent);
		if (!viewer) return 0;
		PageMarkerInterface *pmi=dynamic_cast<PageMarkerInterface*>(viewer->FindInterface("PageMarkerInterface"));
		if (!pmi) return 0;

		ScreenColor color=doc->pages.e[view->map(curpages.e[0])]->labelcolor;
		if (action==SIA_ToggleColor) color=pmi->NextColor(color);
		else color=pmi->PreviousColor(color);

		for (int c=0; c<curpages.n; c++) doc->pages.e[view->map(curpages.e[c])]->labelcolor=color;
		needtodraw=1;
		return 0;

	} else if (action==SIA_ToggleMark) {
		if (ChangeMarks(-1)>=0) needtodraw=1;
		return 0;

	} else if (action==SIA_ToggleMarkR) {
		if (ChangeMarks(-2)>=0) needtodraw=1;
		return 0;

	} else if (action==SIA_Thumbnails) {
		drawthumbnails=!drawthumbnails;
		//DBG cerr <<"-- drawthumbnails: "<<drawthumbnails<<endl;
		if (drawthumbnails) PostMessage(_("Show page previews"));
		else PostMessage(_("Don't show page previews"));
		needtodraw=1;
		return 0;

	} else if (action==SIA_ToggleArrange) {
		view->arrangetype++;
		if (view->arrangetype==ArrangetypeMax+1) view->arrangetype=ArrangetypeMin;

		if (viewport) viewport->postmessage(arrangetypestring(view->arrangetype));
		else app->postmessage(arrangetypestring(view->arrangetype));

		ArrangeSpreads();
		Center(1);
		needtodraw=1;
		return 0;

	} else if (action==SIA_RefreshArrange) {
		view->arrangestate=ArrangeNeedsArranging;
		ArrangeSpreads();
		Center(1);
		needtodraw=1;
		return 0;

	} else if (action==SIA_AddPageAfter || action==SIA_AddPageBefore) {
		if (!doc) return 0;
		int page=curpage;
		if (page<0) {
			if (action==SIA_AddPageAfter) page=doc->pages.n-1;
			else page=0;
		}

        int c=doc->NewPages(page+(action==SIA_AddPageAfter ? 1 : 0),1);
        if (c>=0) {
            char scratch[100];
            sprintf(scratch,_("Page number %d added (%d total)."),page+1,doc->pages.n);
            PostMessage(scratch);
        } else PostMessage(_("Error adding page."));

		clearSelection();
        return 0;


	} else if (action==SIA_HidePages) {
		if (curspreads.n==0) {
			PostMessage(_("Nothing to hide"));
			return 0;
		}

		 //unhides if any in range are hidden
		bool newhide=true;
		for (int c=0; c<curspreads.n; c++) {
			if (curspreads.e[c]->hidden) newhide=false;
		}
		for (int c=0; c<curspreads.n; c++) {
			curspreads.e[c]->hidden=newhide;
		}
		if (newhide) PostMessage(_("Pages hidden."));
		else PostMessage(_("Pages unhidden."));
		needtodraw=1;
		return 0;

	} else if (action==SIA_DeletePages) {
        if (!doc) return 0;
		if (doc->pages.n==1) {
			PostMessage(_("Cannot delete the only page."));
			return 0;
		}

		int page, index;
		if (curpages.n==0 && curpage>=0) curpages.push(curpage);
		while (curpages.n) {
			page=-1;
			index=-1;
			for (int c=0; c<curpages.n; c++) {
				if (curpages.e[c]>index) { index=c; page=curpages.e[c]; }
			}
			if (index==-1) break;

			int c=doc->RemovePages(page,1); //remove page
			if (c==1) {
				 //deleted successfully
				char scratch[100];
				sprintf(scratch,_("Page %d deleted. %d remaining."),page,doc->pages.n);
				PostMessage(scratch);
			} else PostMessage(_("Error deleting page."));
			curpages.remove(index);
		}
		clearSelection();

        // Document sends the notifyDocTreeChanged..
		return 0;
 

	} else if (action==SIA_InsertDummyPage) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;


	} else if (action==SIA_DetachPages) {
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;

	} else if (action==SIA_ExportPages) {
		 //create a new document with the same imposition type, with the selected pages
		cerr <<" *** finish implementing SpreadInterface::Event()"<<endl;
	}

	return 1;
}

//! Key presses.
/*!
 * \todo *** space should arrange if auto arrange
 */
int SpreadInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	//DBG cerr <<"SpreadInterface::CharInput: "<<ch<<" as char: '"<<(char)(ch>20 && ch<127?ch:(int)'?')<<","<<endl;

	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
	    return PerformAction(action);
	}

	if (ch==LAX_Esc && (state&LAX_STATE_MASK)==0 && curspreads.n>0) {
		clearSelection();
		return 0;
	}

	if (ch==LAX_Shift && dragpage>=0 && curpages.n==1) {
		const_cast<LaxMouse*>(d->paired_mouse)->setMouseShape(curwindow,LAX_MOUSE_Exchange); //swap
		return 0;

	//DBG } else if (ch=='i') { //*** for debugging thumbnails....
	//DBG 	if (curpage<0) return 0;
	//DBG 	ImageData *thumb=doc->pages.e[curpage]->Thumbnail();
	//DBG 	cerr <<"'P' image dump:"<<endl;
	//DBG 	thumb->dump_out(stderr,2,0,NULL);
	//DBG 	if (thumb) {
	//DBG 		dp->StartDrawing(curwindow);
	//DBG 		//double i[6];
	//DBG 		//transform_invert(i,thumb->m());
	//DBG 		//dp->PushAndNewTransform(i);
	//DBG 		Laidout::DrawData(dp,thumb,NULL,NULL,DRAW_AXES|DRAW_BOX);
	//DBG 		//dp->PopAxes();
	//DBG 		dp->EndDrawing();
	//DBG 	}
	//DBG 	return 0;

	}
	return 1;
}

//----------------------- SpreadEditorViewport --------------------------------------

/*! \class SpreadEditorViewport
 * \brief Redefine ViewportWindow to handle custom refreshing.
 */
class SpreadEditorViewport : public ViewportWindow
{
  public:
	SpreadInterface *spreadtool;
	SpreadEditorViewport(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						 int xx,int yy,int ww,int hh,int brder, Laxkit::Displayer *ndp,SpreadInterface *s);
	virtual ~SpreadEditorViewport() {}
	virtual void RefreshUnder();
	virtual const char *whattype() { return "SpreadEditorViewport"; }
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
};

SpreadEditorViewport::SpreadEditorViewport(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						 int xx,int yy,int ww,int hh,int brder, Laxkit::Displayer *ndp,
						 SpreadInterface *s)
  : ViewportWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,ndp)
{
	spreadtool=s;
}

/*! Always draw the spreads no matter what other interface is present.
 */
void SpreadEditorViewport::RefreshUnder()
{
	//DBG cerr <<"SpreadEditorViewport::RefreshUnder()..."<<interfaces.n<<" "<<interfaces.findindex(spreadtool)<<endl;
	
	if (interfaces.findindex(spreadtool)<0) {
		spreadtool->needtodraw=1;
		spreadtool->Refresh();
	}
}

#define SPREADE_Help  (VIEWPORT_MAX+1)

Laxkit::ShortcutHandler *SpreadEditorViewport::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

	//DBG cerr <<"Get shortcuts for SpreadEditorViewport, object_id="<<object_id<<endl;
	sc=ViewportWindow::GetShortcuts();

	sc->Add(SPREADE_Help,   LAX_F1,0,0,   _("Help"), _("Help"),NULL,0);

	return sc;
}

int SpreadEditorViewport::PerformAction(int action)
{
	if (action==SPREADE_Help) {
		app->addwindow(newHelpWindow("SpreadInterface"));
		return 0;
	}

	return ViewportWindow::PerformAction(action);
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

	DBG if (viewport) cerr <<"*** there shouldn't be a viewport here, SpreadEditor::SpreadEditor()!!!"<<endl;
	SpreadEditorViewport *sed=new SpreadEditorViewport(this,"spread-editor-viewport","spread-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL,spreadtool); 
	viewport=sed;
	WindowColors *col = win_colors->duplicate();
	installColors(col);
	viewport->installColors(col);
	win_colors->bg=rgbcolor(200,200,200);
	col->dec_count();
	viewport->dp->NewBG(255,255,255); 
	viewer_style |= VIEWPORT_NO_XRULER|VIEWPORT_NO_YRULER;

	//app->reparent(viewport,this);
	//viewport->dec_count();
	//*** doing this here removes memory hole, but adds rulers to window!?!?!?!?

	needtodraw=1;


	//------------Add spread editor interfaces
	//viewport->Push(spreadtool,1); // local, and select it
	spreadtool=new SpreadInterface(viewport->dp,project,doc);
	spreadtool->GetShortcuts();
	sed->spreadtool=spreadtool;
	AddTool(spreadtool,1,1);

	AddTool(new PageRangeInterface(1,viewport->dp, doc),0,1);

	AddTool(new PaperInterface(2,viewport->dp),0,1);
	//AddTool(new NupInterface(1,viewport->dp),1,0);

	AddTool(new PageMarkerInterface(spreadtool,3,viewport->dp), 1,1); //1==select, 1==absorb
}

SpreadEditor::~SpreadEditor()
{
	if (doc) doc->dec_count();
}

//! Passes off to SpreadInterface::dump_out().
void SpreadEditor::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	spreadtool->dump_out(f,indent,what,context);
}

//! Passes off to SpreadInterface::dump_in_atts().
void SpreadEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	spreadtool->dump_in_atts(att,flag,context);
}

/*! Return 0 for success, nonzero error.
 */
int SpreadEditor::UseThisDoc(Document *ndoc)
{
	if (ndoc != doc) {
		if (ndoc) ndoc->inc_count();
		if (doc) doc->dec_count();
		doc=ndoc;
	}

	DocumentUser *d;
	for (int c=0; c<viewport->interfaces.n; c++) {
		d=dynamic_cast<DocumentUser*>(viewport->interfaces.e[c]);
		if (d) d->UseThisDocument(doc);
		else if (dynamic_cast<SpreadInterface*>(viewport->interfaces.e[c])) {
			dynamic_cast<SpreadInterface*>(viewport->interfaces.e[c])->UseThisDoc(doc);
		}
	}
	//spreadtool->UseThisDoc(doc);
	return 0;
}

int SpreadEditor::SelectTool(int id)
{
    int c=ViewerWindow::SelectTool(id);


	SliderPopup *toolselector=dynamic_cast<SliderPopup*>(findChildWindowByName("viewtoolselector"));
    if (toolselector) {
        if (c==0) toolselector->Select(curtool->id);
        else if (c==-1) {
             //overlay toggled, update check mark
            int oi=toolselector->GetItemIndex(id);
            if (oi>=0) {
                toolselector->SetState(oi, MENU_CHECKED, -1);
            }
        }
    }
    return c;
}

/*! Removes rulers and adds Apply, Reset.
 */
int SpreadEditor::init()
{
	//AddWin(***)...
	ViewerWindow::init();

	 //remove refs to nonexistent rulers....
	viewport->UseTheseRulers(NULL,NULL);
	viewport->dec_count(); //remove initial creation count from constructor


	anXWindow *last=NULL;
	Button *tbut;

	AddNull(); // makes the status bar fill whole line

	 //-----document/view list
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
										 NULL,laidout->icons->GetIcon("Laidout")
										);
	menub->tooltip(_("Document list"));
	AddWin(menub,1, menub->win_w,0,50,50,0, menub->win_h,0,50,50,0, -1);
	//AddWin(menub,menub->win_w,0,50,50, menub->win_h,0,50,50, -1);//add before status bar

	//wholelist.e[wholelist.n-1]->pw(100);
	//AddNull(); // makes the status bar fill whole line


	 //---- tool section
	SliderPopup *toolselector;
	 //---- tool section
	last=toolselector=new SliderPopup(this,"viewtoolselector",NULL,SLIDER_LEFT, 0,0,0,0,1, 
			NULL,object_id,"toolselector",
			NULL,0);
	toolselector->tooltip(_("The current tool"));
	LaxImage *img;
	int obji=0;
	int c;
	int numoverlays=0;

	for (c=0; c<tools.n; c++) {
		if (tools.e[c]->interface_type==INTERFACE_Overlay) {
			numoverlays++;
			continue;
		}
		if (!strcmp(tools.e[c]->whattype(),"Spread")) obji=tools.e[c]->id;
		
		img=laidout->icons->GetIcon(tools.e[c]->IconId());
		
		toolselector->AddItem(tools.e[c]->Name(),img,tools.e[c]->id); //does not call inc_count()
		//if (img) img->dec_count();
	}
	if (numoverlays) {
		toolselector->AddSep(_("Overlays"));
		for (c=0; c<tools.n; c++) {
			if (tools.e[c]->interface_type!=INTERFACE_Overlay) continue;
			img=laidout->icons->GetIcon(tools.e[c]->IconId());
			toolselector->AddItem(tools.e[c]->Name(),img,tools.e[c]->id); //does not call inc_count()
			toolselector->SetState(-1,(viewport->HasInterface(tools.e[c]->id)?MENU_CHECKED:0)|MENU_ISTOGGLE|SLIDER_IGNORE_ON_BROWSE,1);
		}
	}

	toolselector->WrapToExtent();
	SelectTool(obji);
	AddWin(toolselector,1,-1);




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

	//last=tbut=new Button(this,"updatethumbs",NULL, 0, 0,0,0,0,1, last,object_id,"updatethumbs",0,_("Update Thumbs"));
	//AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);


	ColorBox *colorbox;
	last=colorbox=new ColorBox(this,"colorbox",NULL,0, 0,0,0,0,1, NULL,object_id,"curcolor",
                               LAX_COLOR_RGB,
                               150./65535,
                               1.,0.,0.,1.);
    AddWin(colorbox,1, 50,0,50,50,0, toolselector->win_h,0,50,50,0, -1);

	Sync(1);	
	return 0;
}

int SpreadEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	//DBG cerr <<"SpreadEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);
		if (!te || te->changer==this) return 1;

		if (te->changetype==TreeObjectRepositioned ||
				te->changetype==TreeObjectReorder ||
				te->changetype==TreeObjectDiffPage ||
				te->changetype==TreeObjectDeleted ||
				te->changetype==TreeObjectAdded) {
			//DBG cerr <<"*** need to make a SpreadEditor:: flag for need to update thumbs"<<endl;

		} else if (te->changetype==TreePagesAdded ||
				te->changetype==TreePagesDeleted ||
				te->changetype==TreePagesMoved) {
			spreadtool->CheckSpreads(te->start,te->end);
			spreadtool->UpdateMarkers(true);

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
		spreadtool->Reset();
		return 0;

	} else if (!strcmp("applybutton",mes)) {
		//cout <<"SpreadEditor got applybutton message"<<endl;
		spreadtool->ApplyChanges();
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
//		menu->AddItem(_("New view"),SIA_NewView);
		SpreadInterface *interf=(SpreadInterface*)tools.e[0];
//		if (!interf->view->doc_id) menu->AddItem(_("Save view"),SIA_SaveView);
//		else {
//			menu->AddItem(_("Delete current view"),SIA_DeleteView);
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
		popup=new PopupMenu(NULL,_("Documents"), TREESEL_LEFT,
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
		//DBG cerr <<"rulercornermenu:"<<i<<endl;

		 //0-999 was document things
		if (i-500>=0 && i-500<laidout->project->docs.n) {
			UseThisDoc(laidout->project->docs.e[i-500]->doc);
			return 0;

		} else if (i>=0 && i<doc->spreadviews.n) {
			interf->SwitchView(i);
		}

	} else if (!strcmp(mes,"toolselector")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		int newtool=s->info1;
		SelectTool(newtool);
		viewport->Needtodraw(1);
		return 0;
	}

	return ViewerWindow::Event(data, mes);
}

int SpreadEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	//DBG cerr <<"SpreadEditor::CharInput: "<<ch<<" as char: '"<<(char)(ch>20 && ch<127?ch:(int)'?')<<","<<endl;

	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
		app->addwindow(newHelpWindow(whattype()));
		return 0;
	}
	return ViewerWindow::CharInput(ch,buffer,len,state,d);
}

//! Trigger an ArrangeSpreads if arrangetype is auto.
int SpreadEditor::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=ViewerWindow::MoveResize(nx,ny,nw,nh);
	if (spreadtool->view->arrangetype==ArrangeAutoTillMod ||
			spreadtool->view->arrangetype==ArrangeAutoAlways) 
		spreadtool->ArrangeSpreads();
	return c;
}

//! Trigger an ArrangeSpreads if arrangetype is auto.
int SpreadEditor::Resize(int nw,int nh)
{
	int c=ViewerWindow::Resize(nw,nh);
	if (spreadtool->view->arrangetype==ArrangeAutoTillMod ||
			spreadtool->view->arrangetype==ArrangeAutoAlways) 
		spreadtool->ArrangeSpreads();
	return c;
}

} // namespace Laidout

