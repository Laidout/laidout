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
// Copyright (c) 2004-2013 Tom Lechner
//

#include <lax/numslider.h>
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/transformmath.h>
#include <lax/sliderpopup.h>
#include <lax/filedialog.h>
#include <lax/colorbox.h>
#include <lax/fileutils.h>
#include <lax/overwrite.h>
#include <lax/laxutils.h>
#include <lax/menubutton.h>
#include <lax/inputdialog.h>
#include <lax/popupmenu.h>
#include <lax/colors.h>
#include <lax/mouseshapes.h>
#include <lax/units.h>
#include <lax/shortcutwindow.h>

#include <cstdarg>
#include <cups/cups.h>
#include <sys/stat.h>

#include <lax/lists.cc>


#include "language.h"
#include "printing/print.h"
#include "printing/psout.h"
#include "impositions/impositioneditor.h"
#include "helpwindow.h"
#include "about.h"
#include "spreadeditor.h"
#include "viewwindow.h"
#include "headwindow.h"
#include "laidout.h"
#include "newdoc.h"
#include "importimage.h"
#include "drawdata.h"
#include "helpwindow.h"
#include "configured.h"
#include "importimages.h"
#include "stylemanager.h"
#include "filetypes/importdialog.h"
#include "filetypes/exportdialog.h"
#include "interfaces/paperinterface.h"
#include "interfaces/documentuser.h"
#include "calculator/shortcuttodef.h"

#include <iostream>
using namespace std;

#define DBG 


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


namespace Laidout {



//---------------------------
 //***standard action ids for corner button menu
 //0..996 are current documents and "none" document
#define ACTION_EditImposition          997
#define ACTION_RemoveCurrentDocument   998
#define ACTION_AddNewDocument          999
		
 //1000..1996 are limbos
#define ACTION_AddNewLimbo             1999
#define ACTION_RenameCurrentLimbo      1998
#define ACTION_DeleteCurrentLimbo      1997

 //2000..2996 are paper groups (2000 is "default")
#define ACTION_AddNewPaperGroup        2999
#define ACTION_RenameCurrentPaperGroup 2998
#define ACTION_DeleteCurrentPaperGroup 2997

#define ACTION_NewProject              3000
#define ACTION_SaveProject             3001
#define ACTION_ToggleSaveAsProject     3002
//---------------------------

 //menu ids from menu to move an object or change context
#define MOVETO_Limbo      1
#define MOVETO_Spread     2
#define MOVETO_Page       3
#define MOVETO_PaperGroup 4
#define MOVETO_OtherPage  5

//---------------------------



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
 *      2 means current papergroup. In future might implement some sort
 *      of infinite scroll, where all spreads are accessible, and you can edit objects
 *      just by zooming in appropriately.
 *  1 = if spread, then 0 is pagestack, 1 is marks on the spread
 *  2 = index of page in spread->pagestack, or in limbo stack if is scratchboard
 *  3 = index of current layer in page
 *  4 = index of object in current layer
 *  5 and above = index in any Group or other containing object.
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

/*! Decs count of nobj.
 */
VObjContext::~VObjContext()
{
	if (obj) { obj->dec_count(); obj=NULL; }
}

//! Assignment operator.
/*! Incs count of obj.
 */
VObjContext &VObjContext::operator=(const VObjContext &oc)
{
	if (obj!=oc.obj) {
		if (obj) obj->dec_count();
		obj=oc.obj;
		if (obj) obj->inc_count();
	}
	context=oc.context;
	return *this;
}

//! Return a new VObjContext that is a copy of this.
LaxInterfaces::ObjectContext *VObjContext::duplicate()
{
	VObjContext *o=new VObjContext();
	*o=*this;
	return o;
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
/*! Incs count of nobj.
 */
int VObjContext::set(LaxInterfaces::SomeData *nobj, int n, ...)
{
	if (obj) obj->dec_count();
	obj=nobj;
	if (obj) obj->inc_count();
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

void VObjContext::clear()
{
	if (obj) { obj->dec_count(); obj=NULL; }
	context.flush(); 
}
//------------------------------- LaidoutViewport ---------------------------
/*! \class LaidoutViewport
 * \brief General viewport to pass to the ViewerWindow base class of ViewWindow
 *
 * Creates different type of ObjectContext so that curobj,
 * firstobj, and foundobj are all VObjContext instances which shadow the ViewportWindow
 * variables of the same names.
 * 
 * \todo *** need to check through that all the searching stuff, and cur obj/page/layer stuff
 *   are reset where appropriate.. 
 * \todo *** need to be able to work only on current zone or only on current layer...
 *   a zone would be: limbo, imposition specific zones (like printer marks), page outline,
 *   the spread itself, the current page only.. the zone could be the objcontext->spread()?,
 *   *** might be useful to have more things potentially represented in curobj.spread()..
 *   possibilities: limbo, main spread, printer marks, --Other Spreads--...
 * \todo *** in paper spread view, perhaps that is where a spread()==printer marks is useful..
 *   also, depending on the imposition, there might be other operations permitted in paper spread..
 *   like in a postering imposition, the page data stays stationary, but multiple paper outlines can latch
 *   on to different parts of it...(or perhaps the page moves around, not paper)
 * \todo please note that LaidoutViewport has 2 anObject base classes.. really it shouldn't..
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
/*! \var double LaidoutViewport::ectm[6]
 * \brief The extended transform.
 *
 * This is the transform that maps from curobj to the dp space.
 */
/*! \var int LaidoutViewport::viewmode
 * \brief 0=single, 1=Page layout, 2=Paper layout, or other imposition dependent spread type number
 *
 * \todo ***perhaps add the object space editor here, that would be viewmode -1. All else goes
 *   away, and only that object and its tool are there. If other tools are selected during object
 *   space editing and new objects are put down, then the curobj becomes a group with the 
 *   former curobj part of that group.
 */
/*! \var Group *LaidoutViewport::limbo
 * \brief Essentially the scratchboard, which persists even when changing spreads and pages.
 *
 * These are saved with the document, and each view can select which limbo to work with.
 */
/*! \fn int LaidoutViewport::n()
 * \brief  Return 2 if spread exists, or 1 for just limbo.
 */
/*! \var int LaidoutViewport::searchmode
 * <pre>
 *  Search_None    0
 *  Search_Find    1
 *  Search_Select  2
 * </pre>
 */
/*! \var int LaidoutViewport::searchcriteria
 * <pre>
 *  Search_Any         0 <-- any held by the viewport
 *  Search_Visible     1 <-- any visible on screen
 *  Search_UnderMouse  2 <-- any under the mouse 
 *  Search_WhereMask   3
 *   -- or with:--
 *  Search_SameLevel        (0<<4) <-- object and its siblings only
 *  Search_SameOrUnderLevel (1<<4) <-- all descendents of object's parent
 *  Search_HowMask          (12)
 * </pre>
 * \todo *** implement these
 */
#define Search_None    0
#define Search_Find    1
#define Search_Select  2

#define Search_Any              0
#define Search_Visible          1
#define Search_UnderMouse       2 
#define Search_WhereMask        3

#define Search_SameLevel        (0<<4)
#define Search_SameOrUnderLevel (1<<4)
#define Search_HowMask          (12)


#define VIEW_NORMAL      0
#define VIEW_GRAB_COLOR  1

//! Constructor, set up initial dp->ctm, init various things, call setupthings(), and set workspace bounds.
/*! Incs count on newdoc. */
LaidoutViewport::LaidoutViewport(Document *newdoc)
	: ViewportWindow(NULL,"laidoutviewport",NULL,ANXWIN_HOVER_FOCUS|ANXWIN_DOUBLEBUFFER|VIEWPORT_ROTATABLE,0,0,0,0,0)
{
	DBG cerr <<"in LaidoutViewport constructor, obj "<<object_id<<endl;

	dp->displayer_style|=DISPLAYER_NO_SHEAR;
	papergroup=NULL;
	win_colors->bg=rgbcolor(255,255,255);

	viewportmode=VIEW_NORMAL;
	showstate=1;
	lfirsttime=1;
	//drawflags=DRAW_AXES;
	drawflags=0;
	doc=newdoc;
	if (doc) doc->inc_count();
	dp->NewTransform(1.,0.,0.,-1.,0.,0.); //***this should be adjusted for physical dimensions of monitor screen
	
	transform_set(ectm,1,0,0,1,0,0);
	spread=NULL;
	spreadi=-1;
	curpage=NULL;
	pageviewlabel=NULL;

	searchmode=Search_None;
	searchcriteria=Search_Any;
	
	limbo=NULL; //start out as NULL in case we are loading from something. If still NULL in init(), then add new limbo
	
	current_edit_area=-1; //which aspect of a drawable object we are working on.. -1 means curobj is not DrawableObject
	edit_area_icon=NULL;

	
	viewmode=-1;
	SetViewMode(PAGELAYOUT,-1);
	//setupthings();
	
	 // Set workspace bounds.
	if (newdoc && newdoc->imposition) {
		DoubleBBox bb;
		newdoc->imposition->GoodWorkspaceSize(&bb);
		dp->SetSpace(bb.minx,bb.maxx,bb.miny,bb.maxy);
		//Center(); //this doesn't do anything because dp->Minx,Maxx... are 0
	} else {
		dp->SetSpace(-8.5,17,-11,22);
	}

	fakepointer=0; //***for lack of screen record for multipointer
}

//! Delete spread, doc and page are assumed non-local.
/*! Decrements count on limbo and doc.
 */
LaidoutViewport::~LaidoutViewport()
{
	DBG cerr <<"in LaidoutViewport destructor, obj "<<object_id<<endl;

	if (spread) delete spread;
	if (papergroup) papergroup->dec_count();

	if (limbo) limbo->dec_count();
	if (doc) doc->dec_count();

	if (edit_area_icon) edit_area_icon->dec_count();;
}

int LaidoutViewport::UseThisPaperGroup(PaperGroup *group)
{
	if (group==papergroup) return 0;
	if (papergroup) papergroup->dec_count();
	papergroup=group;
	if (papergroup) papergroup->inc_count();
	needtodraw=1;
	return 0;
}

//! Replace existing doc with this doc. NULL is ok.
/*! Return 0 for success or doc already that one, or nonzero error or not changed.
 *
 * If new==old, then do nothing and return 0.
 */
int LaidoutViewport::UseThisDoc(Document *ndoc)
{
	if (doc==ndoc) return 0;

	curpage=NULL;

	if (doc) doc->dec_count();
	doc=ndoc;
	if (doc) doc->inc_count();

	if (doc) {
		if (laidout->curdoc) laidout->curdoc->dec_count();
		laidout->curdoc=doc;
		laidout->curdoc->inc_count();
	}
	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent);
	if (viewer) viewer->setCurdoc(doc);

	DocumentUser *d;
	for (int c=0; c<interfaces.n; c++) {
		d=dynamic_cast<DocumentUser*>(interfaces.e[c]);
		if (d) d->UseThisDocument(doc);
	}

	setupthings();
	ClearSearch();
	clearCurobj();
	Center(1);
	needtodraw=1;
	return 0;
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

//! On any FocusIn event, set laidout->lastview to this.
int LaidoutViewport::FocusOn(const Laxkit::FocusChangeData *e)
{
	laidout->lastview=dynamic_cast<ViewWindow *>(win_parent);
	if (e->target==this) {
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); 
		if (viewer) viewer->SetParentTitle((doc && doc->Name(1)) ? doc->Name(1) :_("(no doc)"));
		
		if (doc && laidout->curdoc!=doc) {
			if (laidout->curdoc) laidout->curdoc->dec_count();
			laidout->curdoc=doc;
			laidout->curdoc->inc_count();
		}
	}
	return anXWindow::FocusOn(e);
}

int LaidoutViewport::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"ViewWindow "<<whattype()<<" got message: "<<mes<<endl;


	if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);
		if (!te || (te->changer && te->changer==static_cast<anXWindow *>(this))) return 1;

		if (te->changetype==TreeObjectRepositioned) {
			needtodraw=1;

		} else if (te->changetype==TreeObjectReorder ||
				te->changetype==TreeObjectDiffPage ||
				te->changetype==TreeObjectDeleted ||
				te->changetype==TreeObjectAdded || 
				te->changetype==TreePagesAdded ||
				te->changetype==TreePagesDeleted ||
				te->changetype==TreePagesMoved) {
			 //***
			int pg=curobjPage();
			if (pg<0 && spread) {
				for (int c=0; c<spread->pagestack.n(); c++) {
					pg=spread->pagestack.e[c]->index;
					if (pg>=0) break;
				}
			}
			curobj.set(NULL, 1, 0); //setting to Limbo
			clearCurobj();
			delete spread;
			spread=NULL;
			if (papergroup && !isDefaultPapergroup(1)) { papergroup->dec_count(); papergroup=NULL; }
			//spreadi=-1;
			setupthings(spreadi,-1);
			needtodraw=1;

		} else if (te->changetype==TreeDocGone) {
			DBG cerr <<"  --LaidoutViewport::DataEvent -> TreeDocGone"<<endl;
			if (doc) {
				int c=0;
				for (c=0; c<laidout->project->docs.n; c++) 
					if (doc==laidout->project->docs.e[c]->doc) break;
				if (c==laidout->project->docs.n) {
					UseThisDoc(NULL);
					needtodraw=1;
				}
			}	
		}
		
		return 0;

	} else if (!strcmp(mes,"prefsChange")) {
		 //Maybe the default units changed or something display related
		DBG cerr << "viewwindow got prefsChange"<<endl;
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		if (s->info1!=PrefsDefaultUnits) return 1;
		if (xruler) xruler->SetCurrentUnits(laidout->prefs.default_units);
		if (yruler) yruler->SetCurrentUnits(laidout->prefs.default_units);
		
		return 0;

	} else if (!strcmp(mes,"rename papergroup")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		if (!isblank(s->str) && papergroup) makestr(papergroup->Name,s->str);
		return 0;

	} else if (!strcmp(mes,"rename limbo")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		if (!isblank(s->str)) makestr(limbo->id,s->str);
		return 0;

	} else if (!strcmp(mes,"image properties")) {
		 //pass on to the first active interface that wants it, if any
		const RefCountedEventData *e=dynamic_cast<const RefCountedEventData *>(data);
		if (!e) return 1;
		ImageInfo *imageinfo=dynamic_cast<ImageInfo *>(e->object);
		if (!imageinfo) return 1;

		for (int c=0; c<interfaces.n; c++) {
			if (interfaces.e[c]->UseThis(imageinfo,imageinfo->mask)) {
				needtodraw=1;
				return 0;
			}
		}

		return 1;

	} else if (!strcmp(mes,"newimposition")) {
		 if (data->type==LAX_onCancel) {
            return 0;
        }

        const RefCountedEventData *r=dynamic_cast<const RefCountedEventData *>(data);
        Imposition *i=dynamic_cast<Imposition *>(const_cast<RefCountedEventData*>(r)->TheObject());
        if (!i) return 0;

		doc->ReImpose(i,r->info1);

        return 0;


	} else if (!strcmp(mes,"rulercornermenu")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		int i=s->info2;

		 //0-999 was document things
		if (i>=0 && i<laidout->project->docs.n) {
			UseThisDoc(laidout->project->docs.e[i]->doc);
			return 0;
		}
		if (i==laidout->project->docs.n) {
			UseThisDoc(NULL);
			return 0;
		}
		if (i==ACTION_AddNewDocument) {
			 //add new document
			app->rundialog(new NewDocWindow(NULL,NULL,"New Document",0,0,0,0,0, 0));
			return 0;

		} else if (i==ACTION_RemoveCurrentDocument) {
			 //Remove current document
			if (!doc) { postmessage(_("How does one remove nothing?")); return 0; }
			Document *sdoc=doc;
			UseThisDoc(NULL);
			laidout->project->Pop(sdoc);
			postmessage(_("Document removed"));
			return 0;

		} else if (i==ACTION_EditImposition) {
			app->rundialog(newImpositionEditor(NULL,"impose",_("Impose..."),this->object_id,"newimposition",
						                        NULL, NULL, doc?doc->imposition:NULL, NULL, doc));
			return 0;
		}


		 //1000..1999 are limbo related
		if (i>=1000 && i<1000+laidout->project->limbos.n()) {
			 //select a limbo
			int l=i-1000;
			if (limbo==laidout->project->limbos.e(l)) return 0;
			for (int c=0; c<interfaces.n; c++) interfaces.e[c]->Clear();
			clearCurobj();
			limbo->dec_count();
			limbo=dynamic_cast<Group *>(laidout->project->limbos.e(l));
			limbo->inc_count();
			needtodraw=1;
			return 0;
		}
		if (i<2000) {
			if (i==ACTION_AddNewLimbo) {
				 //add new limbo with name such as "Limbo 3"
				if (limbo) limbo->dec_count();
				limbo=new Group;//group with 1 count
				limbo->obj_flags|=OBJ_Unselectable|OBJ_Zone;
				char txt[30];
				sprintf(txt,_("Limbo %d"),laidout->project->limbos.n()+1);
				makestr(limbo->id,txt);
				laidout->project->limbos.push(limbo);//adds 1 count
				return 0;
			} else if (i==ACTION_RenameCurrentLimbo) {
				 //rename current limbo
				app->rundialog(new InputDialog(NULL,"rename limbo",NULL,ANXWIN_CENTER,
								0,0,0,0,1,
								NULL, object_id, "rename limbo",
								limbo->id,
								_("New name?"),
								_("Rename"), 1,
								_("Cancel"), 0));
				return 0;
			} else if (i==ACTION_DeleteCurrentLimbo) {
				 //remove current limbo from project, and unlink it from this viewport
				 //***other viewports might link to the limbo. In that case, each
				 //   viewport must select another limbo before it stops using the deleted one..
				 //   would be better to do search for any windows that use it and delete?
				if (laidout->project->limbos.n()==1) {
					postmessage(_("Cannot delete the only limbo."));
					return 0;
				}
				int i=laidout->project->limbos.findindex(limbo);
				if (i<0) return 0;
				laidout->project->limbos.remove(i);
				limbo->dec_count();
				i=(i+1)%laidout->project->limbos.n();
				limbo=dynamic_cast<Group *>(laidout->project->limbos.e(i));
				limbo->inc_count();
				needtodraw=1;
				return 0;
			}
			return 0;
		}

		 //2000-2999 was papergroup things
		if (i>=2000 && i<=2000+laidout->project->papergroups.n) {
			int p=i-2000;
			 //select different paper group
			p--; //so "default" translates to -1
			if (p<0) UseThisPaperGroup(NULL); //***use the default one, must reinstall default!
			else UseThisPaperGroup(laidout->project->papergroups.e[p]);
			if (!strcmp(interfaces.e[0]->whattype(),"PaperInterface")) {
				interfaces.e[0]->UseThis(papergroup);
			}
			return 0;

		} else if (i==ACTION_AddNewPaperGroup) {
			 //new papergroup
			PaperGroup *pg=new PaperGroup;
			pg->Name=new_paper_group_name();
			laidout->project->papergroups.push(pg);
			UseThisPaperGroup(pg);
			return 0;

		} else if (i==ACTION_RenameCurrentPaperGroup) {
			 //rename current paper group
			if (!papergroup) return 0;
			int c=laidout->project->papergroups.findindex(papergroup);
			if (c<0) return 0;
			app->rundialog(new InputDialog(NULL,"rename papergroup",NULL,ANXWIN_CENTER,
							0,0,0,0,1,
							NULL, object_id, "rename papergroup",
							papergroup->Name,
							_("New name?"),
							_("Rename"), 1,
							_("Cancel"), 0));
			return 0;

		} else if (i==ACTION_DeleteCurrentPaperGroup) {
			 //delete current paper group
			if (!papergroup) return 0;
			int c=laidout->project->papergroups.findindex(papergroup);
			if (c<0) return 0;
			if (laidout->project->papergroups.e[c]==papergroup) {
				UseThisPaperGroup(NULL);
				needtodraw=1;
			}
			laidout->project->papergroups.remove(c);

			needtodraw=1;
			return 0;
		}


		if (i==ACTION_SaveProject) {
			 //save to the project file
			cerr << " *** implement SaveProject!!"<<endl;
		} else if (i==ACTION_NewProject) {
			cerr << " *** implement NewProject!!"<<endl;
		} else if (i==ACTION_ToggleSaveAsProject) {
			if (laidout->IsProject()) {
				 //make not a project, rulercornerbutton needs refreshing
				makestr(laidout->project->dir,NULL);
				makestr(laidout->project->filename,NULL);
			} else {
				 //save project as...
				cerr << " *** implement ToggleProject on!!"<<endl;
			}
			ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); 
			if (viewer) viewer->updateProjectStatus();
			return 0;
		}

		//**** change zones? other menu?
		return 0;

	} else if (!strcmp(mes,"moveto")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		 //Move current object to a different context
		int i=s->info2, i2=s->info4;

		VObjContext dest;
		if (i==MOVETO_Limbo) {
			dest.push(0);

		} else if (i==MOVETO_Spread) {
			//dest.push(1);  //spread
			//dest.push(1);  //objects

		} else if (i==MOVETO_Page) {
			if (spread && spread->pagestack.n()>i2 && i2>=0) {
				dest.push(1);  //spread
				dest.push(0);  //pages
				dest.push(i2); //page
				dest.push(0);  //layer
			}

        } else if (i==MOVETO_PaperGroup) {
			if (papergroup) {
				dest.push(2);
			}
        } else if (i==MOVETO_OtherPage) {
			//Move object to document page index i2, centered on page.
			//Then change view to that page?
		}
		if (!dest.context.n()) return 0;

		int new_context_index=MoveObject(&curobj, &dest);
		if (new_context_index>=0) {
			dest.push(new_context_index);
			dest.SetObject(curobj.obj);
			setCurobj(&dest);

			//clear current interfaces or send object tree change message
			laidout->notifyDocTreeChanged(NULL,TreeObjectDiffPage,0,0);
			postmessage(_("Object moved."));
		} else {
			postmessage(_("Failed to move object"));
		}

		return 0;
	}

	return ViewportWindow::Event(data,mes);
}

