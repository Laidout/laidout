//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/******* viewwindow.cc *********/

#include <lax/numslider.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/transformmath.h>
#include <lax/strsliderpopup.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/filedialog.h>
#include <lax/refcounter.h>
#include <lax/colorbox.h>
#include <cstdarg>

#include "helpwindow.h"
#include "spreadeditor.h"
#include "viewwindow.h"
#include "laidout.h"
#include "extras.h"
#include "drawdata.h"
#include "helpwindow.h"

#include <iostream>
using namespace std;

#define DBG 


#define SINGLELAYOUT 0
#define PAGELAYOUT   1
#define PAPERLAYOUT  2

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


//------------debugging helper-----------------
void bboxout(DoubleBBox *bbox,const char *mes=NULL)
{
	if (mes) cout <<mes;
	cout <<" x:"<<bbox->minx<<','<<bbox->maxx<<" y:"<<bbox->miny<<','<<bbox->maxy<<endl;
}



//------------------------------- VObjContext ---------------------------
/*! \class VObjContext
 * \brief Expansions of Laxkit:ObjectContext to include a whole tree for where an object is.
 *
 * This is used in LaidoutViewport to specify the objects that it holds.
 */
/*! \var Laxkit::NumStack<int> VObjContext::context
 * \brief How the object fits into things.
 * <pre>
 *  0 = spread number, currently, 0 means on the scratchboard, and 1 means the current spread.
 *      but in future might implement some sort
 *      of infinite scroll, where all spreads are accessible, and you can edit objects
 *      just by zooming in appropriately.
 *  1 = index of page in spread->pagestack, or in limbo stack if is scratchboard
 *  2 = index of current layer in page
 *  3 = index of object in current layer
 *  4 and above = index in any Group or other containing object.
 * </pre>
 */
/*! \fn int VObjContext::limboi()
 * \brief Return index in limbo stack, if context refers to limbo.
 *
 * If context.e[0]==0 (spread==limbo spread) then return context.e[1], else return -1.
 */
/*! \fn int VObjContext::level()
 * \brief Return index level corresponding to what would be a placement context.
 *
 * If obj==0, this means <tt>context.n-2</tt>, otherwise <tt>context.n-1</tt>.
 */
/*! \fn int VObjContext::layer()
 * \brief Return index of layer in a page, if exists, else -1.
 *
 * If object is in limbo, then return -1. Otherwise, return context.e[3].
 */
/*! \fn int VObjContext::layeri()
 * \brief Return index of obj in the layer of a page if possible, else -1.
 *
 * If object is in limbo, then return -1. Otherwise, return context.e[4].
 */
/*! \fn int VObjContext::spreadpage()
 * \brief Return index of the object's page in the spread->pagestack, if possible, else -1.
 *
 * If object is in limbo, then return -1. Otherwise, return context.e[1].
 */
/*! \fn int VObjContext::spread()
 * \brief Return index of the object's spread, if possible (context.e(0)), else -1.
 */
/*! \fn void VObjContext::clear()
 * \brief Flush context, and set obj to NULL.
 */
/*! \fn int VObjContext::operator==(const ObjectContext &oc)
 * \brief Return isequal(oc).
 */
//class VObjContext : public LaxInterfaces::ObjectContext
//{
// public:
//	FieldPlace context;
//	VObjContext() { obj=NULL; context.push(0); }
//	virtual ~VObjContext() {}
//	virtual int isequal(const ObjectContext *oc);
//	virtual int operator==(const ObjectContext &oc) { return isequal(&oc); }
//	virtual VObjContext &operator=(const VObjContext &oc);
//	virtual int set(LaxInterfaces::SomeData *nobj, int n, ...);
//	virtual void clear() { obj=NULL; context.flush(); context.push(0); }
//	virtual void push(int i,int where=-1);
//	virtual int pop(int where=-1);
//	
//	virtual int spread() { return context.e(0); }
//	virtual int spreadpage() { if (context.n()>1 && context.e(0)!=0) return context.e(1); else return -1; }
//	virtual int layer() { if (context.n()>2 && context.e(0)!=0) return context.e(2); else return -1; }
//	virtual int layeri() { if (context.n()>3 && context.e(0)!=0) return context.e(3); else return -1; }
//	virtual int limboi() { if (context.n()>1 && context.e(0)==0) return context.e(1); else return -1; }
//	virtual int level() { if (obj) return context.n()-2; else return context.n()-1; }
//};

//! Assignment operator.
VObjContext &VObjContext::operator=(const VObjContext &oc)
{
	obj=oc.obj;
	context=oc.context;
	return *this;
}

//! Push a single value onto the context.
void VObjContext::push(int i,int where)
{
	context.push(i,where);
}

//! Pop a single value onto the context. Does not clear obj.
int VObjContext::pop(int where)
{
	return context.pop(where);
}

//! Set obj to nobj, and context to the supplied n items.
int VObjContext::set(LaxInterfaces::SomeData *nobj, int n, ...)
{
	obj=nobj;
	context.flush();
	va_list ap;
	va_start(ap,n);
	for (int c=0; c<n; c++) context.push(va_arg(ap,int));
	va_end(ap);
	return context.n();
}


//! Called by operator==(). Return indication of equality between oc and this.
/*! If oc is not an VObjContext, then 0 is returned.
 *
 * Returns 1 for obj match only, 2 for context match only, 3 for both match, 0 for no match.
 * 
 * \todo  *** must check through LaidoutViewport to make sure this is being used correctly!!
 */
int VObjContext::isequal(const ObjectContext *oc)
{
	int match=0;
	const VObjContext *loc=dynamic_cast<const VObjContext *>(oc);
	if (!loc) return 0;
	if (obj==oc->obj) match++;
	if (context==loc->context) match+=2;
	return match;
}

//------------------------------- LaidoutViewport ---------------------------
/*! \class LaidoutViewport
 * \brief General viewport to pass to the ViewerWindow base class of ViewWindow
 *
 * Creates different type of ObjectContext so that curobj,
 * firstobj, and foundobj are all VObjContext instances which shadow the ViewportWindow
 * variables of the same names.
 * 
 * *** need to check through that all the searching stuff, and cur obj/page/layer stuff
 * are reset where appropriate.. And what about when the doc structure is modified outside
 * of this viewportwindow? there should be a mechanism to ensure that viewportwindow does
 * not attempt to access anything that might not be there anymore.... 
 *
 * // *** there should probably be some checking mechanism to ensure that 
 * // data that no longer exists doesn't get accessed.. maybe a "last modified time"
 * // field for the document?? mutex lock on the object tree?
 * 
 * \todo *** need to be able to work only on current zone or only on current layer...
 * a zone would be: limbo, imposition specific zones (like printer marks), page outline,
 * the spread itself, the current page only.. the zone could be the objcontext->spread()?
 * 
 * \todo *** might be useful to have more things potentially represented in curobj.spread()..
 * possibilities: limbo, main spread, printer marks, other spreads...
 *
 * \todo *** in paper spread view, perhaps that is where a spread()==printer marks is useful..
 * also, depending on the imposition, there might be other operations permitted in paper spread..
 * like in a postering imposition, the page data stays stationary, but multiple paper outlines can latch
 * on to different parts of it...(or perhaps the page moves around, not paper)
 */
/*! \var Page *LaidoutViewport::curpage
 * \brief Pointer to the current page.
 *
 * New objects and such are plopped down onto this page.
 */
/*! \var VObjContext LaidoutViewport::curobj
 * \brief Indices where the current object is. 
 *
 * If curobj.obj==NULL, then curobj only
 * holds where new objects are to be plopped down to.
 */
/*! \var int LaidoutViewport::transformlevel
 * \brief Describes to what new data from interfaces should be mapped (see LaidoutViewport::ectm).
 *
 * The dp transform always corresponds to the total view. The spread transform is usually identity.
 * Usually the level will be 3 (for layer), unless a group is the active object.
 * <pre>
 * 0.  view     <--If transformlevel==0 then ectm is identity.
 * 1.   spread  <--usually is identity for this too, currently spread is always placed at real origin
 * 2.    page   <--this level has ectm==page outline transform in spread->pagestack
 * 3.     layer  <-- usually the transform for layer is identity, might allow layers to move later on
 * 4.      obj    <-- obj levels change with dif. objects..
 * 5.       obj
 * 6.        obj
 * 7...     ...
 * </pre>
 */
/*! \var double LaidoutViewport::ectm[6]
 * \brief The extended transform.
 *
 * This is the transform that is in addition to the view, and what its contents are is
 * described by transformlevel.
 */
/*! \var int LaidoutViewport::viewmode
 * \brief 0=single, 1=Page layout, 2=Paper layout
 *
 * \todo ***perhaps add the object space editor here, that would be viewmode 3. All else goes
 * away, and only that object and its tool are there. If other tools are selected during object
 * space editing and new objects are put down, then the curobj becomes a group with the 
 * former curobj part of that group.
 */
/*! \var Group LaidoutViewport::limbo
 * \brief Essentially the scratchboard, which persists even when changing spreads and pages.
 */
/*! \fn int LaidoutViewport::n()
 * \brief  Return 2 if spread exists, or 1 for just limbo.
 */
/*! \fn Laxkit::anObject *LaidoutViewport::object_e(int i)
 * \brief  Return pointer to limbo if i==0, and spread if i==2.
 */
/*! \var int LaidoutViewport::searchmode
 * \brief 0==none, 1==FindObject, 2==SelectObject(prev/next)
 */
//--------------------------------
//class LaidoutViewport : public LaxInterfaces::ViewportWindow
//{
//	char lfirsttime;
// protected:
//	unsigned int drawflags;
//	int viewmode,searchmode;
//	int showstate;
//	int transformlevel;
//	double ectm[6];
//	XdbeBackBuffer backbuffer;
//	Group limbo;
//	virtual void setupthings(int topage=-1);
//	virtual void LaidoutViewport::setCurobj(VObjContext *voc);
//	virtual void LaidoutViewport::findAny();
//	virtual int nextObject(VObjContext *oc);
//	virtual void transformToContext(double *m,FieldPlace &place,int invert=1);
// public:
//	 //*** maybe these should be protected?
//	char *pageviewlabel;
//	
//	 // these all have to refer to proper values in each other!
//	Document *doc;
//	Spread *spread;
//	Page *curpage;
//	 // these shadow viewport window variables of the same name but diff. type
//	VObjContext curobj,firstobj,foundobj,foundtypeobj;
//	
//	LaidoutViewport(Document *newdoc);
//	virtual const char *whattype() { return "LaidoutViewport"; }
//	virtual ~LaidoutViewport();
//	virtual void Refresh();
//	virtual int init();
//	virtual int CharInput(unsigned int ch,unsigned int state);
//	virtual int MouseMove(int x,int y,unsigned int state);
//	virtual int UseThisDoc(Document *ndoc);
//	
//	virtual int ApplyThis(Laxkit::anObject *thing,unsigned long mask);
//	
//	virtual flatpoint realtoscreen(flatpoint r);
//	virtual flatpoint screentoreal(int x,int y);
//	virtual double Getmag(int c=0);
//	virtual double GetVMag(int x,int y);
//
//	virtual const char *Pageviewlabel();
//	virtual void Center(int w=0);
//	virtual int NewData(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext **oc=NULL);
//	virtual int SelectPage(int i);
//	virtual int NextSpread();
//	virtual int PreviousSpread();
//	
//	virtual int ChangeObject(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext *oc);
//	virtual int LaidoutViewport::SelectObject(int i);
//	virtual int FindObject(int x,int y, const char *dtype, 
//					LaxInterfaces::SomeData *exclude, int start,
//					LaxInterfaces::ObjectContext **oc);
//	virtual void ClearSearch();
//	virtual int ChangeContext(int x,int y,LaxInterfaces::ObjectContext **oc);
//	
//	virtual const char *SetViewMode(int m);
//	virtual int PlopData(LaxInterfaces::SomeData *ndata);
//	virtual void postmessage(const char *mes);
//	virtual int DeleteObject();
//	virtual int ObjectMove(LaxInterfaces::SomeData *d);
//	virtual int CirculateObject(int dir, int i,int objOrSelection);
//	virtual int validContext(VObjContext *oc);
//	virtual void clearCurobj();
//	virtual int locateObject(LaxInterfaces::SomeData *d,FieldPlace &place);
//	virtual int n() { if (spread) return 2; return 1; }
//	virtual Laxkit::anObject *object_e(int i);
//	virtual int curobjPage();
//};