//! Select the spread with page number greater than current page not in current spread.
/*! Returns the current page index on success, else a negative number.
 *
 * Return -1 for error or the index of the spread.
 */
int LaidoutViewport::NextSpread()
{
	if (!spread || !(doc && doc->imposition)) return -1;
	
	int max=-1;
	max=doc->imposition->NumSpreads(viewmode);

	if (max>=0) {
		spreadi++;
		if (spreadi>=max) spreadi=0;
	} else spreadi=-1;
	setupthings(spreadi,-1);
	needtodraw=1;
	return spreadi;
}

//! Select the spread with page number less than current page not in current spread.
/*! Returns the current page index on success, else a negative number.
 *
 * Return -1 for error or the index of the spread.
 */
int LaidoutViewport::PreviousSpread()
{
	if (!spread || !(doc && doc->imposition)) return -1;
	
	int max=-1;
	max=doc->imposition->NumSpreads(viewmode);

	if (max>=0) {
		spreadi--;
		if (spreadi<0) spreadi=max-1;
	} else spreadi=-1;
	setupthings(spreadi,-1);
	needtodraw=1;
	return spreadi;
}


//! Return the viewmode, and current page index in doc->pages if page!=null.
int LaidoutViewport::ViewMode(int *page)
{
	if (page) {
		*page=curobjPage();
		if (*page<0 && spread && spread->pagestack.n())
			*page=spread->pagestack.e[0]->index;
	}
	return viewmode;
}

//! Set the view style to single, pagelayout, or paperlayout looking at spread number sprd.
/*! Updates and returns pageviewlabel. This is a string like "Page: " or
 * "Spread[2-4]: " that is suitable for the label of a NumSlider.
 */
const char *LaidoutViewport::SetViewMode(int m,int sprd)
{
	DBG cerr <<"---- setviewmode:"<<m<<endl;
	if (sprd<0) sprd=spreadi;
	if (sprd!=spreadi || m!=viewmode) {
		viewmode=m;
		setupthings(sprd,-1);
		Center(1);
	}
	needtodraw=1;
	return Pageviewlabel();
}

//! Update and return pageviewlabel.
/*! This is a label that says what pages are in the current spread, 
 * and also which is the current page.
 *
 * \todo how to internationalize this?
 */
const char *LaidoutViewport::Pageviewlabel()
{
	if (curobj.spread()==0) makestr(pageviewlabel,_("Limbo "));
	else if (curobj.spread()==2) makestr(pageviewlabel,_("Paper "));
	else if (curobj.spread()==1) {
		int pg=curobjPage();
		if (pg>=0 && pg<doc->pages.n) makestr(pageviewlabel,_("Page "));
		else makestr(pageviewlabel,_("?"));
	}
	return pageviewlabel;
}

//! Basically, post a message to viewer->mesbar.
void LaidoutViewport::postmessage(const char *mes)
{
	ViewerWindow *vw=dynamic_cast<ViewerWindow *>(win_parent);
	if (!vw) return;
	vw->PostMessage(mes);
}

//! Called whenever the spread needs updating. Define curpage, spread, curobj context.
/*! If topage==-1, sets curpage to doc->Curpage().
 * If topage!=-1 sets up with page topage.
 * If tospread==-1, then use whatever spread has topage. If tospread!=-1, then definitely
 * use that spread, trying to select topage only if it is in that spread.
 *
 * curobj is set to have its obj==NULL and context corresponding to topage and layer==0.
 * 
 * Also tries to make ectm be the transform to the current page from view coords,
 * so screenpoint = realpoint * ectm * dp->ctm.
 * 
 * Then refetchs the spread for that page. Curobj is set to the context that items should
 * be plopped down into. This is usually a page and a layer. curobj.obj is set to NULL.
 *
 * If doc==NULL or it returns a NULL spread, that is ok. It is assumed that means it
 * is a "Whatever" mode, which is basically just the limbo scratchboard and no spread.
 */
void LaidoutViewport::setupthings(int tospread, int topage)//tospread=-1
{
	 // set curobj to proper value
	 // Also call Clear() on all interfaces
	if (tospread==-1 && topage==-1) {
		 // initialize from nothing
		if (doc) topage=doc->curpage;
	} 
	if (doc) {
		if (tospread==-1 && topage>=0) {
			tospread=doc->imposition->SpreadFromPage(viewmode,topage);
		}
		
		int max=-1;
		if (doc->imposition) max=doc->imposition->NumSpreads(viewmode);

		 // clamp tospread to the imposition's spread range
		if (max>=0) {
			if (tospread>=max) tospread=max-1;
			else if (tospread<0) tospread=0;
		} else tospread=-1;

		 // clamp topage to the pages in doc
		if (topage<0) topage=0;
		else if (topage>=doc->pages.n) topage=doc->pages.n;
	} else tospread=topage=-1; 

	 // Now tospread and topage should both be >= 0 or both == -1
	
	 // clear old data from being used
	for (int c=0; c<interfaces.n; c++) interfaces.e[c]->Clear();
	ClearSearch();
	clearCurobj(); //*** clear temp selection group?? -- this clears to limbo or page...
	curpage=NULL;

	 // should always delete and re-new the spread, even if page is in current spread.
	 // since page might be in spread, but in old viewmode...
	if (isDefaultPapergroup(1)) {
		if (papergroup) papergroup->dec_count();
		papergroup=NULL;
	}

	if (spread) { 
		delete spread;
		spread=NULL; 
		spreadi=-1;
	} 

	DBG cerr <<"LaidoutViewport::setupthings:  viewmode="<<viewmode<<"  tospread="<<tospread<<endl;
	 // retrieve the proper spread according to viewmode
	if (!spread && tospread>=0 && doc && doc->imposition) {
		spread=doc->imposition->Layout(viewmode,tospread);
		spreadi=tospread;
		if (!papergroup && spread->papergroup) {
			papergroup=spread->papergroup;
			papergroup->inc_count();
		}
	}

	 // setup ectm (see realtoscreen/screentoreal/Getmag), and find spageindex
	 // ectm is transform between dp and the current page (or current spread if no current page)
	transform_set(ectm,1,0,0,1,0,0);
	
	if (!spread) {
		curobj.set(NULL,1, 0); //set to limbo
		return;
	}

	 //find a good curpage
	int curpagei=-1,    //index in doc->pages of the new curpage
		spageindex=-1; //index in spread->pagestack that holds the new curpage
	if (viewmode==SINGLELAYOUT) {
		curpage=doc->pages.e[tospread];
		curpagei=tospread;
		spageindex=0;
	} else {
		int c;
		for (c=0; c<spread->pagestack.n(); c++) {
			if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) {
				if (topage==-1 || topage==spread->pagestack.e[c]->index) {
					curpagei=spread->pagestack.e[c]->index;
					curpage=doc->pages.e[curpagei];
					spageindex=c;
					break;
				}
			}
		}
		if (curpagei==-1) for (c=0; c<spread->pagestack.n(); c++) {
			if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) {
				curpagei=spread->pagestack.e[c]->index;
				curpage=doc->pages.e[curpagei];
				spageindex=c;
				break;
			}
		}
	}
	
	 //set up curobj
	 // in the future, this could involve apply further groups...
	//...like any saved cur group for that page or layer?...
	VObjContext voc;
	voc.set(NULL,4, 1,0,spageindex,0); //spread.pages.pagenumber.layernumber
	setCurobj(&voc);

	char scratch[50];
	sprintf(scratch,_("Viewing spread number %d/%d"),spreadi+1,doc->imposition->NumSpreads(viewmode));
	postmessage(scratch);
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
 * \todo *** imp option to plop near mouse if mouse on screen. maybe just do 
 *    ChangeContext(x,y); then NewData(data,null)?
 */