//! Constructor, set up initial dp->ctm, init various things, call setupthings(), and set workspace bounds.
LaidoutViewport::LaidoutViewport(Document *newdoc)
	: ViewportWindow(NULL,"laidoutviewport",ANXWIN_HOVER_FOCUS|VIEWPORT_ROTATABLE,0,0,0,0,0)
{
	showstate=1;
	backbuffer=0;
	lfirsttime=1;
	drawflags=DRAW_AXES;
	doc=newdoc;
	dp->NewTransform(1.,0.,0.,-1.,0.,0.); //***this should be adjusted for physical dimensions of monitor screen
	
	transformlevel=0;
	transform_set(ectm,1,0,0,1,0,0);
	spread=NULL;
	curpage=NULL;
	pageviewlabel=NULL;
	searchmode=0;
	viewmode=-1;
	SetViewMode(PAGELAYOUT);
//	SetViewMode(SINGLELAYOUT);//*** note that viewwindow has a button for this, which defs to PAGELAYOUT
	//setupthings();
	
	 // Set workspace bounds.
	if (newdoc && newdoc->docstyle && newdoc->docstyle->imposition) {
		DoubleBBox bb;
		newdoc->docstyle->imposition->GoodWorkspaceSize(1,&bb);
		dp->SetSpace(bb.minx,bb.maxx,bb.miny,bb.maxy);
		//Center(); //this doesn't do anything because dp->Minx,Maxx... are 0
	}
}

//! Delete spread, doc and page are assumed non-local.
/*! Also calls limbo.flush().
 */
LaidoutViewport::~LaidoutViewport()
{
	if (spread) delete spread;

	 //checkin limbo objects vie limbo's Group::flush()
	limbo.flush();
}

//! Replace existing doc with this doc.
/*! Return 0 for success, nonzero error.
 */
int LaidoutViewport::UseThisDoc(Document *ndoc)
{
	if (!ndoc) return 1;
	ClearSearch();
	clearCurobj();
	curpage=NULL;

	doc=ndoc;
	setupthings();
	needtodraw=1;
	return 0;
}

Laxkit::anObject *LaidoutViewport::object_e(int i)
{
	if (i==0) return &limbo;
	if (i==1 && spread) return spread;
	return NULL;
}

//! Map real to the current page, layer, or object space.
/*! This assumes that dp currently has plain view coordinates.
 *
 * screen = real * ectm * ctm
 */
flatpoint LaidoutViewport::realtoscreen(flatpoint r)
{
	//dp:return flatpoint(ctm[4] + ctm[0]*p.x + ctm[2]*p.y, ctm[5]+ctm[1]*p.x+ctm[3]*p.y);
	double tt[6];
	transform_mult(tt,ectm,dp->Getctm());
	r=transform_point(tt,r);
	return (r);
}

//! Map real to the current page, layer, or object space.
/*! This assumes that dp currently has plain view coordinates.
 * 
 * real = screen * ictm * (ectm^-1)
 */
flatpoint LaidoutViewport::screentoreal(int x,int y)
{
	//dp:return flatpoint(ictm[4] + ictm[0]*p.x + ictm[2]*p.y, ictm[5]+ictm[1]*p.x+ictm[3]*p.y);
	double tt[6],ttt[6];
	transform_invert(tt,ectm);
	transform_mult(ttt,dp->Getictm(),tt);
	flatpoint r=transform_point(ttt,flatpoint((double)x,(double)y));
	return r;
}

//! Map real to the current page, layer, or object space.
/*! This assumes that dp currently has plain view coordinates.
 */
double LaidoutViewport::Getmag(int c)
{
	//dp:if (c) return sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]);
	//        return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
	return dp->Getmag(c);
}

//! Map real to the current page, layer, or object space.
/*! This assumes that dp currently has plain view coordinates.
 */
double LaidoutViewport::GetVMag(int x,int y)
{
	//dp: flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
	//        return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
	return dp->GetVMag(x,y);
}

//! Select the spread with page number greater than current page not in current spread.
/*! Returns the current page index on success, else a negative number.
 *
 * ***this is wrong for paper layout!! maybe setupthings(page, or paper)
 */
int LaidoutViewport::NextSpread()
{
	if (!spread) return -1;
	int c,i=curobjPage()+1;
	
	while (1) {
		for (c=0; c<spread->pagestack.n; c++) {
			if (i==spread->pagestack.e[c]->index) { i++; c=-1; }
		}
		if (c==spread->pagestack.n) return SelectPage(i);
	}
	return -1;
}

//! Select the spread with page number less than current page not in current spread.
/*! Returns the current page index on success, else a negative number.
 *
 * ***this is wrong for paper layout!! maybe setupthings(page, or paper)
 */
int LaidoutViewport::PreviousSpread()
{
	if (!spread) return -1;
	int c,i=curobjPage()-1;
	while (1) {
		for (c=0; c<spread->pagestack.n; c++) {
			if (i==spread->pagestack.e[c]->index) { i--; c=-1; }
		}
		if (c==spread->pagestack.n) {
			if (i>=0) return SelectPage(i);
			return -1;
		}
	}
	return -1;
}


//! Set the view style to single, pagelayout, or paperlayout.
/*! Updates and returns pageviewlabel. This is a string like "Page: " or
 * "Spread[2-4]: " that is suitable for the label of a NumSlider.
 */
const char *LaidoutViewport::SetViewMode(int m)
{
	DBG cout <<"---- setviewmode:"<<m<<endl;
	if (m!=viewmode) {
		viewmode=m;
		setupthings(curobjPage());
	}
	needtodraw=1;
	return Pageviewlabel();
}

//! Update and return pageviewlabel.
/*! This is a label that says what pages are in the current spread, 
 * and also which is the current page.
 */
const char *LaidoutViewport::Pageviewlabel()
{
	if (viewmode==SINGLELAYOUT) {
		makestr(pageviewlabel,"Page: ");
	} else { // figure out "Spread [2-4]: "
		if (spread) { 
			int *pages=spread->pagesFromSpread();
			int c=0;
			char num[20];
			makestr(pageviewlabel,"Spread [");
			while (pages[c]!=-2) {
				if (pages[c+1]==-1) { // single page
					sprintf(num,"%d%s",pages[c]+1,(pages[c+2]==-2?"":","));
				} else { // range of pages
					sprintf(num,"%d-%d%s",pages[c]+1,pages[c+1]+1,(pages[c+2]==-2?"":","));
				}
				c+=2;
				appendstr(pageviewlabel,num);
			}
			appendstr(pageviewlabel,"]: ");
			delete[] pages;
		} else { makestr(pageviewlabel,"Spread: "); }
	}
	return pageviewlabel;
}

//! Basically, post a message to viewer->mesbar.
void LaidoutViewport::postmessage(const char *mes)
{
	ViewerWindow *vw=dynamic_cast<ViewerWindow *>(win_parent);
	if (!vw) return;
	if (vw->GetMesbar()) vw->GetMesbar()->SetText(mes);
}

//! Called from constructor and from SelectPage. Define curpage, spread, curobj context.
/*! If topage==-1, sets curpage to doc->Curpage().
 * If topage!=-1 sets up with page topage.
 *
 * curobj is set to have its obj==NULL and context corresponding to topage and layer==0.
 * 
 * Also tries to make ectm be the transform to the current page from view coords,
 * so screenpoint = realpoint * ectm * dp->ctm.
 * 
 * Then refetchs the spread for that page. Curobj is set to the context that items should
 * be plopped down into. This is usually a page and a layer. curobj.obj is set to NULL.
 *
 * \todo ***potentially sometimes different paper spreads have references to the same page
 * which might screw up the setviewmode/setupthings system...
 */
void LaidoutViewport::setupthings(int topage)//topage=-1
{
	if (!doc) return;
	
	 // set curobj to proper value
	 // Also call Clear() on all interfaces
	if (topage==-1) {
		 // initialize from nothing
		topage=doc->curpage;
	} 
	if (topage>=doc->pages.n) topage=doc->pages.n;
	else if (topage<0) topage=0;

	for (int c=0; c<interfaces.n; c++) interfaces.e[c]->Clear();
	ClearSearch();
	clearCurobj(); //*** clear temp selection group??
	curpage=NULL;

	 // should always delete and re-new the spread, even if page is in current spread.
	 // since page might be in spread, but in old viewmode...
	if (spread) { delete spread; spread=NULL; } 

	 // retrieve the proper spread according to viewmode
	if (!spread && doc->docstyle && doc->docstyle->imposition) {
		if (viewmode==PAGELAYOUT) spread=doc->docstyle->imposition->PageLayout(topage);
		else if (viewmode==PAPERLAYOUT) {
			int p=doc->docstyle->imposition->PaperFromPage(topage);
			spread=doc->docstyle->imposition->PaperLayout(p);
		} else spread=doc->docstyle->imposition->SingleLayout(topage); // default to singlelayout
	}

	 // setup ectm (see realtoscreen/screentoreal/Getmag), and find spageindex
	 // ectm is transform between dp and the current page (or current spread if no current page)
	transformlevel=0;
	transform_set(ectm,1,0,0,1,0,0);
	
	if (!spread) return;
	curpage=doc->pages.e[topage];
	
	int spageindex;
	for (spageindex=0; spageindex<spread->pagestack.n; spageindex++) 
		if (spread->pagestack.e[spageindex]->index==topage) break;
	if (spageindex==spread->pagestack.n) {
		DBG cout <<"****** topage "<<topage<<" not found in spread!!!"<<endl;
		spageindex=-1;
	}
	
	//****redo this to use setCurobj...
	 // set plop to spread-pagelocindex.
	curobj.set(NULL,2, 1,spageindex);
	
	 // get ectm to be transform from viewer coordinates to the current page's coordinates
	double tt[6];
	transform_set(tt,1,0,0,1,0,0);
	//transform_mult(tt,***spreadtrans,ectm); // apply spread transform (assume identity for now..)
	if (spageindex>=0 && spageindex<spread->pagestack.n) {
		transform_mult(ectm,spread->pagestack.e[spageindex]->outline->m(),tt); // apply page transform
		 //layer transform is assumed to be identity
		curobj.push(0); // apply layer 0 in topage
		
		transformlevel=3;
	} else {
		transformlevel=1; //ectm transforms to spread only
	}
	
	 // apply further groups...
	//...like any saved cur group for that page or layer?...
	
	Center(1);
}

//! Insert ndata into the curobj context.
/*!  Currently, just calls NewData(ndata,NULL), calls ViewWindow::SelectToolFor on it, 
 * and sets needtodraw=1.
 * Returns 1 if data was plopped, 0 if not.
 * If data is plopped, then its count is incremented.
 *
 * \todo *** need a mechanism to rescale the object from one context to
 * the curobj context.
 *
 * \todo *** need option to plop near mouse if mouse on screen
 */
int LaidoutViewport::PlopData(LaxInterfaces::SomeData *ndata)
{
	if (!ndata) return 0;
	NewData(ndata,NULL);
	if (curobj.obj) {
		 // activate right tool for object
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
		if (viewer) viewer->SelectToolFor(curobj.obj->whattype(),curobj.obj);
	}
	needtodraw=1;
	return 1;
}