int LaidoutViewport::PlopData(LaxInterfaces::SomeData *ndata,char nearmouse)
{ // *** is this function useful at all??? duplicates NewData too much perhaps??
	if (!ndata) return 0;
	
	//if (nearmouse) {
	//	***
	//}
	
	NewData(ndata,NULL);
	if (curobj.obj) {
		 // activate right tool for object
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
		if (viewer) viewer->SelectToolFor(curobj.obj->whattype(),&curobj);
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
 */
int LaidoutViewport::DeleteObject()
{
	 // calls Clear(d) on all interfaces, checks in data
	SomeData *d=curobj.obj;
	if (!d) return -1;

	 // remove d from wherever it's at:
	Group *o=dynamic_cast<Group*>(getanObject(curobj.context,0,curobj.context.n()-1));
	if (!o && curobj.spread()==2) o=&papergroup->objs; // *** must find way to automate this!!
	if (!o) return -1; //parent object not found!
	o->remove(curobj.context.e(curobj.context.n()-1));
	
	 // clear d from interfaces and check in 
	for (int c=0; c<interfaces.n; c++) {
		interfaces.e[c]->Clear(d);
	}
	laidout->notifyDocTreeChanged(this,TreeObjectDeleted, curobjPage(), -1);
	clearCurobj(); //this calls dec_count() on the object
	//ClearSearch();
	needtodraw=1;
	return 1;
}

int LaidoutViewport::NewData(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext **oc)
{
	if (!d) return -1;

	DBG curobj.context.out(curobj.obj?"NewData (at obj) with context:":"NewData (no obj) with context");

	 //convert curobj into a context if it currently points to an object
	if (curobj.obj) {
		curobj.context.pop();
		curobj.SetObject(NULL);
	}
	Group *o=dynamic_cast<Group*>(getanObject(curobj.context,0,-1));
	if (!o) return -1; //context had to be a Group!

	int c=o->push(d);
	curobj.context.push(c);
	curobj.SetObject(d);

	if (oc) *oc=&curobj;

	laidout->notifyDocTreeChanged(this,TreeObjectAdded, curobjPage(), -1);
	needtodraw=1;
	
	return c;
}

//! -2==prev, -1==next obj, else select obj i??
/*! Return 0 if curobj not changed, else nonzero.
 *
 * \todo *** this is rather broken just at the moment, does only -1 and -2.
 * \todo *** implement searchcriteria, one criteria is search under particular coords, another
 *   criteria is step through all on screen objects, another step through all in spread,
 *   another step through all in current layer, etc
 */
int LaidoutViewport::SelectObject(int i)
{
	if (!curobj.obj) {
		findAny();
		if (firstobj.obj) setCurobj(&firstobj);
		else return 0;

	} else if (i==-2 || i==-1) { //prev or next
		VObjContext o;
		o=curobj;
		DBG curobj.context.out("Finding Object, curobj:");
		if (searchmode!=Search_Select) {
			ClearSearch();
			firstobj=curobj;
			searchmode=Search_Select;
		}
		DBG o.context.out("Finding Object adjacent to :");
		if (nextObject(&o,i==-2?0:1)!=1) {
			firstobj=curobj;
			o=curobj;
			if (nextObject(&o,i==-2?0:1)!=1) { searchmode=Search_None; return 0; }
		}
		setCurobj(&o);

	} else return 0;
	
	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
	if (viewer) {
		viewer->SelectToolFor(curobj.obj->whattype(),&curobj);
		viewer->updateContext(1);
	}
	return 1;
}

//! Change the current object to be d.
/*! Item must be in the tree already. You may not push objects from here. For
 * that, use NewData().
 *
 * If d!=NULL, then try to make that object the current object. It must be within
 * the current spread somewhere. If d==NULL, then the same goes for where oc points to.
 * The first interface to report being able to handle d->whattype() will be activated.
 *
 * Returns 1 for current object changed, otherwise 0 for not changed or d not found.
 *
 * \todo *** for laxkit also, but must have some mechanism to optionally pass the LBDown grab
 * to new interface in control of new object..
 */
int LaidoutViewport::ChangeObject(LaxInterfaces::ObjectContext *oc, int switchtool)
{
	if (!oc || !oc->obj) return 0;

	VObjContext *voc=dynamic_cast<VObjContext *>(oc);
	if (voc==NULL || !validContext(voc)) return 0;
	setCurobj(voc);
	
	 // makes sure curtool can take it, and makes it take it.
	if (switchtool) {
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
		if (viewer) {
			viewer->SelectToolFor(curobj.obj->whattype(),&curobj);
			viewer->updateContext(1);
		}
	}

	return 1;
}

//! Find object in current spread underneath screen coordinate x,y.
/*! If an interfaces receives a lbdown outside of their object, then it would
 * call viewport->FindObject, which will possibly return an object that the 
 * interface can handle. If so, the interface would call ChangeData with it. The interface
 * can keep searching until it finds one it can handle.
 * If FindObject does not return something the interface can handle, then
 * it should call ChangeObject(with that other data).
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
	if (searchmode!=Search_Find || start || x!=searchx || y!=searchy) { //init search
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
		searchmode=Search_Find;
		
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
	DBG cerr <<"lov.FindObject: "<<p.x<<','<<p.y<<endl;

	double m[6];
	DBG firstobj.context.out("firstobj");
	
	//while (start || (!start && !(nextindex==firstobj))) {
	int nob=1;
	while (nob==1) {
		if (start) start=0;
		if (nextindex.obj==exclude) {
			nob=nextObject(&nextindex);
			continue;
		}
		
		 //transform point to be in nextindex coords
		transformToContext(m,nextindex.context,1,nextindex.context.n()-1);

		pp=transform_point(m,p);
		DBG cerr <<"lov.FindObject oc: "; nextindex.context.out("");
		DBG cerr <<"lov.FindObject pp: "<<pp.x<<','<<pp.y<<"  check on "
		DBG		<<nextindex.obj->object_id<<" ("<<nextindex.obj->whattype()<<") "<<endl;

		if (nextindex.obj->pointin(pp)) {
			DBG cerr <<" -- found"<<endl;
			if (!foundobj.obj) foundobj=nextindex;
			if (searchtype && strcmp(nextindex.obj->whattype(),searchtype)) {
				nob=nextObject(&nextindex);
				continue;
			}
			 // matching object found!
			foundtypeobj=nextindex;
			if (oc) *oc=&foundtypeobj;
			DBG foundtypeobj.context.out("  foundtype");//for debugging
			return 1;
		}
		DBG cerr <<" -- not found in "<<nextindex.obj->object_id<<endl;
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

//! Find multiple objects in box.
/*! Return value is the number of objects found. If none, then date_ret and c_ret 
 * are set to NULL.
 *
 * The returned object contexts must be delete'd.
 *
 * \todo *** this needs more thought, as does FindObject(), need to implement checking
 *   of permissions for selecting objects, zones, and rect touching/inside. Also, would
 *   be nice to have a lasso selector.
 */
int LaidoutViewport::FindObjects(Laxkit::DoubleBBox *box, char real, char ascurobj,
							SomeData ***data_ret, ObjectContext ***c_ret)
{
	 //init the search, if necessary
	VObjContext nextindex;
	
	 //init search
	foundobj.clear();
	firstobj.clear();
	 
	 // Set up firstobj
	FieldPlace context;
	firstobj=curobj; //***need validation check
	if (!firstobj.obj) findAny();
	if (!firstobj.obj) return 0;
	nextindex=firstobj;
	
	searchmode=Search_Find;
	
	foundtypeobj.clear(); // this one is always reset?
	if (!firstobj.obj) return 0;

	 // nextindex now points to the first object to consider.
	 
	double mm[6];
	//DoubleBBox obox;
	DBG firstobj.context.out("firstobj");
	
	int nob=1;
	VObjContext *obj=NULL;
	PtrStack<VObjContext> objects;
	do {
		 //find transform from nextindex coords
		transformToContext(mm,nextindex.context,0,-1);

		DBG cerr <<"lov.FindObject oc: "; nextindex.context.out("");
		DBG	if (nextindex.obj) cerr <<nextindex.obj->object_id<<" ("<<nextindex.obj->whattype()<<") "<<endl;

		if (box->intersect(mm,nextindex.obj,1,0)) {
			// *** 1st approximation of intersection! maybe now check for actual touching?

			 // matching object found! add to list that gets returned
			DBG cerr <<" -- found"<<endl;
			if (!foundobj.obj) foundobj=nextindex;

			obj=new VObjContext;
			*obj=nextindex;
			objects.push(obj,1);
			DBG foundobj.context.out("  foundobj");//for debugging
		} else {
			DBG cerr <<" -- not found in "<<nextindex.obj->object_id<<endl;
		}
		nob=nextObject(&nextindex);
	} while (nob);

	int n=0;
	if (c_ret) *c_ret=(ObjectContext **)objects.extractArrays(NULL,&n);
	if (data_ret) *data_ret=NULL;
	
	return n; // search ended
}

double *LaidoutViewport::transformToContext(double *m,ObjectContext *oc,int invert,int full)
{
	VObjContext *o=dynamic_cast<VObjContext*>(oc);
	if (!o) return NULL;

	if (!m) m=new double[6];
	transformToContext(m,o->context,invert,full?-1:o->context.n()-1);
	return m;
}

//! Make a transform that takes a point from viewer past all objects in place.
/*! 
 * If invert!=0, then return a transform that transforms a point from the place
 * to the viewer, which is analogous to the SomeData::m().
 * 
 * \todo *** perhaps return how many transforms were applied, so 0 means success, 
 *   >0 would mean that many ignored. maybe have some other special 
 *   return for invalid context?
 * \todo ***work this out!! Say an object is g1.g2.g3.obj corresponding to a place something like
 *   "1.2.3", then m gets filled with g3->m()*g2->m()*g1->m(),
 *   where the m() are the matrix as used in Laxkit::Displayer discussion.
 */
void LaidoutViewport::transformToContext(double *m,FieldPlace &place, int invert, int depth)
{
	if (depth<0) depth=place.n();
	transform_identity(m);
	ObjectContainer *oc=this;
	const double *om;
	double mm[6];
	for (int i=0; i<depth; i++) {
		if (!oc) break;

		om=oc->object_transform(place.e(i));
		if (om) {
			transform_mult(mm,om,m);
			transform_copy(m,mm);
		}

		oc=dynamic_cast<ObjectContainer*>(oc->object_e(place.e(i)));
	}

	//now m is a transform that transforms points at place's object coordinates to base coordinates

	if (invert) {
		transform_invert(mm,m); 
		transform_copy(m,mm);
	}
}

//! Step oc to next object. 
/*! If inc==1 then increment indices, else decrement indices.
 *
 * \todo *** update this:
 *
 * This is used to step through object tree no matter how deep.
 * For groups, steps through the group's contents first, then the group
 * itself, then moves on to a sibling of that group, etc.
 *
 * If oc points to similar position in firstobj, then it is assumed there has been
 * wrap around at this level. This also implies that all subobjects of what oc points
 * to have already been stepped through.
 * 
 * Returns 0 if there isn't a next obj for some reason, and 1 if there is.
 * If the object stepped to == firstobj, then return 0;
 */
int LaidoutViewport::nextObject(VObjContext *oc,int inc)//inc=0
{
	int c;
	anObject *d=NULL;
	DBG int cn=1;
	do {
		DBG cerr <<"LaidoutViewport->nextObject count="<<cn++<<endl;

		c=ObjectContainer::nextObject(oc->context,0,Next_SkipLockedKids|(inc?Next_Increment:Next_Decrement),&d);
		oc->SetObject(dynamic_cast<SomeData *>(d));
		if (c==Next_Error) return 0; //error finding a next

		 //at this point oc/d might be a somedata, or it might be a container
		 //like this, or a spread..
		if (*oc==firstobj) return 0; //wrapped around to first
		
		 //if is Unselectable, then continue. Else return the found object.
		if (!(oc->obj)) continue;
		if (dynamic_cast<ObjectContainer*>(oc->obj) 
			  && (dynamic_cast<ObjectContainer*>(oc->obj)->object_flags() & OBJ_Unselectable)) {
			continue;
		}

		return 1;
	} while (1);
	
	return 0;
}

// ****replace the other locateObject with this one:
// return the object at place(offset).place(offset+1)...place(offset+n-1)
// If n==0, then use the rest of place from offset.
//LaxInterfaces::SomeData *LaidoutViewport::locateObject(FieldPlace &place,int offset,int n)
//{***}

//! Return place.n if d is found in the displayed pages or in limbo somewhere, and put location in place.
/*! Flushes place whether or not the object is found, it does not append to an existing spot.
 *
 * If object is not found, then return 0, despite whatever place.n is.
 */
int LaidoutViewport::locateObject(LaxInterfaces::SomeData *d,FieldPlace &place)
{
	place.flush();
	 // check limbo
	if (limbo->n()) {
		if (limbo->contains(d,place)) {
			place.push(0,0);
			DBG place.out(" locateObject limbo: ");
			return place.n();
		}
	}

	 //check displayed pages
	if (!spread || !spread->pagestack.n()) return 0;
	int page;
	Page *pagep;
	for (int spage=0; spage<spread->pagestack.n(); spage++) {
		page=spread->pagestack.e[spage]->index;
		if (page<0) continue;
		pagep=doc->pages.e[page];
		place.flush();
		if (pagep->contains(d,place)) { // this pushes location onto top of place..
			place.push(spage,0);
			place.push(0,0);
			place.push(1,0);
			DBG place.out(" locateObject spread: ");
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
		for (c=0; c<spread->pagestack.n(); c++) {
			page=dynamic_cast<Page *>(spread->pagestack.object_e(c));
			if (!page) continue;
			for (c2=page->layers.n()-1; c2>=0; c2--) {
				DBG cerr <<" findAny: pg="<<c<<":"<<spread->pagestack.e[c]->index<<"  has "<<page->e(c2)->n()<<" objs"<<endl;
				if (!page->e(c2)->n()) continue;
				firstobj.set(page->e(c2)->e(page->e(c2)->n()-1),5, 1,0,c,c2,page->e(c2)->n()-1);
				DBG firstobj.context.out(" findAny found: ");
				return;
			}
		}
	}
	if (!obj) {
		if (limbo->n()) {
			firstobj.set(limbo->e(0),2,0,0);
			DBG firstobj.context.out(" findAny found: ");
			return;
		}	
	}
	if (!obj && papergroup && papergroup->n()) {
		firstobj.set(dynamic_cast<SomeData*>(papergroup->object_e(0)), 2, 2,0);
		DBG firstobj.context.out(" findAny found: ");
		return;
	}
}

//! Check whether oc is a valid object context.
/*! Assumes oc->obj is what you actually want, and checks oc->context against it.
 * Object has to be in the spread or in limbo for it to be valid.
 * Note that oc->obj is a SomeData. So if the context is limbo or a spread,
 * rather than an object contained in it, it is technically an invalid context..
 *
 * Return 0 if invalid,
 * otherwise return 1 if it is valid.
 *
 * \todo might be useful to check against all objects in tree, not just for
 *   selectable SomeDatas
 */
int LaidoutViewport::validContext(VObjContext *oc)
{
	anObject *anobj=getanObject(oc->context,0,-1);
	return anobj==oc->obj;
}

//! Set curobj to limbo, which is always valid. This is done after page removal, for instance, to prevent crashing.
int LaidoutViewport::wipeContext()
{
	curobj.set(NULL,1, 0); //default just setting to limbo, since limbo is always valid.
	return 1;
}

//! Clear firstobj,foundobj,foundobjtype.
void LaidoutViewport::ClearSearch()
{
	firstobj.clear();
	foundobj.clear();
	foundtypeobj.clear();
	searchx=searchy=-1;
	searchtype=NULL;
	searchmode=Search_None;
}

//! Update ectm. If voc!=NULL, then make curobj point to the same thing that voc points to.
/*! Incs count of curobj if new obj is different than old curobj.obj
 * and decrements the old curobj.
 */
void LaidoutViewport::setCurobj(VObjContext *voc)
{
	if (voc) {
		curobj=*voc;//incs voc->obj count, decs old curobj.obj
	}

	FieldPlace place; 
	place=curobj.context;
	if (curobj.obj) place.pop();
	
	transformToContext(ectm,place,0,-1);
	 
	 //find curpage
	if (curobj.spread()==0 || !spread) curpage=NULL;
	else {
		if (curobj.spreadpage()>=0) {
			curpage=spread->pagestack.e[curobj.spreadpage()]->page;
			if (!curpage) {
				DBG cerr <<"** warning! in setCurobj, curpage was not defined for curobj context"<<endl;
				curpage=doc->pages.e[spread->pagestack.e[curobj.spreadpage()]->index];
			}
		}
		else curpage=NULL;
	}
	
	DBG if (curobj.obj) cerr <<"setCurobj: "<<curobj.obj->object_id<<" ("<<curobj.obj->whattype()<<") ";
	DBG curobj.context.out("setCurobj: ");//debugging

	if (!lfirsttime) {
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); 
		if (viewer) viewer->updateContext(1);
	}
}

//! Strip down curobj so that it points to only a context, not an object. Calls dec_count() on the old object if any.
void LaidoutViewport::clearCurobj()
{
	//***for nested objects, this does not quite work!! This zaps to base of current zone, whether limbo,
	//papergroup, or page layer.

	if (!doc || curobj.spread()==0) { // is limbo
		curobj.set(NULL,1, 0);
		return;
	}
	if (curobj.spread()==2 && papergroup) {
		curobj.set(NULL,1, 2);
		return;
	}
	curobj.set(NULL,4,curobj.spread(),     //spread
					  0,                   //pages
					  curobj.spreadpage(), //pagenumber
					  curobj.layer());     //layer num
	setCurobj(NULL);
}

//! Intercept for the color grabber.
int LaidoutViewport::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse)
{
	if (viewportmode==VIEW_GRAB_COLOR) {
		buttondown.down(mouse->id,LEFTBUTTON);
		MouseMove(x,y,state,mouse);
		return 0;
	}
	return ViewportWindow::LBDown(x,y,state,count,mouse);
}

//! Intercept to reset to normal from the color grabber.
int LaidoutViewport::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (viewportmode==VIEW_GRAB_COLOR) {
		buttondown.up(mouse->id,~LEFTBUTTON);
		viewportmode=VIEW_NORMAL;
		const_cast<LaxMouse*>(mouse)->setMouseShape(this,0);
		return 0;
	}
	return ViewportWindow::LBUp(x,y,state,mouse);
}

//! *** for debugging, show which page mouse is over..
int LaidoutViewport::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	//**** for screencasting, do a fake mouse pointer for my second mouse
	//cerr <<"xid:"<<mouse->xid<<endl;
	//if (mouse->xid==15) { needtodraw=1; fakepointer=1; fakepos=flatpoint(x,y); }

	last_mouse=mouse->id;

	if (viewportmode==VIEW_GRAB_COLOR) {
		//***on focus off, reset mode to normal
			//click down starts grabbing color
			//click up ends grabbing, and sets viewmode no to normal
		if (buttondown.isdown(mouse->id,LEFTBUTTON)) {
			unsigned long pix=screen_color_at_mouse(mouse->id);
			int r,g,b;
			colorrgb(pix,&r,&g,&b);
			DBG cerr << "grab color:"<<r<<','<<g<<','<<b<<endl;

			SimpleColorEventData *e=new SimpleColorEventData(255,r,g,b,255, 0);
			app->SendMessage(e,win_parent->object_id,"curcolor",object_id);
		}
		return 0;
	}

	DBG if (!buttondown.any()) {
	DBG 	int c=-1;
	DBG 	flatpoint p=dp->screentoreal(x,y);//viewer coordinates
	DBG 	cerr <<" realp:"<<p.x<<","<<p.y<<"  ";
	DBG 	if (spread) {
	DBG 		for (c=0; c<spread->pagestack.n(); c++) {
	DBG 			if (spread->pagestack.e[c]->outline->pointin(p)) break;
	DBG 		}
	DBG 		if (c==spread->pagestack.n()) c=-1;
	DBG 	}
	DBG 	cerr <<"mouse over: "<<c<<", page "<<(c>=0?spread->pagestack.e[c]->index:-1)<<endl;
	DBG }
	//DBG ObjectContext *oc=NULL;
	//DBG int cc=FindObject(x,y,NULL,NULL,1,&oc);
	//DBG cerr <<"============mouse move found:"<<cc;
	//DBG if (oc) dynamic_cast<VObjContext*>(oc)->context.out("  "); else cerr <<endl;

	return ViewportWindow::MouseMove(x,y,state,mouse);
}

//! Set up the viewport context to oc.
/*! If oc contains an object, then the final entry in its context is ignored,
 * and only the context before it is used. If oc has no object then oc's full
 * context is used.
 *
 * This clears the current object, and resets the search.
 */
int LaidoutViewport::ChangeContext(LaxInterfaces::ObjectContext *oc)
{
	DBG cerr <<"ChangeContext to supplied oc"<<endl;
	VObjContext *loc=dynamic_cast<VObjContext *>(oc);
	if (!loc) return 0;
	if (loc->obj) {
		 // strip down obj, we want only the plain context
		VObjContext noc;
		noc=*loc;
		noc.SetObject(NULL);
		noc.context.pop();
		setCurobj(&noc);
	} else setCurobj(loc);
	ClearSearch(); //these must be called after setCurobj() since
	clearCurobj(); //  oc is likely a search context

	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); 
	if (viewer) viewer->updateContext(1);
	return 1;
}
	
//! Call this to update the context for screen coordinate (x,y).
/*! This sets curobj to be at layer level of curobj if possible, otherwise
 * to the top level of whatever page is under screen coordinate (x,y).
 *
 * Clears old curobj (via clearCurobj()) and sets oc to point to the new curobj.
 * The previous contents of oc are ignored.
 *
 * Return 0 for context changed, nonzero for not.
 */
int LaidoutViewport::ChangeContext(int x,int y,LaxInterfaces::ObjectContext **oc)
{
	ClearSearch();
	clearCurobj();
	flatpoint p=dp->screentoreal(x,y);
	if (curobj.spread()==1 && curobj.spreadpage()>=0 
			&& spread->pagestack.e[curobj.spreadpage()]->outline->pointin(p)) {
		DBG curobj.context.out("context change");
		return 1; // context was cur page***
	}
	 // else must search for it
	curobj.context.flush();
	int c;

	 //check if in pages of a spread
	if (spread) {
		for (c=0; c<spread->pagestack.n(); c++) {
			if (spread->pagestack.e[c]->outline->pointin(p)) break;
		}
		if (c<spread->pagestack.n() && spread->pagestack.e[c]->index>=0) {
			curobj.context.push(1); //for spread
			curobj.context.push(0); //for pages
			curobj.context.push(c); //particular page
			curobj.context.push(spread->pagestack.e[c]->page->layers.n()-1); //top layer
			 // apply page transform
			transform_copy(ectm,spread->pagestack.e[c]->outline->m()); // *** only if first few contexts do not add transforms!
		}
	}

	 //check if in current papergroup
	if (curobj.context.n()==0 && papergroup && papergroup->papers.n) {
		flatpoint p;
		for (int c=0; c<papergroup->papers.n; c++) {
			p=dp->screentoreal(x,y);
			if (papergroup->papers.e[c]->pointin(p)) {
				curobj.context.push(2);
				transform_identity(ectm);
				break;
			}
		}
	}

	 //otherwise assume limbo
	if (curobj.context.n()==0) { // is limbo
		curobj.context.push(0);
		transform_identity(ectm);
	}

	if (curobjPage()>=0) curpage=doc->pages.e[curobjPage()];
		else curpage=NULL;

	if (oc) *oc=&curobj;

	ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); 
	if (viewer) viewer->updateContext(1);

	DBG curobj.context.out("context change"); 
	return 1;
}

//! Return whether LaidoutViewport::papergroup is the default papergroup of the current spread, or optionally if known by the current project.
int LaidoutViewport::isDefaultPapergroup(int yes_if_in_project)
{
	if (!papergroup) return 0;
	if (papergroup && spread && spread->papergroup==papergroup) return 1;
	if (!yes_if_in_project) return 0;

	for (int c=0; c<laidout->project->papergroups.n; c++) {
		if (laidout->project->papergroups.e[c]==papergroup) return 1;
	}
	return 0;
}


/*! Called when moving objects around, this ensures that a move can
 * make the object go to a different page, or to limbo.
 *
 * A change of context only occurs if the layer is a direct child of either
 * limbo, papergroup, or a page layer. Otherwise, it stays in its context.
 *
 * If the parent is limbo, then any partial containment transfers the object to 
 * the papergroup or page. Conversely, if the parent is a layer on a page,
 * the object has to become totally uncontained by the layer for it to enter something else.
 * If the object is in the papergroup, it has to totally leave to enter limbo, but only
 * partially leave to enter a page.
 *
 * The object's new parent is the first one found that at least partially contains the object,
 * with a preference of pages, then papergroup, then limbo. The object is placed on the top layer 
 * of the page.
 *
 * Returns oc if changes made (and modifies oc to new context), or NULL if no modifications or
 * invalid context.
 *
 * \todo *** the intersection detection for laidout in general is currently rather poor. goes
 *   entirely by the rectangular bounds. need some way to do that as a first check,
 *   then check internals some how.. means intersection arbitrary non-convex polygons... something
 *   of the kind will be necessary eventually anyway to compute runaround.
 */
LaxInterfaces::ObjectContext *LaidoutViewport::ObjectMoved(LaxInterfaces::ObjectContext *oc, int modifyoc)
{
	DBG cerr <<"ObjectMoved "<<oc->obj->object_id<<": ";
	DBG curobj.context.out(NULL);
	
	if (!oc || !oc->obj) return NULL;

	if (oc->obj!=curobj.obj) {
		if (!validContext(dynamic_cast<VObjContext*>(oc))) return NULL;
		setCurobj(dynamic_cast<VObjContext*>(oc));
	}

	SomeData *d=oc->obj;

	 //only reparent when is a base object, that is, not in a group object.
	 //This means it is a direct child of the papergroup, spread, a page layer, or limbo.
	VObjContext *voc=&curobj;
	ObjectContainer *objc=this;
	for (int c=0; c<voc->context.n(); c++) {
		objc=dynamic_cast<ObjectContainer*>(objc->object_e(voc->context.e(c)));
		if (!objc || !(objc->object_flags()&OBJ_Zone)) {
			if (c!=voc->context.n()-1) return NULL;
			break;
		}
	}

	 //Current bases are:
	 //  1. limbo          view.0
	 //  2. papergroup     view.2  <- currently papergroup.objs only. May change to apply objs to specific papers
	 //  3. spread marks   view.1.1
	 //  4. page layer     view.1.0.*.*
	 //
	 //  Todo: Need some way to automate this a bit. maybe rank zones for preference,
	 //        so that page layers would be 1st, then papergroup, then spread marks, then limbo.
	 //        Spread is tricky, as it has no bounds. no clear way to move from spread to limbo,
	 //        but easy to move from spread to papergroup or page layer.
#define dest_limbo        1
#define dest_papergroup   2
#define dest_spread_marks 3
#define dest_page_layer   4

	DoubleBBox bbox,bbox2;
	SomeData *outline;
	int i=-1;
	double m[6],mm[6],mmm[6];

	 // If is on a page, check whether page still contains curobj
	if (curobj.spreadpage()>=0) { // is on a page layer
		i=curobj.spreadpage();
		outline=spread->pagestack.e[i]->outline;
		bbox.addtobounds(outline->m(),outline);

		transformToContext(m,curobj.context,0,-1);
		bbox2.addtobounds(m,curobj.obj);
		if (bbox.intersect(&bbox2)) {
			DBG cerr <<"  still on page"<<endl;
			return NULL; //still on page
		}
	}

	 //at this point the object is ok to try to move, so find a possibly new destination
	Group *destgroup=NULL;
	VObjContext destcontext;
	 
	 //First, search for some page for the object to be in
	if (spread) {
		for (int c=0; c<spread->pagestack.n(); c++) {
			if (c==i) continue; // don't check current page again!
			if (!spread->pagestack.e[c]->page) continue; //can't move to blank page!

			 //*** this is rather poor, but fast and good enough for now:
			outline=spread->pagestack.e[c]->outline;
			bbox.clear();
			bbox.addtobounds(outline->m(),outline);
			transformToContext(mm,curobj.context,0,-1);
			//transformToContext(m,curobj.context,0,curobj.context.n()-1);
			//transform_mult(mm,curobj.obj->m(),m);
			bbox2.clear();
			bbox2.addtobounds(mm,curobj.obj);
			if (bbox.intersect(&bbox2)) {
				if (!spread->pagestack[c]->page)
					spread->pagestack[c]->page=dynamic_cast<Page *>(doc->object_e(spread->pagestack.e[c]->index));
				destgroup=dynamic_cast<Group*>(spread->pagestack.e[c]->page->layers.e(0));
				destcontext.push(1); //for spread
				destcontext.push(0); //for pages
				destcontext.push(c); //page location stack
				destcontext.push(0); //page layer
				break; //found one
			}
		}
	}

	 //Now search papergroup for overlap with papers
	if (!destgroup && papergroup && papergroup->papers.n) {
		for (int c=0; c<papergroup->papers.n; c++) {
			outline=papergroup->papers.e[c];
			bbox.clear();
			bbox.addtobounds(outline->m(),outline);
			transformToContext(m,curobj.context,0,curobj.context.n()-1);
			transform_mult(mm,curobj.obj->m(),m);
			bbox2.clear();
			bbox2.addtobounds(mm,curobj.obj);
			if (bbox.intersect(&bbox2)) {
				destgroup=&papergroup->objs;
				destcontext.push(2); //for papergroup
				break; //found one
			}
		}
	}

	 //If no destination found, then assume limbo, don't auto drag to spread object
	if (!destgroup) {
		destgroup=limbo;
		destcontext.push(0); //for limbo
	}
	
	 // pop from old place 
	Group *orig=dynamic_cast<Group*>(getanObject(curobj.context,0,curobj.context.n()-1));
	if (orig==destgroup) return NULL; //just in case
	if (!orig && curobj.spread()==2) orig=&papergroup->objs;
	if (!orig) return NULL;

	transformToContext(mm,curobj.context,0,-1); //full transform from old object to base view

	orig->popp(d);
	i=destgroup->push(d);
	d->dec_count();

	transformToContext(mmm,destcontext.context,1,-1); //trans from view to new curobj place
	transform_mult(m,mm,mmm); //compute new object transform
	//transform_mult(m,mmm,mm);
	d->m(m);

	DBG cerr <<"=========NEW MATRIX: "; dumpctm(d->m());
	destcontext.push(i);
	destcontext.SetObject(d);

	setCurobj(&destcontext);

	DBG cerr <<"  moved "<<d->object_id<<" to: ";
	DBG curobj.context.out(NULL);
	needtodraw=1;

	if (!modifyoc) return new VObjContext(destcontext);
	
	VObjContext *noc=dynamic_cast<VObjContext*>(oc);
	*noc=destcontext;
	return oc;
}

//! Move the object from points to, into context to.
/*! Return index of object in the destination context, or -1 if there was a problem putting object there.
 */
int LaidoutViewport::MoveObject(LaxInterfaces::ObjectContext *fromoc, LaxInterfaces::ObjectContext *tooc)
{
	VObjContext *from=dynamic_cast<VObjContext*>(fromoc);
	VObjContext *to  =dynamic_cast<VObjContext*>(tooc);
	if (!from || !to) return -1;

	 // pop from old place 
	Group *orig=dynamic_cast<Group*>(getanObject(from->context,0,from->context.n()-1));
	Group *destgroup=dynamic_cast<Group*>(getanObject(to->context,0,to->context.n()));
	SomeData *d=from->obj;

	 //not all contexts are Groups!
	if (!orig && from->spread()==2) orig=&papergroup->objs;
	if (!destgroup && to->spread()==2) destgroup=&papergroup->objs;
	if (!orig || !destgroup)  return -1;
	if (orig==destgroup) return -1; //just in case

	double m[6],mm[6],mmm[6];
	transformToContext(mm,from->context,0,-1); //full transform from old object to base view

	orig->popp(d);
	int i=destgroup->push(d);
	d->dec_count();

	transformToContext(mmm,to->context,1,-1); //trans from view to new place
	transform_mult(m,mm,mmm); //compute new object transform
	//transform_mult(m,mmm,mm);
	d->m(m);

	DBG cerr <<"=========NEW MATRIX: "; dumpctm(d->m());

	DBG cerr <<"  moved "<<d->object_id<<" to: ";
	DBG to->context.out(NULL);
	needtodraw=1;
	return i;
}

//! Zoom and center various things on the screen
/*! Default is center the current page (w==0).\n
 * w==0 Center page\n
 * w==1 Center spread\n 
 * w==2 Center current selection group***imp me\n
 * w==3 Center current object ***test and fix me\n
 *
 * \todo *** need a center that centers ALL objects in spread and in limbo
 */
void LaidoutViewport::Center(int w)
{
	if (dp->Minx>=dp->Maxx) return;

	if (w==0) { // center page
		 //find the bounding box in dp real units of the page in question...
		if (!spread) return;
		int c=curobj.spreadpage();
		if (c<0) return;
		DoubleBBox bbox;
		bbox.setbounds(spread->pagestack.e[c]->outline);
		double dw,dh;
		dw=(bbox.maxx-bbox.minx)*.05;
		dh=(bbox.maxy-bbox.miny)*.05;
		bbox.minx-=dw;
		bbox.maxx+=dw;
		bbox.miny-=dh;
		bbox.maxy+=dh;
		dp->Center(spread->pagestack.e[c]->outline->m(),&bbox);
		//dp->Center(m,spread->pagestack.e[c]->outline);
		syncrulers();
		needtodraw=1;

	} else if (w==1) { // center spread
		DoubleBBox box,box2;
		if (spread && spread->path) {
			double w=spread->path->maxx-spread->path->minx,
		       	   h=spread->path->maxy-spread->path->miny;
			box2.addtobounds(spread->path->minx-.05*w, spread->path->miny-.05*h);
			box2.addtobounds(spread->path->maxx+.05*w, spread->path->maxy+.05*h);
			box.addtobounds(spread->path->m(),&box2);
		}
		if (papergroup) {
			double w,h;
			for (int c=0; c<papergroup->papers.n; c++) {
				box2.clear();
				w=papergroup->papers.e[c]->maxx-papergroup->papers.e[c]->minx;
		       	h=papergroup->papers.e[c]->maxy-papergroup->papers.e[c]->miny;
				box2.addtobounds(papergroup->papers.e[c]->minx-.05*w, papergroup->papers.e[c]->miny-.05*h);
				box2.addtobounds(papergroup->papers.e[c]->maxx+.05*w, papergroup->papers.e[c]->maxy+.05*h);
				box.addtobounds(papergroup->papers.e[c]->m(),&box2);
			}
		}
			
		dp->Center(&box);
		syncrulers();
		needtodraw=1;

	} else if (w==3) { // center curobj
		if (!curobj.obj) return;
		double m[6];
		transformToContext(m,curobj.context,0,curobj.context.n()-1);
		dp->Center(m,curobj.obj);
	}
}

/*! Establish a new limbo if don't have one yet.
 */
int LaidoutViewport::init()
{
	if (!limbo) {
		limbo=new Group;//group with 1 count
		limbo->obj_flags|=OBJ_Unselectable|OBJ_Zone;
		char txt[30];
		sprintf(txt,_("Limbo %d"),laidout->project->limbos.n()+1);
		makestr(limbo->id,txt);
		laidout->project->limbos.push(limbo);//adds 1 count
	}

	//int e=ViewportWindow::init();
	dp->NewBG(win_colors->bg);
	return 0;
}

//! Make sure that new rulers have the proper units.
int LaidoutViewport::UseTheseRulers(RulerWindow *x,RulerWindow *y)
{
	ViewportWindow::UseTheseRulers(x,y);
	if (xruler) {
		xruler->SetBaseUnits(UNITS_Inches);
		xruler->SetCurrentUnits(laidout->prefs.default_units);
	}
	if (yruler) {
		yruler->SetBaseUnits(UNITS_Inches);
		yruler->SetCurrentUnits(laidout->prefs.default_units);
	}
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
 *
 * </pre>
 *
 * \todo *** have choice whether to back buffer, also smart refreshing, and in diff.
 *   thread.. periodically check to see if more recent refresh requested?
 * \todo *** this is rather horrible, needs near complete revamp, have to decide
 *   on graphics backend. cairo? antigrain?
 * \todo *** implement the 'whatever' page, which is basically just a big whiteboard
 * \todo *** this should be modified so order things are drawn is adjustible,
 *   so limbo then pages, or pages then limbo, or just limbo, etc: drawing zones,
 *   including limbo, spread(s), paper objects, imposition control objects, etc..
 */
void LaidoutViewport::Refresh()
{
	//if (!needtodraw || !win_on) return; assume this is checked for already?
	needtodraw=0;

	if (lfirsttime) { 
		 //setup doublebuffer
		if (lfirsttime==1) Center(1); 
		lfirsttime=0; 
	}
	DBG cerr <<"======= Refreshing LaidoutViewport..";
	
	//DBG flatpoint fp;
	//DBG fp=dp->screentoreal(fp);
	//DBG cerr <<"viewport *****ARG***** "<<fp.x<<','<<fp.y<<endl;

	dp->StartDrawing(this);

	 // draw the scratchboard, just blank out screen..
	dp->ClearWindow();

	if (drawflags&DRAW_AXES) dp->drawaxes();
	int c,c2;


	dp->Updates(0);

	 // draw limbo objects
	DBG cerr <<"drawing limbo objects.."<<endl;
	for (c=0; c<limbo->n(); c++) {
		DrawData(dp,limbo->e(c),NULL,NULL,drawflags);
	}

	 //draw papergroup
	DBG cerr <<"drawing viewport->papergroup.."<<endl;
	if (papergroup) {
		ViewerWindow *vw=dynamic_cast<ViewerWindow *>(win_parent);
		PaperInterface *pi=dynamic_cast<PaperInterface *>(vw->FindInterface("PaperInterface"));
		if (pi) pi->DrawGroup(papergroup,1,1,0);
	}
	
	DBG cerr <<"drawing spread objects.."<<endl;
	if (spread && showstate==1) {
		dp->BlendMode(LAXOP_Source);

//		 //draw the spread's papergroup *** done above
//		PaperGroup *pgrp=NULL;
//		if (!pgrp) pgrp=spread->papergroup;
//		if (pgrp) {
//			ViewerWindow *vw=dynamic_cast<ViewerWindow *>(win_parent);
//			PaperInterface *pi=dynamic_cast<PaperInterface *>(vw->FindInterface("PaperInterface"));
//			if (pi) pi->DrawGroup(pgrp,1,1,0);
//		}

		 // draw 5 pixel offset heavy line like shadow for page first, then fill draw the path...
		 // draw shadow
		if (spread->path) {
			 //draw shadow if papergroup does not exist
			FillStyle fs(0,0,0,0xffff, WindingRule,FillSolid,GXcopy);
			if (!(papergroup && papergroup->papers.n)) {
			//if (!pgrp && !(papergroup && papergroup->papers.n)) { ***
				dp->NewFG(0,0,0);
				dp->PushAxes();
				dp->ShiftScreen(laidout->prefs.pagedropshadow,laidout->prefs.pagedropshadow);
				DrawData(dp,spread->path,NULL,&fs,drawflags); //***,linestyle,fillstyle)
				dp->PopAxes();
			}

			 // draw outline *** must draw filled with paper color
			fs.Color(0xffff,0xffff,0xffff,0xffff);
			//DrawData(dp,spread->path->m(),spread->path,NULL,&fs,drawflags);
			DrawData(dp,spread->path,NULL,&fs,drawflags);
		}

		if (spread->marks) DrawData(dp,spread->marks,NULL,NULL,drawflags);
		 
		 // draw the pages
		Page *page=NULL;
		int pagei=-1;
		flatpoint p;
		SomeData *sd=NULL;
		for (c=0; c<spread->pagestack.n(); c++) {
			DBG cerr <<" drawing from pagestack.e["<<c<<"], which has page "<<spread->pagestack.e[c]->index<<endl;
			page=spread->pagestack.e[c]->page;
			pagei=spread->pagestack.e[c]->index;
			if (!page) { // try to look up page in doc using pagestack->index
				if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) {
					pagei=spread->pagestack.e[c]->index;
					page=spread->pagestack.e[c]->page=doc->pages.e[pagei];
				}
			}
			//if (spread->pagestack.e[c]->index<0) {
			if (!page) {
				 //if no page, then draw an x through the page stack outline
				if (!spread->pagestack.e[c]->outline) continue;
				PathsData *paths=dynamic_cast<PathsData *>(spread->pagestack.e[c]->outline);
				if (!paths || !paths->paths.n) continue;
				Coordinate *p=paths->paths.e[0]->path;
				if (!p) continue;

				flatpoint center=flatpoint((paths->minx+paths->maxx)/2,(paths->miny+paths->maxy)/2);
				p=p->firstPoint(1);
				Coordinate *tp=p;
				dp->LineAttributes(0,LineSolid, CapButt, JoinMiter);
				dp->NewFG(0,0,0);
				do {
					if (tp->flags&POINT_VERTEX) 
						dp->drawline(transform_point(paths->m(),center), transform_point(paths->m(),tp->fp));
					tp=tp->next;
				} while (tp!=p);


				continue;
			}
			 //else we have a page, so draw it all
			sd=spread->pagestack.e[c]->outline;
			dp->PushAndNewTransform(sd->m()); // transform to page coords
			if (drawflags&DRAW_AXES) dp->drawaxes();
			
			if (page->pagestyle->flags&PAGE_CLIPS) {
				 // setup clipping region to be the page
				dp->PushClip(1);
				SetClipFromPaths(dp,sd,dp->Getctm());
			}
			
			 //*** debuggging: draw X over whole page...
	//		DBG dp->NewFG(255,0,0);
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->miny));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->maxy), flatpoint(sd->minx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->maxy), flatpoint(sd->minx,sd->miny));
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->minx,sd->maxy));
	
			 // write page number near the page..
			 // mostly for debugging at the moment, might be useful to have
			 // this be a togglable feature.
			p=dp->realtoscreen(flatpoint(0,0));
			if (page==curpage) dp->NewFG(0,0,0);
			dp->DrawScreen();
			if (page->label) dp->textout((int)p.x,(int)p.y,page->label,-1);
			  else dp->drawnum((int)p.x,(int)p.y,spread->pagestack.e[c]->index+1);
			dp->DrawReal();

			 // Draw page margin path, if any
			SomeData *marginoutline=doc->imposition->GetPageMarginOutline(pagei,1);
			if (marginoutline) {
				DBG cerr <<"********outline bounds ll:"<<marginoutline->minx<<','<<marginoutline->miny
				DBG      <<"  ur:"<<marginoutline->maxx<<','<<marginoutline->maxy<<endl;
				// ***DrawData(dp,marginoutline,&margin_linestyle,NULL,drawflags);
				LineStyle ls(0xa000,0xa000,0xa000,0xffff, 0,CapButt,JoinBevel,0,GXcopy);
				DrawData(dp,marginoutline,&ls,NULL,drawflags);
				marginoutline->dec_count();
			}

			 // Draw all the page's objects.
			for (c2=0; c2<page->layers.n(); c2++) {
				DBG cerr <<"  num objs in page: "<<page->n()<<endl;
				DBG cerr <<"  Layer "<<c2<<", objs.n="<<page->e(c2)->n()<<endl;
				DrawData(dp,page->e(c2),NULL,NULL,drawflags);
			}
			
			if (page->pagestyle->flags&PAGE_CLIPS) {
				 //remove clipping region
				dp->PopClip();
			}

			dp->PopAxes(); // remove page transform
		}
	}
	
	 // Call Refresh for each interface that needs it, ignoring clipping region
	
	 // Refresh interfaces, should draw whatever SomeData they have locked
	 //*** maybe Refresh(drawonly decs?) then remove lock check above?
	//DBG cerr <<"  drawing interface..";
	ObjectContext *oc;
	double m[6];
	for (int c=0; c<interfaces.n; c++) {
		//DBG cerr <<" \ndrawing "<<interfaces.e[c]->whattype()<<" "<<c<<endl;

		oc=interfaces.e[c]->Context();
		if (oc) {
			transformToContext(m,oc,0,-1);
			dp->PushAndNewTransform(m); // transform to curobj coords
		}
		interfaces.e[c]->needtodraw|=2; // should draw decs? *** when interf draws must be worked out..
		interfaces.e[c]->Refresh();
		if (oc) dp->PopAxes();
	}

	 //draw area selector box
	//((LaidoutViewport *)viewport)->locateObject(d,oc.context);
	if (current_edit_area>=0) {
		const char *editareaicons[10]={
				"Streams",
				"Content",
				"EditObject",
				"Wrap",
				"Inset",
				"Clip",
				"Filters",
				"Meta",
				"Chains",
				NULL
				};
		LaxImage *img=laidout->icons.GetIcon(editareaicons[current_edit_area]);
		dp->DrawScreen();
		dp->imageout(img,0,0);
		dp->DrawReal();
		img->dec_count();
	}



	//***for lack of screen record for multipointer
	if (fakepointer) {
		fakepointer=0;
		dp->DrawScreen();
		flatpoint v(-5,-10);
		dp->drawarrow(fakepos-v,v,0,1,2);
		dp->DrawReal();
	}

	dp->Updates(1);

	 // swap buffers
	SwapBuffers();

	DBG cerr <<"======= done refreshing LaidoutViewport.."<<endl;
}


enum ViewActions {
	VIEW_Save,
	VIEW_SaveAs,
	VIEW_NewDocument,
	VIEW_NextTool,
	VIEW_PreviousTool,
	VIEW_NextPage,
	VIEW_PreviousPage,
	VIEW_ObjectTool,
	VIEW_CommandPrompt,
	VIEW_ObjectIndicator,

	VIEW_Help,
	VIEW_About,
	VIEW_SpreadEditor,

	VIEW_MAX
};

enum LaidoutViewportActions {
	LOV_DeselectAll=VIEWPORT_MAX,
	LOV_CenterDrawing,
	LOV_ZoomToPage,
	LOV_GrabColor,
	LOV_ToggleShowState,
	LOV_MoveObjects,
	LOV_ObjUp,
	LOV_ObjDown,
	LOV_ObjFirst,
	LOV_ObjLast,
	LOV_ToggleDrawFlags,
	LOV_ObjectIndicator,
	LOV_MAX
};