//! Delete curobj or current selection.
/*! In addition to checking in, also removes object(s) from the document.
 *
 * Return 1 if object deleted, -1 if no current object, or 0 for object not deleted.
 * 
 * \todo *** implement curselection, this would erase all of that, not just curobj
 * 
 * \todo *** ultimately, must have insert/delete functions in Document.
 *
 * \todo *** if curobj is a nested object somewhere, must implement Group->findandremove..
 * 
 * \todo *** currently, only deletes if object is right in current layer, not nested
 */
int LaidoutViewport::DeleteObject()
{
	 // calls Clear(d) on all interfaces, checks in data
	SomeData *d=curobj.obj;
	if (!d) return -1;

	 // remove d from wherever it's at:
	if (curobj.spread()==0) { //in limbo
		limbo.remove(curobj.limboi());
	} else { // is somewhere in document
		Group *g=curpage->layers.e[curobj.layer()];
		g->remove(g->findindex(d));
		//***g->findandremove(d,curobj.context,4); //removes at offset 4 in context
		//*** when deleting a group, deleting one might make the other objs out
		//of whack, have to either modify their context or do fresh search for them..
	}
	
	 // clear d from interfaces and check in 
	InterfaceWithDp *i;
	for (int c=0; c<interfaces.n; c++) {
		if (i=dynamic_cast<InterfaceWithDp *>(interfaces.e[c]), i) i->Clear(d);
		else interfaces.e[c]->Clear();
	}
	clearCurobj(); //this calls dec_count() on the object
	laidout->notifyDocTreeChanged(this);
	//ClearSearch();
	needtodraw=1;
	return 1;
}

//! Shove this data onto curobj context.
/*! This function should only be called directly by interfaces. It is an aid to 
 * set up curobj to some object found in a search, and for new data the interfaces create.
 * If the object already exists in the document, then do not increase its count. Otherwise,
 * add it and set its count accordingly.
 *
 * Points curobj at the object and sets oc to point to it.
 * It does NOT select the tool for the object to be active. That
 * is the job of ChangeObject.
 *
 * Returns -1 for spread or curpage not exist, 1 if d is NULL and oc is invalid.
 * -2 if supplied context is not VObjContext. Else 0 for success.
 *  
 * Please note that ChangeObject() calls this function, as do many interfaces via 
 * Laxkit::InterfaceWithDp::newData(). Also note that this function simply inserts data 
 * into the object tree somewhere. It likely does not scale appropriately for 
 * arbitrary data. For a generic data insertion function please us PlopData().
 *
 * This is called from interfaces when they select new data, OR create new data.
 * If oc is not NULL, then if it is a valid pointer for d, then make curobj correspond to it.
 * If oc Is NULL, search the document to find d. If d is not found, then insert the data
 * in the current context.
 *
 * If d is not NULL, but there is another object in oc, then d us used, but oc->context
 * is also used.
 */
int LaidoutViewport::NewData(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext **oc)
{
	 //*** this is probably wrong, oc might itself be where to put
	 //*** d, rather than the context for d itself
	if (d && oc && !(*oc)->obj) (*oc)->obj=d;
	else if (!d && oc) d=(*oc)->obj;
	if (!d) return 1;
	// now d and oc->obj should be same (if oc) 
	 
	VObjContext *context;
	if (oc){
		context=dynamic_cast<VObjContext *>(*oc); 
		if (!context) return -2; //supplied context was wrong type
	} else context=NULL;
	if (context && !validContext(context)) context=NULL;
	if (!context) context=new VObjContext;
	if (!context->obj) context->obj=d;

	 //If oc not provided or is invalid, but the object is in the spread somwhere
	if (!(oc && context==*oc) && locateObject(d,context->context)>0) {
		 // object already exists in spread
		setCurobj(context); //incs count 1 for the curobj ref
		if (curobj.obj) curobj.obj->inc_count();//this is the normal NewData count
		if (oc) *oc=&curobj;
		return 0;
	}
	
	// now oc is either valid and context==oc, or invalid and context!=oc and obj not in spread
	
	if (oc && *oc==context) { // supplied oc was valid
		setCurobj(context); //incs count 1 for the curobj ref
		if (curobj.obj) curobj.obj->inc_count();//this is the normal NewData count
		if (oc) *oc=&curobj;
		return 0;
	}
	
	 // obj not in spread, must insert the presumed new data d
	int i=-1;
	if (!spread || !curpage || curobj.layer()<0 || curobj.layer()>=curpage->layers.n) {
		 // add object to limbo, this likely does not install proper transform
		i=limbo.pushnodup(d,0);
		context->set(d,2,0,i>=0?i:limbo.n()-1);
	} else {
		 // push onto cur spread/page/layer
		 //*** this should push on curobj level, not simply page->layer
		i=curpage->layers.e[curobj.layer()]->pushnodup(d,0);//this is Group::pushnodup()
		context->set(d,4, 
					1,
					curobj.spreadpage(),
					curobj.layer(),
					i>=0 ? i : curpage->layers.e[curobj.layer()]->n()-1);
	}
	
	//*** should clear curselection
	laidout->notifyDocTreeChanged(this);
	setCurobj(context);
	if (curobj.obj) curobj.obj->inc_count();//this is the normal NewData count
	if (oc) *oc=&curobj;
	return 0;
}

//! -2==prev, -1==next obj, else select obj i??
/*! Return 0 if curobj not changed, else nonzero.
 *
 * \todo *** this is rather broken just at the moment
 *
 * \todo *** could have search mode, one mode is search under particular coords, another
 * mode is step through all on screen objects, another step through all in spread.
 */
int LaidoutViewport::SelectObject(int i)
{//***
	if (!curobj.obj) {
		findAny();
		if (firstobj.obj) setCurobj(&firstobj);
	} else if (i==-2) { //prev
		VObjContext prev;
		prev=curobj;
		if (searchmode!=2) {
			ClearSearch();
			firstobj=curobj;
			searchmode=2;
		}
		if (nextObject(&prev)!=1) {
			firstobj=curobj;
			prev=curobj;
			if (nextObject(&prev)!=1) { searchmode=0; return 0; }
		}
		setCurobj(&prev);
	} else if (i==-1) { //next
		cout <<"***next obj: imp me!"<<endl;
	} else return 0;
	
	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
	if (viewer) viewer->SelectToolFor(curobj.obj->whattype(),curobj.obj); 		
	return 1;
}

//! Change the current object to be d.
/*! Item must be on the spread or in limbo already. Does not push objects from here. For
 * that, call NewData(). 
 *
 * If d!=NULL, then try to make that object the current object. It must be within
 * the current spread somewhere. If d==NULL, then the same goes for where oc points to.
 *
 * Returns 1 for current object changed, otherwise 0 for not changed or d not found.
 *
 * \todo *** for laxkit also, but must have some mechanism to optionally pass the LBDown grab
 * to new interface in control of new object..
 */
int LaidoutViewport::ChangeObject(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext *oc)
{
	if (d==NULL || (d && oc && d==oc->obj)) { // use oc
		if (!oc) return 0;
		
		VObjContext *voc=dynamic_cast<VObjContext *>(oc);
		if (voc==NULL || !validContext(voc)) return 0;
		setCurobj(voc);
	} else { 
		 // d!=NULL, Search for object d in current spread and limbo
		FieldPlace mask;
		int c=locateObject(d,mask);
		if (c<=0) {  // obj not found...
			return 0;
		}
		curobj.context=mask;
		curobj.obj=d;
		setCurobj(NULL); //updates ectm but doesn't set curobj=NULL
	}
	
	 // makes sure curtool can take it, and makes it take it.
	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
	if (viewer) viewer->SelectToolFor(curobj.obj->whattype(),curobj.obj); 		

	return 1;
}

//! Find object in current spread underneath screen coordinate x,y.
/*! If an interfaces receives a lbdown outside of their object, then it would
 * call viewport->FindObject, which will possibly return an object that the 
 * interface can handle. If so, the interface would call NewData with it. The interface
 * can keep searching until it finds one it can handle.
 * If FindObject does not return something the interface can handle, then
 * it should call ChangeObject(that other data).
 * 
 * If an object of the proper type is found, its context is put in oc and 1 is returned.
 * Internally, this object will be in LaidoutViewport::foundtypeobj. This context gets
 * reset with each call to FindObject.
 *
 * If an object matching the given (x,y) was found, but was of the wrong type, 
 * that object can be found 
 * in LaidoutViewport::foundobj. When such an object is found and a search is
 * over, and an object of the correct type was not found, then -1 is returned and
 * foundobj is put in oc.
 *
 * If no matching object was found at all,
 * then 0 is returned and NULL is put in oc.
 *
 * \todo *** currently searches ALL available objects in the spread and scratchboard. must
 * be able to search only in current layer, and exclude locked/invisible objects...
 */
int LaidoutViewport::FindObject(int x,int y, 
										const char *dtype,
										LaxInterfaces::SomeData *exclude,
										int start,
										LaxInterfaces::ObjectContext **oc)
{
	
	 //init the search, if necessary
	VObjContext nextindex;
	if (searchmode!=1 || start || x!=searchx || y!=searchy) { //init search
		foundobj.clear();
		firstobj.clear();
		 
		 // Set up firstobj
		FieldPlace context;
		if (exclude) {
			if (locateObject(exclude,context)) {
				firstobj.context=context;
				firstobj.obj=exclude;
			} else firstobj=curobj;
		} else {
			firstobj=curobj; //***need validation check
		}
		if (!firstobj.obj) findAny();
		if (!firstobj.obj) return 0;
		nextindex=firstobj;
		
		searchx=x;
		searchy=y;
		searchtype=NULL;
		if (start==2 || start==0) exclude=NULL;
		start=1;
		searchmode=1;
		
		if (exclude && nextindex.obj==exclude) nextObject(&nextindex);
	} else {
		nextindex=foundtypeobj;
	}
	foundtypeobj.clear(); // this one is always reset?
	if (!firstobj.obj) return 0;
	if (dtype) searchtype=dtype;

	if (!start) nextObject(&nextindex);

	 // nextindex now points to the next object to consider.
	 
	flatpoint p,pp;
	p=dp->screentoreal(x,y); // so this is in viewer coordinates
	DBG cout <<"lov.FindObject: "<<p.x<<','<<p.y<<endl;

	double m[6];
	firstobj.context.out("firstobj");
	
	//while (start || (!start && !(nextindex==firstobj))) {
	int nob=1;
	while (nob==1) {
		if (start) start=0;
		if (nextindex.obj==exclude) {
			nob=nextObject(&nextindex);
			continue;
		}
		
		 //transform point to be in nextindex coords
		transformToContext(m,nextindex.context);

		pp=transform_point(m,p);
		DBG cout <<"lov.FindObject oc: "; nextindex.context.out("");
		DBG cout <<"lov.FindObject pp: "<<pp.x<<','<<pp.y<<"  check on "
		DBG		<<nextindex.obj->object_id<<" ("<<nextindex.obj->whattype()<<") "<<endl;

		if (nextindex.obj->pointin(pp)) {
			DBG cout <<" -- found"<<endl;
			if (!foundobj.obj) foundobj=nextindex;
			if (searchtype && strcmp(nextindex.obj->whattype(),searchtype)) {
				nob=nextObject(&nextindex);
				continue;
			}
			 // matching object found!
			foundtypeobj=nextindex;
			if (oc) *oc=&foundtypeobj;
			foundtypeobj.context.out("  foundtype");//for debugging
			return 1;
		}
		DBG cout <<" -- not found in "<<nextindex.obj->object_id<<endl;
		nob=nextObject(&nextindex);
	}
	 
	 // no object found, search over
	firstobj.clear(); //this is a signal for future searches that end was reached
	if (foundobj.obj) {
		if (oc) *oc=&foundobj;
		return -1;
	} 
	if (oc) *oc=NULL;
	return 0; // search ended, found nothing
}

//! Make a transform that takes a point from viewer past all objects in place.
/*! *** perhaps return how many transforms were applied, so 0 means success, 
 * >0 would mean that many ignored. maybe have some other special 
 * return for invalid context?
 *
 * If invert!=0, then return a transform that transforms a point from the place
 * to the viewer, which is analogous to the SomeData::m().
 * 
 * ***work this out!! Say an object is g1.g2.g3.obj corresponding to a place something like
 * "1.2.3", then m gets filled with g3->m()*g2->m()*g1->m(),
 * where the m() are the matrix as used in Laxkit::Displayer discussion.
 *
 * *** since no multiple nesting yet, i keep confusing myself whether these
 * matrices are multiplied correctly!!
 * 
 * \todo *** occassionally, there is a PageLocation with a proper index,
 * but its page element is NULL, must find where this happens!!
 */
void LaidoutViewport::transformToContext(double *m,FieldPlace &place, int invert)
{
	transform_set(m,1,0,0,1,0,0);
	int i=place.e(0);
	Group *g=NULL;
	double mm[6];
	if (i==0) { 
		 // in limbo, which adds only identity, so proceed to object transforms
		i++;
		g=dynamic_cast<Group *>(limbo.e(place.e(1)));
	} else if (i==1 && spread) {
		 // in a spread, must apply the page transform
		 // spread->pagelocation->layer->layeri->objs...
		i=place.e(1);
		if (i>=0 && i<spread->pagestack.n) {
			PageLocation *pl=spread->pagestack.e[i];
			if (pl && pl->outline) {
				if (!pl->page) {
					if (pl->index>=0 && pl->index<doc->pages.n) pl->page=doc->pages.e[pl->index];
					DBG else cout <<"*-*-* bad index in pl, but requesting a transform!!"<<endl;
				}
				transform_copy(m,pl->outline->m());
				ObjectContainer *o;
				if (pl->page) o=dynamic_cast<ObjectContainer *>(pl->page->object_e(place.e(2)));
				  else o=NULL;
				if (o) g=dynamic_cast<Group *>(o->object_e(place.e(3)));
				i=4;
			}
		}
	} else {}
	
	while (g && i<place.n()) {
		transform_mult(mm,g->m(),m);
		transform_copy(m,mm);
				
		g=dynamic_cast<Group *>(g->e(place.e(i++)));
	}
	if (invert) {
		transform_invert(mm,m); 
		transform_copy(m,mm);
	}
}

//! Step oc to next object. 
/*! This is used to step through object tree no matter how deep.
 * For groups, steps through the group's contents first, then the group
 * itself, then moves on to a sibling of that group, etc.
 *
 * If oc points to similar position in firstobj, then it is assumed there has been
 * wrap around at this level. This also implies that all subobjects of what oc points
 * to have already been stepped through.
 * 
 * Returns 0 if there isn't a next obj for some reason.
 */
int LaidoutViewport::nextObject(VObjContext *oc)
{
	int c;
	anObject *d;
	do {
		c=ObjectContainer::nextObject(oc->context,firstobj.context,0,&d);
		oc->obj=dynamic_cast<SomeData *>(d);
		if (c!=1 || !(oc->obj)) return 0;
		if (!validContext(oc)) {
			DBG cout <<"**** damnation, invalid context found in lov.nextObject!"<<endl;//debugging
			//exit(1);
		}
		
		 //if is NOT (limbo or page or page->layer or spread) then break, else continue.
		if (!(oc->spread()==0 && oc->context.n()<=1 ||   
			oc->spread()==1 && oc->context.n()<=3)) {
			oc->context.out("  lov-next");
			return 1;
		}
	} while (1);
	
	return 0;
}

//! Return place.n if d is found in the displayed pages or in limbo somewhere, and put location in place.
/*! Flushes place, it does not append to an existing spot.
 *
 * If object is not found, then return 0, despite whatever place.n is.
 */
int LaidoutViewport::locateObject(LaxInterfaces::SomeData *d,FieldPlace &place)
{
	place.flush();
	 // check limbo
	if (limbo.n()) {
		if (limbo.contains(d,place)) {
			place.push(0,0);
			return place.n();
		}
	}

	 //check displayed pages
	if (!spread || !spread->pagestack.n) return 0;
	int page;
	Page *pagep;
	for (int spage=0; spage<spread->pagestack.n; spage++) {
		page=spread->pagestack.e[spage]->index;
		pagep=doc->pages.e[page];
		place.flush();
		if (pagep->contains(d,place)) { // this pushes location onto top of place..
			place.push(spage,0);
			place.push(1,0);
			return place.n();
		}
	}
	return 0;
}

//! Initialize firstobj to the first object the viewport can find.
void LaidoutViewport::findAny()
{
	int c,c2;
	SomeData *obj=NULL;
	firstobj.clear();
	Page *page;
	if (spread) {
		for (c=0; c<spread->pagestack.n; c++) {
			page=dynamic_cast<Page *>(spread->object_e(c));
			if (!page) continue;
			for (c2=0; c2<page->layers.n; c2++) {
				DBG cout <<" findAny: pg="<<c<<":"<<spread->pagestack.e[c]->index<<"  has "<<page->layers.e[c2]->n()<<" objs"<<endl;
				if (!page->layers.e[c2]->n()) continue;
				firstobj.set(page->layers.e[c2]->e(0),4, 1,c,c2,0);
				return;
			}
		}
	}
	if (!obj) {
		if (limbo.n()) {
			firstobj.set(limbo.e(0),2,0,0);
			return;
		}	
	}
}

//! Check whether oc is a valid object context.
/*! Assumes oc->obj is what you actually want, and checks oc->context against it.
 * Object has to be in the spread or in limbo for it to be valid.
 *
 * Return 0 if invalid,
 * otherwise return 1 if it is valid.
 */
int LaidoutViewport::validContext(VObjContext *oc)
{
	if (!oc || oc->spread()<0) return 0;
	
	SomeData *d=oc->obj;
	
	if (oc->spread()==0) { // scratchboard/limbo
		if (d==limbo.getanObject(oc->context,1)) return 1;
		return 0;
	}
	
	if (!spread || oc->spread()!=1) return 0;

	int sp=oc->spreadpage();
	if (sp<0 || sp>=spread->pagestack.n) return 0;
	
	int p=spread->pagestack.e[sp]->index;
	if (p<0 || p>=doc->pages.n) return 0;

	int l=oc->layer();
	if (l<0 || l>=doc->pages.e[p]->layers.n) return 0;

	if (d==doc->pages.e[p]->layers.e[l]->getanObject(oc->context,3)) return 1;
	
	return 0;
}

//! Clear firstobj,foundobj,foundobjtype.
void LaidoutViewport::ClearSearch()
{
	firstobj.clear();
	foundobj.clear();
	foundtypeobj.clear();
	searchx=searchy=-1;
	searchtype=NULL;
	searchmode=0;
}

//! Set curobj (if voc!=NULL). Also update ectm.
/*! Incs count of curobj if new obj is different than old curobj.obj
 * and decrements the old curobj.
 * 
 */
void LaidoutViewport::setCurobj(VObjContext *voc)
{
	if (voc) {
		if (curobj.obj && curobj.obj!=voc->obj) {
			curobj.obj->dec_count();
			if (voc->obj) voc->obj->inc_count();
		} else if (!curobj.obj && voc->obj) {
			voc->obj->inc_count();
		}
		curobj=*voc;
	}
	FieldPlace place; 
	place=curobj.context;
	if (curobj.obj) place.pop();
	
	transformToContext(ectm,place,0);
	transformlevel=place.n()-1;
	if (curobj.spread()==0) curpage=NULL;
	else {
		if (curobj.spreadpage()>=0) {
			curpage=spread->pagestack.e[curobj.spreadpage()]->page;
			if (!curpage) {
				DBG cout <<"** warning! in setCurobj, curpage was not defined for curobj context"<<endl;
				curpage=doc->pages.e[spread->pagestack.e[curobj.spreadpage()]->index];
			}
		}
		else curpage=NULL;
	}
	
	DBG if (curobj.obj) cout <<"setCurobj: "<<curobj.obj->object_id<<" ("<<curobj.obj->whattype()<<") ";
	DBG curobj.context.out("setCurobj");//debugging
}

//! Strip down curobj so that it is at most a layer or limbo. Calls dec_count() on the object.
void LaidoutViewport::clearCurobj()
{
	if (curobj.obj) curobj.obj->dec_count();
	if (curobj.spread()==0) { // is limbo
		curobj.set(NULL,1, 0);
		return;
	}
	curobj.set(NULL,3,curobj.spread(),
					  curobj.spreadpage(),
					  curobj.layer());
	setCurobj(NULL);
}

//! *** for debugging, show which page mouse is over..
int LaidoutViewport::MouseMove(int x,int y,unsigned int state)
{
	if (!buttondown) {
		int c=-1;
		flatpoint p=dp->screentoreal(x,y);
		if (spread) {
			for (c=0; c<spread->pagestack.n; c++) {
				if (spread->pagestack.e[c]->outline->pointin(p)) break;
			}
			if (c==spread->pagestack.n) c=-1;
		}
		DBG cout <<"mouse over: "<<c<<endl;
	}
	return ViewportWindow::MouseMove(x,y,state);
}

//! Call this to update the context for screen coordinate (x,y).
/*! This sets curobj to be at layer level of curobj if possible, otherwise
 * to the top level of whatever page is under screen coordinate (x,y).
 *
 * Sets oc to point to the new curobj.
 *
 * Return 0 for context changed, nonzero for not.
 */
int LaidoutViewport::ChangeContext(int x,int y,LaxInterfaces::ObjectContext **oc)
{
	ClearSearch();
	clearCurobj();
	flatpoint p=dp->screentoreal(x,y);
	if (curobj.spread()==1 && spread->pagestack.e[curobj.spreadpage()]->outline->pointin(p)) {
		curobj.context.out("context change");
		return 1; // context was cur page
	}
	 // else must search for it
	curobj.context.flush();
	int c;
	if (spread) {
		for (c=0; c<spread->pagestack.n; c++) {
			if (spread->pagestack.e[c]->outline->pointin(p)) break;
		}
		if (c<spread->pagestack.n) {
			curobj.context.push(1);
			curobj.context.push(c);
			curobj.context.push(spread->pagestack.e[c]->page->layers.n-1);
			 // apply page transform
			transform_copy(ectm,spread->pagestack.e[c]->outline->m());
		}
	}
	if (curobj.context.n()==0) { // is limbo
		curobj.context.push(0);
		transform_identity(ectm);
	}
	if (curobjPage()>=0) curpage=doc->pages.e[curobjPage()];
		else curpage=NULL;
	if (oc) *oc=&curobj;
	curobj.context.out("context change");
	 
	return 1;
}

/*! Called when moving objects around, this ensures that a move can
 * make the object go to a different page, or to limbo.
 *
 * If d!=curobj.obj, then nothing special is done. If the containing object
 * of d is neither limbo, nor a layer of a page, then nothing special is done.
 *
 * The object changes parents when it is no longer contained in its old parent.
 * If the parent is limbo, then any partial containment transfers the object to 
 * a page. Conversely, if the parent is a layer on a page, the object has to become
 * totally uncontained by the layer for it to enter limbo or another page.
 * Its new parent is the first one found that at least partially contains the object,
 * with a preference of pages over limbo. The object is placed on the top layer 
 * of the page.
 *
 * Returns nonzero if changes made, 0 if no modifications.
 *
 * \todo *** the intersection detection for laidout in general is currently rather poor. goes
 * entirely by the rectangular bounds. need some way to do that as a first check,
 * then check internals some how.. means intersection arbitrary non-convex polygons... something
 * of the kind will be necessary eventually anyway to compute runaround.
 */