Laxkit::ShortcutHandler *LaidoutViewport::GetShortcuts()
{
	if (sc) return sc;

	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("LaidoutViewport");
	if (sc) return sc;

	ViewportWindow::GetShortcuts();

	sc->Add(LOV_DeselectAll,    'A',ShiftMask|ControlMask,0, _("DeselectAll"), _("Deselect all"),NULL,0);
	sc->Add(LOV_CenterDrawing,  '4',0,0,        _("CenterDrawing"),  _("Center drawing"),NULL,0);
	sc->AddShortcut(' ',0,0, LOV_CenterDrawing);
	sc->Add(LOV_ZoomToPage,     '5',0,0,        _("ZoomToPage"),     _("Zoom to the current page"),NULL,0);
	sc->AddShortcut(' ',ShiftMask,0, LOV_ZoomToPage);
	sc->Add(LOV_GrabColor,      'g',0,0,        _("GrabColor"),      _("Grab color"),NULL,0);
	sc->Add(LOV_ToggleShowState,'l',0,0,        _("ToggleShow"),     _("Toggle show state"),NULL,0);
	sc->Add(LOV_MoveObjects,    'm',0,0,        _("MoveObjs"),       _("Move objects"),NULL,0);
	sc->Add(LOV_ObjUp,          LAX_Pgup,0,0,   _("MoveObjUp"),      _("Move object up within layer"),NULL,0);
	sc->Add(LOV_ObjDown,        LAX_Pgdown,0,0, _("MoveObjDown"),    _("Move object down within layer"),NULL,0);
	sc->Add(LOV_ObjFirst,       LAX_Home,0,0,   _("MoveObjFirst"),   _("Move object to first in layer"),NULL,0);
	sc->Add(LOV_ObjLast,        LAX_End,0,0,    _("MoveObjLast"),    _("Move object to last in layer"),NULL,0);
	sc->Add(LOV_ToggleDrawFlags,'D',ShiftMask,0,_("ToggleDrawFlags"),_("Toggle drawing flags"),NULL,0);
	sc->Add(LOV_ObjectIndicator,'i',0,0,       _("Object Info"),    _("Toggle showing object information"),NULL,0);

	return sc;
}

int LaidoutViewport::PerformAction(int action)
{
	// *****
	if (action==LOV_DeselectAll) {
		if (!curobj.obj) return 0;
		
		 // clear curobj from interfaces and check in 
		SomeData *d=curobj.obj;
		for (int c=0; c<interfaces.n; c++) {
			interfaces.e[c]->Clear(d);
		}
		clearCurobj(); //this calls dec_count() on the object
		needtodraw=1;
		return 0;

	} else if (action==LOV_ObjectIndicator) {
		 //switch on indicator overlay
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
		viewer->PerformAction(VIEW_ObjectIndicator);
		needtodraw=1;
		return 0;

	} else if (action==LOV_CenterDrawing) {
		Center(1);
		return 0;

	} else if (action==LOV_ZoomToPage) {
		Center(0);
		return 0;

	} else if (action==LOV_GrabColor) {
		if (viewportmode!=VIEW_GRAB_COLOR) { 
			viewportmode=VIEW_GRAB_COLOR;
			DBG cerr <<"***********************viewport:CURSOR***********************"<<endl;
			LaxMouse *mouse=app->devicemanager->findMouse(last_mouse);
			if (mouse) mouse->setMouseShape(this,LAX_MOUSE_Circle);

		} else if (viewportmode==VIEW_GRAB_COLOR) {
			viewportmode=VIEW_NORMAL;
			LaxMouse *mouse=app->devicemanager->findMouse(last_mouse);
			if (mouse) mouse->setMouseShape(this,0);
		}
		return 0;

	} else if (action==LOV_ToggleShowState) {
		if (showstate==0) showstate=1;
		else showstate=0;
		needtodraw=1;
		return 0;

	} else if (action==LOV_MoveObjects) {
		if (!spread && !papergroup) return 0; //the only context is limbo!
		if (!curobj.obj) return 0; //only move objects!

		DBG cerr<<"**** move object to another page: imp me!"<<endl;
		//if (CirculateObject(9,i,0)) needtodraw=1;

		//Move to:
		//  ---limbo:----
		//  Limbo name
		//  ---papergroup---
		//  Papergroup proper
		//  Paper 1  
		//  Paper 2
		//  ---page---
		//  page 1 of spread
		//* page 2 of spread
		//  Some other page...
		//  ------
		//  Spread

		MenuInfo *menu;
		menu=new MenuInfo("Move to");

		 //---add document list, numbers start at 0
		char buffer[500], *str;

		if (curobj.obj) menu->AddSep(_("Move To"));
		else menu->AddSep(_("New Context"));

		str=limbo->id;
		if (isblank(str)) str=_("Limbo");
		else sprintf(buffer,_("Limbo: %s"),str);
		menu->AddItem(buffer,MOVETO_Limbo,LAX_OFF|LAX_ISTOGGLE|(curobj.spread()==0?LAX_CHECKED:0));

		if (spread) {
			//menu->AddItem(_("Spread"),MOVETO_Spread,LAX_OFF|LAX_ISTOGGLE); //*****how to check
			for (int c=0; c<spread->pagestack.n(); c++) {
				buffer[0]='\0';
				if (spread->pagestack.e[c]->page) {
					str=spread->pagestack.e[c]->page->label;
					if (!isblank(str)) sprintf(buffer,_("Page %s"),str);
				}
				//if (buffer[0]=='\0') sprintf(buffer,_("Page #%d"),c);
				if (buffer[0]!='\0') menu->AddItem(buffer,MOVETO_Page,
									LAX_OFF
									  | LAX_ISTOGGLE
									  | ((curobj.spreadpage()==c)?LAX_CHECKED:0),
									c);
			}
			menu->AddItem(_("Some other page.."),MOVETO_OtherPage,LAX_OFF|LAX_ISTOGGLE);
		}
		if (papergroup) {
			menu->AddItem(_("Paper Group"),MOVETO_PaperGroup,LAX_OFF|LAX_ISTOGGLE|(curobj.spread()==2?LAX_CHECKED:0));
		}

		PopupMenu *popup=new PopupMenu(NULL,_("Move"), 0,
						0,0,0,0, 1, 
						object_id,"moveto", 
						last_mouse, //mouse to position near?
						menu,1, NULL,
						MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT|MENUSEL_DESTROY_ON_LEAVE);
		popup->pad=5;
		popup->WrapToMouse(None);
		app->rundialog(popup);
		return 0;

	} else if (action==LOV_ObjUp) {
		if (!curobj.obj) return 0;

		 //raise selection within layer
		if (CirculateInLayer(1,0,0)) needtodraw=1;
		return 0;

//	} else if (action==LOV_ObjUpLayer) {
//		 //move sel up a layer
//		if (CirculateObject(5,0,0)) needtodraw=1;
//		return 0;
//
//	} else if (action==LOV_LayerUp) {
//		 //layer up 1
//		if (CirculateObject(20,0,0)) needtodraw=1;
//		return 0;

	} else if (action==LOV_ObjDown) {
		if (!curobj.obj) return 0;

		 //lower selection within layer
		if (CirculateInLayer(-1,0,0)) needtodraw=1;
		return 0;

//	} else if (action==LOV_ObjDownLayer) {
//		 //move sel down a layer
//		if (CirculateObject(6,0,0)) needtodraw=1;
//		return 0;
//
//	} else if (action==LOV_LayerDown) {
//		 //move whole layer down 1
//		if (CirculateObject(21,0,0)) needtodraw=1;
//		return 0;

	} else if (action==LOV_ObjFirst) {
		if (curobj.obj) {	//raise selection to top
			if (CirculateInLayer(2,0,0)) needtodraw=1;
		}
		return 0;

	} else if (action==LOV_ObjLast) {
		if (curobj.obj) { //lower selection to bottom
			if (CirculateInLayer(-2,0,0)) needtodraw=1;
		}
		return 0;

	} else if (action==LOV_ToggleDrawFlags) {
		DBG cerr << "----drawflags: "<<drawflags;
		if (drawflags==DRAW_AXES) drawflags=DRAW_BOX;
		else if (drawflags==DRAW_BOX) drawflags=DRAW_BOX|DRAW_AXES;
		else if (drawflags==(DRAW_AXES|DRAW_BOX)) drawflags=0;
		else drawflags=DRAW_AXES;
		DBG cerr << " --> "<<drawflags<<endl;
		needtodraw=1;
		return 0;
	}

	return ViewportWindow::PerformAction(action);
}

//! Key presses...
/*! 
 * \todo **** how best to notify all the indicator widgets? like curtool palettes, curpage indicator, mouse location
 *   indicator, etc...
 *
 * Arrow keys reserved for interface use...
 */
int LaidoutViewport::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	//DBG  //show object mouse is over:
	//DBG if (ch=='f') { //note that these preempt the Laxkit::ViewportWindow reset view. these are dealt separately below
	//DBG 	ObjectContext *oc=NULL;
	//DBG		int x,y;
	//DBG 	d->paired_mouse->getInfo(this,NULL,NULL,&x,&y, NULL,NULL,NULL,NULL);
	//DBG 	int cc=FindObject(x,y,NULL,NULL,1,&oc);
	//DBG 	cerr <<"============mouse move found:"<<cc;
	//DBG 	if (oc) dynamic_cast<VObjContext*>(oc)->context.out("  "); else cerr <<endl;
	//DBG 	return 0;
	//DBG }


	if (ch==LAX_Esc && (state&LAX_STATE_MASK)==0 && viewportmode==VIEW_GRAB_COLOR) {
		 //escape out of grabbing color
		viewportmode=VIEW_NORMAL;
		const_cast<LaxMouse*>(d->paired_mouse)->setMouseShape(this,0);
		return 0;
	}

	 // ask interfaces, and default viewport stuff, which queries all action based activity.
	if (ViewportWindow::CharInput(ch,buffer,len,state,d)==0) return 0;



	if (ch=='M' && (state&LAX_STATE_MASK)==(ShiftMask|AltMask)) {
		DBG  //for debugging to make a delineation in the cerr stuff
		DBG cerr<<"----------------=========<<<<<<<<< *** >>>>>>>>========--------------"<<endl;
		return 0;

	}

	return 1;
}

//! Move curobj object or selection in various ways.
/*! 
 * \todo *** currently ignores objOrSelection until I figure out what i wanted it for anyway...
 * \todo *** most of these things ought to be functions of Document!!
 * \todo *** need an option to move past all overlapping objects! if no overlapping
 *   objs, then just move as normal
 *
 * <pre>
 *  1   Raise in layer
 *  -1   Lower in layer
 *  2   Raise to top
 *  -2   Lower to bottom
 *  4   To index in layer *** imp me!
 *  5   To next layer up *** imp me!
 *  6   To next layer down *** imp me!
 *  7   to previous page *** imp me!
 *  8   to next page *** imp me!
 *  9   to page i (-1==limbo) *** imp me!
 *  10  de-parent obj, put in ancestor layer *** imp me!
 *  
 *  20  Raise layer by 1 *** imp me!
 *  21  Lower layer by 1 *** imp me!
 *  22  Raise layer to top *** imp me!
 *  23  Lower Layer to bottom *** imp me!
 *
 *  30  Throw obj to limbo? *** imp me!
 *  31  Throw layer to limbo? *** imp me!
 * </pre>
 *
 * Returns 1 if object moved, else 0.
 */
int LaidoutViewport::CirculateInLayer(int dir, int i,int objOrSelection)
{
	DBG cerr <<"Circulate: dir="<<dir<<"  i="<<i<<endl;
	if (!curobj.obj) return 0;

	if (dir==1) { // raise in layer
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==g->n()-1) { 
			curobj.push(curpos);
			//DBG curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->swap(curpos,curpos+1);
		curobj.push(curpos+1);
		//DBG curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;

	} else if (dir==-1) { // lower in layer
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==0) { 
			curobj.push(curpos);
			//DBG curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->swap(curpos,curpos-1);
		curobj.push(curpos-1);
		//DBG curobj.context.out("lower by 1");
		needtodraw=1;
		return 1;

	} else if (dir==2) { // raise to top
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==g->n()-1) { 
			curobj.push(curpos);
			//DBG curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->slide(curpos,g->n()-1);
		curobj.push(g->n()-1);
		//DBG curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;

	} else if (dir==-2) { // lower to bottom
		int curpos=curobj.pop();
		Group *g=dynamic_cast<Group *>(getanObject(curobj.context,0));
		if (!g || curpos==0) { 
			curobj.push(curpos);
			//DBG curobj.context.out("raise by 1 fail");
			return 0; 
		}
		g->slide(curpos,0);
		curobj.push(0);
		//DBG curobj.context.out("raise by 1");
		needtodraw=1;
		return 1;

//	} else if (dir==4) { // move to place i in current context
//		cout <<"***move to place i in current context not implemented!"<<endl;
//	} else if (dir==5) { // move to layer above
//		//***move to bottom of next upper layer
//		cout <<"*** move to layer above not implemented!"<<endl;
//	} else if (dir==6) { // move to layer below
//		//***put on top of lower layer
//		cout <<"***move to layer below not implemented!"<<endl;
//	} else if (dir==7) { // move to previous page
//		cout <<"***move to previous page not implemented!"<<endl;
//	} else if (dir==8) { // move to next page
//		cout <<"***move to next page not implemented!"<<endl;
//	} else if (dir==9) { // move to page number i
//		cout <<"***move to page number i not implemented!"<<endl;
	}

	return 0;
}

//! Return the page index in document of the page in curobj, or -1 if not on a page
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
 * \todo *** should be able to remember what was current on different pages...?
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
	setupthings(-1,i); //this always deletes and new's spread
	Center(1);
	DBG cerr <<" SelectPage made page=="<<curobjPage()<<endl;

	needtodraw=1;
	return curobjPage();
}

/*! *** Currently does nothing....should it? ViewerWindow can do right?
 */
int LaidoutViewport::ApplyThis(Laxkit::anObject *thing,unsigned long mask)
{
	return 0;
}


//! For scripting function calls, build the context we are calling from..
ValueHash *LaidoutViewport::build_context()
{
	return NULL;
//	-----
//	ValueHash *context=new ValueHash;
//
//	context->push("viewport", this);
//	context->push("object", curobj***)//current object
//	context->push("selection", ***);//current selection
//	context->push("tools",***); //stack of active tools
//	context->push("parent",win_parent); //parent context
//
//	return context;
}

int LaidoutViewport::type()
{ return VALUE_Fields; }

//! Returns this, but count is incremented.
Value *LaidoutViewport::duplicate()
{ 
	this->inc_count();
	return this;
}

int LaidoutViewport::n()
{
	return 3; //limbo, spread, papergroup
}

#define VOBJE_Limbo       0
#define VOBJE_Spread      1
#define VOBJE_Papergroup  2

//! Return pointer to limbo if i==0, and spread if i==1.
Laxkit::anObject *LaidoutViewport::object_e(int i)
{
	if (i==0) return limbo;
	if (i==1 && spread) return spread;
	if (i==2 && papergroup) return papergroup;
	return NULL;
}

//! Return 0 for success or nonzero for element not found.
int LaidoutViewport::object_e_info(int i, const char **name, const char **Name, int *isarray)
{
	if (i<0 || i>2) return 1;

	if (name) {
		if (i==0) *name="limbo";
		else if (i==1 && spread) *name="spread";
		else if (i==2 && papergroup) *name="papergroup";
		else *name=NULL;
	}
	if (Name) {
		if (i==0) *Name=_("limbo");
		else if (i==1 && spread) *Name=_("spread");
		else if (i==2 && papergroup) *Name=_("papergroup");
		else *Name=NULL;
	}
	if (isarray) {
		*isarray=0;
		 //isarray==1 means you can write page(3) rather than page.3
		 //isarray==-1 means that field is currently null
	}
	return 0;
}

const double *LaidoutViewport::object_transform(int i)
{
	return NULL;
}

const char *LaidoutViewport::object_e_name(int i)
{
	if (i==0) return "limbo";
	if (i==1 && spread) return "spread";
	if (i==2 && papergroup) return "papergroup";
	return NULL;
}

ObjectDef *LaidoutViewport::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("Viewport");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"Viewport",
            _("Viewport"),
            _("Document and spread view"),
            "class",
            NULL,NULL);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	sd->pushFunction("SelectTool",_("Select Tool"),_("Select Tool"),
					 NULL,
			 		 "tool",NULL,_("Tool to use"),"string", NULL, "Object",
					 NULL);

	sd->pushFunction("PlopData",_("Plop Data"),_("Plop Data"),
					 NULL,
			 		 "data",NULL,_("Data to drop into the viewer"),"any", NULL, NULL,
					 NULL);

	sd->push("limbo",     _("Limbo"),      _("Current limbo"),"any", NULL,NULL,0,NULL);
	sd->push("papergroup",_("Paper Group"),_("Paper Group"),  "any", NULL,NULL,0,NULL);
	sd->push("spread",    _("Spread"),     _("Spread"),       "any", NULL,NULL,0,NULL);

	stylemanager.AddObjectDef(sd,0);

//  "NextSpread"
//  "PreviousSpread"
//
//
	return sd;
}


/*!
 * Return
 *  0 for success, value optionally returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int LaidoutViewport::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
	if (extequal(func,len, "SelectTool")) {
		const char *str=parameters->findString("tool");
		if (!str) return -1;
		ViewWindow *viewer=dynamic_cast<ViewWindow *>(win_parent); // always returns non-null
		viewer->SelectToolFor(str,NULL);
		*value_ret=NULL;
		return 0;

	} else if (extequal(func,len, "PlopData")) {
		Value *v=parameters->find("data");
		if (!dynamic_cast<DrawableObject*>(v)) return -1;
		PlopData(dynamic_cast<DrawableObject*>(v));
		*value_ret=NULL;
		return 0;
	}

	return 1;
}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int LaidoutViewport::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LaidoutViewport::dereference(const char *extstring, int len)
{
	//possible state:
	//  tool
	//  object
	//  selection
	//  viewport
	return NULL;
}

void LaidoutViewport::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	anXWindow::dump_out(f,indent,what,context);
}

LaxFiles::Attribute *LaidoutViewport::dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context)
{
	return anXWindow::dump_out_atts(att,what,context);
}

void LaidoutViewport::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	anXWindow::dump_in_atts(att,flag,context);
}



//-------------------------------------------- ViewWindow -----------------------------------------------
/*! \class ViewWindow
 * \brief Basic view window, for looking at pages, spreads, etc.
 *
 * \todo *** please note that this window and its little buttons and such is a complete
 * hack at the moment.. must work out that useful way of creating windows that I
 * haven't gotten around to coding..
 */


//	ViewerWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//						int xx,int yy,int ww,int hh,int brder,
//						int npad=0);
//! Passes in a new LaidoutViewport.
/*! Incs count on newdoc, if any.
 */
ViewWindow::ViewWindow(Document *newdoc)
	: ViewerWindow(NULL,NULL,(newdoc ? newdoc->Name(1) : _("Untitled")),0,
					0,0,500,600,1,new LaidoutViewport(newdoc))
{ 
	viewport->dec_count(); //remove extra creation count
	var1=var2=var3=NULL;
	project=NULL;
	pagenumber=NULL;
	toolselector=NULL;
	colorbox=NULL;
	pageclips=NULL;
	doc=newdoc;
	if (doc) doc->inc_count();
	setup();
}

//! More general constructor. Passes in a new LaidoutViewport.
ViewWindow::ViewWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Document *newdoc)
	: ViewerWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,new LaidoutViewport(newdoc))
{
	viewport->dec_count(); //remove extra creation count
	viewport->GetShortcuts();
	project=NULL;
	var1=var2=var3=NULL;
	toolselector=NULL;
	pagenumber=NULL;
	doc=newdoc;
	if (doc) doc->inc_count();
	setup();
}

//! Make sure laidout->lastview gets reset if necessary.
/*! \todo *** should really reset to next most relevant view, is just set to null here..
 */
ViewWindow::~ViewWindow()
{
	DBG cerr <<"in ViewWindow destructor (object "<<object_id<<")"<<endl;

	if (laidout->lastview==this) laidout->lastview=NULL;
	if (doc) doc->dec_count();
}

/*! Write out something like:
 * <pre>
 *   document /path/to/doc  # which document
 *   pagelayout            # or 'paperlayout' or 'singlelayout'
 *   page 3               # the page number currently on
 *   matrix 1 0 0 1 0 0  # viewport matrix
 *   xbounds -100 100   # x viewport bounds
 *   ybounds -100 100  # y viewport bounds
 * </pre>
 *
 * If what==-1 return a pseudocode mockup of file format.
 * 
 * \todo *** still need some standardizing for the little helper controls..
 * \todo *** need to dump_out the space, not just the matrix!!
 * \todo *** dump out limbo...
 */
void ViewWindow::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%ssinglelayout        #put the view mode to singles\n",spc);
		fprintf(f,"%s#pagelayout         #put the view mode to page spreads\n",spc);
		fprintf(f,"%s#paperlayout        #put the view mode to paper spreads\n",spc);
		fprintf(f,"%s#layout Single      #see individual impositions for layout types\n",spc);
		
		fprintf(f,"%sspread 1            #the index of the spread for the acting imposition\n",spc);
		fprintf(f,"%spage   0            #the document page index of the page to set context to\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #transform between screen and real space\n",spc);
		fprintf(f,"%sxbounds -20 20      #what distance a horizontal scrollbar represents\n",spc);
		fprintf(f,"%sybounds -20 20      #what distance a vertical scrollbar represents\n",spc);
		fprintf(f,"%slimbo name          #limbo name, or subattributes with objects (optional)\n",spc);
		return;
	}
	if (doc && doc->saveas) fprintf(f,"%sdocument %s\n",spc,doc->saveas);

	LaidoutViewport *vp=((LaidoutViewport *)viewport);
	int vm=vp->viewmode;
	if (doc && doc->imposition->LayoutName(vm)) 
		fprintf(f,"%slayout %s\n",spc,doc->imposition->LayoutName(vm));
	
	if (vp)	fprintf(f,"%sspread %d\n",spc,vp->spreadi);
	if (vp->spread && vp->spread->pagestack.n())
		fprintf(f,"%spage %d\n",spc,vp->spread->pagestack.e[0]->index);
	
	const double *m=vp->dp->Getctm();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
	double x1,x2,y1,y2;
	viewport->dp->GetSpace(&x1,&x2,&y1,&y2);
	fprintf(f,"%sxbounds %.10g %.10g\n",spc,x1,x2); 
	fprintf(f,"%sybounds %.10g %.10g\n",spc,y1,y2); 

	 //dump out limbo
	int c;
	Group *g;
	for (c=0; c<laidout->project->limbos.n(); c++) {
		if (laidout->project->limbos.e(c)==((LaidoutViewport *)viewport)->limbo) {
			g=dynamic_cast<Group *>(laidout->project->limbos.e(c));
			if (!isblank(g->id)) fprintf(f,"%slimbo %s\n",spc,g->id);
			else fprintf(f,"%slimbo Limbo%d\n",spc,c);
			break;
		}
	} 
	if (c==laidout->project->limbos.n() && vp->limbo->n()) {
		fprintf(f,"%slimbo\n",spc); 
		for (int c=0; c<vp->limbo->n(); c++) {
			fprintf(f,"%s  object %d %s\n",spc,c,vp->limbo->e(c)->whattype());
			vp->limbo->e(c)->dump_out(f,indent+4,what,context);
		}
	}
}

LaxFiles::Attribute *ViewWindow::dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context)
{
	cout << "******** ViewWindow::dump_out_atts("<<endl;
	return NULL;
}

//! Reverse of dump_out().
/*! \todo *** need to dump_out the space, not just the matrix!!
 */
void ViewWindow::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int vm=PAGELAYOUT, spr=0, pn=0;
	double m[6],x1=0,x2=0,y1=0,y2=0;
	int n=0;
	const char *layouttype=NULL;
	if (doc) doc->dec_count();
	doc=NULL;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			n=DoubleListAttribute(value,m,6);
		} else if (!strcmp(name,"xbounds")) {
			if (DoubleAttribute(value,&x1,&value))
				DoubleAttribute(value,&x2,NULL);
		} else if (!strcmp(name,"ybounds")) {
			if (DoubleAttribute(value,&y1,&value))
				DoubleAttribute(value,&y2,NULL);
		} else if (!strcmp(name,"pagelayout")) {
			vm=PAGELAYOUT;
		} else if (!strcmp(name,"paperlayout")) {
			vm=PAPERLAYOUT;
		} else if (!strcmp(name,"singlelayout")) {
			vm=SINGLELAYOUT;
		} else if (!strcmp(name,"layout")) {
			layouttype=value;
		} else if (!strcmp(name,"spread")) {
			IntAttribute(value,&spr);
		} else if (!strcmp(name,"page")) {
			IntAttribute(value,&pn);
		} else if (!strcmp(name,"document")) {
			doc=laidout->findDocument(value);
			if (doc) doc->inc_count();
		} else if (!strcmp(name,"limbo")) {
			LaidoutViewport *vp=(LaidoutViewport *)viewport;
			Group *g;
			for (int c2=0; c2<laidout->project->limbos.n(); c2++) {
				g=dynamic_cast<Group *>(laidout->project->limbos.e(c2));
				if (!isblank(value) && !isblank(g->id) && !strcmp(value,g->id)) {
					if (vp->limbo) vp->limbo->dec_count();
					vp->limbo=g;
					vp->limbo->inc_count();
					break;
				}
			} 
			if (vp->limbo) vp->limbo->dump_in_atts(att->attributes.e[c],0,context);
		}
	}
	//*** there should be error checking on x,y
	viewport->dp->SetSpace(x1,x2,y1,y2);
	viewport->dp->syncPanner();
	((LaidoutViewport *)viewport)->UseThisDoc(doc);
	if (layouttype && doc && doc->imposition) {
		for (int c=0; c<doc->imposition->NumLayoutTypes(); c++) {
			if (!strcmp(layouttype,doc->imposition->LayoutName(c))) { vm=c; break; }
		}
	}
	((LaidoutViewport *)viewport)->SetViewMode(vm,spr);
	if (n==6) {
		viewport->dp->NewTransform(m);
		if (((LaidoutViewport *)viewport)->lfirsttime) ((LaidoutViewport *)viewport)->lfirsttime++;
		if (((LaidoutViewport *)viewport)->firsttime)  ((LaidoutViewport *)viewport)->firsttime++;
	}
}


//! Called from constructors, configure the viewport and ***add the extra pager, layer indicator, etc...
/*! Adds local copies of all the interfaces in interfacespool.
 */
void ViewWindow::setup()
{
	if (viewport) viewport->dp->NewBG(rgbcolor(255,255,255));

	//***this should be making dups of interfaces stack? or set current tool, etc...
	for (int c=0; c<laidout->interfacepool.n; c++) {
		 //path interface not ready for prime time, so disable except in debugging mode
		//DBG if (!strcmp(laidout->interfacepool.e[c]->whattype(),"PathInterface")) continue;

		AddTool(laidout->interfacepool.e[c]->duplicate(NULL),0,1);
	}
	SelectTool(0);
	//AddWin(new LayerChooser);...
}

//--------- ***special page flipper
/*! \class PageFlipper
 * \ingroup misc
 * \brief Special page flipper.
 * \todo put me somewhere meaningful!
 */
class PageFlipper : public NumInputSlider
{
 public:
	Document *doc;
	char *currentcontext;
	PageFlipper(Document *ndoc,anXWindow *parnt,const char *ntitle,
				anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel);
	~PageFlipper();
	virtual void Refresh();
	virtual const char *tooltip(int mouseid);
};

/*! Incs count on doc.
 */
PageFlipper::PageFlipper(Document *ndoc,anXWindow *parnt,const char *ntitle,
						 anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel)
	: NumInputSlider(parnt,ntitle,ntitle,NUMSLIDER_WRAP,0,0,0,0,1, prev,nowner,nsendthis,nlabel,0,1)
{
	doc=ndoc;
	if (doc) doc->inc_count();
	currentcontext=NULL;
}

/*! Decs count on doc */
PageFlipper::~PageFlipper()
{ if (doc) doc->dec_count(); }

//! Return the current context.
const char *PageFlipper::tooltip(int mouseid)
{
	if (!currentcontext) return _("The current page");
	return currentcontext;
}

void PageFlipper::Refresh()
{
	if (!win_on || !needtodraw) return;
	background_color(win_colors->bg);
	clear_window(this);

	int x,y;
	unsigned int state;
	if (buttondown) {
		mouseposition(buttondowndevice,this,&x,&y,&state,NULL);
		if ((x>=0 && x<win_w && y>0 && y<win_h)) {
			if (x<win_w/2) {
				 // draw left arrow
				foreground_color(coloravg(win_colors->bg,win_colors->fg,.1));
				draw_thing(this, win_w/4,win_h/2, win_w/4,win_h/2,1, THING_Triangle_Left);
			} else {
				 // draw right arrow
				foreground_color(coloravg(win_colors->bg,win_colors->fg,.1));
				draw_thing(this, win_w*3/4,win_h/2, win_w/4,win_h/2,1, THING_Triangle_Right);
			}
		}
	}

	foreground_color(win_colors->fg);
	
	char *str=newstr(label);
	if (doc && curitem>=0 && curitem<doc->pages.n) appendstr(str,doc->pages.e[curitem]->label);
	else appendstr(str,"limbo");

	textout(this, str,-1,win_w/2,win_h/2,LAX_CENTER);
	delete[] str;

	needtodraw=0;
}

//-------------end PageFlipper

//! Dec count of old doc, inc count of newdoc.
/*! Does not currently update any context labels....
 */
void ViewWindow::setCurdoc(Document *newdoc)
{
	if (doc) doc->dec_count();
	doc=newdoc;
	if (doc) doc->inc_count();
}

//! Add extra goodies to viewerwindow stack, like page/layer/mag/obj indicators
/*! \todo internationalize tool names
 * \todo ***** import/export menus should be dynamically created when clicked... this will only be
 *   relevant when plugins are implemented
 */