int LaidoutViewport::ObjectMove(LaxInterfaces::SomeData *d)
{
	DBG cout <<"ObjectMove "<<d->object_id<<": ";
	DBG curobj.context.out(NULL);
	
	if (!d || d!=curobj.obj) return 0;
	if (curobj.spread()==0 && curobj.context.n()>2) return 0;
	if (curobj.spread()==1 && curobj.context.n()>4) return 0;
	if (curobj.spread()!=0 && curobj.spread()!=1) return 0;
	if (!validContext(&curobj)) return 0;
	DoubleBBox bbox,bbox2;
	SomeData *outline;
	int c,i=-1;
	if (curobj.spread()==1) { // is not limbo object
		 // check whether containing page still contains curobj
		 //*** in future, the outline might become property of page, rather
		 //***than passed around separately
		//page=spread->pagestack.e[curobj.spreadpage()]->page;
		//if (!page) {
		//	cout <<"*-*-* should have been a page in pagestack!"<<endl;
		//	int i=spread->pagestack.e[curobj.spreadpage()]->index;
		//	if (i>=0 && i<doc->pages.n) page=doc->pages.e[i];
		//} //it is assumed that if it is a valid context, then page 
		//  //is real, but maybe not!!
		i=curobj.spreadpage();
		outline=spread->pagestack.e[i]->outline;
		bbox.setbounds(outline);
		bbox2.addtobounds(curobj.obj->m(),curobj.obj);
		if (bbox.intersect(&bbox2)) {
			DBG cout <<"  still on page"<<endl;
			return 0; //still on page
		}
	}
	 // so at this point the object is ok to try to move
	// 
	//search for some page for the object to be in
	double m[6],mm[6];
	for (c=0; c<spread->pagestack.n; c++) {
		if (c==i) continue; // don't check same page twice
		 //*** this is rather poor, but fast and good enough for now:
		outline=spread->pagestack.e[c]->outline;
		bbox.clear();
		bbox.addtobounds(outline->m(),outline);
		transformToContext(m,curobj.context,0);
		transform_mult(mm,curobj.obj->m(),m);
		bbox2.clear();
		bbox2.addtobounds(mm,curobj.obj);
		if (bbox.intersect(&bbox2)) break; //found one
	}
	Page *topage=NULL;
	int tosp=-1;
	if (c==spread->pagestack.n) { // new page not found, obj is to go to limob
		if (curobj.spread()==0) {
			DBG cout <<"  already in limbo"<<endl;
			return 0; //already in limbo
		}
		 //if not page found partially containing object, then transfer to limbo
		DBG cout <<" --moving to limbo "<<endl;
	} else {
		 // found a page for it to be in, so set topage to point to it..
		 //
		DBG cout <<" --moving to spreadpage "<<c<<endl;
		topage=spread->pagestack.e[c]->page;
		if (!topage) {
			DBG cout <<"*** warning! page was not defined in pagestack"<<endl;
			topage=spread->pagestack[c]->page=dynamic_cast<Page *>(doc->object_e(spread->pagestack.e[c]->index));
		}
		tosp=c;
	}
	
	 // pop from old place 
	int islocal;
	transformToContext(m,curobj.context,0);
	if (curobj.spread()==0) {
		 // pop from limbo
		c=limbo.popp(curobj.obj,&islocal); // does not modify obj count
	} else {
		 // pop from old page
		Page *frompage=spread->pagestack.e[curobj.spreadpage()]->page;
		if (!frompage) {
			DBG cout <<"*** warning! page was not defined in pagestack"<<endl;
			frompage=spread->pagestack[curobj.spreadpage()]->page=
				dynamic_cast<Page *>(doc->object_e(spread->pagestack.e[curobj.spreadpage()]->index));
		}
		i=curobj.layer();
		if (i<0 || i>=frompage->layers.n) {
			DBG cout <<"*** warning! bad context in LaidoutViewport::ObjectMove!"<<endl;
			return 0;
		}
		c=frompage->layers.e[i]->popp(curobj.obj,&islocal); // does not modify obj count
	}
	if (c!=1) {
		DBG cout <<"*** warning ObjectMove asked to move an invalid object!"<<endl;
		return 0;
	}
	
	 // push on new place and transform obj->m() to new context
	if (topage) {
		i=topage->layers.e[topage->layers.n-1]->push(d,islocal); //adds 1 count
		d->dec_count();
		curobj.set(curobj.obj,4, 1,tosp,topage->layers.n-1,topage->layers.e[topage->layers.n-1]->n()-1);
		double mmm[6];
		transform_mult(mm,curobj.obj->m(),m);//old m??
		transformToContext(mmm,curobj.context,1); //trans from view to new curobj place
		transform_mult(m,mm,mmm); //(new obj m)=(old obj m)*(old context)*(newcontext)^-1
		transform_copy(curobj.obj->m(),m);
	} else {
		transform_mult(mm,curobj.obj->m(),m);
		transform_copy(curobj.obj->m(),mm);
		limbo.push(d,islocal); //adds 1 count
		curobj.obj->dec_count();
		curobj.set(curobj.obj,2, 0,limbo.n()-1);
	}
	setCurobj(NULL);
	DBG cout <<"  moved "<<d->object_id<<" to: ";
	DBG curobj.context.out(NULL);
	needtodraw=1;
	return 1;
}

//! Zoom and center various things on the screen
/*! Default is center the current page (w==0).\n
 * w==0 Center page***fixme\n
 * w==1 Center spread\n 
 * w==2 Center current selection group***imp me\n
 * w==3 Center current object ***imp me\n
 *
 * \todo *** need a center that centers ALL objects in spread and in limbo
 */
void LaidoutViewport::Center(int w)
{
	if (!curpage || !curpage->pagestyle || !spread || dp->Minx>=dp->Maxx) return;
	if (w==0) { // center page
		 //find the bounding box in dp real units of the page in question...
		int c=curobj.spreadpage();
		dp->Center(spread->pagestack.e[c]->outline->m(),spread->pagestack.e[c]->outline);
//		dp->Center(-.05*curpage->pagestyle->w(),1.05*curpage->pagestyle->w(), 
//					-.05*curpage->pagestyle->h(),1.05*curpage->pagestyle->h());
		syncrulers();
		needtodraw=1;
	} else if (w==1) { // center spread *** this is broken..
		double w=spread->path->maxx-spread->path->minx,
		       h=spread->path->maxy-spread->path->miny;
		dp->Center(spread->path->minx-.05*w,spread->path->maxx+.05*w, 
				spread->path->miny-.05*h,spread->path->maxy+.05*h);
		syncrulers();
		needtodraw=1;
	}
}

/*! currently, just sets window background to white..
 */
int LaidoutViewport::init()
{
	XSetWindowBackground(app->dpy,window,~0);
	
	return 0;
}

//! Draw the whole business.
/*!
 * <pre>
 * 1. draw scratchboard background
 * 2. draw page outline
 * 3. draw lowest to highest layers
 *  3.a Draw lowest to highest objs
 *   3.a.a draw lowest to highest children of the object, then the object itself.
 *
 * spread
 *  pagestack
 *   index
 *   page
 *   transform
 * doc
 *  pages (PtrStack)
 *   pagenumber (?) maybe not to enable duplicate pages or NULLs..
 *   thumbnail
 *   etc..
 *  docstyle
 *   imposition
 *
 * </pre>
 *
 * *** have choice whether to back buffer, also smart refreshing, and in diff.
 * thread.. periodically check to see if more recent refresh requested...
 *
 * \todo *** this is rather horrible, needs near complete revamp, have to decide
 * on graphics backend. cairo? antigrain?
 *
 * \todo *** implement the 'whatever' page, which is basically just a big whiteboard
 *
 * \todo *** this should be modified so order things are drawn is adjustible,
 * so limbo then pages, or pages then limbo, or just limbo, etc..
 */
void LaidoutViewport::Refresh()
{
	//if (!needtodraw || !win_on) return; assume this is checked for already?
	needtodraw=0;

	if (lfirsttime) { 
		 //setup doublebuffer
		if (!backbuffer) backbuffer=XdbeAllocateBackBufferName(app->dpy,window,XdbeBackground);
		lfirsttime=0; 
		Center(); 
	}
	DBG cout <<"======= Refreshing LaidoutViewport..";
	
	 // draw the scratchboard, just blank out screen..
	//XClearWindow(app->dpy,backbuffer?backbuffer:window);// *** clearwindow(backbuffer) does screwy things!!

	if (!doc || !doc->docstyle) {
		DBG cout <<"=====done refreshing, no doc or doc->docstyle"<<endl;
		return;
	}
	
	dp->StartDrawing(this,backbuffer);
	if (drawflags&DRAW_AXES) dp->drawaxes();
	int c,c2;

	 // draw page outline..
	 //*** different modes: 
	 //		pagelayout <-- only does this now
	 //		paperlayout <-- this has other printer marks...
	 //		single page
	 //		whatever <-- doesn't draw page outline.. is just big whiteboard
	dp->Updates(0);

	 // draw limbo objects
	DBG cout <<"drawing limbo objects.."<<endl;
	for (c=0; c<limbo.n(); c++) {
		DrawData(dp,limbo.e(c),NULL,NULL,drawflags);
	}
	
	if (spread && showstate==1) {
		 // draw 5 pixel offset heavy line like shadow for page first, then fill draw the path...
		 // draw shadow
		dp->NewFG(0,0,0);
		dp->PushAxes();
		dp->ShiftScreen(5,5);
		if (spread->path) {
			FillStyle fs(0,0,0, WindingRule,FillSolid,GXcopy);
			//DrawData(dp,spread->path->m(),spread->path,NULL,&fs,drawflags); //***,linestyle,fillstyle)
			DrawData(dp,spread->path,NULL,&fs,drawflags); //***,linestyle,fillstyle)
			 // draw outline *** must draw filled with paper color
			fs.color=~0;
			dp->PopAxes();
			//DrawData(dp,spread->path->m(),spread->path,NULL,&fs,drawflags);
			DrawData(dp,spread->path,NULL,&fs,drawflags);
		}
		 
		 // draw the pages
		Page *page;
		flatpoint p;
		SomeData *sd=NULL;
		for (c=0; c<spread->pagestack.n; c++) {
			DBG cout <<" drawing from pagestack.e["<<c<<"]"<<endl;
			page=spread->pagestack.e[c]->page;
			if (!page) { // try to look up page in doc using pagestack->index
				if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) 
					spread->pagestack.e[c]->page=page=doc->pages.e[spread->pagestack.e[c]->index];
			}
			if (!page) continue;
			sd=spread->pagestack.e[c]->outline;
			dp->PushAndNewTransform(sd->m()); // transform to page coords
			if (drawflags&DRAW_AXES) dp->drawaxes();
			
			if (page->pagestyle->flags&PAGE_CLIPS) {
//				//-------debugging:---vvv
//				XPoint clip[]={{0,win_h/2},{win_w/2,0},{win_w,win_h/2},{win_w/2,win_h},{0,win_h/2}};
//				XDrawLines(dp->GetDpy(),dp->GetWindow(),dp->GetGC(),clip,5,CoordModeOrigin);
//				Region region=XPolygonRegion(clip,5,WindingRule);
//				XSetRegion(dp->GetDpy(),dp->GetGC(),region);
//				XDestroyRegion(region);
//				//-------------^^^
				 // setup clipping region to be the page
				Region region;
				region=GetRegionFromPaths(sd,dp->m());
				if (!XEmptyRegion(region)) {
					dp->clip(region,3);
					//XSetRegion(dp->GetDpy(),dp->GetGC(),region);
					DBG cout <<"***** set clip path!"<<endl;
				} else {
					DBG cout <<"***** no clip path to set."<<endl;
				}
				//XDestroyRegion(region);
			}
			
			 //*** debuggging: draw X over whole page...
	//		dp->NewFG(255,0,0);
	//		dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->miny));
	//		dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		dp->drawrline(flatpoint(sd->maxx,sd->maxy), flatpoint(sd->minx,sd->maxy));
	//		dp->drawrline(flatpoint(sd->minx,sd->maxy), flatpoint(sd->minx,sd->miny));
	//		dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->minx,sd->maxy));
	
			 // write page number near the page..
			 // mostly for debugging at the moment, might be useful to have
			 // this be a togglable feature.
			p=dp->realtoscreen(flatpoint(0,0));
			if (page==curpage) dp->NewFG(0,0,0);
			dp->drawnum((int)p.x,(int)p.y,spread->pagestack.e[c]->index+1);

			 // Draw all the page's objects.
			for (c2=0; c2<page->layers.n; c2++) {
				DBG cout <<"  Layer "<<c2<<", objs.n="<<page->layers.e[c2]->n()<<endl;
				DrawData(dp,page->layers.e[c2],NULL,NULL,drawflags);
			}
			
			if (page->pagestyle->flags&PAGE_CLIPS) {
				 //remove clipping region
				dp->clearclip();
				XSetClipMask(dp->GetDpy(),dp->GetGC(),None);
			}

			dp->PopAxes(); // remove page transform
		}
	}
	
	 // Call Refresh for each interface that needs it, ignoring clipping region
	//if (curobj.obj) { 
		 //*** this needs a bit more thought, might have several interfaces, that
		 //don't all want curobj transform!!
		 
		double m[6];
		transformToContext(m,curobj.context,0);
		dp->PushAndNewTransform(m); // transform to curobj coords
		
		 // Refresh interfaces, should draw whatever SomeData they have locked
		 //*** maybe Refresh(drawonly decs?) then remove lock check above?
		//DBG cout <<"  drawing interface..";
		SomeData *dd;
		for (int c=0; c<interfaces.n; c++) {
			//if (interfaces.e[c]->Needtodraw()) { // assume always needs to draw??
				//DBG cout <<" \ndrawing "<<interfaces.e[c]->whattype()<<" "<<c<<endl;
				if (dynamic_cast<InterfaceWithDp *>(interfaces.e[c]))
					dd=((InterfaceWithDp *)interfaces.e[c])->Curdata();
					else dd=NULL;
				if (dd) dp->PushAndNewTransform(dd->m());
				interfaces.e[c]->needtodraw=2; // should draw decs? *** when interf draws must be worked out..
				interfaces.e[c]->Refresh();
				if (dd) dp->PopAxes();
			//}
			DBG cout <<interfaces.e[c]->whattype()<<" needtodraw="<<interfaces.e[c]->Needtodraw()<<endl;
				
		}
		dp->PopAxes(); // remove curpage transform
	//}

	dp->Updates(1);

	 // swap buffers
	if (backbuffer) {
		XdbeSwapInfo swapinfo;
		swapinfo.swap_window=window;
		swapinfo.swap_action=XdbeBackground;
		XdbeSwapBuffers(app->dpy,&swapinfo,1);
	}

	DBG cout <<"======= done refreshing LaidoutViewport.."<<endl;
}