int ViewWindow::init()
{
	ViewerWindow::init();
	mesbar->tooltip("Status bar");
	
//	if (!doc) {
//		if (laidout->project && laidout->project->docs.n) {
//			doc=laidout->project->docs.e[0]->doc;
//			((LaidoutViewport *)viewport)->UseThisDoc(doc);
//		}
//	}
	
	int buttongap=0;
	
	MenuButton *menub;
	 //add a menu button thingy in corner between rulers
	//**** menu would hold a list of the available documents, plus other control stuff, dialogs, etc..
	//**** mostly same as would be in right-click in viewport.....	
	rulercornerbutton=menub=new MenuButton(this,"rulercornerbutton",NULL,
						 MENUBUTTON_CLICK_CALLS_OWNER|MENUBUTTON_ICON_ONLY,
						 //MENUBUTTON_DOWNARROW|MENUBUTTON_CLICK_CALLS_OWNER,
						 0,0,0,0,0,
						 NULL,object_id,"rulercornerbutton",
						 0,
						 NULL,0, //menu
						 "o", //label
						 NULL,laidout->icons.GetIcon(laidout->IsProject()?"LaidoutProject":"Laidout"),
						 buttongap);
	menub->tooltip(_("Display list"));
	dynamic_cast<WinFrameBox *>(wholelist.e[0])->NewWindow(menub);
	menub->dec_count(); //remove extra creation count
	
	AddNull();//makes the status bar take up whole line.
	anXWindow *last=NULL;
	Button *ibut;
	
	 // tool section
	 //*** clean me! sampling diff methods of tool selector
	last=toolselector=new SliderPopup(this,"viewtoolselector",NULL,SLIDER_LEFT, 0,0,0,0,1, 
			NULL,object_id,"viewtoolselector",
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
		if (!strcmp(tools.e[c]->whattype(),"ObjectInterface")) obji=tools.e[c]->id;
		
		img=laidout->icons.GetIcon(tools.e[c]->IconId());
		
		toolselector->AddItem(tools.e[c]->Name(),img,tools.e[c]->id); //does not call inc_count()
		//if (img) img->dec_count();
	}
	if (numoverlays) {
		toolselector->AddSep(_("Overlays"));
		for (c=0; c<tools.n; c++) {
			if (tools.e[c]->interface_type!=INTERFACE_Overlay) continue;
			img=laidout->icons.GetIcon(tools.e[c]->IconId());
			toolselector->AddItem(tools.e[c]->Name(),img,tools.e[c]->id); //does not call inc_count()
			toolselector->SetState(-1,MENU_ISTOGGLE|SLIDER_IGNORE_ON_BROWSE,1);
		}
	}

	toolselector->WrapToExtent();
	SelectTool(obji);
	AddWin(toolselector,1,-1);
	


	 //----- Page Flipper
	pagenumber=NULL;
//	last=pagenumber=new PageFlipper(doc,this,"page number", 
//									last,object_id,"newPageNumber",
//									_("Page: "));
//	pagenumber->tooltip(_("The current page"));
//	AddWin(pagenumber,1, 90,0,50,50,0, pagenumber->win_h,0,50,50,0, -1);
	
	last=ibut=new Button(this,"prev spread",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"prevSpread",-1,
						 "<",NULL,laidout->icons.GetIcon("PreviousSpread"),buttongap);
	ibut->tooltip(_("Previous spread"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	last=ibut=new Button(this,"next spread",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"nextSpread",-1,
						 ">",NULL,laidout->icons.GetIcon("NextSpread"),buttongap);
	ibut->tooltip(_("Next spread"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

//	NumSlider *num=new NumSlider(this,"layer number",NUMSLIDER_WRAP, 0,0,0,0,1, 
//								NULL,object_id,"newLayerNumber",
//								"Layer: ",1,1,1); //*** get cur page, use those layers....
//	num->tooltip(_("Sorry, layer control not well\nimplemented yet"));
//	AddWin(num,num->win_w,0,50,50,0, num->win_h,0,50,50,0);
	
	SliderPopup *p=new SliderPopup(this,"view type",NULL,0, 0,0,0,0,1, NULL,object_id,"newViewType");
	int vm=((LaidoutViewport *)viewport)->ViewMode(NULL);
	//*****this needs dynamic adjustment for imposition layout options....
	//****update layout type
	LaidoutViewport *vp=((LaidoutViewport *)viewport);
	Imposition *imp=NULL;
	if (vp->doc) imp=vp->doc->imposition;
	if (imp) {
		for (int c=0; c<imp->NumLayoutTypes(); c++) {
			p->AddItem(imp->LayoutName(c),c);
		}
	} else {
		p->AddItem(_("Single"),SINGLELAYOUT);
		p->AddItem(_("Page Layout"),PAGELAYOUT);
		p->AddItem(_("Paper Layout"),PAPERLAYOUT);
	}
	p->Select(vm);
	p->WrapToExtent();
	AddWin(p,1, p->win_w,0,50,50,0, p->win_h,0,50,50,0, -1);

	last=colorbox=new ColorBox(this,"colorbox",NULL,0, 0,0,0,0,1, NULL,object_id,"curcolor",
							   LAX_COLOR_RGB,
							   65535,150,
							   65535,0,0,65535);
	AddWin(colorbox,1, 50,0,50,50,0, p->win_h,0,50,50,0, -1);
		
	last=pageclips=new Button(this,"pageclips",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"pageclips",-1,
							  _("Page Clips"),NULL,laidout->icons.GetIcon("PageClips"),buttongap);
	pageclips->tooltip(_("Whether pages clips its contents"));
	AddWin(pageclips,1, pageclips->win_w,0,50,50,0, pageclips->win_h,0,50,50,0, -1);
	updateContext(1);

	last=ibut=new Button(this,"delete page",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"deletePage",-1,
						 _("Delete Page"),NULL,laidout->icons.GetIcon("DeletePage"),buttongap);
	ibut->tooltip(_("Delete the current page"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	last=ibut=new Button(this,"add page",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"addPage",-1,
						 _("Add Page"),NULL,laidout->icons.GetIcon("AddPage"),buttongap);
	ibut->tooltip(_("Add 1 page after this one"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	last=ibut=new Button(this,"import image",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"importImage",-1,
						 _("Import Images"),NULL,laidout->icons.GetIcon("ImportImage"),buttongap);
	ibut->tooltip(_("Import one or more images"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);


	 //-------------import
	 //*** this can be somehow combined with import images maybe?...
	last=ibut=new Button(this,"import",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, last,object_id,"import",-1,
						 _("Import"),NULL,laidout->icons.GetIcon("Import"),buttongap);
	ibut->tooltip(_("Try to import various vector based files into the document"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	 //---------------******** export
	 //*** this needs an automatic way to do it, needs a pool to access somewhere..
	 //Export:
	 // Ppt
	 // svg,...
	 // -----
	 // Export style 1: ppt pg 3-6
	 // Export style 2: SVG pg 2
	 //then Print button would have the last export settings, single click does that..
//	MenuInfo *menu=NULL;
//	if (laidout->exportfilters.n) {
//		menu=new MenuInfo(_("Export"));
//		for (int c=0; c<laidout->exportfilters.n; c++) {
//			//***** this should be dynamically created when clicked
//			menu->AddItem(laidout->exportfilters.e[c]->VersionName(),c);
//		}
//	}
//	last=menub=new MenuButton(this,"export",IBUT_ICON_ONLY, 0,0,0,0,1, last,object_id,"export",-1,
//							 menu,1,
//							 laidout->icons.GetIcon("Export"),_("Export"));
//	menub->tooltip(_("Export the document in various ways"));
//	AddWin(menub,menub->win_w,0,50,50,0, menub->win_h,0,50,50,0);
//-----------------
	last=ibut=new Button(this,"export",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, last,object_id,"export",-1,
						 _("Export"),NULL,laidout->icons.GetIcon("Export"),buttongap);
	ibut->tooltip(_("Export the document in various ways"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	
	 //-----------print
	last=ibut=new Button(this,"print",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, last,object_id,"print",-1,
						 _("Print"),NULL,laidout->icons.GetIcon("Print"),buttongap);
	ibut->tooltip(_("Print the document"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);


	last=ibut=new Button(this,"open doc",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"openDoc",-1,
						 _("Open"),NULL,laidout->icons.GetIcon("Open"),buttongap);
	ibut->tooltip(_("Open a document from disk"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	last=ibut=new Button(this,"save doc",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"saveDoc",-1,
						 _("Save"),NULL,laidout->icons.GetIcon("Save"),buttongap);
	ibut->tooltip(_("Save the current document"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	

	last=ibut=new Button(this,"help",NULL,IBUT_ICON_ONLY, 0,0,0,0,1, NULL,object_id,"help",-1,
						 _("Help!"),NULL,laidout->icons.GetIcon("Help"),buttongap);
	ibut->tooltip(_("Popup a list of shortcuts"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	//**** add screen x,y
	//		   real x,y
	//         page x,y
	//         object x,y
	
	updateContext(1);
	Sync(1);	
	return 0;
}

/*! Currently accepts:\n
 * <pre>
 *  "new image"
 *  "saveAsPopup"
 *  "docTreeChange" <-- updateContext() and pass event to viewport
 * </pre>
 *
 * \todo *** the response to saveAsPopup could largely be put elsewhere
 */
int ViewWindow::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"ViewWindow "<<(WindowTitle())<<" got "<<(mes?mes:"unknown")<<endl;

	if (!strcmp(mes,"docTreeChange")) { // doc tree was changed somehow
		int c=viewport->Event(data,mes);
		updateContext(0);
		return c;

	} else if (!strcmp(mes,"prefsChange")) { // doc tree was changed somehow
		return viewport->Event(data,mes);

	} else if (!strcmp(mes,"import new image")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (!s) return 1;

		char mes[50];
		mes[0]=0;
		if (s->info1>=0) {
			if (s->info1>1) sprintf(mes,_("%d images imported."),s->info1);
			else sprintf(mes,_("Image imported."));
		} else { 
			sprintf(mes,_("No images imported."));
		}
		((LaidoutViewport *)viewport)->postmessage(mes);
		return 0;

	} else if (!strcmp(mes,"reallysave")) {
		 // save without overwrite check
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		
		Document *sdoc=doc;
		if (!sdoc) sdoc=laidout->curdoc;
		if (sdoc && sdoc->Saveas(s->str)) SetParentTitle(sdoc->Name(1));
		
		ErrorLog log;
		if (sdoc && sdoc->Save(1,1,log)==0) {
			char blah[strlen(sdoc->Saveas())+15];
			sprintf(blah,_("Saved to %s."),sdoc->Saveas());
			PostMessage(blah);
			if (!isblank(laidout->project->filename)) laidout->project->Save(log);
		} else {
			if (log.Total()) {
				PostMessage(log.MessageStr(log.Total()-1));
			} else PostMessage(_("Problem saving. Not saved."));
		}

		return 0;

	} else if (!strcmp(mes,"statusMessage")) {
		 //user entered a new file name to save document as
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || !s->str) return 1;
		PostMessage(s->str);
		return 0;

	} else if (!strcmp(mes,"saveAsPopup")) {
		 //user entered a new file name to save document as
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		if (s->str && s->str[0]) {
			char *dir,*bname;
			dir=newstr(s->str);
			convert_to_full_path(dir,NULL);
			bname=strrchr(dir,'/');
			
			 // if there is a directory component, then cd to there first..
			if (!bname) {
				bname=strrchr(dir,'/');
			}
			if (bname) {
				*bname='\0';
				bname=newstr(bname+1);
				if (chdir(dir)!=0) {
					 // could not chdir;
					char mes[50+strlen(dir)];
					sprintf(mes,_("File not saved: Could not change directory to \"%s\""),dir);
					((LaidoutViewport *)viewport)->postmessage(mes);
					delete[] bname;
					delete[] dir;
					return 0;
				}
			} 
			
			 // now orig file is in dir/bname, make file be full path to file
			 // current dir is now where file goes
			char *file=newstr(dir);
			if (file[strlen(file)-1]!='/') appendstr(file,"/");
			appendstr(file,bname);
			
			int c,err;
			c=file_exists(file,1,&err);
			if (c && c!=S_IFREG) {
				 // has to be a regular file to overwrite
				char mes[50];
				sprintf(mes,"Cannot overwrite that type of file.");
				((LaidoutViewport *)viewport)->postmessage(mes);
				delete[] bname;
				delete[] dir;
				delete[] file;
				return 0;
			}
			 // file existed, so ask to overwrite ***
			if (c) {
				anXWindow *ob=new Overwrite(object_id,"reallysave", file, s->info1, s->info2, s->info3);
				app->rundialog(ob);
			} else {
				if (!is_good_filename(file)) {//***how does it know?
					PostMessage(_("Illegal characters in file name. Not saved."));
				} else {
					 //set name in doc and headwindow
					DBG cerr <<"*** file by this point should be absolute path name:"<<file<<endl;
					Document *sdoc=doc;
					if (!sdoc) sdoc=laidout->curdoc;
					if (sdoc) {
						SetParentTitle(sdoc->Name(1));
						sdoc->Saveas(file);
					}
					
					ErrorLog log;
					if (sdoc && sdoc->Save(1,1,log)==0) {
						char blah[strlen(sdoc->Saveas())+15];
						sprintf(blah,_("Saved to %s."),sdoc->Saveas());
						PostMessage(blah);
					} else {
						if (log.Total()) {
							PostMessage(log.MessageStr(log.Total()-1));
						} else PostMessage(_("Problem saving. Not saved."));
					}
				}
			}

			if (bname) delete[] bname;
			if (dir)   delete[] dir;
			if (file)  delete[] file;
		}
		return 0;

	} else if (!strcmp(mes,"print config")) {
		 //sent from PrintingDialog
		//***
		return 0;

	} else if (!strcmp(mes,"export config")) {
		 //sent from ExportDialog
		 //*** Replace this with log link window
		const ConfigEventData *d=dynamic_cast<const ConfigEventData *>(data);
		if (!d || !d->config->filter) return 1;
		ErrorLog log;
		mesbar->SetText(_("Exporting..."));
		mesbar->Refresh();
		//XSync(app->dpy,False);
		int err=export_document(d->config,log);
		if (err==0) mesbar->SetText(_("Exported."));
		else mesbar->SetText(_("Error exporting."));
		if (log.Total()) {
			char *error=NULL;
			char scratch[100];
			ErrorLogNode *e;
			for (int c=0; c<log.Total(); c++) {
				e=log.Message(c);
				if (e->severity==ERROR_Ok) ;
				else if (e->severity==ERROR_Warning) appendstr(error,"Warning: ");
				else if (e->severity==ERROR_Fail) appendstr(error,"Error! ");
			
				if (e->objectstr_id) {
					sprintf(scratch,"id:%s, ",e->objectstr_id);
					appendstr(error,scratch);
				}
				appendstr(error,e->description);
				appendstr(error,"\n");
			}
			MessageBox *mbox=new MessageBox(NULL,NULL,err?_("Error exporting."):_("Warning!"),ANXWIN_CENTER, 0,0,0,0,0,
										NULL,0,NULL, error);
			mbox->AddButton(BUTTON_OK);
			mbox->AddButton(_("Dammit!"),0);
			app->rundialog(mbox);
			delete[] error;
		}
		return 0;

	} else if (!strcmp(mes,"curcolor")) {
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(data);
		if (!ce) return 1;

		 // apply message as new current color, pass on to viewport
		 // (sent from color box)
		LineStyle linestyle;
		float max=ce->max;
		linestyle.color.red=  (unsigned short) (ce->channels[0]/max*65535);
		linestyle.color.green=(unsigned short) (ce->channels[1]/max*65535);
		linestyle.color.blue= (unsigned short) (ce->channels[2]/max*65535);
		if (ce->numchannels>3) linestyle.color.alpha=(unsigned short) (ce->channels[3]/max*65535);
		else linestyle.color.alpha=65535;
		linestyle.mask=GCForeground;

		char blah[100];
		colorbox->SetRGB(linestyle.color.red,linestyle.color.green,linestyle.color.blue,linestyle.color.alpha);
		sprintf(blah,_("New Color r:%.4f g:%.4f b:%.4f a:%.4f"),
				(float) linestyle.color.red   / 65535,
				(float) linestyle.color.green / 65535,
				(float) linestyle.color.blue  / 65535,
				(float) linestyle.color.alpha / 65535);
		mesbar->SetText(blah);
		if (curtool)
			if (curtool->UseThis(&linestyle,GCForeground)) ((anXWindow *)viewport)->Needtodraw(1);
		
		return 0;

	} else if (!strcmp(mes,"make curcolor")) {
		ViewerWindow::Event(data,mes);

		 //update status message
		char blah[100];
		sprintf(blah,_("New Color r:%.4f g:%.4f b:%.4f a:%.4f"),
				(float) colorbox->Red()   / 65535,
				(float) colorbox->Green() / 65535,
				(float) colorbox->Blue()  / 65535,
				(float) colorbox->Alpha() / 65535);
		mesbar->SetText(blah);
		return 0;

	} else if (!strcmp(mes,"rulercornerbutton")) {
		 //pop up a list of available documents and limbos
		 //*** in future, this will be more full featured, with:
		 //Doc1
		 //Doc2
		 //None
		 //Add new
		 //Remove current
		 //---
		 //limbo1
		 //limbo2
		 //Delete Current Limbo
		 //Add New Limbo
		 //----
		 //zone1
		 //zone2

		MenuInfo *menu;
		menu=new MenuInfo("Viewer");

		 //---add document list, numbers start at 0
		int c,pos;
		menu->AddSep("Documents");
		for (c=0; c<laidout->project->docs.n; c++) {
			pos=menu->AddItem(laidout->project->docs.e[c]->doc->Name(1),c)-1;
			menu->menuitems.e[pos]->state|=LAX_ISTOGGLE;
			if (laidout->project->docs.e[c]->doc==doc) {
				menu->menuitems.e[pos]->state|=LAX_CHECKED;
			}
		}
		c=menu->AddItem(_("None"),c)-1;
		
		menu->menuitems.e[c]->state|=LAX_ISTOGGLE;
		if (!doc) menu->menuitems.e[c]->state|=LAX_CHECKED;
		if (doc) {
			menu->AddItem(_("Edit imposition"),ACTION_EditImposition);
			menu->AddItem(_("Remove current document"),ACTION_RemoveCurrentDocument);
		}
		menu->AddItem(_("Add new document"),ACTION_AddNewDocument);

		 //----add limbo list, numbers starting at 1000...
		char txt[40];
		Group *g;
		int where;
		if (laidout->project->limbos.n()) {
			menu->AddSep(_("Scratch"));
			for (c=0; c<laidout->project->limbos.n(); c++) {
				g=dynamic_cast<Group *>(laidout->project->limbos.e(c));
				if (!isblank(g->id)) where=menu->AddItem(g->id,1000+c)-1; 
				else {
					sprintf(txt,_("(Limbo %d)"),c);
					where=menu->AddItem(txt,1000+c)-1;
				}
				menu->menuitems.e[where]->state|=LAX_ISTOGGLE;
				if (g==((LaidoutViewport *)viewport)->limbo) {
					menu->menuitems.e[where]->state|=LAX_CHECKED;
				}
			}
		}
		menu->AddItem(_("Add new limbo"),ACTION_AddNewLimbo);
		c++;
		menu->AddItem(_("Rename current limbo"),ACTION_RenameCurrentLimbo);
		c++;
		if (laidout->project->limbos.n()>1) menu->AddItem(_("Delete current limbo"),ACTION_DeleteCurrentLimbo);

		 //----add papergroup list, numbers starting at 2000...
		PaperGroup *pg;
		char *temp;
		where=-1;
		int defaultoption;
		c=0;
		menu->AddSep(_("Paper Groups"));
		defaultoption=menu->AddItem(_("default"),2000,LAX_ISTOGGLE|LAX_OFF|(((LaidoutViewport *)viewport)->isDefaultPapergroup(0)?LAX_CHECKED:0))-1;

		for (c=0; c<laidout->project->papergroups.n; c++) {
			pg=dynamic_cast<PaperGroup *>(laidout->project->papergroups.e[c]);
			if (!isblank(pg->Name)) temp=pg->Name;
			else if (!isblank(pg->name)) temp=pg->name;
			else {
				sprintf(txt,_("(Paper Group %d)"),c);
				temp=txt;
			}
			pos=menu->AddItem(temp,2000+c+1,LAX_ISTOGGLE|LAX_OFF)-1; 
			if (pg==((LaidoutViewport *)viewport)->papergroup) where=pos;
		}
		 //viewport is using a non-default papergroup when where>=0
		menu->menuitems.e[where>=0 ? where : defaultoption]->state|=LAX_CHECKED;

		menu->AddItem(_("Add new paper group"),ACTION_AddNewPaperGroup);
		if (where>=0) {
			menu->AddItem(_("Rename current paper group"),ACTION_RenameCurrentPaperGroup);
			menu->AddItem(_("Delete current paper group"),ACTION_DeleteCurrentPaperGroup);
		}

		 //---- Project menu
		//****
		//menu->AddSep();
		//menu->AddItem(_("Project"));
		//menu->SubMenu(_("Project"));
		//menu->AddItem(_("Save project..."),ACTION_SaveProject);
		//menu->AddItem(_("New project..."),ACTION_NewProject);
		//menu->AddItem(laidout->IsProject()?_("Do not save as a project"):_("Save as a project..."), ACTION_ToggleSaveAsProject);
		//menu->EndSubMenu();

//						 laidout->icons.GetIcon(laidout->IsProject()?"LaidoutProject":"Laidout"),

		DBG menuinfoDump(menu,0);

		 //create the actual popup menu...
		MenuSelector *popup;
//		popup=new MenuSelector(NULL,_("Documents"), ANXWIN_BARE|ANXWIN_HOVER_FOCUS,
//						0,0,0,0, 1, 
//						NULL,viewport->object_id,"rulercornermenu", 
//						MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS
//						 //| MENUSEL_SEND_STRINGS
//						 | MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
//						 | MENUSEL_GRAB_ON_MAP|MENUSEL_OUT_CLICK_DESTROYS
//						 | MENUSEL_CLICK_UP_DESTROYS|MENUSEL_DESTROY_ON_FOCUS_OFF
//						 | MENUSEL_CHECK_ON_LEFT|MENUSEL_LEFT,
//						menu,1);
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		popup=new PopupMenu(NULL,_("Documents"), MENUSEL_LEFT|MENUSEL_CHECK_ON_LEFT,
						0,0,0,0, 1, 
						viewport->object_id,"rulercornermenu", 
						s->info3, //id of device that triggered the send
						menu,1);
		popup->pad=5;
		popup->Select(0);
		popup->WrapToMouse(None);
		app->rundialog(popup);
		return 0;

	} else if (!strcmp(mes,"help")) {
		//app->addwindow(new HelpWindow());
		app->addwindow(new ShortcutWindow(NULL,"Shortcuts","Shortcuts",ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,0,0,400,600,0));
		return 0;

	} else if (!strcmp(mes,"contextChange")) { // curobj was changed, now maybe diff page, spread, etc.
		//***
		updateContext(1);
		return 0;

	} else if (!strcmp(mes,"pageclips")) { // 
		 //toggle pageclips
		 //*** this sucks need to dup the style if necessary and
		 //*** make it based on the old style
		if (!doc) return 0;
		int c=((LaidoutViewport *)viewport)->curobjPage();
		if (c>=0) {
			PageStyle *ps=doc->pages.e[c]->pagestyle;
			if (!(ps->flags&PAGESTYLE_AUTONOMOUS)) {
				ps=(PageStyle *)ps->duplicate();
				doc->pages.e[c]->InstallPageStyle(ps);
			}
			doc->pages.e[c]->pagestyle->set("pageclips",-1);
			pageclips->State(doc->pages.e[c]->pagestyle->flags&PAGE_CLIPS?LAX_ON:LAX_OFF);
		}
		((anXWindow *)viewport)->Needtodraw(1);
		return 0;

	} else if (!strcmp(mes,"addPage")) { // 
		if (!doc) return 0;
		int curpage=((LaidoutViewport *)viewport)->curobjPage();
		int c=doc->NewPages(curpage+1,1); //add after curpage
		if (c>=0) {
			char scratch[100];
			sprintf(scratch,_("Page number %d added (%d total)."),curpage+1,doc->pages.n);
			PostMessage(scratch);

			((LaidoutViewport *)viewport)->SelectPage(curpage+1);
			updateContext(0);
		} else PostMessage(_("Error adding page."));
		return 0;

	} else if (!strcmp(mes,"deletePage")) { // 
		if (!doc) return 0;

		 // this in response to delete button command
		LaidoutViewport *vp=((LaidoutViewport *)viewport);
		int curpage=vp->curobjPage();

		int c=doc->RemovePages(curpage,1); //remove curpage
		if (c==1) {
			char scratch[100];
			sprintf(scratch,_("Page %d deleted. %d remaining."),curpage,doc->pages.n);
			PostMessage(scratch);
			
			if (curpage>=doc->pages.n) {
				curpage=doc->pages.n-1;
				((LaidoutViewport *)viewport)->wipeContext();
			}
			
			((LaidoutViewport *)viewport)->SelectPage(curpage);
			updateContext(0);
		} else if (c==-2) PostMessage(_("Cannot delete the only page."));
		else PostMessage(_("Error deleting page."));
		

		// Document sends the notifyDocTreeChanged..

		return 0;

	} else if (!strcmp(mes,"newPageNumber")) {
		if (!doc) return 0;

		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int p=s->info1;
		if (p>doc->pages.n) {
			p=0;
			if (pagenumber) pagenumber->Select(p);
		} else if (p<0) {
			p=doc->pages.n-1;
			if (pagenumber) pagenumber->Select(p);
		}
		((LaidoutViewport *)viewport)->SelectPage(p);
		updateContext(0);
		return 0;

	} else if (!strcmp(mes,"prevSpread")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info4==-1) ((LaidoutViewport *)viewport)->PreviousSpread();
		else if (s->info4==1) ((LaidoutViewport *)viewport)->NextSpread();
		else ((LaidoutViewport *)viewport)->PreviousSpread();
		updateContext(0);
		return 0;

	} else if (!strcmp(mes,"nextSpread")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info4==-1) ((LaidoutViewport *)viewport)->PreviousSpread();
		else if (s->info4==1) ((LaidoutViewport *)viewport)->NextSpread();
		else ((LaidoutViewport *)viewport)->NextSpread();
		updateContext(0);
		return 0;

	} else if (!strcmp(mes,"newLayerNumber")) { // 
		//((LaidoutViewport *)viewport)->SelectPage(s->info1);
		cout <<"***** new layer number.... *** imp me!"<<endl;
		return 0;

	} else if (!strcmp(mes,"viewtoolselector")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		DBG cerr <<"***** viewtoolselector change to id:"<<s->info1<<endl;

		SelectTool(s->info1);
		return 0;

	} else if (!strcmp(mes,"newViewType")) {
		 // must update labels have Spread [2-3]: 2
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		int v=s->info1;
		//pagenumber->Label(((LaidoutViewport *)viewport)->SetViewMode(v,-1));
		((LaidoutViewport *)viewport)->SetViewMode(v,-1);
		return 0;

	} else if (!strcmp(mes,"importImage")) {
		Group *toobj=NULL;
		if (!doc) {
			 //create a group object with the same aspect as the viewport, and dump images
			 //into that. This group object gets sent back to the viewer in an event message.
			 //The objects are then inserted into limbo.
			double aspect=(viewport->dp->Maxy-viewport->dp->Miny)/(float)(viewport->dp->Maxx-viewport->dp->Minx);
			toobj=new Group;
			toobj->maxx=1;
			toobj->maxy=aspect;
		}
		app->rundialog(new ImportImagesDialog(NULL,"Import Images",_("Import Images"),
					FILES_FILES_ONLY|FILES_OPEN_MANY|FILES_PREVIEW,
					0,0,0,0,0, object_id,"import new image",
					NULL,NULL,NULL,
					toobj,
					doc,((LaidoutViewport *)viewport)->curobjPage(),0));
		if (toobj) toobj->dec_count();
		return 0;

	} else if (!strcmp(mes,"import")) { 
		if (laidout->importfilters.n==0) {
			mesbar->SetText(_("Sorry, there are no import filters installed."));
			return 0;
		} else {
			mesbar->SetText(_("Importing..."));
			Group *toobj=NULL;
			if (!doc) {
				 //create a group object with the same aspect as the viewport, and dump images
				 //into that. This group object gets sent back to the viewer in an event message.
				 //The objects are then inserted into limbo.
				double aspect=(viewport->dp->Maxy-viewport->dp->Miny)/(float)(viewport->dp->Maxx-viewport->dp->Minx);
				toobj=new Group;
				toobj->maxx=1;
				toobj->maxy=aspect;
			}
			app->rundialog(new ImportFileDialog(NULL,NULL,_("Import File"),
						FILES_FILES_ONLY|FILES_OPEN_ONE|FILES_PREVIEW,
						0,0,0,0,0, object_id,"import file",
						NULL,NULL,NULL,
						toobj,
						doc,((LaidoutViewport *)viewport)->curobjPage(),0));
			if (toobj) toobj->dec_count();
			return 0;
		}
		return 0;

	} else if (!strcmp(mes,"export")) { 
		 //user clicked down on the export button, and selected an export type from menu..
		PaperGroup *pg=((LaidoutViewport *)viewport)->papergroup;
		Group *l;
		if (!pg || !pg->papers.n) l=NULL; else l=((LaidoutViewport *)viewport)->limbo;
		char *file=NULL;
		if (doc) {
			file=lax_dirname(doc->Saveas(),1);
			convert_to_full_path(file,NULL);
			appendstr(file,"exported-file.huh");
		} else {
			file=full_path_for_file("exported-file.huh",NULL);
		}
		ExportDialog *d=new ExportDialog(0,object_id,"export config", 
										 doc,
										 l,
										 pg,
										 NULL,//***should be last filter...
										 file,
										 PAPERLAYOUT,
										 0,
										 doc?doc->imposition->NumPapers()-1:0,
										 doc?doc->imposition->PaperFromPage(
											((LaidoutViewport *)viewport)->curobjPage()):0);
		app->rundialog(d);
		delete[] file;
		return 0;

	} else if (!strcmp(mes,"openDoc")) { 
		 //sends the open message to the head window... hmm...
		app->rundialog(new FileDialog(NULL,NULL,_("Open Document"),
					ANXWIN_REMEMBER,
					0,0,0,0,0, win_parent->object_id,"open document",
					FILES_FILES_ONLY|FILES_OPEN_MANY|FILES_PREVIEW,
					NULL,NULL,NULL,"Laidout"));
		return 0;

	} else if (!strcmp(mes,"saveDoc")) { 
		if (!doc) return 0;
		
		 //user clicked save button
		if (isblank(doc->Saveas()) || !strncasecmp(doc->Saveas(),_("untitled"), strlen(_("untitled")))) { //***or shift-click for saveas??
			 // launch saveas!!
			//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
						//int xx,int yy,int ww,int hh,int brder,
						//anXWindow *prev,Window nowner,const char *nsend,
						//const char *newlabel,const char *newtext,unsigned int ntstyle,
						//int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
			app->rundialog(new FileDialog(NULL,NULL,_("Save As..."),
						ANXWIN_REMEMBER,
						0,0,0,0,0, object_id,"saveAsPopup",
						FILES_FILES_ONLY|FILES_SAVE_AS,
						doc->Saveas()));
		} else {
			ErrorLog log;
			if (doc->Save(1,1,log)==0) {
				char blah[strlen(doc->Saveas())+15];
				sprintf(blah,"Saved to %s.",doc->Saveas());
				PostMessage(blah);
			} else {
				if (log.Total()) {
					PostMessage(log.MessageStr(log.Total()-1));
				} else PostMessage(_("Problem saving. Not saved."));
			}
		}
		return 0;

	} else if (!strcmp(mes,"print")) { // print to output.ps
		 //user clicked print button
		LaidoutViewport *vp=((LaidoutViewport *)viewport);
		int curpage=doc?doc->imposition->PaperFromPage(vp->curobjPage()):0;
		if (curpage<0 && vp->doc && vp->spread) {
			 //grab what is first page found in spread->pagestack
			int c;
			for (c=0; c<vp->spread->pagestack.n(); c++) {
				if (vp->spread->pagestack.e[c]->index>=0) {
					curpage=vp->spread->pagestack.e[c]->index>=0;
					break;
				}
			}
		}
		PaperGroup *pg=vp->papergroup;
		Group *l;
		if (!pg || !pg->papers.n) {
			l=NULL;
			if (vp->doc) pg=vp->doc->imposition->papergroup;
			if (pg && pg->papers.n==0) pg=NULL;
			if (!pg) {
				int c;
				for (c=0; c<laidout->papersizes.n; c++) {
					if (!strcasecmp(laidout->prefs.defaultpaper,laidout->papersizes.e[c]->name)) 
						break;
				}
				PaperStyle *ps;
				if (c==laidout->papersizes.n) c=0;
				ps=(PaperStyle *)laidout->papersizes.e[0]->duplicate();
				pg=new PaperGroup(ps);
				ps->dec_count();
			} else pg->inc_count();
		} else l=vp->limbo;
		PrintingDialog *p=new PrintingDialog(doc,object_id,"export config",
										"output.ps", //file
										"lp",        //command
										NULL,        //thisfile
										PAPERLAYOUT, 
										0,              //min
										doc?doc->imposition->NumPapers()-1:0, //max
										curpage,       //cur
										pg,           //papergroup
										l,           //limbo
										mesbar);     //progress window
		pg->dec_count();
		app->rundialog(p);
		return 0;
	}
	
	return ViewerWindow::Event(data,mes);
}


//! Set the title of the top level window containing this window.
void ViewWindow::SetParentTitle(const char *str)
{
	if (!str) return;
	WindowTitle(str);
	anXWindow *win=win_parent;
	while (win && win->win_parent) win=win->win_parent;
	if (win) win->WindowTitle(str);
}

//! Make the ruler corner button have the right icon.
void ViewWindow::updateProjectStatus()
{
	((MenuButton *)rulercornerbutton)->SetIcon(laidout->icons.GetIcon(laidout->IsProject()?"LaidoutProject":"Laidout"));
}

//! Make the pagenumber label be correct.
/*! Also set the pageclips thing.
 *
 * If messagetoo!=0, then update the viewwindow message bar to state the new context.
 *
 * \todo *** need to implement the updating all of helper windows to cur context
 */
void ViewWindow::updateContext(int messagetoo)
{
	if (!doc) return;
	int page=((LaidoutViewport *)viewport)->curobjPage();
	if (pagenumber) {
		pagenumber->Label(((LaidoutViewport *)viewport)->Pageviewlabel());
		pagenumber->Select(page);
		pagenumber->NewMax(doc->pages.n-1);
	}

	if (pageclips) {
		if (page>=0) pageclips->State(doc->pages.e[page]->pagestyle->flags&PAGE_CLIPS?LAX_ON:LAX_OFF);
		else pageclips->State(LAX_OFF);
	}

	if (messagetoo) {
		LaidoutViewport *v=((LaidoutViewport *)viewport);
		const char *f;
		int i;
		int cplen=20,curpageindex=((LaidoutViewport *)viewport)->curobjPage();

		char blah[cplen+v->curobj.context.n()*20+50]; //*** warning! crash magnet when field names are long!
		blah[0]='\0';

		if (curpageindex>=0 && doc->pages.e[curpageindex]->label) {
			sprintf(blah,_("(page %s)"),doc->pages.e[curpageindex]->label);
		}
		strcat(blah," view");

		ObjectContainer *o=dynamic_cast<ObjectContainer*>(viewport);
		for (int c=0; o && c<v->curobj.context.n(); c++) {
			strcat(blah,".");
			i=v->curobj.context.e(c);
			f=o->object_e_name(i);
			if (f) strcat(blah,f);
			else sprintf(blah+strlen(blah),"%d",i);
			o=dynamic_cast<ObjectContainer*>(o->object_e(i));
		}
		strcat(blah,":");
		if (v->curobj.obj) strcat(blah,v->curobj.obj->whattype());
		else strcat(blah,"none");
		if (mesbar) mesbar->SetText(blah);

		PageFlipper *pageflipper=dynamic_cast<PageFlipper*>(findChildWindowByName("page number"));
		if (pageflipper) makestr(pageflipper->currentcontext,blah);
	}


	//update layout type, this could probably be more efficient...
	SliderPopup *layouttype=dynamic_cast<SliderPopup*>(findChildWindowByName("view type"));
	if (layouttype) {
		layouttype->Flush();
		LaidoutViewport *vp=((LaidoutViewport *)viewport);
		Imposition *imp;
		if (vp->doc) imp=vp->doc->imposition;
		if (imp) {
			for (int c=0; c<imp->NumLayoutTypes(); c++) {
				layouttype->AddItem(imp->LayoutName(c),c);
			}
		}
	}
}


//! On any FocusIn event, set laidout->lastview to this.
int ViewWindow::FocusOn(const Laxkit::FocusChangeData *e)
{
	//if (doc) laidout->curdoc=doc;
	laidout->lastview=this;
	return anXWindow::FocusOn(e);
}

//! Override to use ObjectInterface when object's contents are locked.
int ViewWindow::SelectToolFor(const char *datatype,LaxInterfaces::ObjectContext *oc)
{
	 //use object tool for SomeDataRef or locked objects
	if (oc && oc->obj &&
			((oc->obj->flags&SOMEDATA_LOCK_CONTENTS)
			 || !strcmp(oc->obj->whattype(),"SomeDataRef"))) {

		for (int c=0; c<tools.n; c++) {
			if (!strcmp(tools.e[c]->whattype(),"ObjectInterface")) {
				SelectTool(tools.e[c]->id);
				((ObjectInterface*)tools.e[c])->FreeSelection();
				((ObjectInterface*)tools.e[c])->AddToSelection(oc);
				updateContext(1);
				break;
			}
		}
		return 0;
	}
	return ViewerWindow::SelectToolFor(datatype,oc);
}

int ViewWindow::SelectTool(int id)
{
	int c=ViewerWindow::SelectTool(id);
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
	DocumentUser *d;
	for (int c2=0; c2<viewport->interfaces.n; c2++) {
		d=dynamic_cast<DocumentUser*>(viewport->interfaces.e[c2]);
		if (d) d->UseThisDocument(doc);
	}
	return c;
}

//enum ViewActions {
//	VIEW_Save,
//	VIEW_SaveAs,
//	VIEW_NewDocument,
//	VIEW_NextTool,
//	VIEW_PreviousTool,
//	VIEW_NextPage,
//	VIEW_PreviousPage,
//	VIEW_ObjectTool,
//	VIEW_CommandPrompt,
//	VIEW_ObjectIndicator,
//
//	VIEW_Help,
//	VIEW_About,
//	VIEW_SpreadEditor,
//
//	VIEW_MAX
//};

Laxkit::ShortcutHandler *ViewWindow::GetShortcuts()
{
	//parent class LaxInterfaces::ViewerWindow only defines shortcuts for next tool and previous tool.
	//we are loosing nothing by totally ignoring it here.

	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("ViewWindow");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("ViewWindow");

	sc->Add(VIEW_ObjectTool,     LAX_Esc,0,0,      _("ObjectTool"),   _("Switch to object tool"),NULL,0);
	sc->Add(VIEW_Save,           's',ControlMask,0, _("Save"),         _("Save document"),NULL,0);
	sc->Add(VIEW_SaveAs,         'S',ShiftMask|ControlMask,0,_("SaveAs"), _("Save as"),NULL,0);
	sc->Add(VIEW_NewDocument,    'n',ControlMask,0, _("NewDoc"),       _("New document"),NULL,0);
	sc->Add(VIEW_NextTool,       't',0,0,           _("NextTool"),     _("Next tool"),NULL,0);
	sc->Add(VIEW_PreviousTool,   'T',ShiftMask,0,   _("PreviousTool"), _("Previous tool"),NULL,0);
	sc->Add(VIEW_NextPage,       '>',ShiftMask,0,   _("NextPage"),     _("Next page"),NULL,0);
	sc->Add(VIEW_PreviousPage,   '<',ShiftMask,0,   _("PreviousPage"), _("Previous page"),NULL,0);
	sc->Add(VIEW_Help,           LAX_F1,0,0,        _("Help"),         _("Help"),NULL,0);
	sc->Add(VIEW_About,          LAX_F2,0,0,        _("About"),        _("About Laidout"),NULL,0);
	sc->Add(VIEW_SpreadEditor,   LAX_F5,0,0,        _("SpreadEditor"), _("Popup a spread editor"),NULL,0);
	sc->Add(VIEW_CommandPrompt,  '/',0,0,           _("Prompt"),       _("Popup a the graphical shell"),NULL,0);

	manager->AddArea("ViewWindow",sc);
	return sc;
}

int ViewWindow::PerformAction(int action)
{
	if (action==VIEW_ObjectTool) {
		 //change to object tool
		for (int c=0; c<tools.n; c++) {
			if (!strcmp(tools.e[c]->whattype(),"ObjectInterface")) {
				SelectTool(tools.e[c]->id);
				((ObjectInterface*)tools.e[c])->AddToSelection(&((LaidoutViewport*)viewport)->curobj);
				updateContext(1);
				break;
			}
		}
		return 0;

	} else if (action==VIEW_CommandPrompt) {
		for (int c=0; c<tools.n; c++) {
			if (!strcmp(tools.e[c]->whattype(),"GraphicalShell")) {
				SelectTool(tools.e[c]->id);
				updateContext(1);
				break;
			}
		}
		return 0; 

	} else if (action==VIEW_ObjectIndicator) {
		for (int c=0; c<tools.n; c++) {
			if (!strcmp(tools.e[c]->whattype(),"ObjectIndicator")) {
				SelectTool(tools.e[c]->id);
				updateContext(1);
				break;
			}
		}
		return 0; 

	} else if (action==VIEW_Save || action==VIEW_SaveAs) {
		 // save file
		Document *sdoc=doc;
		if (!sdoc) sdoc=laidout->curdoc;
		if (!sdoc) {
			if (!isblank(laidout->project->filename)) {
				//***need to collect error msg
				
				ErrorLog log;
				if (laidout->project->Save(log)==0) {
					PostMessage(_("Project saved."));
					return 0;
				}
			}
			PostMessage(_("No document to save!"));
			return 0;
		}
		DBG cerr <<"....viewwindow says save.."<<endl;
		if (isblank(sdoc->Saveas()) 
				|| strstr(sdoc->Saveas(),_("untitled"))
				|| action==VIEW_SaveAs) {
			 // launch saveas!!
			//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
						//int xx,int yy,int ww,int hh,int brder,
						//anXWindow *prev,Window nowner,const char *nsend,
						//const char *newlabel,const char *newtext,unsigned int ntstyle,
						//int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
						
			char *where=newstr(isblank(sdoc->Saveas())?NULL:sdoc->Saveas());
			if (!where && !isblank(laidout->project->filename)) where=lax_dirname(laidout->project->filename,0);

			app->rundialog(new FileDialog(NULL,NULL,_("Save As..."),
						ANXWIN_REMEMBER,
						0,0,0,0,0, object_id,"saveAsPopup",
						FILES_FILES_ONLY|FILES_SAVE_AS,
						where));
			if (where) delete[] where;
		} else {
			ErrorLog log;
			if (sdoc->Save(1,1,log)==0) {
				char blah[strlen(sdoc->Saveas())+15];
				sprintf(blah,"Saved to %s.",sdoc->Saveas());
				PostMessage(blah);
				if (!isblank(laidout->project->filename)) laidout->project->Save(log);
			} else {
				if (log.Total()) {
					PostMessage(log.MessageStr(log.Total()-1));
				} else PostMessage(_("Problem saving. Not saved."));
			}
		}
		return 0;

	} else if (action==VIEW_NewDocument) {
		app->rundialog(new NewDocWindow(NULL,NULL,"New Document",0,0,0,0,0, 0));
		return 0;

	} else if (action==VIEW_NextTool) {
		SelectTool(-1);
		return 0;

	} else if (action==VIEW_PreviousTool) {
		SelectTool(-2);
		return 0;

	} else if (action==VIEW_NextPage) {
		int pg=((LaidoutViewport *)viewport)->SelectPage(-1);
		for (int c=0; c<_kids.n; c++) if (!strcmp(_kids.e[c]->win_title,"page number")) {
			((NumSlider *)_kids.e[c])->Select(pg+1);
			break;
		}
		updateContext(0);
		return 0;

	} else if (action==VIEW_PreviousPage) {
		int pg=((LaidoutViewport *)viewport)->SelectPage(-2);
		curtool->Clear();
		for (int c=0; c<_kids.n; c++) if (!strcmp(_kids.e[c]->win_title,"page number")) {
			((NumSlider *)_kids.e[c])->Select(pg+1);
			break;
		}
		updateContext(0);
		return 0;

	} else if (action==VIEW_Help) {
		//app->addwindow(new HelpWindow());
		app->addwindow(new ShortcutWindow(NULL,"Shortcuts","Shortcuts",ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,0,0,400,600,0,"LaidoutViewport"));
		return 0;

	} else if (action==VIEW_About) {
		app->rundialog(new AboutWindow());
		return 0;

	} else if (action==VIEW_SpreadEditor) {
		 //popup a SpreadEditor
		char blah[30+strlen(doc->Name(1))+1];
		sprintf(blah,"Spread Editor for %s",doc->Name(0));
		app->addwindow(newHeadWindow(doc,"SpreadEditor"));
		return 0;
	}

	return 1;
}

int ViewWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG if (ch==LAX_F8) dumpctm(viewport->dp->Getctm());
	DBG if (ch=='r') {
	DBG 	//**** for debugging:
	DBG 	pid_t pid=getpid();
	DBG 	char blah[100];
	DBG 	sprintf(blah,"more /proc/%d/status",pid);
	DBG 	system(blah);
	DBG 	return 0;
	DBG }


	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}


	return ViewerWindow::CharInput(ch,buffer,len,state,d);
}


} // namespace Laidout