//! Key presses...
/*! **** how to notify all the idicator widgets? like curtool palettes, curpage indicator, mouse location
 * indicator, etc...
 *
 * <pre>
 * arrow keys reserved for interface use...
 * 
 * 'D'       toggle drawing of axes for each object
 * 's'       toggle showing of the spread (shows only limbo)
 * 'm'       move current selection to another page, popus up a dialog ***imp me!
 * ' '       Center(), *** need center obj, page, spread, center+fit obj,page,spread,objcomponent
 *  // these are like inkscape
 * pgup      ***raise selection by 1 within layer
 * pgdown    ***lower selection by 1 within layer
 * home      ***bring selection to top within layer
 * end       ***drop selection to bottom within layer
 * +pgup     ***move selection up a layer
 * +pgdown   ***move selection down a layer
 * +^pgup    ***raise layer 1
 * +^pgdown  ***lower layer 1
 * +^home    ***layer to top
 * +^end     ***layer to bottom
 *
 *  // These are implemented in the other classes indicated
 * left      previous tool <-- done in viewwindow
 * right     next tool     <-- done in viewwindow
 * ','       previous object <-- done in ViewportWindow, which calls SelectObject
 * '.'       next object     <-- done in ViewportWindow, which calls SelectObject
 * '<'       previous page   <-- done in ViewWindow
 * '>'       next page       <-- done in ViewWindow
 * </pre>
 */
int LaidoutViewport::CharInput(unsigned int ch,unsigned int state)
{
	
	 // check these first, before asking interfaces
	if (ch==' ') {
		if ((state&LAX_STATE_MASK)==0) {
			Center();
			return 0;
		} 
		if ((state&LAX_STATE_MASK)==ShiftMask) {
			Center(1);
			return 0;
		} 
	}
	 // ask interfaces, and default viewport stuff
	if (ViewportWindow::CharInput(ch,state)==0) return 0;

	 // deal with all other LaidoutViewport specific stuff
	if (ch=='x' &&  (state&LAX_STATE_MASK)==0) {
		if (!DeleteObject()) return 1;
		return 0;
	} else if (ch=='s' && (state&LAX_STATE_MASK)==0) {
		if (showstate==0) showstate=1;
		else showstate=0;
		needtodraw=1;
		return 0;
	} else if (ch=='M' && (state&LAX_STATE_MASK)==ShiftMask|Mod1Mask) {
		 //for debugging to make a delineation in the cout stuff
		DBG cout<<"----------------=========<<<<<<<<< *** >>>>>>>>========--------------"<<endl;
		return 0;
	} else if (ch=='m' && (state&LAX_STATE_MASK)==0) {
		if (curobj.obj) {
			cout<<"**** move object to another page: imp me!"<<endl;
			//if (CirculateObject(9,i,0)) needtodraw=1;
			return 0;
		}
	} else if (ch==LAX_Pgup) { //pgup
		if (!curobj.obj) return ViewportWindow::CharInput(ch,state);
		if ((state&LAX_STATE_MASK)==0) { //raise selection within layer
			if (CirculateObject(0,0,0)) needtodraw=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask) { //move sel up a layer
			if (CirculateObject(5,0,0)) needtodraw=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask|ControlMask) { //layer up 1
			if (CirculateObject(20,0,0)) needtodraw=1;
			return 0;
		}
	} else if (ch==LAX_Pgdown) { //pgdown
		if (!curobj.obj) return ViewportWindow::CharInput(ch,state);
		if ((state&LAX_STATE_MASK)==0) { //lower selection within layer
			if (CirculateObject(1,0,0)) needtodraw=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask) { //move sel down a layer
			if (CirculateObject(6,0,0)) needtodraw=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask|ControlMask) { //layer down 1
			if (CirculateObject(21,0,0)) needtodraw=1;
			return 0;
		}
	} else if (ch==LAX_Home) { //home
		if (curobj.obj && (state&LAX_STATE_MASK)==0) {	//raise selection to top
			if (CirculateObject(2,0,0)) needtodraw=1;
			return 0;
		}
	} else if (ch==LAX_End) { //end
		if (curobj.obj && (state&LAX_STATE_MASK)==0) {	//lower selection to bottom
			if (CirculateObject(3,0,0)) needtodraw=1;
			return 0;
		}
	} else if (ch=='D' && (state&LAX_STATE_MASK)==ShiftMask) {
		DBG cout << "----drawflags: "<<drawflags;
		if (drawflags==DRAW_AXES) drawflags=DRAW_BOX;
		else if (drawflags==DRAW_BOX) drawflags=DRAW_BOX|DRAW_AXES;
		else if (drawflags==(DRAW_AXES|DRAW_BOX)) drawflags=0;
		else drawflags=DRAW_AXES;
		DBG cout << " --> "<<drawflags<<endl;
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Move curobj object or selection in various ways.
/*! \todo ***imp me!
 * 
 * *** currently ignores objOrSelection until I figure out what i wanted
 * it for anyway...
 *
 * \todo *** most of these things ought to be functions of Document!!
 * 
 * \todo *** need an option to move past all overlapping objects! if no overlapping
 * objs, then just move as normal
 *
 * <pre>
 *  0   Raise in layer
 *  1   Lower in layer
 *  2   Raise to top
 *  3   Lower to bottom
 *  4   To index in layer
 *  5   To next layer up
 *  6   To next layer down
 *  7   to previous page
 *  8   to next page
 *  9   to page i (-1==limbo)
 *  10  de-parent obj, put in ancestor layer
 *  
 *  20  Raise layer by 1
 *  21  Lower layer by 1
 *  22  Raise layer to top
 *  23  Lower Layer to bottom
 *
 *  30  Throw obj to limbo?
 *  31  Throw layer to limbo?
 * </pre>
 *
 * Returns 1 if object moved, else 0.
 */
int LaidoutViewport::CirculateObject(int dir, int i,int objOrSelection)
{
	DBG cout <<"Circulate: dir="<<dir<<"  i="<<i<<endl;
	if (!curobj.obj) return 0;
	if (dir==0) { // raise in layer
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==g->n()-1) { 
			curobj.push(curpos);
			//curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->swap(curpos,curpos+1);
		curobj.push(curpos+1);
		//curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;
	} else if (dir==1) { // lower in layer
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==0) { 
			curobj.push(curpos);
			//curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->swap(curpos,curpos-1);
		curobj.push(curpos-1);
		//curobj.context.out("lower by 1");
		needtodraw=1;
		return 1;
	} else if (dir==2) { // raise to top
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==g->n()-1) { 
			curobj.push(curpos);
			//curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->slide(curpos,g->n()-1);
		curobj.push(g->n()-1);
		//curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;
	} else if (dir==3) { // lower to bottom
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==0) { 
			curobj.push(curpos);
			//curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->slide(curpos,0);
		curobj.push(0);
		//curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;
	} else if (dir==4) { // move to place i in current context
		cout <<"***move to place i in current context not implemented!"<<endl;
	} else if (dir==5) { // move to layer above
		//***move to bottom of next upper layer
		cout <<"*** move to layer above not implemented!"<<endl;
	} else if (dir==6) { // move to layer below
		//***put on top of lower layer
		cout <<"***move to layer below not implemented!"<<endl;
	} else if (dir==7) { // move to previous page
		cout <<"***move to previous page not implemented!"<<endl;
	} else if (dir==8) { // move to next page
		cout <<"***move to next page not implemented!"<<endl;
	} else if (dir==9) { // move to page number i
		cout <<"***move to page number i not implemented!"<<endl;
	}
	return 0;
}

//! Return the page index in document of the page in curobj.
int LaidoutViewport::curobjPage()
{
	if (!spread || curobj.spreadpage()<0) return -1;
	PageLocation *pl=spread->pagestack.e[curobj.spreadpage()];
	if (pl) return pl->index;
	return -1;
}

//! Select page. i>=0 for index, i==-1 for next, i==-2 for previous.
/*! Return curpage (>=0) on success, else a negative number indicating
 * no change. Returns -2 if asked for page is the current page.
 *
 * Calls Clear() on all interfaces on the stack (from a call to setupthings()).
 * 
 * \todo *** should be able to remember what was current on different pages...
 *
 * \todo *** if i==-2 uses curobj, but what if curobj is in limbo?
 */
int LaidoutViewport::SelectPage(int i)
{
	if (!doc || !doc->pages.n) return -1;
	if (i==-2) {
		i=curobjPage()-1;
		if (i<0) i=doc->pages.n-1;
	} else if (i==-1) {
		i=curobjPage()+1;
		if (i>=doc->pages.n) i=0;
	} 
	if (i<0) i=0; else if (i>=doc->pages.n) i=doc->pages.n-1;
	if (i==curobjPage()) return -2;
	
	 //setupthings(): clears search, clears any interfacedata... curinterface->Clear()
	setupthings(i); //***this always deletes and new's spread
	DBG cout <<" SelectPage made page=="<<curobjPage()<<endl;

	needtodraw=1;
	return curobjPage();
}

/*! *** Currently does nothing....should it? ViewerWindow can do right?
 */
int LaidoutViewport::ApplyThis(Laxkit::anObject *thing,unsigned long mask)
{
	return 0;
}


//------------------------------- ViewWindow ---------------------------
/*! \class ViewWindow
 * \brief Basic view window, for looking at pages, spreads, etc.
 *
 * \todo *** please note that this window and its little buttons and such is a complete
 * hack at the moment.. must work out that useful way of creating windows that I
 * haven't gotten around to coding..
 */
//class ViewWindow : public Laxkit::ViewerWindow
//{
// protected:
//	void setup();
//	Laxkit::NumSlider *pagenumber;
//	Laxkit::LineEdit *loaddir;
// public:
//	Project *project;
//	Document *doc;
//
//	ViewWindow(Document *newdoc);
//	ViewWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//						int xx,int yy,int ww,int hh,int brder,
//						Document *newdoc);
//	virtual const char *whattype() { return "ViewWindow"; }
//	virtual int CharInput(unsigned int ch,unsigned int state);
//	virtual int DataEvent(Laxkit::SendData *data,const char *mes);
//	virtual int init();
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//	virtual void updatePagenumber();
//	virtual void SetParentTitle(const char *str);
//
//	virtual void dump_out(FILE *f,int indent) =0;
//	virtual void dump_in_atts(Attribute *att) =0;
//};

//	ViewerWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//						int xx,int yy,int ww,int hh,int brder,
//						int npad=0);
//! Passes in a new LaidoutViewport.
ViewWindow::ViewWindow(Document *newdoc)
	: ViewerWindow(NULL,((newdoc && newdoc->Name())?newdoc->Name() :"untitled"),ANXWIN_DELETEABLE,
					0,0,500,600,1,new LaidoutViewport(newdoc))
{ 
	var1=var2=var3=NULL;
	project=NULL;
	pagenumber=NULL;
	loaddir=NULL;
	doc=newdoc;
	setup();

	win_x=1280-win_w;
}

//! More general constructor. Passes in a new LaidoutViewport.
ViewWindow::ViewWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Document *newdoc)
	: ViewerWindow(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,new LaidoutViewport(newdoc))
{
	project=NULL;
	var1=var2=var3=NULL;
	pagenumber=NULL;
	loaddir=NULL;
	doc=newdoc;
	setup();
}

/*! Write out something like:
 * <pre>
 *   document ***docid  # which document
 *   pagelayout         # or 'paperlayout' or 'singlelayout'
 *   page 3             # the page number currently on
 *   matrix 1 0 0 1 0 0 # viewport matrix
 * </pre>
 *
 * \todo *** still need some standardizing for the little helper controls..
 */
void ViewWindow::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	fprintf(f,"%sdocument %s\n",spc,doc->Name());

	LaidoutViewport *vp=((LaidoutViewport *)viewport);
	int vm=vp->viewmode;
	if (vm==SINGLELAYOUT) fprintf(f,"%ssinglelayout\n",spc);
	else if (vm==PAGELAYOUT) fprintf(f,"%spagelayout\n",spc);
	else fprintf(f,"%spaperlayout\n",spc);
	
	if (vp->spread && vp->spread->pagestack.n)
		fprintf(f,"%spage %d\n",spc,vp->spread->pagestack.e[0]->index);
	
	const double *m=vp->dp->Getctm();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
}

//! Reverse of dump_out().
void ViewWindow::dump_in_atts(Attribute *att)
{
	if (!att) return;
	char *name,*value;
	int vm=PAGELAYOUT, pn=0;
	double m[6];
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			DoubleListAttribute(value,m,6);
		} else if (!strcmp(name,"pagelayout")) {
			vm=PAGELAYOUT;
		} else if (!strcmp(name,"paperlayout")) {
			vm=PAPERLAYOUT;
		} else if (!strcmp(name,"singlelayout")) {
			vm=SINGLELAYOUT;
		} else if (!strcmp(name,"page")) {
			IntAttribute(value,&pn);
		}
	}
//	*** set doc
//	if (m) ***
//	***set vm
//	*** set pn
}


//! Called from constructors, configure the viewport and ***add the extra pager, layer indicator, etc...
/*! Adds local copies of all the interfaces in interfacespool.
 */
void ViewWindow::setup()
{
	win_xatts.event_mask|=KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ExposureMask;
	win_xattsmask|=CWEventMask;
	if (viewport) viewport->dp->NewBG(app->rgbcolor(255,255,255));

	//***this should be making dups of interfaces stack? or set current tool, etc...
	for (int c=0; c<laidout->interfacepool.n; c++) {
		AddTool(laidout->interfacepool.e[c]->duplicate(),1,0);
	}
	SelectTool(0);
	//AddWin(new LayerChooser);...
}

//! Add extra goodies to viewerwindow stack, like page/layer/mag/obj indicators
int ViewWindow::init()
{
	ViewerWindow::init();
	mesbar->tooltip("Status bar");
	
	if (!doc) {
		if (laidout->project && laidout->project->docs.n) {
			doc=laidout->project->docs.e[0];
			((LaidoutViewport *)viewport)->UseThisDoc(doc);
		}
	}
	
	if (!win_sizehints) win_sizehints=XAllocSizeHints();
	if (win_sizehints && !win_parent) {
		DBG cout <<"doingwin_sizehintsfor"<<(win_title?win_title:"untitled")<<endl;

		//*** The initial x and y become the upper left corner of the window
		//manager decorations. ***how to figure out how much room those decorations take,
		//so as to place things on the screen accurately? like full screen view?
		win_sizehints->x=win_x;
		win_sizehints->y=win_y;
		win_sizehints->width=win_w;
		win_sizehints->height=win_h;
	//	win_sizehints->width_inc=1;
	//	win_sizehints->height_inc=1;
	//	win_sizehints->flags=PMinSize|PResizeInc|USPosition|USSize
		win_sizehints->flags=USPosition|USSize;
	}
	
	AddNull();//makes the status bar take up whole line.
	
	StrSliderPopup *toolselector;
	toolselector=new StrSliderPopup(this,"viewtoolselector",0, 0,0,0,0,1, NULL,window,"viewtoolselector");
	for (int c=0; c<tools.n; c++) {
		toolselector->AddItem(tools.e[c]->whattype(),tools.e[c]->id);
		DBG cout <<"make tool selector, "<<tools.e[c]->whattype()<<" "<<c<<": id="<<tools.e[c]->id<<endl;
	}
	toolselector->WrapWidth();
	AddWin(toolselector);
	
	anXWindow *last=NULL;
	last=pagenumber=new NumInputSlider(this,"page number",NUMSLIDER_WRAP, 0,0,0,0,1, 
								NULL,window,"newPageNumber",
								"Page: ",1,1000000,1);
	pagenumber->tooltip("The pages in the spread\nand the current page");
	AddWin(pagenumber,90,0,50,50, pagenumber->win_h,0,50,50);
	
	TextButton *tbut;
	tbut=new TextButton(this,"page less",0, 0,0,0,0,1, NULL,window,"pageLess","<");
	tbut->tooltip("Previous spread");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	tbut=new TextButton(this,"page more",0, 0,0,0,0,1, NULL,window,"pageMore",">");
	tbut->tooltip("Next spread");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	NumSlider *num=new NumSlider(this,"layer number",NUMSLIDER_WRAP, 0,0,0,0,1, 
								NULL,window,"newLayerNumber",
								"Layer: ",1,1,1); //*** get cur page, use those layers....
	num->tooltip("Sorry, layer control not well\nimplemented yet");
	AddWin(num,num->win_w,0,50,50, num->win_h,0,50,50);
	
	StrSliderPopup *p=new StrSliderPopup(this,"view type",0, 0,0,0,0,1, NULL,window,"newViewType");
	p->AddItem("Single",SINGLELAYOUT);
	p->AddItem("Page Layout",PAGELAYOUT);
	p->AddItem("Paper Layout",PAPERLAYOUT);
	p->Select(1);
	p->WrapWidth();
	AddWin(p,p->win_w,0,50,50, p->win_h,0,50,50);

	ColorBox *colorbox;
	last=colorbox=new ColorBox(this,"colorbox",0, 0,0,0,0,1, NULL,window,"curcolor",255,0,0);
	colorbox->tooltip("Current color:\nDrag left for red,\n middle for green,\n right for red");
	AddWin(colorbox, 50,0,50,50, p->win_h,0,50,50);
		
	last=tbut=new TextButton(this,"add page",0, 0,0,0,0,1, NULL,window,"addPage","Add Page");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	last=tbut=new TextButton(this,"delete page",1, 0,0,0,0,1, NULL,window,"deletePage","Delete Page");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	tbut=new TextButton(this,"import image",0, 0,0,0,0,1, NULL,window,"importImage","Import Image");
	tbut->tooltip("Import a single image");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	tbut=new TextButton(this,"import images",0, 0,0,0,0,1, NULL,window,"dumpImages","Dump in Images");
	tbut->tooltip("Import a whole lot of images\nand put across multiple pages\n(see the other buttons)");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	tbut=new TextButton(this,"ppt out",0, 0,0,0,0,1, NULL,window,"pptout","ppt out");
	tbut->tooltip("Save to a Passepartout file.\nOnly saves images");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	tbut=new TextButton(this,"print",0, 0,0,0,0,1, NULL,window,"print","Print");
	tbut->tooltip("Print to output.ps, a postscript file");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	loaddir=new LineEdit(this,"load directory",0, 0,0,0,0,1, 
								NULL,window,"loaddir",
								app->load_dir,0);
	loaddir->tooltip("'Dump in images' dumps in\nall images from this directory");
	AddWin(loaddir,150,0,50,250, loaddir->win_h,0,50,50);
	

	var1=new NumInputSlider(this,"var1",NUMSLIDER_WRAP, 0,0,0,0,1, 
								NULL,window,"var1",
								NULL,-10000,100000,1);
	var1->tooltip("Number of images per page to dump in,\n-1=as many as possible,\nShift-click to type a number");
	AddWin(var1,var1->win_w,0,50,50, var1->win_h,0,50,50);
	
	var2=new NumInputSlider(this,"var2",NUMSLIDER_WRAP, 0,0,0,0,1, 
								NULL,window,"var2",
								NULL,-10000,100000,1);
	var2->tooltip("Default dpi of images dumped in.\nShift-click to type a number");
	AddWin(var2,var2->win_w,0,50,50, var2->win_h,0,50,50);
	
	var3=new NumInputSlider(this,"var3",NUMSLIDER_WRAP, 0,0,0,0,1, 
								NULL,window,"var3",
								NULL,-10000,100000,1);
	var3->tooltip("(undefined)");
	AddWin(var3,var3->win_w,0,50,50, var3->win_h,0,50,50);
	
	tbut=new TextButton(this,"help",0, 0,0,0,0,1, NULL,window,"help","Help!");
	tbut->tooltip("Popup a list of shortcuts");
	AddWin(tbut,tbut->win_w,0,50,50, tbut->win_h,0,50,50);

	//**** add screen x,y
	//		   real x,y
	//         page x,y
	//         object x,y
	
	Sync(1);	
	return 0;
}

/*! Currently accepts:\n
 * <pre>
 *  "new image"
 *  "saveAsPopup"
 * </pre>
 */
int ViewWindow::DataEvent(Laxkit::SendData *data,const char *mes)
{
	if (!strcmp(mes,"new image")) {
		StrSendData *s=dynamic_cast<StrSendData *>(data);
		if (!s) return 1;
		Imlib_Image image=imlib_load_image(s->str);
		if (image) {
			ImageData *newdata=new ImageData();
			newdata->SetImage(image);
			makestr(newdata->filename,s->str);
			//*** must scale image to fit the page at default dpi!!
			newdata->xaxis(newdata->xaxis()/300); // set to 300 dpi, assuming 1 unit==1 inch!!
			newdata->yaxis(newdata->yaxis()/300); // set to 300 dpi, assuming 1 unit==1 inch!!
			if (!((LaidoutViewport *)viewport)->PlopData(newdata)) delete newdata;
		} else { 
			char mes[30+strlen(s->str)+1];
			sprintf(mes,"Couldn't load image from \"%s\".",s->str);
			((LaidoutViewport *)viewport)->postmessage(mes);
		}
		delete data;
		return 0;
	} else if (!strcmp(mes,"saveAsPopup")) {
		StrSendData *s=dynamic_cast<StrSendData *>(data);
		if (!s) return 1;
		if (s->str && s->str[0]) {
			if (doc && doc->Name(s->str)) {
				SetParentTitle(doc->Name());
			}
		}
		//**** ask to overwrite
		doc->Save();
		delete data;
		char blah[strlen(doc->Name())+15];
		sprintf(blah,"Saved to %s.",doc->Name());
		GetMesbar()->SetText(blah);
		return 0;
	}
	return 1;
}

//! Set the title of the top level window containing this window.
void ViewWindow::SetParentTitle(const char *str)
{
	if (!str) return;
	XStoreName(app->dpy,window,str);
	anXWindow *win=win_parent;
	while (win && win->win_parent) win=win->win_parent;
	if (win) XStoreName(app->dpy,win->window,str);
}

//! Make the pagenumber label be correct.
void ViewWindow::updatePagenumber()
{
	pagenumber->Label(((LaidoutViewport *)viewport)->Pageviewlabel());
	pagenumber->Select(((LaidoutViewport *)viewport)->curobjPage()+1);
}

//! Deal with various indicator/control events
/*! Accepts
 *    curcolor,
 *    docTreeChange *** imp me!,
 *    newPageNumber,
 *    newLayerNumber,
 *    newViewType, 
 *    importImage,
 *    dumpImages,
 *    deletePage,
 *    addPage,
 *    help (pops up a HelpWindow)
 *
 * \todo ***imp contextChange, sent from LaidoutViewport
 */
int ViewWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"curcolor")) {
		 // apply message as new current color, pass on to viewport
		LineStyle linestyle;
		linestyle.red=e->data.l[0];
		linestyle.green=e->data.l[1];
		linestyle.blue=e->data.l[2];
		linestyle.alpha=e->data.l[3];
		linestyle.color=app->rgbcolor(e->data.l[0],e->data.l[1],e->data.l[2]);
		if (curtool)
			if (curtool->UseThis(&linestyle,GCForeground)) ((anXWindow *)viewport)->Needtodraw(1);
		
		return 0;
	} else if (!strcmp(mes,"docTreeChange")) { // doc tree was changed somehow
		cout <<"ViewWindow got docTreeChange *** imp me!! IMPORTANT!!!"<<endl;
		((anXWindow *)viewport)->Needtodraw(1);
		//viewport->ClientEvent(e,mes);
		return 0;
	} else if (!strcmp(mes,"help")) {
		app->addwindow(new HelpWindow());
		return 0;
	} else if (!strcmp(mes,"contextChange")) { // curobj was changed, now maybe diff page, spread, etc.
		//***
		updatePagenumber();
		return 0;
	} else if (!strcmp(mes,"addPage")) { // 
		int curpage=((LaidoutViewport *)viewport)->curobjPage();
		int c=doc->NewPages(curpage+1,1); //add after curpage
		if (c==0) GetMesbar()->SetText("Page added.");
			else GetMesbar()->SetText("Error adding page.");
		return 0;
	} else if (!strcmp(mes,"deletePage")) { // 
		int curpage=((LaidoutViewport *)viewport)->curobjPage();
		int c=doc->RemovePages(curpage,1); //remove curpage
		if (c==0) GetMesbar()->SetText("Page deleted.");
			else GetMesbar()->SetText("Error deleting page.");
		return 0;
	} else if (!strcmp(mes,"newPageNumber")) { // 
		if (e->data.l[0]>doc->pages.n) {
			e->data.l[0]=1;
			pagenumber->Select(e->data.l[0]);
		} else if (e->data.l[0]<1) {
			e->data.l[0]=doc->pages.n;
			pagenumber->Select(e->data.l[0]);
		}
		((LaidoutViewport *)viewport)->SelectPage(e->data.l[0]-1);
		updatePagenumber();
		return 0;
	} else if (!strcmp(mes,"pageLess")) {
		((LaidoutViewport *)viewport)->PreviousSpread();
		updatePagenumber();
		return 0;
	} else if (!strcmp(mes,"pageMore")) {
		((LaidoutViewport *)viewport)->NextSpread();
		updatePagenumber();
		return 0;
	} else if (!strcmp(mes,"newLayerNumber")) { // 
		//((LaidoutViewport *)viewport)->SelectPage(e->data.l[0]);
		cout <<"***** new layer number.... *** imp me!"<<endl;
		return 0;
	} else if (!strcmp(mes,"viewtoolselector")) {
		DBG cout <<"***** viewtoolselector change to id:"<<e->data.l[0]<<endl;
		SelectTool(e->data.l[0]);
		return 0;
	} else if (!strcmp(mes,"newViewType")) {
		 // must update labels have Spread [2-3]: 2
		int v=e->data.l[0];
		if (v==SINGLELAYOUT) {
			pagenumber->Label(((LaidoutViewport *)viewport)->SetViewMode(SINGLELAYOUT));
		} else if (v==PAGELAYOUT) {
			pagenumber->Label(((LaidoutViewport *)viewport)->SetViewMode(PAGELAYOUT));
		} else if (v==PAPERLAYOUT) {
			pagenumber->Label(((LaidoutViewport *)viewport)->SetViewMode(PAPERLAYOUT));
		}
		return 0;
	} else if (!strcmp(mes,"importImage")) {
		app->rundialog(new FileDialog(NULL,"Import Image",
					FILES_FILES_ONLY|FILES_OPEN_ONE|FILES_PREVIEW,
					0,0,500,500,0, window,"new image"));
		return 0;
	} else if (!strcmp(mes,"dumpImages")) {
		//DBG cout <<" --- dumpImages...."<<endl;
		dumpImages(doc,((LaidoutViewport *)viewport)->curobjPage(),app->load_dir,var1->Value(),var2->Value());
		pagenumber->NewMinMax(1,doc->pages.n);
		((anXWindow *)viewport)->Needtodraw(1);
		return 0;
	} else if (!strcmp(mes,"loaddir")) {
		if (loaddir->GetCText()) makestr(app->load_dir,loaddir->GetCText());
		return 0;
	} else if (!strcmp(mes,"pptout")) { // dump to passepartout file
		if (!doc->Save(Save_PPT)) {
			char blah[strlen(doc->saveas+10)];
			sprintf(blah,"Saved as a Passepartout file to %s.ppt",doc->saveas);
			mesbar->SetText(blah);
		}
		return 0;
	} else if (!strcmp(mes,"print")) { // print to output.ps
		mesbar->SetText("Printing, please wait....");
		if (!doc->Save(Save_PS)) mesbar->SetText("Printed to output.ps.");
		else mesbar->SetText("Failed to print to output.ps");
		return 0;
	}
	
	return 1;
}

//! *** this is a dirty hack to keep loaddir updated, and should be removed
int ViewWindow::event(XEvent *e)
{
	//*** here is a quick cheap, VERY dirty hack to keep loaddir updated:
	if (strcmp(app->load_dir,loaddir->GetCText())) loaddir->SetText(app->load_dir);
	return ViewerWindow::event(e);
}

/*! <pre>
 * left    prev tool, 
 * right   next tool. *** maybe this should be in ViewerWindow??
 * '<'     previous page 
 * '>'     next page
 * ^'s'    save file
 * ^+'s'   save as -- just change the file name?? (not imp)
 * F5      popup new spread editor window
 *
 * 'r'     ---for debugging, does system call: "more /proc/(pid)/status"
 * </pre>
 */
int ViewWindow::CharInput(unsigned int ch,unsigned int state)
{
	if (ch=='S' && (state&LAX_STATE_MASK)==(ControlMask|ShiftMask) || 
			ch=='s' && (state&LAX_STATE_MASK)==ControlMask) { // save file
		if (!doc) return 1;
		DBG cout <<"....viewwindow says save.."<<endl;
		if (strstr(doc->Name(),"untitled")==doc->Name() || (state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
			 // launch saveas!!
			//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
						//int xx,int yy,int ww,int hh,int brder,
						//anXWindow *prev,Window nowner,const char *nsend,
						//const char *newlabel,const char *newtext,unsigned int ntstyle,
						//int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
			app->rundialog(new LineInput(NULL,"Save As...",
						ANXWIN_CENTER|ANXWIN_DELETEABLE|LINP_ONTOP|LINP_CENTER|LINP_POPUP,
						100,100,200,100,0, 
						NULL,window,"saveAsPopup",
						"Enter new filename:",doc->Name(),0,
						0,0,5,5,3,3));
		} else {
			if (doc->Save()==0) {
				char blah[strlen(doc->Name())+15];
				sprintf(blah,"Saved to %s.",doc->Name());
				GetMesbar()->SetText(blah);
			} else GetMesbar()->SetText("Problem saving. Not saved.");
		}
		return 0;
	} else if (ch=='t' || ch=='T') {
		//*** this is rather a hack..
		ViewerWindow::CharInput(ch,state);
		int c;
		WinFrameBox *wb;
		for (c=0; c<wholelist.n; c++) {
			wb=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
			if (wb && wb->win && !strcmp(wb->win->win_title,"viewtoolselector"))  {
				static_cast<StrSliderPopup *>(wb->win)->Select(curtool->id);
				break;
			}
		}
		return 0;
	} else if (ch==LAX_Left) {  // left, prev tool
		SelectTool(-2);
		return 0;
	} else if (ch==LAX_Right) { //right, next tool
		SelectTool(-1);
		return 0;
	} else if (ch=='<') { //prev page
		DBG cout <<"'<'  should be prev page"<<endl;
		int pg=((LaidoutViewport *)viewport)->SelectPage(-2);
		curtool->Clear();
		for (int c=0; c<kids.n; c++) if (!strcmp(kids.e[c]->win_title,"page number")) {
			((NumSlider *)kids.e[c])->Select(pg+1);
			break;
		}
		updatePagenumber();
		return 0;
	} else if (ch=='>') { //next page
		DBG cout <<"'>' should be prev page"<<endl;
		int pg=((LaidoutViewport *)viewport)->SelectPage(-1);
		for (int c=0; c<kids.n; c++) if (!strcmp(kids.e[c]->win_title,"page number")) {
			((NumSlider *)kids.e[c])->Select(pg+1);
			break;
		}
		updatePagenumber();
		return 0;
	} else if (ch=='r') {
		//**** for debugging:
		pid_t pid=getpid();
		char blah[100];
		sprintf(blah,"more /proc/%d/status",pid);
		system(blah);
		return 0;
	} else if (ch=='n' && (state&LAX_STATE_MASK)==ControlMask) {
		app->rundialog(new NewDocWindow(NULL,"New Document",0,0,0,500,600, 0));
		return 0;
	} else if (ch=='n' && (state&LAX_STATE_MASK)==ControlMask|ShiftMask) { //reset document
		//***
	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
		app->addwindow(new HelpWindow());
		return 0;
	} else if (ch==LAX_F5 && (state&LAX_STATE_MASK)==0) {
		//*** popup a SpreadEditor
		char blah[30+strlen(doc->Name())+1];
		sprintf(blah,"Spread Editor for %s",doc->Name());
		app->addwindow(new SpreadEditor(NULL,blah,0, 0,0,500,500,0, project,doc));
		return 0;
	}
	return ViewerWindow::CharInput(ch,state);
}


