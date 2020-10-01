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
// Copyright (C) 2010-2019 by Tom Lechner
//


#include "../language.h"
#include "signatureinterface.h"
#include "../core/utils.h"
#include "../version.h"
#include "../core/drawdata.h"
#include "../ui/papersizewindow.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/filedialog.h>
#include <lax/units.h>

// DBG !!!!!
#include <lax/displayer-cairo.h>

//template implementation:
#include <lax/lists.cc>


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


void SignatureInterface::NewLengthInputWindow(const char *name, ActionArea *area, const char *message, double startvalue)
{
	DoubleBBox bounds(area->minx,area->maxx+area->boxwidth(), area->miny,area->maxy);
	char scratch[30];
	sprintf(scratch, "%g", startvalue);
	viewport->SetupInputBox(object_id, name, scratch, message, bounds);
}


// *** for debugging:
void dumpfoldinfo(FoldedPageInfo **finfo, int numhfolds, int numvfolds)
{
    cerr <<" -- foldinfo: --"<<endl;
    for (int r=0; r<numhfolds+1; r++) {
        for (int c=0; c<numvfolds+1; c++) {
            cerr <<"  ";
            for (int i=0; i<finfo[r][c].pages.n; i++) {
                cerr<<finfo[r][c].pages.e[i]<< (i<finfo[r][c].pages.n-1?"/":"");
            }
            //finfo[r][c].x_flipped=0;
            //finfo[r][c].y_flipped=0;
        }
        cerr <<endl;
    }
}





//size of the fold indicators on left of screen
#define INDICATOR_SIZE 10


enum SignatureInterfaceAreas {
	SP_None = 0,

	SP_Tile_X_top,
	SP_Tile_X_bottom,
	SP_Tile_Y_left,
	SP_Tile_Y_right,
	SP_Tile_Gap_X,
	SP_Tile_Gap_Y,

	SP_Inset_Top,
	SP_Inset_Bottom,
	SP_Inset_Left,
	SP_Inset_Right,

	SP_H_Folds_left,
	SP_H_Folds_right,
	SP_V_Folds_top,
	SP_V_Folds_bottom,

	SP_Trim_Top,
	SP_Trim_Bottom,
	SP_Trim_Left,
	SP_Trim_Right,

	SP_Margin_Top,
	SP_Margin_Bottom,
	SP_Margin_Left,
	SP_Margin_Right,

	SP_Binding,

	SP_Sheets_Per_Sig,
	SP_Num_Pages,
	SP_Paper_Name,
	SP_Paper_Width,
	SP_Paper_Height,
	SP_Paper_Orient,
	SP_Current_Sheet,

	SP_Automarks,

	 //these three currently ignored:
	SP_Up,
	SP_X,
	SP_Y,

	SP_On_Stack,
	SP_New_First_Stack,
	SP_New_Last_Stack,
	SP_New_Insert,
	SP_Delete_Stack,

	SP_FOLDS = 100
};


enum SignatureInterfaceActions {
	SIA_Decorations,
	SIA_Thumbs,
	SIA_Center,
	SIA_CenterStacks,
	SIA_NextFold,
	SIA_PreviousFold,
	SIA_InsetMask,
	SIA_InsetInc,
	SIA_InsetDec,
	SIA_GapInc,
	SIA_GapDec,
	SIA_TileXInc,
	SIA_TileXDec,
	SIA_TileYInc,
	SIA_TileYDec,
	SIA_NumFoldsVInc,
	SIA_NumFoldsVDec,
	SIA_NumFoldsHInc,
	SIA_NumFoldsHDec,
	SIA_BindingEdge,
	SIA_BindingEdgeR,
	SIA_PageOrientation,
	SIA_TrimMask,
	SIA_TrimInc,
	SIA_TrimDec,
	SIA_MarginMask,
	SIA_MarginInc,
	SIA_MarginDec,
	SIA_MAX
};


//------------------------------------- SignatureInterface --------------------------------------
	
/*! \class SignatureInterface 
 * \brief Interface to fold around a Signature on the edit space.
 */
/*! \var int SignatureInterface::foldindex
 * \brief Where a potential fold is.
 *
 * If moving up or down, foldindex is the row immediately above the fold.
 * If moving left or right, foldindex is the column immediately to the right of the fold.
 */
/*! \var int SignatureInterface::foldlevel
 * \brief The current unfolding. 0 is totally unfolded.
 */

SignatureInterface::SignatureInterface(LaxInterfaces::anInterface *nowner,int nid,Laxkit::Displayer *ndp,
									   SignatureImposition *sig, PaperStyle *p, Document *ndoc)
	: ImpositionInterface(nowner,nid,ndp) 
{
	document=ndoc;
	if (document) document->inc_count();

	rescale_pages=1;

	showdecs=0;
	showthumbs=1;
	showsplash=0;
	insetmask=15; trimmask=15; marginmask=15;
	firsttime=1;

	if (sig) {
		sigimp=(SignatureImposition*)sig->duplicate();
	} else {
		sigimp=new SignatureImposition;
	}
	siginstance=sigimp->GetSignature(0,0);
	signature=siginstance->pattern;
	reallocateFoldinfo();

	currentPaperSpread=0;
	pageoffset=0;
	midpageoffset=0;
	sigpaper=0; //this is which paper within the current SignatureInstance is currentPaperSpread
	siggroup=0; //how many complete stack groups precede the current group

	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	hasfinal=0;
	finalc=finalr=-1;
	foldinfo=signature->foldinfo;
	signature->resetFoldinfo(NULL);
	applyFold(NULL);
	checkFoldLevel(1);
	foldlevel=0;

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	if (p) SetPaper(p);

//	if (!p && !sig) {
//		SetTotalDimensions(5,5);
//	}

	color_inset  =rgbcolorf(.5,0,0);
	color_margin =rgbcolorf(.75,.75,.75);
	color_trim   =rgbcolorf(1.,0,0);
	color_binding=rgbcolorf(0,1,0);
	color_h      =rgbcolorf(.85,.85,.85);
	color_text   =rgbcolorf(.1,.1,.1);

	onoverlay=0;
	onoverlay_i=-1;
	onoverlay_ii=-1;
	onoverlay=0;
	overoverlay=0;
	arrowscale=1;
	activetilex=activetiley=0;
	//remapHandles();

	sc=NULL;

	if (document) sigimp->NumPages(document->pages.n);
}

SignatureInterface::~SignatureInterface()
{
	DBG cerr <<"SignatureInterface destructor.."<<endl;

	if (sigimp) sigimp->dec_count();
	if (sc) sc->dec_count();
	if (document) document->dec_count();
}

const char *SignatureInterface::Name()
{ return _("Signature Folder"); }

//! Reallocate signature->foldinfo, usually after adding fold lines.
/*! this will flush any folds stored in the signature.
 */
void SignatureInterface::reallocateFoldinfo()
{
	signature->folds.flush();
	signature->reallocateFoldinfo();
	foldinfo=signature->foldinfo;
	hasfinal=0;

	if (viewport) {
		char str[200];
		sprintf(str,_("Base holds %d pages."),2*(signature->numvfolds+1)*(signature->numhfolds+1));
		PostMessage(str);
	}
}

// *** temp! note to self: remove when not needed
void SignatureInterface::dumpFoldinfo()
{
	for (int r=signature->numhfolds; r>=0; r--) {
		for (int c=0; c<signature->numvfolds+1; c++) {
			cerr <<"pages:"<<foldinfo[r][c].pages.n
				<< " x,yff:"<<foldinfo[r][c].finalxflip<<","<<foldinfo[r][c].finalyflip
				<< " x,yf:"<<foldinfo[r][c].x_flipped<<","<<foldinfo[r][c].y_flipped
				<<" ir,c:"<<foldinfo[r][c].finalindexfront<<","<<foldinfo[r][c].finalindexback
				<<" -- ";
		}
		cerr << endl;
	}
	cerr << endl;
}

//! Call the other applyFold() with the value found in the given fold.
void SignatureInterface::applyFold(Fold *fold)
{
	if (!fold) return;
	applyFold(fold->direction, fold->whichfold, fold->under);
}

//! Low level flipping across folds.
/*! This will flip everything on one side of a fold to the other side (if possible).
 * It is not a selective flipping.
 *
 * This is called to ONLY apply the fold. It does not check and apply final index settings
 * or check for validity of the fold.
 */
void SignatureInterface::applyFold(char folddir, int index, int under)
{
	signature->applyFold(foldinfo, folddir,index,under);
}

//! Check if the signature is totally folded or not.
/*! Remember that if there are no fold lines, then we need to be hasfinal==1 for
 * totally folded, letting us set margin, final trim, and binding.
 *
 * If update!=0, then if the pattern is not totally folded, then make hasfinal=0,
 * make sure binding and updirection is applied to foldinfo.
 */
void SignatureInterface::checkFoldLevel(int update)
{
	hasfinal=signature->checkFoldLevel(foldinfo,&finalr,&finalc);
}

#define SIGM_Portrait        2000
#define SIGM_Landscape       2001
#define SIGM_SaveAsResource  2002
#define SIGM_FinalFromPaper  2003
#define SIGM_CustomPaper     2004
#define SIGM_Thumbs          2005
#define SIGM_Rescale_Pages   2006

/*! Return whether every siginstance has a totally folded pattern.
 */
int SignatureInterface::IsFinal()
{
	SignatureInstance *s=sigimp->GetSignature(0,0);
	SignatureInstance *s2;
	while (s) {
		s2=s;
		while (s2) {
			if (!s2->pattern->checkFoldLevel(NULL,NULL,NULL)) return 0;
			s2=s2->next_insert;
		}
		s=s->next_stack;
	}
	return 1;
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *SignatureInterface::ContextMenu(int x,int y, int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo(_("Signature Interface"));
	else if (menu->n()==0) menu->AddSep(_("Signatures"));

	int landscape=0;
	const char *paper="";
	if (siginstance->partition->paper) {
		landscape=siginstance->partition->paper->landscape();
		paper=siginstance->partition->paper->name;
	}

	menu->AddToggleItem(_("Portrait"),  SIGM_Portrait,  0, !landscape);
	menu->AddToggleItem(_("Landscape"), SIGM_Landscape, 0, landscape);
	menu->AddSep(_("Paper"));

	menu->AddItem(_("Paper Size"),999);
	menu->SubMenu(_("Paper Size"));
	for (int c=0; c<laidout->papersizes.n; c++) {
		//if (!strcmp(laidout->papersizes.e[c]->name,"Custom")) continue; // *** 
		if (!strcmp(laidout->papersizes.e[c]->name,"Whatever")) continue;

		menu->AddToggleItem(laidout->papersizes.e[c]->name, c, 0, (!strcasecmp(paper,laidout->papersizes.e[c]->name)));
		//menu->AddItem(laidout->papersizes.e[c]->name,c,
		//		LAX_ISTOGGLE
		//		| (!strcmp(paper,laidout->papersizes.e[c]->name) ? LAX_CHECKED : 0));
	}
	menu->EndSubMenu();
	//menu->AddItem(_("Custom paper size"),SIGM_CustomPaper);
	menu->AddItem(_("Paper Size to Final Size"),SIGM_FinalFromPaper);

	if (IsFinal()) {
		menu->AddSep();
		menu->AddItem(_("Save as resource..."),SIGM_SaveAsResource);
	}

	menu->AddSep();

	menu->AddToggleItem(_("Show page images"), SIGM_Thumbs, 0, showthumbs);
	//menu->AddItem(_("Show page images"), SIGM_Thumbs, LAX_ISTOGGLE|(showthumbs?LAX_CHECKED:0));

	menu->AddToggleItem(_("Scale pages when reimposing"), SIGM_Rescale_Pages, 0, rescale_pages);
	//menu->AddItem(_("Scale pages when reimposing"), SIGM_Rescale_Pages, LAX_ISTOGGLE|(rescale_pages?LAX_CHECKED:0));
	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int SignatureInterface::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int i=s->info2;

		if (i==SIGM_SaveAsResource) {
			char *impdir=laidout->default_path_for_resource("impositions");
			app->rundialog(new FileDialog(NULL,NULL,_("Save As..."),
						ANXWIN_REMEMBER,
						0,0,0,0,0, object_id,"saveAsPopup",
						FILES_FILES_ONLY|FILES_SAVE_AS,
						NULL,impdir));
			delete[] impdir;
			return 0;

		} else if (i==SIGM_Rescale_Pages) {
			rescale_pages=!rescale_pages;
			SimpleMessage *m=new SimpleMessage();
			m->info1=1;
			m->info2=rescale_pages;
			app->SendMessage(m, curwindow->win_parent->object_id, "settings", object_id);
			needtodraw=1;
			return 0;

		} else if (i==SIGM_Thumbs) {
			showthumbs=!showthumbs;
			needtodraw=1;
			return 0;

		} else if (i==SIGM_FinalFromPaper) {
			sigimp->SetPaperFromFinalSize(siginstance->partition->paper->w(),siginstance->partition->paper->h());
			remapHandles();
			needtodraw=1;
			return 0;

		} else if (i==SIGM_CustomPaper) {
			cerr <<" *** need to implement edit custom paper size!!"<<endl;
			return 0;

		} else if (i==SIGM_Landscape) {
			PaperStyle *paper=siginstance->partition->paper;
			if (paper && !paper->landscape()) {
				paper->landscape(1);
				siginstance->SetPaper(paper,1);
				remapHandles();
				needtodraw=1;
			}
			if (siginstance==sigimp->GetSignature(0,0)) sigimp->SetDefaultPaperSize(paper);
			return 0;

		} else if (i==SIGM_Portrait) {
			PaperStyle *paper=siginstance->partition->paper;
			if (paper && paper->landscape()) {
				paper->landscape(0);
				siginstance->SetPaper(paper,1);
				remapHandles();
				needtodraw=1;
			}
			if (siginstance==sigimp->GetSignature(0,0)) sigimp->SetDefaultPaperSize(paper);
			return 0;

		} else if (i<999) {
			 //selecting new paper size
			if (i>=0 && i<laidout->papersizes.n) {
				if (!strcmp(laidout->papersizes.e[i]->name,"Custom")) {
					makestr(siginstance->partition->paper->name, _("Custom"));
					remapHandles();
					return 0;
				}
				SetPaper(laidout->papersizes.e[i]);
				remapHandles();
				needtodraw=1;
			}
			return 0;
		}
		return 1;

	} else if (!strcmp(mes,"paperwidth")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		double dd=strtof(s->str,NULL);
		makestr(siginstance->partition->paper->name, _("Custom"));
		if (siginstance->partition->paper->landscape()) siginstance->partition->paper->height=dd;
		siginstance->partition->paper->width=dd;
		siginstance->SetPaper(siginstance->partition->paper,0);
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"paperheight")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		double dd=strtof(s->str,NULL);
		makestr(siginstance->partition->paper->name, _("Custom"));
		if (siginstance->partition->paper->landscape()) siginstance->partition->paper->width=dd;
		siginstance->partition->paper->height=dd;
		siginstance->SetPaper(siginstance->partition->paper,0);
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"papersize")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(data);
		PaperStyle *paper = dynamic_cast<PaperStyle*>(s->object);
		if (!paper) return 0;
		SetPaper(paper);
		remapHandles();
		return 0;

	} else if (!strcmp(mes,"saveAsPopup")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;

		ErrorLog log;
		FILE *f=open_file_for_writing(s->str,1,&log);
		if (!f) {
			if (log.Total()) PostMessage(log.MessageStr(log.Total()-1));
			return 0;
		}

		fprintf(f,"#Laidout %s Imposition\n",LAIDOUT_VERSION);
		fprintf(f,"type SignatureImposition\n\n");

		sigimp->dump_out(f,0,0,NULL);

		fclose(f);

		PostMessage(_("Saved."));

		return 0;
	}

	return 1;
}

/*! This only allocates the controls, does not position or anything else.
 */
void SignatureInterface::createHandles()
{
	unsigned long c,c2;

	//ActionArea categories:
	//  0 single instance
	//  1 in pattern area
	//  2 in page area
	//  3 in gap area

 	 //main top of window controls
	c =color_h;
	c2=color_text;
	controls.push(new ActionArea(SP_Paper_Name       , AREA_H_Slider, siginstance->partition->paper->name, ("Paper to use"),0,1,c,0,c2));
	controls.push(new ActionArea(SP_Paper_Width      , AREA_H_Slider, siginstance->partition->paper->name, ("Paper width"), 0,1,c,0,c2));
	controls.push(new ActionArea(SP_Paper_Height     , AREA_H_Slider, siginstance->partition->paper->name, ("Paper height"),0,1,c,0,c2));
	controls.push(new ActionArea(SP_Paper_Orient     , AREA_H_Slider, "--",        _("Paper orientation"),0,1,c,0,c2));
	controls.push(new ActionArea(SP_Current_Sheet    , AREA_H_Slider, "Sheet",     _("Current sheet"),    0,1,c,0,c2));
	controls.push(new ActionArea(SP_Num_Pages        , AREA_H_Slider, "Pages",     _("Wheel or drag changes number of pages"),      0,1,c,0,c2));
	controls.push(new ActionArea(SP_Automarks        , AREA_H_Slider, "Automarks", _("Wheel or click to select auto printer marks"),0,1,c,0,c2));

	c=color_inset;
	controls.push(new ActionArea(SP_Inset_Top        , AREA_Handle, NULL, _("Inset top"),   2,1,c,0));
	controls.push(new ActionArea(SP_Inset_Bottom     , AREA_Handle, NULL, _("Inset bottom"),2,1,c,0));
	controls.push(new ActionArea(SP_Inset_Left       , AREA_Handle, NULL, _("Inset left"),  2,1,c,0));
	controls.push(new ActionArea(SP_Inset_Right      , AREA_Handle, NULL, _("Inset right"), 2,1,c,0));

	c=color_trim;
	controls.push(new ActionArea(SP_Trim_Top         , AREA_Handle, NULL, _("Trim top of page"),   2,1,c,2));
	controls.push(new ActionArea(SP_Trim_Bottom      , AREA_Handle, NULL, _("Trim bottom of page"),2,1,c,2));
	controls.push(new ActionArea(SP_Trim_Left        , AREA_Handle, NULL, _("Trim left of page"),  2,1,c,2));
	controls.push(new ActionArea(SP_Trim_Right       , AREA_Handle, NULL, _("Trim right of page"), 2,1,c,2));

	c=coloravg(color_margin,0,.3333);
	controls.push(new ActionArea(SP_Margin_Top       , AREA_Handle, NULL, _("Margin top of page"),   2,1,c,2));
	controls.push(new ActionArea(SP_Margin_Bottom    , AREA_Handle, NULL, _("Margin bottom of page"),2,1,c,2));
	controls.push(new ActionArea(SP_Margin_Left      , AREA_Handle, NULL, _("Margin left of page"),  2,1,c,2));
	controls.push(new ActionArea(SP_Margin_Right     , AREA_Handle, NULL, _("Margin right of page"), 2,1,c,2));

	c=color_binding;
	controls.push(new ActionArea(SP_Binding          , AREA_Handle, NULL, _("Binding edge, drag to place"),1,0,c,2));

	c=color_inset;
	controls.push(new ActionArea(SP_Tile_X_top       , AREA_H_Slider, NULL, _("Tile horizontally, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_X_bottom    , AREA_H_Slider, NULL, _("Tile horizontally, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_Y_left      , AREA_V_Slider, NULL, _("Tile vertically, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_Y_right     , AREA_V_Slider, NULL, _("Tile vertically, wheel or drag changes"),1,0,c,0));

	controls.push(new ActionArea(SP_Tile_Gap_X       , AREA_Handle, NULL, _("Vertical gap between tiles"),1,0,c,3));
	controls.push(new ActionArea(SP_Tile_Gap_Y       , AREA_Handle, NULL, _("Horizontal gap between tiles"),1,0,c,3));

	c=color_margin;
	controls.push(new ActionArea(SP_H_Folds_left     , AREA_V_Slider, NULL, _("Number of horizontal folds"),1,0,c,1));
	controls.push(new ActionArea(SP_H_Folds_right    , AREA_V_Slider, NULL, _("Number of horizontal folds"),1,0,c,1));
	controls.push(new ActionArea(SP_V_Folds_top      , AREA_H_Slider, NULL, _("Number of vertical folds"),1,0,c,1));
	controls.push(new ActionArea(SP_V_Folds_bottom   , AREA_H_Slider, NULL, _("Number of vertical folds"),1,0,c,1));

	 //stack placeholders, mainly for tooltip lookup
	controls.push(new ActionArea(SP_On_Stack         , AREA_Slider, NULL,    _("Wheel to change sheets"),1,0,0,0));
	controls.push(new ActionArea(SP_New_First_Stack  , AREA_Button, NULL,    _("New first stack"),1,0,0,0));
	controls.push(new ActionArea(SP_New_Last_Stack   , AREA_Button, NULL,    _("New last stack"),1,0,0,0));
	controls.push(new ActionArea(SP_New_Insert       , AREA_Button, NULL,    _("New insert"),1,0,0,0));
	controls.push(new ActionArea(SP_Delete_Stack     , AREA_Button, NULL,    _("Delete this one"),1,0,0,0));
	//controls.push(new ActionArea(SP_Sheets_Per_Sig   , AREA_Slider, NULL, _("Wheel or drag changes"),1,1,0,0));
}

ActionArea *SignatureInterface::control(int what)
{
	for (int c=0; c<controls.n; c++) if (what==controls.e[c]->action) return controls.e[c];
	return NULL;
}

void SignatureInterface::ViewportResized()
{
	remapHandles(0);
}

//! Position the handles to be where they should.
/*! which==0 means do all.
 *  which&1 do singletons,
 *  which&2 pattern area (category 1),
 *  which&4 page area (category 2)
 */
void SignatureInterface::remapHandles(int which)
{
	if (!dp) return;
	if (controls.n==0) createHandles();

	DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(dp);
	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf remapHandles, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	ActionArea *area;
	flatpoint *p;
	int n;
	double ww=0,hh=0;
	GetDimensions(ww,hh);
	double w=signature->PageWidth(0), h=signature->PageHeight(0); // *** patternwidth not updating with landscape change?
	double www=siginstance->PatternWidth(), hhh=siginstance->PatternHeight();

	SimpleUnit *units=GetUnitManager();

	arrowscale=2*INDICATOR_SIZE;
	//arrowscale=2*INDICATOR_SIZE/dp->Getmag();


	 //----- sheets and pages, and other general settings
	if (which==0 || which&1) {
		// Final page size: ....                      [Current paper]
		// [Letter] [width] [height] [Portrait]           [num pages]
		// [automarks]


		char buffer[100];
		double hhhh = dp->textheight()*1.4;
		double wwww;

		//----------left side
		 //SP_Paper_Name
		area=control(SP_Paper_Name);
		makestr(area->text,siginstance->partition->paper->name);
		wwww=dp->textextent(area->text,-1, NULL,NULL)+hhhh;
		area->SetRect(0,hhhh, wwww,hhhh);
		double xxxx=wwww;

		 //SP_Paper_Width
		area=control(SP_Paper_Width);
		sprintf(buffer,"%g",units->Convert(siginstance->partition->paper->w(), UNITS_Inches,laidout->prefs.default_units,NULL));
		makestr(area->text,buffer);
		wwww=dp->textextent(area->text,-1, NULL,NULL)+hhhh;
		area->SetRect(xxxx,hhhh, wwww,hhhh);
		xxxx+=wwww;

		 //SP_Paper_Height
		area=control(SP_Paper_Height);
		sprintf(buffer,"%g",units->Convert(siginstance->partition->paper->h(), UNITS_Inches,laidout->prefs.default_units,NULL));
		makestr(area->text,buffer);
		wwww=dp->textextent(area->text,-1, NULL,NULL)+hhhh;
		area->SetRect(xxxx,hhhh, wwww,hhhh);
		xxxx+=wwww;

		 //SP_Paper_Orient
		area=control(SP_Paper_Orient);
		if (siginstance->partition->paper->landscape()) makestr(area->text,_("Landscape"));
		else makestr(area->text,_("Portrait"));
		wwww=dp->textextent(area->text,-1, NULL,NULL)+hhhh;
		area->SetRect(xxxx,hhhh, wwww,hhhh);

		 //SP_Automarks
		area = control(SP_Automarks);
		if (siginstance->automarks==0) makestr(area->text, _("No automarks"));
		else if (siginstance->automarks==AUTOMARK_Margins)  makestr(area->text, _("Automarks outside"));
		else if (siginstance->automarks==AUTOMARK_InnerDot) makestr(area->text, _("Automarks inside"));
		else if (siginstance->automarks==(AUTOMARK_InnerDot|AUTOMARK_Margins)) makestr(area->text,_("Automarks in and out"));
		wwww = dp->textextent(area->text,-1, NULL,NULL)+hhhh;
		area->SetRect(0,2*hhhh, wwww,hhhh);


		//----------right side
		 //SP_Current_Sheet
		area=control(SP_Current_Sheet);
		sprintf(buffer,"Sheet %d/%d, %s",(int(currentPaperSpread/2)+1),
										 sigimp->NumPapers()/2,
										 (OnBack()?"Back":"Front"));
		makestr(area->text,buffer);
		//wwww=dp->textextent("Sheet 0000/0000, Front",-1,NULL,NULL);
		wwww=dp->textextent(buffer,-1,NULL,NULL)+hhhh;
		area->SetRect(dp->Maxx-wwww,0, wwww,hhhh);

		 //SP_Num_Pages
		area=control(SP_Num_Pages);
		sprintf(buffer,_("%d pages"),sigimp->NumPages());
		makestr(area->text,buffer);
		wwww=dp->textextent(buffer,-1, NULL,NULL) + hhhh;
		area->SetRect(dp->Maxx-wwww,hhhh,wwww,hhhh);

	}


	if (which==0 || which&1) { //paper specific
		area=control(SP_Tile_X_top); //trapezoidal area above paper
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,hh);  p[1]=flatpoint(ww,hh); p[2]=flatpoint(ww*1.1,hh*1.1); p[3]=flatpoint(-ww*.1,hh*1.1);

		area=control(SP_Tile_X_bottom); //trapezoidal area below paper
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,0);  p[1]=flatpoint(ww,0); p[2]=flatpoint(ww*1.1,-hh*.1); p[3]=flatpoint(-ww*.1,-hh*.1);

		area=control(SP_Tile_Y_left);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,0);  p[1]=flatpoint(0,hh); p[2]=flatpoint(-ww*.1,hh*1.1); p[3]=flatpoint(-ww*.1,-hh*.1);

		area=control(SP_Tile_Y_right);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(ww,0);  p[1]=flatpoint(ww,hh); p[2]=flatpoint(ww*1.1,hh*1.1); p[3]=flatpoint(ww*1.1,-hh*.1);

		area=control(SP_Tile_Gap_X);
		p=draw_thing_coordinates(THING_Double_Arrow_Horizontal, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale/2);
		area->Position(0,0,3);

		area=control(SP_Tile_Gap_Y);
		p=draw_thing_coordinates(THING_Double_Arrow_Vertical, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale/2);
		area->Position(0,0,3);

		 //------inset arrows
		area=control(SP_Inset_Top);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(ww/2,hh-siginstance->partition->insettop);

		area=control(SP_Inset_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(ww/2,siginstance->partition->insetbottom);

		area=control(SP_Inset_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(siginstance->partition->insetleft,hh/2);

		area=control(SP_Inset_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(ww-siginstance->partition->insetright,hh/2);
	} //which&1


	if (which==0 || which&2) { //pattern area specific
		 //-----folds
		area=control(SP_H_Folds_left);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,0);  p[1]=flatpoint(www*.1,hhh*.1); p[2]=flatpoint(www*.1,hhh*.9); p[3]=flatpoint(0,hhh);

		area=control(SP_H_Folds_right);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(www,0);  p[1]=flatpoint(www,hhh); p[2]=flatpoint(www*.9,hhh*.9); p[3]=flatpoint(www*.9,hhh*.1);

		area=control(SP_V_Folds_top);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,hhh);  p[1]=flatpoint(www,hhh); p[2]=flatpoint(www*.9,hhh*.9); p[3]=flatpoint(www*.1,hhh*.9);

		area=control(SP_V_Folds_bottom);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,0);  p[1]=flatpoint(www,0); p[2]=flatpoint(www*.9,hhh*.1); p[3]=flatpoint(www*.1,hhh*.1);
	}


	if (which==0 || which&4) { //final page area specific
		 //----trim arrows
		area=control(SP_Trim_Top);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(w/3,h-signature->trimtop);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(w/3,signature->trimbottom);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(signature->trimleft,h/3);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(w-signature->trimright,h/3);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		 //------margins arrows
		area=control(SP_Margin_Top);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(w*2/3,h-signature->margintop);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(w*2/3,signature->marginbottom);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(signature->marginleft,h*2/3);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,-1, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(w-signature->marginright,h*2/3);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		 //--------binding line
		area=control(SP_Binding);
		//area->Points(NULL,4,1); //doesn't actually need points, it is checked for very manually
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);
	} //page area items

	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf remapHandles end, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;
}



/*! PaperGroup or PaperBoxData.
 */
int SignatureInterface::draws(const char *atype)
{ return !strcmp(atype,"SignatureData"); }


//! Return a new SignatureInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *SignatureInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new SignatureInterface(NULL,id,NULL);
	else if (!dynamic_cast<SignatureInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int SignatureInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;

	 //hack to sync a setting with ImpositionEditor window:
	SimpleMessage *m=new SimpleMessage();
	m->info1=1;
	m->info2=rescale_pages;
	app->SendMessage(m, curwindow->win_parent->object_id, "settings", object_id);

	return 0;
}

int SignatureInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void SignatureInterface::Clear(SomeData *d)
{
}

	
int SignatureInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) { remapHandles(); firsttime=0; }


	DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(dp);
	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	double patternheight=siginstance->PatternHeight();
	double patternwidth =siginstance->PatternWidth();

	double x,y,w,h;
	static char str[150];

	//int front = OnBack();
	//double xl = (front ? siginstance->partition->insetleft  : siginstance->partition->insetright);
	//double xr = (front ? siginstance->partition->insetright : siginstance->partition->insetleft);

	 //----------------draw whole outline
	dp->NewFG(1.,0.,1.); //purple for paper outline, like custom papergroup border color
	GetDimensions(w,h);
	dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
	dp->LineWidthScreen(1);
	dp->drawline(0,0, w,0);
	dp->drawline(w,0, w,h);
	dp->drawline(w,h, 0,h);
	dp->drawline(0,h, 0,0);
	
	 //----------------draw inset
	dp->NewFG(color_inset); //dark red for inset
	if (OnBack()) {
		if (siginstance->partition->insetleft)   dp->drawline(siginstance->partition->insetleft,0,    siginstance->partition->insetleft, h);
		if (siginstance->partition->insetright)  dp->drawline(w-siginstance->partition->insetright,0, w-siginstance->partition->insetright, h);
	} else {
		if (siginstance->partition->insetleft)   dp->drawline(siginstance->partition->insetright,0,    siginstance->partition->insetright, h);
		if (siginstance->partition->insetright)  dp->drawline(w-siginstance->partition->insetleft,0, w-siginstance->partition->insetleft, h);
	}
	if (siginstance->partition->insettop)    dp->drawline(0,h-siginstance->partition->insettop,   w, h-siginstance->partition->insettop);
	if (siginstance->partition->insetbottom) dp->drawline(0,siginstance->partition->insetbottom,  w, siginstance->partition->insetbottom);


	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh draw pattern, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	 //------------------draw fold pattern in each tile
	double ew=patternwidth/(signature->numvfolds+1);
	double eh=patternheight/(signature->numhfolds+1);
	w=patternwidth;
	h=patternheight;

	flatpoint pts[4],fp;
	//int facedown=0;
	int hasface;
	int rrr,ccc;
	int urr,ucc;
	int ff,tt;
	double xx,yy;
	//int xflip;
	int yflip;
	int i=-1;
	ImageData *thumb;

	//DBG dumpfoldinfo(foldinfo, signature->numhfolds, signature->numvfolds);

	int rangeofpapers = 2*siginstance->sheetspersignature;
	int npageshalf = siginstance->PagesPerSignature(0,1)/2;
	double apparentleft = OnBack() ? siginstance->partition->insetleft : siginstance->partition->insetright; 

	x = apparentleft;
	for (int tx=0; tx<siginstance->partition->tilex; tx++) {
	  y = siginstance->partition->insetbottom;

	  for (int ty=0; ty<siginstance->partition->tiley; ty++) {

		 //fill in light gray for elements with no current faces
		 //or draw orientation arrow and number for existing faces
		for (int rr=0; rr<signature->numhfolds+1; rr++) {
		  for (int cc=0; cc<signature->numvfolds+1; cc++) {
			hasface = foldinfo[rr][cc].pages.n;

			 //when on back page, flip horizontal placements
			if (foldlevel==0 && OnBack()) {
				urr = rr;
				ucc = signature->numvfolds-cc;
			} else {
				urr = rr;
				ucc = cc;
			}

			 //first draw filled face, grayed if no current faces
			dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
			dp->LineWidthScreen(1);
			pts[0]=flatpoint(x+ucc*ew,y+urr*eh); //lower left
			pts[1]=pts[0]+flatpoint(ew,0);      //lower right
			pts[2]=pts[0]+flatpoint(ew,eh);    //upper right
			pts[3]=pts[0]+flatpoint(0,eh);    //upper left

			if (hasface) dp->NewFG(1.,1.,1.);
			else dp->NewFG(.75,.75,.75);

			dp->drawlines(pts,4,1,1);

			 //draw thumbnails or page numbers if cell has something in it
			if (hasface && foldlevel==0) {
			//if (hasface) { <- *** TODo: show numbers at any fold level
				 //rrr,ccc are row,col of where top page at rr,cc started in the beginning
				rrr=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-2];
				ccc=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-1];

				if (foldinfo[rr][cc].finalindexfront>=0) {
					 //there are faces in this spot, draw arrow and page number
					dp->NewFG(.75,.75,.75);
					if (hasfinal) {
						//xflip=foldinfo[rrr][ccc].x_flipped^foldinfo[rrr][ccc].finalxflip;
						yflip=foldinfo[rrr][ccc].y_flipped^foldinfo[rrr][ccc].finalyflip;
					} else {
						//xflip=foldinfo[rrr][ccc].x_flipped;
						yflip=foldinfo[rrr][ccc].y_flipped;
					}

					//facedown=((xflip && !yflip) || (!xflip && yflip));
					//if (facedown) dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
					//else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

					 //compute range of pages for this cell and print range of pages at bottom of arrow
					if (foldlevel==0) {
						 //all unfolded, show only page for currentPaperSpread
						tt=foldinfo[rrr][ccc].finalindexfront;
						ff=foldinfo[rrr][ccc].finalindexback;
						if (ff>tt) {
							tt*=rangeofpapers/2;
							ff=tt + rangeofpapers - 1;
						} else {
							ff*=rangeofpapers/2;
							tt=ff + rangeofpapers-1;
						}

						if (ff>tt) i=ff-sigpaper;
						else i=ff+sigpaper;
						if (i<npageshalf) i+=pageoffset; else i+=midpageoffset;
						i++; //make first page 1, not 0

						if (document && i>0 && i<=document->pages.n && document->pages.e[i-1]->label)
							sprintf(str,"%s",document->pages.e[i-1]->label);
						else sprintf(str,"%d",i);

					} else {
						 //partially folded, need to figure out which page is on top
						tt=rangeofpapers*foldinfo[rrr][ccc].finalindexfront;
						ff=rangeofpapers*foldinfo[rrr][ccc].finalindexback;

						 //show range of pages represented at this fold stage
						if (ff<npageshalf) { ff+=pageoffset; tt+=pageoffset; }
						else { ff+=midpageoffset; tt+=midpageoffset; }
						if (document && ff>=0 && ff<document->pages.n && document->pages.e[ff]->label)
							sprintf(str,"%s",document->pages.e[ff]->label);
						else sprintf(str,"%d",ff+1);
						//sprintf(str,"%d-%d",ff+1,tt+1); //shows whole range of pages
					}


					 //show thumbnails
					if (foldlevel==0) {
						if (showthumbs && document && i-1>=0 && i-1<document->pages.n) {
							DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh show thumbs, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

							 //draw page i in box defined by pts
							thumb=document->pages.e[i-1]->Thumbnail();
							if (thumb) {
								Affine tr;
								flatpoint p1;
								if (yflip) { p1=pts[2]; tr.origin(p1); tr.xaxis(pts[3]-p1); tr.yaxis(pts[1]-p1); }
								else { p1=pts[0]; tr.origin(p1); tr.xaxis(pts[1]-p1); tr.yaxis(pts[3]-p1); }
								double neww=tr.xaxis().norm();
								double newh=tr.yaxis().norm();
								tr.Normalize();
								dp->PushAndNewTransform(tr.m());

								if (rescale_pages) {
									 //thumb min/maxx, min/maxy are same as page outline. thumb origin is page origin
									
									flatpoint offset, fp2;
									offset=thumb->transformPoint(flatpoint(thumb->maxx,thumb->maxy));
									fp2   =thumb->transformPoint(flatpoint(thumb->minx,thumb->miny));
									double oldw=offset.x-fp2.x; if (oldw<0) oldw=-oldw;
									double oldh=offset.y-fp2.y; if (oldh<0) oldh=-oldh;
									double scaling=1;

									if (neww/newh > oldw/oldh) {
										scaling=newh/oldh;
										offset=flatpoint((neww-scaling*oldw)/2, 0);
									} else {
										scaling=neww/oldw;
										offset=flatpoint(0, (newh-scaling*oldh)/2);
									}
									offset+=flatpoint(0-thumb->minx*scaling, 0-thumb->miny*scaling);
									Affine newt;
									newt.Scale(scaling);
									newt.origin(offset);
									dp->PushAndNewTransform(newt.m());
								}

								 // always setup clipping region to be the page
								//dp->PushClip(1);
								//SetClipFromPaths(dp,view->spreads.e[c]->spread->pagestack.e[c2]->outline,dp->Getctm());

								Laidout::DrawData(dp,thumb,NULL,NULL);

								 //remove clipping region
								//dp->PopClip();

								if (rescale_pages) dp->PopAxes();
								dp->PopAxes();

							}
							DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh show thumbs end, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;
						}
					}

					dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
					dp->LineWidthScreen(1);

					pts[0]=flatpoint(x+(ucc+.5)*ew,y+(urr+.25+.5*(yflip?1:0))*eh);
					pts[1]=flatpoint(0,yflip?-1:1)*eh/4; //a vector, not a point
					dp->drawarrow(pts[0],pts[1], 0,eh/2,1);
					fp=pts[0]-pts[1]/2;

					fp=dp->realtoscreen(fp);
					dp->DrawScreen();
					dp->textout(fp.x,fp.y, str,-1, LAX_CENTER);
					dp->DrawReal();

				} //if has final
			} //if location rr,cc hasface


			DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh draw final decs, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

			 //draw markings for final page binding edge, up, trim, margin
			 //draws only when totally folded
			if (hasfinal && foldlevel==signature->folds.n && rr==finalr && cc==finalc) {
				dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
				dp->LineWidthScreen(2);

				xx=x+ucc*ew;
				yy=y+urr*eh;

				 //draw gray margin edge
				dp->NewFG(color_margin);
				dp->drawline(xx,yy+signature->marginbottom,   xx+ew,yy+signature->marginbottom);			
				dp->drawline(xx,yy+eh-signature->margintop,   xx+ew,yy+eh-signature->margintop);			
				dp->drawline(xx+signature->marginleft,yy,     xx+signature->marginleft,yy+eh);			
				dp->drawline(xx+ew-signature->marginright,yy, xx+ew-signature->marginright,yy+eh);			

				 //draw red trim edge
				dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
				dp->LineWidthScreen(1);
				dp->NewFG(color_trim);
				if (signature->trimbottom>0) dp->drawline(xx,yy+signature->trimbottom, xx+ew,yy+signature->trimbottom);			
				if (signature->trimtop>0)    dp->drawline(xx,yy+eh-signature->trimtop, xx+ew,yy+eh-signature->trimtop);			
				if (signature->trimleft>0)   dp->drawline(xx+signature->trimleft,yy, xx+signature->trimleft,yy+eh);			
				if (signature->trimright>0)  dp->drawline(xx+ew-signature->trimright,yy, xx+ew-signature->trimright,yy+eh);			

				 //draw green binding edge
				dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
				dp->LineWidthScreen((overoverlay==SP_Binding?4:2));
				dp->NewFG(color_binding);

				 //todo: draw a solid line, but a dashed line toward the inner part of the page...??
				int b=signature->binding;
				double in=ew*.05;
				if (b=='l')      dp->drawline(xx+in,yy,    xx+in,yy+eh);
				else if (b=='r') dp->drawline(xx-in+ew,yy, xx-in+ew,yy+eh);
				else if (b=='t') dp->drawline(xx,yy-in+eh, xx+ew,yy-in+eh);
				else if (b=='b') dp->drawline(xx,yy+in,    xx+ew,yy+in);

			}
		  } //cc
		}  //rr

		 //draw fold pattern outline
		dp->NewFG(1.,0.,0.);
		dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
		dp->LineWidthScreen(1);
		dp->drawline(x,    y, x+w,  y);
		dp->drawline(x+w,  y, x+w,y+h);
		dp->drawline(x+w,y+h, x  ,y+h);
		dp->drawline(x,  y+h, x,y);

		 //draw all fold lines
		dp->NewFG(.5,.5,.5);
		dp->LineAttributes(-1,LineOnOffDash, CapButt, JoinMiter);
		dp->LineWidthScreen(1);
		for (int c=0; c<signature->numvfolds; c++) { //verticals
			dp->drawline(x+(c+1)*ew,y, x+(c+1)*ew,y+h);
		}

		for (int c=0; c<signature->numhfolds; c++) { //horizontals
			dp->drawline(x,y+(c+1)*eh, x+w,y+(c+1)*eh);
		}
		
		y+=patternheight+siginstance->partition->tilegapy;
	  } //tx
	  x+=patternwidth+siginstance->partition->tilegapx;
	} //ty


	 //draw in progress folding
	int device=0;
	DBG cerr <<"----------------any device down"<<buttondown.any(0,LEFTBUTTON,&device)<<endl;

	if (buttondown.any(0,LEFTBUTTON,&device) && folddirection && folddirection!='x') {
		 //this will draw a light gray tilting region across foldindex, in folddirection, with foldunder.
		 //it will correspond to foldr1,foldr2, and foldc1,foldc2.

		DBG cerr <<"--------------------------------showing dir"<<endl;
		//int mx,my;
		//buttondown.getinitial(device,LEFTBUTTON,&mx,&my);

		//flatpoint p=dp->screentoreal(mx,my);
		flatpoint dir;
		if (folddirection=='t') dir.y=1;
		else if (folddirection=='b') dir.y=-1;
		else if (folddirection=='l') dir.x=-1;
		else if (folddirection=='r') dir.x=1;

		dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
		dp->LineWidthScreen(1);
		//dp->drawarrow(p,dir,0,25,0);


		 //draw partially folded region foldr1..foldr2, foldc1..foldc2
		double ew=patternwidth/(signature->numvfolds+1);
		double eh=patternheight/(signature->numhfolds+1);
		w=ew*(foldc2-foldc1+1);
		h=eh*(foldr2-foldr1+1);

		double rotation=foldprogress*M_PI;
		if (folddirection=='r' || folddirection=='t') rotation=M_PI-rotation;
		flatpoint axis;
		if (folddirection=='l' || folddirection=='r')
			axis=rotate(flatpoint(1,0),rotation,0);
		else axis=rotate(flatpoint(0,1),-rotation,0);

		if (foldunder) {
			dp->NewFG(.2,.2,.2);
			dp->LineAttributes(-1, LineOnOffDash, CapButt, JoinMiter);
			dp->LineWidthScreen(2);
			if (folddirection=='l' || folddirection=='r') axis.y=-axis.y;
			else axis.x=-axis.x;
		} else {
			dp->NewFG(.9,.9,.9);
			dp->LineAttributes(-1, LineSolid, CapButt, JoinMiter);
			dp->LineWidthScreen(1);
		}

		x=siginstance->partition->insetleft;
		flatpoint pts[4];
		for (int tx=0; tx<siginstance->partition->tilex; tx++) {
		  y=siginstance->partition->insetbottom;
		  for (int ty=0; ty<siginstance->partition->tiley; ty++) {

			if (folddirection=='l' || folddirection=='r') { //horizontal fold
				pts[0]=flatpoint(x+foldindex*ew,y+eh*foldr1);
				pts[1]=pts[0]+axis*w;
				pts[2]=pts[1]+flatpoint(0,h);
				pts[3]=pts[0]+flatpoint(0,h);
			} else { 							//vertical fold
				pts[0]=flatpoint(x+ew*foldc1,y+foldindex*eh);
				pts[1]=pts[0]+axis*h;
				pts[2]=pts[1]+flatpoint(w,0);
				pts[3]=pts[0]+flatpoint(w,0);
			}
			if (foldunder) dp->drawlines(pts,4,0,0);
			else dp->drawlines(pts,4,1,1);
			
			y+=patternheight+siginstance->partition->tilegapy;
		  }
		  x+=patternwidth+siginstance->partition->tilegapx;
		}
	}


	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh fold indicator, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	 //draw fold indicator overlays on left side of screen
	dp->LineAttributes(-1, LineSolid, CapButt, JoinMiter);

	DrawThingTypes thing;
	dp->DrawScreen();
	dp->LineWidthScreen(1);
	for (int c=signature->folds.n-1; c>=-1; c--) {
		if (c==-1) thing=THING_Circle;
		else if (c==signature->folds.n-1 && hasfinal) thing=THING_Square;
		else thing=THING_Triangle_Down;
		getFoldIndicatorPos(c, &x,&y,&w,&h);

		 //color hightlighted to show which fold we are currently on
		if (foldlevel==c+1) dp->NewFG(1.,.5,1.);
		else dp->NewFG(1.,1.,1.);

		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 1, thing); //filled
		dp->NewFG(1.,0.,1.);
		dp->drawthing(x+w/2,y+h/2, w/2,h/2, 0, thing); //outline
	}
	dp->DrawReal();


	 //write out final page dimensions
	dp->NewFG(0.,0.,0.);
	dp->DrawScreen();
	SimpleUnit *units=GetUnitManager();
	sprintf(str,_("Final size: %g x %g %s"),
				units->Convert(signature->PageWidth(1), UNITS_Inches,laidout->prefs.default_units,NULL),
				units->Convert(signature->PageHeight(1),UNITS_Inches,laidout->prefs.default_units,NULL),
				laidout->prefs.unitname);
	dp->textout(0,0, str,-1, LAX_LEFT|LAX_TOP);
	dp->DrawReal();


	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh draw stacks, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	 //-----------------draw stacks
	drawStacks();


	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh draw handles, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	 //-----------------draw control handles
	ActionArea *area;
	double d;
	Signature *s=signature;
	PaperPartition *pp=siginstance->partition;
	double totalheight, totalwidth;
	GetDimensions(totalwidth,totalheight);
	flatpoint dv;
	dp->LineAttributes(-1, LineSolid, CapButt, JoinMiter);

	for (int c=controls.n-1; c>=0; c--) {
		area=controls.e[c];
		if (area->hidden) continue;

//		if (area->action==SP_Automarks) {
//			if (!area->hidden) {
//				dp->NewFG(color_inset);
//				dv=flatpoint((area->minx+area->maxx)/2,(area->miny+area->maxy)/2);
//				dv=dp->realtoscreen(dv);
//				dp->DrawScreen();
//				dp->textout(dv.x,dv.y,area->text,-1,LAX_CENTER);
//				dp->DrawReal();
//			}
//
//		} else
		if (area->action==SP_Tile_Gap_X) {
			if (overoverlay==SP_Tile_Gap_X) {
				dp->LineWidthScreen(5);
				for (int c2=0; c2<siginstance->partition->tilex-1; c2++) {
					d = (OnBack() ? pp->insetleft : pp->insetright) + (c2+1)*(pp->tilegapx+s->PatternWidth()) - pp->tilegapx/2;
					dp->drawline(d,0, d,totalheight);
				}
			}

		} else if (area->action==SP_Tile_Gap_Y) {
			if (overoverlay==SP_Tile_Gap_Y) {
				dp->LineWidthScreen(5);
				for (int c2=0; c2<siginstance->partition->tiley-1; c2++) {
					d=pp->insetbottom+(c2+1)*(pp->tilegapy+s->PatternHeight()) - pp->tilegapy/2;
					dp->drawline(0,d, totalwidth,d);
				}
			}

		} else if (overoverlay==area->action &&
				(area->action==SP_V_Folds_top || area->action==SP_V_Folds_bottom ||
				 area->action==SP_Tile_X_top || area->action==SP_Tile_X_bottom)) {

			if (area->action==SP_Tile_X_top || area->action==SP_Tile_X_bottom) dp->NewFG(color_inset);
			else dp->NewFG(color_h);

			dp->DrawScreen();
			dp->LineWidthScreen(1);
			dp->drawthing(lasthover.x-INDICATOR_SIZE,lasthover.y, INDICATOR_SIZE/2,INDICATOR_SIZE/2,0,THING_Triangle_Left);
			dp->drawthing(lasthover.x+INDICATOR_SIZE,lasthover.y, INDICATOR_SIZE/2,INDICATOR_SIZE/2,0,THING_Triangle_Right);
			dp->DrawReal();
			dp->LineWidthScreen(1);

		} else if (overoverlay==area->action &&
				(area->action==SP_H_Folds_left || area->action==SP_H_Folds_right ||
				 area->action==SP_Tile_Y_left || area->action==SP_Tile_Y_right)) {

			if (area->action==SP_Tile_Y_left || area->action==SP_Tile_Y_right) dp->NewFG(color_inset);
			else dp->NewFG(color_h);

			dp->DrawScreen();
			dp->LineWidthScreen(1);
			dp->drawthing(lasthover.x,lasthover.y-INDICATOR_SIZE, INDICATOR_SIZE/2,INDICATOR_SIZE/2,0,THING_Triangle_Up);
			dp->drawthing(lasthover.x,lasthover.y+INDICATOR_SIZE, INDICATOR_SIZE/2,INDICATOR_SIZE/2,0,THING_Triangle_Down);
			dp->DrawReal();
			dp->LineWidthScreen(1);

		} else if (area->outline && area->visible) {
			 //catch all for remaining areas
			dp->LineWidthScreen(1);
			dv.x=dv.y=0;
			if (area->category==2) { //page specific
				dv.x = (OnBack() ? pp->insetleft : pp->insetright)  +activetilex*(s->PatternWidth() +pp->tilegapx) + finalc*s->PageWidth(0);
				dv.y = pp->insetbottom+activetiley*(s->PatternHeight()+pp->tilegapy) + finalr*s->PageHeight(0);
			}
			drawHandle(area,dv);
		}
	}
	dp->DrawReal();

	if (showsplash) {
		dp->DrawScreen();
		dp->NewFG(0,0,0);
		char scratch[200];
		sprintf(scratch,_("Laidout %s\n(impose only)\nBy Tom Lechner"),LAIDOUT_VERSION);
		dp->textout(viewport->win_w/2,viewport->win_h/2,
				scratch, 
				-1,LAX_CENTER);
		dp->DrawReal();
	}

	DBG if (ddp && ddp->GetCairo()) cerr <<" Siginterf refresh end, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	return 0;
}

void SignatureInterface::drawAdd(double x,double y,double r, int fill)
{
	dp->LineWidthScreen(1);
	r*=2/3.;
	//r*=dp->Getmag();
	if (fill) {
		dp->NewFG(.9,.9,.9);
		dp->drawellipse(x,y, r*1.5,r*1.5, 0,0, 1);
	}
	dp->NewFG(color_text);
	dp->drawellipse(x,y, r*1.5,r*1.5, 0,0, 0);
	
	dp->drawline(x,y-r/2, x,y+r/2);
	dp->drawline(x-r/2,y, x+r/2,y);
}

/*! Draw the stack connection info below the paper sheet.
 */
void SignatureInterface::drawStacks()
{
	double w,h;
	GetDimensions(w,h);
	double textheight=dp->textheight()/dp->Getmag();
	double yoff=3*INDICATOR_SIZE/dp->Getmag();
	double sh=3*textheight;
	double a=textheight/4;
	double blockh=sh;
	double blockw=sh+textheight+dp->textextent("00 sheet",-1,NULL,NULL)/dp->Getmag();
	char scratch[50];
	
	int n=sigimp->NumStacks(-1);
	double totalw=blockw*n + blockh*(n-1)/2;
	double x,y;

	flatpoint fp;
	flatpoint pts[8];
	pts[0]=flatpoint(a,-blockh);
	pts[1]=flatpoint(0,-blockh+a);
	pts[2]=flatpoint(0,-a);
	pts[3]=flatpoint(a,0);
	pts[4]=flatpoint(blockw-a,0);
	pts[5]=flatpoint(blockw,-a);
	pts[6]=flatpoint(blockw,-blockh+a);
	pts[7]=flatpoint(blockw-a,-blockh);

	dp->NewFG(0,0,0);

	 //add new first stack
	y=-yoff-blockh/2;
	x=w/2-totalw/2-blockh/2-textheight*.75;
	drawAdd(x,y, textheight*.75, overoverlay==SP_New_First_Stack);
	x+=textheight*.75;
	dp->drawarrow(flatpoint(x,y),flatpoint(blockh/2,0), 0, 1,2,3);

	 //draw stacks
	x=w/2-totalw/2;
	int si=0, ii;
	double off=0;
	dp->LineWidthScreen(1);

	for (SignatureInstance *s=sigimp->GetSignature(0,0); s; s=s->next_stack) {
		y=-yoff;
		ii=0;
		for (SignatureInstance *i=s; i; i=i->next_insert) {
			 //outline
			dp->ShiftReal(x,y);
			if (i==siginstance) {
				 //slightly highlight current instance
				dp->NewFG(.83,.83,.83);
				dp->drawlines(pts,8, 1, 1); 
			}
			if (overoverlay==SP_On_Stack && onoverlay_i==si && onoverlay_ii==ii) {
				 //currently moused over..
				dp->NewFG(.9,.9,.9);
				dp->drawlines(pts,8, 1, 1); 
			}
			if (i==siginstance) {
				 //set attributes to draw thick line around current
				dp->LineWidthScreen(2);
				dp->NewFG(0.,.5,0.);
			}
			dp->drawlines(pts,8, i->next_insert && i!=siginstance?0:1, 0);
			dp->ShiftReal(-x,-y);

			dp->LineWidthScreen(1);
			if (onoverlay_i==si && onoverlay_ii==ii && sigimp->TotalNumStacks()>1) {
				 //draw delete thing
				if (overoverlay==SP_Delete_Stack) {
					dp->NewFG(1.,.7,.7);
					dp->drawrectangle(x+blockw-textheight,y-a-textheight, textheight,textheight, 1);
					dp->NewFG(color_text);
					dp->drawline(x+blockw-textheight*.75,y-a-textheight*.25, x+blockw-a-textheight*.25,y-a-textheight*.75);
					dp->drawline(x+blockw-textheight*.75,y-a-textheight*.75, x+blockw-a-textheight*.25,y-a-textheight*.25);

				} else if (overoverlay==SP_On_Stack) {
					dp->NewFG(color_text);
					dp->drawline(x+blockw-textheight*.75,y-a-textheight*.25, x+blockw-a-textheight*.25,y-a-textheight*.75);
					dp->drawline(x+blockw-textheight*.75,y-a-textheight*.75, x+blockw-a-textheight*.25,y-a-textheight*.25);
				}
			}

			dp->NewFG(color_text);
			if (ii==0) dp->drawarrow(flatpoint(x+blockw,y-blockh/2),flatpoint(blockh/2,0), 0, 1,2,3);

			 //paper fold graphic
			dp->NewFG(color_inset);
			off=blockh/2 - (textheight + ((i->sheetspersignature>4?4:i->sheetspersignature)) * textheight/4)/2;
			for (int c=0; c<i->sheetspersignature && c<4; c++) {
				dp->drawline(x+textheight,y-(off+c*textheight/4+textheight), x+1.5*textheight,y-(off+c*textheight/4)); 
				dp->drawline(x+1.5*textheight,y-(off+c*textheight/4), x+2*textheight,y-(off+c*textheight/4+textheight)); 
			}

			 //n sheets/m pages
			dp->NewFG(color_text);
			dp->DrawScreen();
			dp->LineWidthScreen(1);
			double angle=-dp->XAxis().angle();

			sprintf(scratch,"%c%d sheet",(i->autoaddsheets?'*':' '), i->sheetspersignature);
			fp=dp->realtoscreen(x+blockh,y-textheight/2);
			dp->textout(angle, fp.x,fp.y, scratch,-1, LAX_LEFT|LAX_TOP);

			sprintf(scratch,"%d pgs",i->PagesPerSignature(0,1));
			fp=dp->realtoscreen(x+blockh,y-textheight*1.5);
			dp->textout(angle, fp.x,fp.y, scratch,-1, LAX_LEFT|LAX_TOP);
			dp->DrawReal();
			dp->LineWidthScreen(1);

			ii++;
			y-=blockh;
		}
		 //add new insert
		y-=blockh*.5;
		drawAdd(x+textheight*1.5,y, textheight*.75, overoverlay==SP_New_Insert && onoverlay_i==si);
		dp->drawarrow(flatpoint(x+textheight*1.5,y+textheight*.75),flatpoint(0,blockh*.5-textheight*.75), 0, 1,2,3);
		
		si++;
		x+=blockw+blockh/2;
	}

	 //add new last stack
	y=-yoff-blockh/2;
	x+=textheight*.75;
	drawAdd(x,y, textheight*.75, overoverlay==SP_New_Last_Stack);
}

void SignatureInterface::drawHandle(ActionArea *area, flatpoint offset)
{
	if (!area->visible) return;

	dp->NewFG(area->color);
	dp->PushAxes();

	if (!OnBack()) dp->ShiftReal(siginstance->partition->insetright - siginstance->partition->insetleft, 0);

	if (area->real==1) dp->DrawReal(); else dp->DrawScreen();
	dp->LineWidthScreen(1);

	if (area->outline) {
		flatpoint shift=offset+area->offset;

		if (area->real==0) {
			 //screen coordinates
			dp->drawlines(area->outline,area->npoints,1,area->action==overoverlay);

		} else if (area->real==1) {
			 //real coordinates
			dp->ShiftReal(shift.x,shift.y);
			dp->drawlines(area->outline,area->npoints,1,area->action==overoverlay);

		} else {
			 //real position, screen outline
			shift=dp->realtoscreen(shift);
			shift-=area->hotspot;
			dp->moveto(shift + area->outline[0]);
			for (int c=1; c<area->npoints; c++) {
				dp->lineto(shift + area->outline[c]);
			}
			dp->closed();
			if (area->action==overoverlay) dp->fill(0); else dp->stroke(0);
		}


		//if (area->action==overoverlay) *** draw numeric value next to handle
	}

	if (!isblank(area->text)) {
		dp->NewFG(area->color_text);
		flatpoint p=area->Center()+area->offset;
		dp->textout(p.x,p.y, area->text,-1, LAX_CENTER);
	}

	dp->PopAxes();
	dp->DrawReal();
	dp->LineWidthScreen(1);

}

//! Return a screen rectangle containing the specified fold level indicator.
/*! which==-1 means all unfolded. 0 or more means that fold.
 */
void SignatureInterface::getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h)
{
	int radius=INDICATOR_SIZE;

	*x=dp->Minx;
	*y=(dp->Maxy+dp->Miny)/2 - (signature->folds.n+1)*(radius-3);
	*w=2*radius;
	*h=2*radius;

	*y+=(which+1)*(2*radius-3);
}

//! Returns 0 for the circle, totally unfolded, or else fold index+1, or -1 for not found.
int SignatureInterface::scanForFoldIndicator(int x, int y, int ignorex)
{
	int radius=INDICATOR_SIZE;

	if (!ignorex && (x>2*radius || x<0)) return -1;

	double xx,yy,w,h;
	for (int c=signature->folds.n-1; c>=-1; c--) {
		getFoldIndicatorPos(c, &xx,&yy,&w,&h);
		if (x>=xx && x<xx+w && y>=yy && y<yy+h) {
			return c+1;
		}
	}
	return -1;
}

/*! Scan for hovering over the stacks.
 */
int SignatureInterface::scanStacks(int xx,int yy, int *stacki, int *inserti)
{
	flatpoint fp=screentoreal(xx,yy);
	double w,h;
	GetDimensions(w,h);

	double textheight=dp->textheight()/dp->Getmag();
	double yoff=3*INDICATOR_SIZE/dp->Getmag();
	double sh=3*textheight;
	double blockh=sh;
	double blockw=sh+textheight+dp->textextent("00 sheet",-1,NULL,NULL)/dp->Getmag();

	if (fp.y>-yoff) return SP_None;

	
	int n=sigimp->NumStacks(-1);
	double totalw=blockw*n + blockh*(n-1)/2;
	double x,y;

	 //add new first stack
	y=-yoff;
	x=w/2-totalw/2;
	if (fp.x>=x-blockh/2-textheight*1.5 && fp.x<=x-blockh/2 && fp.y<=y && fp.y>=y-blockh) return SP_New_First_Stack;

	 //draw stacks
	x=w/2-totalw/2;
	int si=0, ii;
	for (SignatureInstance *s=sigimp->GetSignature(0,0); s; s=s->next_stack) {
		y=-yoff;
		ii=0;
		for (SignatureInstance *i=s; i; i=i->next_insert) {
			 //outline
			if (fp.x>=x && fp.x<=x+blockw && fp.y<=y && fp.y>=y-blockh) {
				*stacki =si;
				*inserti=ii;

				if (sigimp->TotalNumStacks()>1
						&& fp.x>=x+blockw-textheight*1.25 && fp.x<=x+blockw && fp.y<=y && fp.y>=y-1.25*textheight)
					return SP_Delete_Stack;
				return SP_On_Stack;
			}
			ii++;
			y-=blockh;
		}

		 //add new insert
		y-=blockh*.5;
		if (fp.x>=x+textheight*.25 && fp.x<=x+textheight*2.25 && fp.y<=y+textheight*.75 && fp.y>=y-textheight*.75) {
			*stacki=si;
			*inserti=ii;
			return SP_New_Insert;
		}
		
		si++;
		x+=blockw+blockh/2;
	}

	 //add new last stack
	y=-yoff-blockh/2;
	x+=textheight*.75;
	if (fp.x>=x-textheight*.75 && fp.x<=x+textheight*.75 && fp.y>=y-textheight*.75 && fp.y<=x+textheight*.75) {
		return SP_New_Last_Stack;
	}

	return SP_None;
}

//! Scan for handles to control variables, not for row and column.
/*! Returns which handle screen point is in, or none.
 */
int SignatureInterface::scanHandle(int x,int y, int *i1, int *i2)
{
	flatpoint fp=screentoreal(x,y);
	flatpoint tp, ffp;
	Signature *s=signature;
	PaperPartition *pp=siginstance->partition;
	double totalheight, totalwidth;
	GetDimensions(totalwidth,totalheight);

	if (fp.y<-3*INDICATOR_SIZE/dp->Getmag()) {
		int si=-1, ii=-1;
		int aa=scanStacks(x,y, &si, &ii);
		if (aa!=SP_None) {
			onoverlay_i =si;
			onoverlay_ii=ii;
			return aa;
		}
	}

	double insetleft = OnBack() ? pp->insetleft  : pp->insetright;
	double insetright= OnBack() ? pp->insetright : pp->insetleft;

	for (int c=0; c<controls.n; c++) {
		if (controls.e[c]->hidden) continue;

		//DBG if (controls.e[c]->action!=SP_Trim_Left) continue;

		if (controls.e[c]->category==0) {
			ffp=fp-controls.e[c]->offset;
			ffp.y=-ffp.y;
			ffp*=Getmag();

			 //normal, single instance handles
			if (controls.e[c]->real==0 && controls.e[c]->PointIn(x,y))         return controls.e[c]->action;
			if (controls.e[c]->real==1 && controls.e[c]->PointIn(fp.x,fp.y))   return controls.e[c]->action;
			if (controls.e[c]->real==2 && controls.e[c]->PointIn(ffp.x,ffp.y)) return controls.e[c]->action;

		} else if (controls.e[c]->category==1) {
			 //fold lines, in the pattern area
			for (int x=0; x<siginstance->partition->tilex; x++) {
			  for (int y=0; y<siginstance->partition->tiley; y++) {
				tp=fp-flatpoint(insetleft,pp->insetbottom);
				tp-=flatpoint(x*(pp->tilegapx+s->PatternWidth()), y*(pp->tilegapy+s->PatternHeight()));

				if (controls.e[c]->PointIn(tp.x,tp.y)) return controls.e[c]->action;
			  }
			}

		} else if (controls.e[c]->category==2) {
			 //in a final page area
			double w=s->PageWidth(0);
			double h=s->PageHeight(0);
			tp.x=fp.x-(insetleft  +activetilex*(s->PatternWidth() +pp->tilegapx) + finalc*w);
			tp.y=fp.y-(pp->insetbottom+activetiley*(s->PatternHeight()+pp->tilegapy) + finalr*h);

			ffp=tp-controls.e[c]->offset;
			ffp.y=-ffp.y;
			ffp*=Getmag();
			if (controls.e[c]->real==2) tp=ffp;

			if (controls.e[c]->action==SP_Binding) {
				if (s->binding=='l') {
					if (tp.x>0 && tp.x<w*.1 && tp.y>0 && tp.y<h) return SP_Binding;
				} else if (s->binding=='r') {
					if (tp.x>w*.9 && tp.x<w && tp.y>0 && tp.y<h) return SP_Binding;
				} else if (s->binding=='t') {
					if (tp.x>0 && tp.x<w && tp.y>h*.9 && tp.y<h) return SP_Binding;
				} else if (s->binding=='b') {
					if (tp.x>0 && tp.x<w && tp.y>0 && tp.y<h*.1) return SP_Binding;
				}
			} else if (controls.e[c]->PointIn(tp.x,tp.y)) return controls.e[c]->action;

		} else if (controls.e[c]->category==3) {
			 //tile gaps
			double d;
			double zone=5/Getmag(); //selection zone override for really skinny gaps

			if (controls.e[c]->action==SP_Tile_Gap_X) {
				for (int c=0; c<siginstance->partition->tilex-1; c++) {
					 //first check for inside gap itself
					d=insetleft+(c+1)*(pp->tilegapx+s->PatternWidth())-pp->tilegapx/2;
					if (pp->tilegapx>zone) zone=pp->tilegapx;
					if (fp.y>0 && fp.y<totalheight && fp.x<=d+zone/2 && fp.x>=d-zone/2) return SP_Tile_Gap_X;
				}

			} else { //tile gap y
				for (int c=0; c<siginstance->partition->tiley-1; c++) {
					 //first check for inside gap itself
					d=pp->insetbottom+(c+1)*(pp->tilegapy+s->PatternHeight())-pp->tilegapy/2;
					if (pp->tilegapy>zone) zone=pp->tilegapy;
					if (fp.x>0 && fp.x<totalwidth && fp.y<=d+zone/2 && fp.y>=d-zone/2) return SP_Tile_Gap_Y;
				}
			}
		}
	}

	 //check for gross areas for insets 
	if (fp.x>0 && fp.x<pp->totalwidth) {
		if (fp.y>0 && fp.y<pp->insetbottom) return SP_Inset_Bottom;
		if (fp.y>totalheight-pp->insettop && fp.y<totalheight) return SP_Inset_Top;
	}
	if (fp.y>0 && fp.y<pp->totalheight) {
		if (fp.x>0 && fp.x<insetleft) return SP_Inset_Left;
		if (fp.x>totalwidth-insetright && fp.x<totalwidth) return SP_Inset_Right;
	}


	return SP_None;
}

//! Return 0 for not in pattern, or the row and column in a folding section.
/*! x and y are real coordinates.
 *
 * Returns the real coordinate within an element of the folding pattern in ex,ey.
 */
int SignatureInterface::scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col)
{
	flatpoint fp=screentoreal(x,y);
	DBG cerr <<"fp:"<<fp.x<<','<<fp.y<<endl;

	fp.x-=siginstance->partition->insetleft;
	fp.y-=siginstance->partition->insetbottom;

	double patternheight=siginstance->PatternHeight();
	double patternwidth =siginstance->PatternWidth();
	double elementheight=patternheight/(signature->numhfolds+1);
	double elementwidth =patternwidth /(signature->numvfolds+1);

	int tilex,tiley;
	tilex=floorl(fp.x/(patternwidth +siginstance->partition->tilegapx));
	tiley=floorl(fp.y/(patternheight+siginstance->partition->tilegapy));
	if (tile_col) *tile_col=tilex;
	if (tile_row) *tile_row=tiley;

	fp.x-=tilex*(patternwidth +siginstance->partition->tilegapx);
	fp.y-=tiley*(patternheight+siginstance->partition->tilegapy);

	DBG cerr <<"tilex,y: "<<tilex<<","<<tiley<<endl;

	*row=floorl(fp.y/elementheight);
	*col=floorl(fp.x/elementwidth);

	 //find coordinates within an element cell
	if (ey) *ey=fp.y-(*row)*elementheight;
	if (ex) *ex=fp.x-(*col)*elementwidth;

	return *row>=0 && *row<=signature->numhfolds && *col>=0 && *col<=signature->numvfolds;
}

//! Respond to spinning controls.
int SignatureInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	//flatpoint fp=screentoreal(x,y);

	if (state&LAX_STATE_MASK) return 1;

	int handle=scanHandle(x,y);
	adjustControl(handle,-1);

	return 0; //this will always absorb plain wheel
}

//! Respond to spinning controls.
int SignatureInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	//flatpoint fp=screentoreal(x,y);

	if (state&LAX_STATE_MASK) return 1;

	int handle=scanHandle(x,y);
	adjustControl(handle,1);

	return 0; //this will always absorb plain wheel
}

//! For controls that are wheel accessible. dir can be 1 or -1.
/*! Return 0 for control adjusted, or 1 for not able to do that to that control.
 */
int SignatureInterface::adjustControl(int handle, int dir)
{
	DBG cerr <<"adjustControl "<<handle<<" dir:"<<dir<<endl;

	if (handle==SP_None) return 1;

	if (handle==SP_On_Stack) {
		SignatureInstance *i=siginstance;
		if (handle==SP_On_Stack) i=sigimp->GetSignature(onoverlay_i,onoverlay_ii);
		if (i) {
			if (dir==-1) {
				if (i->autoaddsheets==0) {
					i->sheetspersignature--;
					if (i->sheetspersignature<=0) {
						i->autoaddsheets=1;
						i->sheetspersignature=1;
					}
					sigimp->NumPages(sigimp->numdocpages);
					remapHandles(1);
					needtodraw=1;
				}
				return 0;

			} else {
				if (i->autoaddsheets==0) i->sheetspersignature++;
				i->autoaddsheets=0;
				sigimp->NumPages(sigimp->numdocpages);
				remapHandles(1);
				needtodraw=1;
				return 0;
			}
		}
		return 0;

	} else if (handle==SP_Tile_X_top || handle==SP_Tile_X_bottom) {
		if (dir==-1) {
			siginstance->partition->tilex--;
			if (siginstance->partition->tilex<=1) siginstance->partition->tilex=1;
		} else {
			siginstance->partition->tilex++;
		}
		signature->patternwidth =siginstance->PatternWidth();//update hint in Signature
		signature->patternheight=siginstance->PatternHeight();
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (handle==SP_Tile_Y_left || handle==SP_Tile_Y_right) {
		if (dir==-1) {
			siginstance->partition->tiley--;
			if (siginstance->partition->tiley<=1) siginstance->partition->tiley=1;
		} else {
			siginstance->partition->tiley++;
		}
		signature->patternwidth =siginstance->PatternWidth();//update hint in Signature
		signature->patternheight=siginstance->PatternHeight();
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (handle==SP_H_Folds_left || handle==SP_H_Folds_right) {
		if (dir==1) {
			if (foldlevel!=0) return 0;
			signature->numhfolds++;
			reallocateFoldinfo();
			checkFoldLevel(1);

		} else {
			if (foldlevel!=0) return 0;
			int old=signature->numhfolds;
			signature->numhfolds--;
			if (signature->numhfolds<=0) signature->numhfolds=0;
			if (old!=signature->numhfolds) {
				reallocateFoldinfo();
				checkFoldLevel(1);
				needtodraw=1;
			}
		}
		sigimp->NumPages(sigimp->numdocpages);
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (handle==SP_V_Folds_top || handle==SP_V_Folds_bottom) {
		if (dir==1) {
			if (foldlevel!=0) return 0;
			signature->numvfolds++;
			reallocateFoldinfo();
			checkFoldLevel(1);

		} else {
			if (foldlevel!=0) return 0;
			int old=signature->numvfolds;
			signature->numvfolds--;
			if (signature->numvfolds<=0) signature->numvfolds=0;
			if (old!=signature->numvfolds) {
				reallocateFoldinfo();
				checkFoldLevel(1);
				needtodraw=1;
			}
		}
		sigimp->NumPages(sigimp->numdocpages);
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (handle==SP_Automarks) {
		if (dir==1) {
			if (siginstance->automarks==0) siginstance->automarks=AUTOMARK_Margins;
			else if (siginstance->automarks==AUTOMARK_Margins) siginstance->automarks=AUTOMARK_InnerDot;
			else if (siginstance->automarks==AUTOMARK_InnerDot) siginstance->automarks=AUTOMARK_InnerDot|AUTOMARK_Margins;
			else siginstance->automarks=0;
		} else {
			if (siginstance->automarks==0) siginstance->automarks=AUTOMARK_InnerDot|AUTOMARK_Margins;
			else if (siginstance->automarks==(AUTOMARK_InnerDot|AUTOMARK_Margins)) siginstance->automarks=AUTOMARK_InnerDot;
			else if (siginstance->automarks==AUTOMARK_InnerDot) siginstance->automarks=AUTOMARK_Margins;
			else siginstance->automarks=0;
		}
		ActionArea *area = control(SP_Automarks);
		if (siginstance->automarks==0) makestr(area->text,"No automarks");
		else if (siginstance->automarks==AUTOMARK_Margins) makestr(area->text,"Automarks outside");
		else if (siginstance->automarks==AUTOMARK_InnerDot) makestr(area->text,"Automarks inside");
		else if (siginstance->automarks==(AUTOMARK_InnerDot|AUTOMARK_Margins)) makestr(area->text,"Automarks inside and outside");
		remapHandles(1);
		needtodraw=1;
		return 0;

	} else if (handle==SP_Num_Pages) {
		if (dir==1) {
			int dp=sigimp->numdocpages;
			int oldpages=sigimp->NumPages();
			int npages=oldpages;
			while (npages==oldpages) {
				dp++;
				npages=sigimp->NumPages(dp);
			}
		} else {
			int dp=sigimp->numdocpages;
			int oldpages=sigimp->NumPages();
			int npages=oldpages;
			while (dp>2 && npages==oldpages) {
				dp--;
				npages=sigimp->NumPages(dp);
			}
		}

		ShowThisPaperSpread(currentPaperSpread);
		remapHandles(1);

		needtodraw=1;
		return 0;

	} else if (handle==SP_Current_Sheet) {
        currentPaperSpread+=(dir>0?1:-1);
        if (currentPaperSpread>=sigimp->NumPapers()) currentPaperSpread=0;
        else if (currentPaperSpread<0) currentPaperSpread=sigimp->NumPapers()-1;

        if (foldlevel!=0) {
            signature->resetFoldinfo(NULL);
            foldlevel=0;
        }

		 //locate corresponding paper, update siginstance,
		ShowThisPaperSpread(currentPaperSpread);

        needtodraw=1;
        return 0;

   } else if (handle==SP_Paper_Name) {
		PaperStyle *paper=siginstance->partition->paper;
		int i=-1;
		for (i=0; i<laidout->papersizes.n-2; i++) {
			if (!strcasecmp(paper->name,laidout->papersizes.e[i]->name))
				break;
		}
		if (dir>0) i++; else i--;
		if (i<0) i=laidout->papersizes.n-3;
		else if (i>laidout->papersizes.n-3) i=0;
		int landscape=paper->landscape();

		paper=(PaperStyle*)laidout->papersizes.e[i]->duplicate();
		paper->landscape(landscape);
		SetPaper(paper);
		paper->dec_count();
		remapHandles();
		PerformAction(SIA_Center);
		needtodraw=1;

	} else if (handle==SP_Paper_Orient) {
		PaperStyle *paper=siginstance->partition->paper;
		if (paper) {
			paper->landscape(!paper->landscape());
			siginstance->SetPaper(paper,1);
			remapHandles();
			PerformAction(SIA_Center);
			needtodraw=1;
		}
		return 0;

	}


	return 1;
}

int SignatureInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	int row,col,tilerow,tilecol;
	DBG int over=
	scan(x,y, &row,&col, NULL,NULL, &tilerow,&tilecol);
	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<endl;

	if (buttondown.any()) return 0;

	buttondown.down(d->id,LEFTBUTTON, x,y, row,col);

	onoverlay=SP_None;

	 //check overlays first
	int i=scanForFoldIndicator(x,y,0);
	if (i>=0) {
		onoverlay=SP_FOLDS+i;
		buttondown.moveinfo(d->id,LEFTBUTTON, i,0); //record which indicator is clicked down in
		foldprogress=-1;
		return 0;
	}
	onoverlay=scanHandle(x,y);
	if (onoverlay!=SP_None) {
		ActionArea *a=control(onoverlay);
		if (!a) return 0;
		if (a->type!=AREA_Display_Only) return 0;
		onoverlay=SP_None;
	}

	 //check for on something to fold
	if (row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds
		  || foldinfo[row][col].pages.n==0
		  || tilerow<0 || tilecol<0
		  || tilerow>siginstance->partition->tiley || tilecol>siginstance->partition->tilex) {
		lbdown_row=lbdown_col=-1;
	} else {
		if (tilerow>=0 && tilerow<siginstance->partition->tiley && tilerow!=activetiley) { needtodraw=1; activetiley=tilerow; }
		if (tilecol>=0 && tilecol<siginstance->partition->tilex && tilecol!=activetilex) { needtodraw=1; activetilex=tilecol; }

		lbdown_row=row;
		lbdown_col=col;
		folddirection=0;
		foldprogress=0;
	}


	return 0;
}

/*! Figure out first paper spread for this instance,
 * then call ShowThisPaperSpread(that_spread).
 */
void SignatureInterface::SetPaperFromInstance(SignatureInstance *sig)
{
	int i=0;
	while (sig->prev_insert) {
		sig=sig->prev_insert;
		i+=sig->PaperSpreadsPerSignature(0,1);
	}
	while (sig->prev_stack) {
		sig=sig->prev_stack;
		i+=sig->PaperSpreadsPerSignature(0,0);
	}
	ShowThisPaperSpread(i);
}

int SignatureInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!(buttondown.isdown(d->id,LEFTBUTTON))) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);


	if (onoverlay) {
		int curhandle=scanHandle(x,y);

		if (!dragged && onoverlay==SP_On_Stack) {
			SignatureInstance *sig=sigimp->GetSignature(onoverlay_i,onoverlay_ii);
			SetPaperFromInstance(sig);
			needtodraw=1;

		} else if (!dragged && onoverlay==SP_New_Insert) {
			//DBG cerr <<" New_Insert i,ii:"<<onoverlay_i<<" "<<onoverlay_ii<<endl;

			SignatureInstance *s=sigimp->GetSignature(onoverlay_i,-1);
			if (!s) return 0;
			sigimp->AddStack(onoverlay_i,-1, NULL);
			onoverlay=SP_None;
			onoverlay_i=onoverlay_ii=-1;
			SetPaperFromInstance(siginstance);
			needtodraw=1;			

		} else if (!dragged && onoverlay==SP_Delete_Stack) {
			SignatureInstance *s=sigimp->GetSignature(onoverlay_i,onoverlay_ii);
			if (s==siginstance) siginstance=NULL;
			sigimp->RemoveStack(onoverlay_i,onoverlay_ii);
			ShowThisPaperSpread(currentPaperSpread);
			remapHandles();
			needtodraw=1;

		} else if (!dragged && onoverlay==SP_New_First_Stack) {
			sigimp->AddStack(-1,0, NULL);
			onoverlay=SP_None;
			onoverlay_i=onoverlay_ii=-1;
			SetPaperFromInstance(siginstance);
			needtodraw=1;			

		} else if (!dragged && onoverlay==SP_New_Last_Stack) {
			sigimp->AddStack(-2,0, NULL);
			onoverlay=SP_None;
			onoverlay_i=onoverlay_ii=-1;
			SetPaperFromInstance(siginstance);
			needtodraw=1;			

		} else if (onoverlay == curhandle && onoverlay == SP_Paper_Width) {
			 //create input edit
			ActionArea *area = control(SP_Paper_Width);
			NewLengthInputWindow(_("Paper Width"), area, "paperwidth",siginstance->partition->paper->w());
			return 0;

		} else if (onoverlay == curhandle && onoverlay == SP_Paper_Height) {
			 //create input edit
			ActionArea *area = control(SP_Paper_Height);
			NewLengthInputWindow(_("Paper Height"), area, "paperheight",siginstance->partition->paper->h());
			return 0;

		} else if (onoverlay == curhandle && onoverlay == SP_Paper_Name) {
			PaperStyle *paper = siginstance->partition->paper;
			PaperSizeWindow *psizewindow = new PaperSizeWindow(nullptr, "psizewindow", nullptr, 0, object_id, "papersize", 
										paper, false, false, false, false);
			app->rundialog(psizewindow);
			return 0;

		} else if (onoverlay>=SP_FOLDS) {
			 //selecting different fold maybe...
			if (!dragged) {
				 //we clicked down then up on the same overlay

				int folds=onoverlay-SP_FOLDS; //0 means totally unfolded

				if (foldlevel==folds) return 0; //already at that fold level

				 //we must remap the folds to reflect the new fold level
				signature->resetFoldinfo(NULL);
				for (int c=0; c<folds; c++) {
					applyFold(signature->folds.e[c]);
				}
				foldlevel=folds;
				//checkFoldLevel(1);
			}

			folddirection=0;
			remapHandles();
			needtodraw=1;

		} else if (!dragged) {
			ActionArea *area = control(onoverlay);
			if (area->visible && (area->type == AREA_H_Slider || area->type == AREA_Slider)) {
				flatpoint d = flatpoint(x,y) - area->Center();
				if (d.x>0) adjustControl(onoverlay,1);
				else if (d.x < 0) adjustControl(onoverlay,-1);

			} else {
				// *** check for tap on controls
			}
		
		}
		
		return 0;
	}

	if (folddirection && folddirection!='x' && foldprogress>.9) {
		 //apply the fold, and add to the signature...

		applyFold(folddirection,foldindex,foldunder);

		if (foldlevel<signature->folds.n) {
			 //we have tried to fold when there are already further folds, so we must remove any
			 //after the current foldlevel.
			while (foldlevel<signature->folds.n) signature->folds.remove(-1);
			hasfinal=0;
		}

		signature->folds.push(new Fold(folddirection,foldunder,foldindex),1);
		foldlevel=signature->folds.n;

		checkFoldLevel(1);
		if (hasfinal) remapHandles(0);

		folddirection=0;
		foldprogress=0;
	}

	needtodraw=1;
	return 0;
}

/*! Return 0 for offsetted, or 1 for could not.
 * Just calls offsetHandle(control(which),d).
 */
int SignatureInterface::offsetHandle(int which, flatpoint d)
{ 
	return offsetHandle(control(which),d);
}

/*! Return 0 for offsetted, or 1 for could not.
 */
int SignatureInterface::offsetHandle(ActionArea *area, flatpoint d)
{ 
	if (!area) return 1;
	int which=area->action;

	if (area->type==AREA_Handle) {
		area->offset+=d;
		Signature *s=signature;
		PaperPartition *pp=siginstance->partition;
		char scratch[100];
		
		double totalheight, totalwidth;
		GetDimensions(totalwidth,totalheight);

		if (which==SP_Inset_Top) {
			 //adjust signature
			siginstance->partition->insettop-=d.y;
			if (siginstance->partition->insettop<0) {
				siginstance->partition->insettop=0;
			} else if (pp->insettop>totalheight - ((pp->tiley-1)*pp->tilegapy+pp->insetbottom)) {
				pp->insettop= totalheight - ((pp->tiley-1)*pp->tilegapy+pp->insetbottom);
			}
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			 //adjust handle to match signature
			d=area->Position();
			if (d.x<0) area->Position(0,0,1);
			else if (d.x>totalwidth) area->Position(totalwidth,0,1);
			area->Position(0,totalheight-siginstance->partition->insettop,2);

			sprintf(scratch,_("Top Inset")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->insettop);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Inset_Bottom  ) {
			 //adjust signature
			siginstance->partition->insetbottom+=d.y;
			if (siginstance->partition->insetbottom<0) {
				siginstance->partition->insetbottom=0;
			} else if (pp->insetbottom>totalheight - ((pp->tiley-1)*pp->tilegapy+pp->insettop)) {
				pp->insetbottom= totalheight - ((pp->tiley-1)*pp->tilegapy+pp->insettop);
			}
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			 //adjust handle to match signature
			d=area->Position();
			if (d.x<0) area->Position(0,0,1);
			else if (d.x>totalwidth) area->Position(totalwidth,0,1);
			area->Position(0,siginstance->partition->insetbottom,2);

			sprintf(scratch,_("Bottom Inset")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->insetbottom);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Inset_Left    ) {
			siginstance->partition->insetleft+=d.x;
			if (siginstance->partition->insetleft<0) {
				siginstance->partition->insetleft=0;
			} else if (pp->insetleft>totalwidth - ((pp->tilex-1)*pp->tilegapx+pp->insetright)) {
				pp->insetleft= totalwidth - ((pp->tilex-1)*pp->tilegapx+pp->insetright);
			}
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			 //adjust handle to match signature
			d=area->Position();
			if (d.y<0) area->Position(0,0,2);
			else if (d.y>totalheight) area->Position(0,totalheight,2);
			area->Position(siginstance->partition->insetleft,0,1);

			sprintf(scratch,_("Left Inset")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->insetleft);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Inset_Right   ) {
			siginstance->partition->insetright-=d.x;
			if (siginstance->partition->insetright<0) {
				siginstance->partition->insetright=0;
			} else if (pp->insetright>totalwidth - ((pp->tilex-1)*pp->tilegapx+pp->insetleft)) {
				pp->insetright= totalwidth - ((pp->tilex-1)*pp->tilegapx+pp->insetleft);
			}
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			 //adjust handle to match signature
			d=area->Position();
			if (d.y<0) area->Position(0,0,2);
			else if (d.y>totalheight) area->Position(0,totalheight,2);
			area->Position(totalwidth-siginstance->partition->insetright,0,1);

			sprintf(scratch,_("Right Inset")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->insetright);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Tile_Gap_X    ) {
			pp->tilegapx+=d.x;

			if ((pp->tilex-1)*pp->tilegapx > totalwidth-pp->insetleft-pp->insetright+pp->tilex*s->PatternWidth()) {
				if (pp->tilex==1) pp->tilegapx=0;
				else pp->tilegapx=(totalwidth-pp->insetleft-pp->insetright+pp->tilex*s->PatternWidth())/(pp->tilex-1);
			} else if (pp->tilegapx<0) pp->tilegapx=0;
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			sprintf(scratch,_("Tile gap")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->tilegapx);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Tile_Gap_Y    ) {
			pp->tilegapy+=d.y;

			if ((pp->tiley-1)*pp->tilegapy > totalheight-pp->insettop-pp->insetbottom+pp->tiley*s->PatternHeight()) {
				if (pp->tiley==1) pp->tilegapy=0;
				else pp->tilegapy=(totalheight-pp->insettop-pp->insetbottom+pp->tiley*s->PatternHeight())/(pp->tiley-1);
			} else if (pp->tilegapy<0) pp->tilegapy=0;
			s->patternwidth =pp->PatternWidth();//update hint in Signature
			s->patternheight=pp->PatternHeight();

			sprintf(scratch,_("Tile gap")); //to make fewer things to translate
			sprintf(scratch+strlen(scratch),_(" %.10g"),pp->tilegapy);
			PostMessage(scratch);

			remapHandles(2|4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Trim_Top      ) {
			s->trimtop-=d.y;

			double h=s->PageHeight(0);
			if (s->trimtop<0) s->trimtop=0;
			else if (s->trimtop>h-s->trimbottom) s->trimtop=h-s->trimbottom;

			sprintf(scratch,_("Top Trim"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->trimtop);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Trim_Bottom   ) {
			s->trimbottom+=d.y;

			double h=s->PageHeight(0);
			if (s->trimbottom<0) s->trimbottom=0;
			else if (s->trimbottom>h-s->trimtop) s->trimbottom=h-s->trimtop;

			sprintf(scratch,_("Bottom Trim"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->trimbottom);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Trim_Left     ) {
			s->trimleft+=d.x;

			double w=s->PageWidth(0);
			if (s->trimleft<0) s->trimleft=0;
			else if (s->trimleft>w-s->trimright) s->trimleft=w-s->trimright;

			sprintf(scratch,_("Left Trim"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->trimleft);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Trim_Right    ) {
			s->trimright-=d.x;

			double w=s->PageWidth(0);
			if (s->trimright<0) s->trimright=0;
			else if (s->trimright>w-s->trimleft) s->trimright=w-s->trimleft;

			sprintf(scratch,_("Right Trim"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->trimright);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Margin_Top      ) {
			s->margintop-=d.y;

			double h=s->PageHeight(0);
			if (s->margintop<0) s->margintop=0;
			else if (s->margintop>h-s->marginbottom) s->margintop=h-s->marginbottom;

			sprintf(scratch,_("Top Margin"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->margintop);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Margin_Bottom   ) {
			s->marginbottom+=d.y;

			double h=s->PageHeight(0);
			if (s->marginbottom<0) s->marginbottom=0;
			else if (s->marginbottom>h-s->margintop) s->marginbottom=h-s->margintop;

			sprintf(scratch,_("Bottom Margin"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->marginbottom);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Margin_Left     ) {
			s->marginleft+=d.x;

			double w=s->PageWidth(0);
			if (s->marginleft<0) s->marginleft=0;
			else if (s->marginleft>w-s->marginright) s->marginleft=w-s->marginright;

			sprintf(scratch,_("Left Margin"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->marginleft);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Margin_Right    ) {
			s->marginright-=d.x;

			double w=s->PageWidth(0);
			if (s->marginright<0) s->marginright=0;
			else if (s->marginright>w-s->marginleft) s->marginright=w-s->marginleft;

			sprintf(scratch,_("Right Margin"));
			sprintf(scratch+strlen(scratch),_(" %.10g"),s->marginright);
			PostMessage(scratch);

			remapHandles(4);
			needtodraw=1;
			return 0;

		} else if (which==SP_Binding       ) {
		}

	} else if (area->type==AREA_H_Slider || area->type==AREA_Slider) {
		if (d.x>0) adjustControl(which,1);
		else if (d.x<0) adjustControl(which,-1);
		needtodraw=1;
		return 1;

    } else if (area->type==AREA_V_Slider) {
        if (d.y>0) adjustControl(which,-1);
        else if (d.y<0) adjustControl(which,1);
        needtodraw=1;
        return 1;


	}


	
	return 1;
}

int SignatureInterface::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }
	return anInterface::RBDown(x,y,state,count,d);
}

int SignatureInterface::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	if (overoverlay==SP_On_Stack) {
		SignatureInstance *s=sigimp->GetSignature(onoverlay_i,onoverlay_ii);
		if (s!=siginstance) SetPaperFromInstance(s);
	}	

	return anInterface::RBDown(x,y,state,count,d);
}

int SignatureInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int row,col,tilerow,tilecol;
	flatpoint mm;
	lasthover.set(x,y);
	//fp now holds coordinates relative to the element cell

	DBG int over=
	scan(x,y, &row,&col, &mm.x,&mm.y, &tilerow,&tilecol);
	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<"  mm="<<mm.x<<','<<mm.y<<"  tile r,c:"<<tilerow<<','<<tilecol;
	DBG if (row>=0 && row<signature->numhfolds+1 && col>=0 && col<signature->numvfolds+1)
	DBG    cerr <<"  xflip: "<<foldinfo[row][col].x_flipped<<"  yflip:"<<foldinfo[row][col].y_flipped
	DBG         <<"  pages:"<<foldinfo[row][col].pages.n<<endl;

	DBG int stacki=-1, inserti=-1;
	DBG int ostack=scanStacks(x,y, &stacki,&inserti);
	DBG cerr <<"over stack:"<<ostack<<"  stacki:"<<stacki<<"  inserti:"<<inserti<<endl;


	int mx,my;
	int lx,ly;
	if (!buttondown.any()) {
		int i1=onoverlay_i;
		int i2=onoverlay_ii;
		int handle=scanHandle(x,y);
		DBG cerr <<"found handle "<<handle<<endl;
		if (overoverlay!=handle) needtodraw=1;
		if (handle==SP_H_Folds_left || handle==SP_H_Folds_right || handle==SP_Tile_Y_left || handle==SP_Tile_Y_right
				|| handle==SP_V_Folds_top || handle==SP_V_Folds_bottom || handle==SP_Tile_X_top || handle==SP_Tile_X_bottom) 
			needtodraw=1;
		if (handle==SP_H_Folds_left || handle==SP_Tile_Y_right) lasthover.x+=INDICATOR_SIZE;
		if (handle==SP_H_Folds_right || handle==SP_Tile_Y_left) lasthover.x-=INDICATOR_SIZE;
		if (handle==SP_V_Folds_top || handle==SP_Tile_X_bottom) lasthover.y+=INDICATOR_SIZE;
		if (handle==SP_V_Folds_bottom || handle==SP_Tile_X_top) lasthover.y-=INDICATOR_SIZE;

		if (handle==SP_On_Stack && (i1!=onoverlay_i || i2!=onoverlay_ii)) needtodraw=1;
		if (handle!=SP_None) {
			overoverlay=handle;
			ActionArea *a=control(handle);
			PostMessage(a?a->tip:"TOOLTIP NEEDED!!!!!!!!");
			return 0;
		}
		if (handle==SP_None) {
			overoverlay=0;
			PostMessage(" ");
		}
		return 0;
	}
	buttondown.move(mouse->id,x,y, &lx,&ly);

	if (onoverlay!=SP_None) {
		if (onoverlay<SP_FOLDS) {
			 //dragging a handle
			flatpoint d=dp->screentoreal(x,y)-dp->screentoreal(lx,ly);
			if ((state&LAX_STATE_MASK)==ShiftMask) d*=.1;
			if ((state&LAX_STATE_MASK)==ControlMask) d*=.01;
			if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) d*=.001;

			if (onoverlay==SP_Binding) {
				d=dp->screentoreal(x,y);
				Signature *s=signature;
				PaperPartition *pp=siginstance->partition;

				d.x-=pp->insetleft  +activetilex*(s->PatternWidth() +pp->tilegapx) + (finalc+.5)*s->PageWidth(0);
				d.y-=pp->insetbottom+activetiley*(s->PatternHeight()+pp->tilegapy) + (finalr+.5)*s->PageHeight(0);
				d.x/=s->PageWidth(0);
				d.y/=s->PageHeight(0);
				if (d.y>d.x) {
					if (d.y>-d.x) { if (s->binding!='t') needtodraw=1; s->binding='t'; }
					else  { if (s->binding!='l') needtodraw=1; s->binding='l'; }
				} else {
					if (d.y>-d.x) { if (s->binding!='r') needtodraw=1; s->binding='r'; }
					else  { if (s->binding!='b') needtodraw=1; s->binding='b'; }
				}
			} else {
				ActionArea *area=control(onoverlay);
				if (area->type==AREA_Slider || area->type==AREA_H_Slider || area->type==AREA_V_Slider) {
					double slidestep=20.;
					int initialmousex, initialmousey;
					buttondown.getinitial(mouse->id, LEFTBUTTON, &initialmousex, &initialmousey);

					int oldpos, newpos;
					if (area->type==AREA_V_Slider) {
						oldpos=floor((ly-initialmousey)/slidestep);
						newpos=floor(( y-initialmousey)/slidestep);
						d.x=0;  d.y=newpos-oldpos;
					} else {
						oldpos=floor((lx-initialmousex)/slidestep);
						newpos=floor(( x-initialmousex)/slidestep);
						d.x=newpos-oldpos;  d.y=0;
					}
				}
				offsetHandle(area,d);
			}
			return 0;

		} else if (onoverlay>=SP_FOLDS) {
			 //drag on the fold stack indicator
			if (ly-y==0) return 0; //return if not moved vertically

			int startindicator,lasty;
			buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);
			buttondown.getlast(mouse->id,LEFTBUTTON, NULL,&lasty);
			buttondown.getextrainfo(mouse->id,LEFTBUTTON, &startindicator,NULL);
			double curdist =(    y-my)/(2.*INDICATOR_SIZE-3) + startindicator;
			double lastdist=(lasty-my)/(2.*INDICATOR_SIZE-3) + startindicator;

			if (curdist<0) curdist=0;
			else if (curdist>signature->folds.n) curdist=signature->folds.n;
			if (lastdist<0) lastdist=0;
			else if (lastdist>signature->folds.n) lastdist=signature->folds.n;

			 //curdist is 0 for very start, totally unfolded paper. Each fold adds 1 to curdist

			DBG cerr <<"curdist:"<<curdist<<"  lastdist:"<<lastdist<<endl;

			if (foldprogress==-1) {
				 //we have not moved before, so we must do an initial map of affected cells
				remapAffectedCells((int)curdist);
			}
			foldprogress=curdist-floor(curdist);

			if ((int)curdist==(int)lastdist || (lastdist<0 && curdist<0) 
					|| (lastdist>=signature->folds.n && curdist>=signature->folds.n)) {
				 //only change foldprogress, still in same fold indicator
				foldprogress=curdist-floor(curdist);
				if (foldprogress<0) foldprogress=0;
				if (foldprogress>1) foldprogress=1;

				int f=(int)curdist;
				if (f<0) f=0; else if (f>=signature->folds.n) f=signature->folds.n-1;
				folddirection=signature->folds.e[f]->direction;
				foldunder=signature->folds.e[f]->under;
				foldindex=signature->folds.e[f]->whichfold;

				needtodraw=1;
				return 0;

			} else if ((int)curdist<(int)lastdist) {
				 //need to unfold a previous fold, for this, we need to get the map of state not including
				 //the current fold, to know which cells are actually affected by the unfolding..
				foldlevel=curdist;
				signature->applyFold(NULL,(int)curdist);
				remapAffectedCells((int)curdist);
				foldprogress=curdist-floor(curdist);
				remapHandles();
				needtodraw=1;
				return 0;

			} else {
				 //need to advance one fold
				if (curdist>signature->folds.n) curdist=signature->folds.n;
				foldlevel=curdist;
				folddirection='x';
				applyFold(signature->folds.e[(int)curdist-1]);
				remapAffectedCells((int)curdist);
				foldprogress=curdist-floor(curdist);
				remapHandles();
				needtodraw=1;
				return 0;
			}
		}
		return 0;
	}


	if (lbdown_row<0 || lbdown_col<0) return 0;
	if ((hasfinal && foldlevel==signature->folds.n)
			|| row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds) {
		if (folddirection!=0) {
			folddirection=0;
			needtodraw=1;
		}
		return 0;
	}
	foldunder=(state&LAX_STATE_MASK)!=0;

	buttondown.getinitial(mouse->id,LEFTBUTTON, &mx,&my);
	int ocol,orow, otrow,otcol;
	flatpoint om;
	scan(mx,my, &orow,&ocol, &om.x,&om.y, &otrow,&otcol);

	if (tilerow<otrow) row-=100;
	else if (tilerow>otrow) row+=100;
	if (tilecol<otcol) col-=100;
	else if (tilecol>otcol) col+=100;
	

	flatpoint d=screentoreal(x,y)-screentoreal(mx,my); 

	 //find the direction we are trying to fold in
	if (folddirection==0) {
		if (fabs(d.x)>fabs(d.y)) {
			if (d.x>0) {
				folddirection='r';
			} else if (d.x<0) {
				folddirection='l';
			}
		} else {
			if (d.y>0) {
				folddirection='t';
			} else if (d.y<0) {
				folddirection='b';
			}
		}
	}

	 //find how far we fold based on proximity of mouse to fold crease

	 //figure out which elements are affected by folding
	double elementwidth =signature->PageWidth();
	double elementheight=signature->PageHeight();
	if (folddirection=='r') {
		int adjacentcol=ocol+1; //edge is between ocol and adjacentcol
		int prevcol=ocol;
		while (prevcol>0 && foldinfo[orow][prevcol-1].pages.n!=0) prevcol--;
		int nextcol=adjacentcol;
		while (nextcol<signature->numvfolds) nextcol++; //it's ok to fold over onto blank areas

		if (nextcol>signature->numvfolds || nextcol-adjacentcol+1<ocol-prevcol+1
				|| (adjacentcol<=signature->numvfolds && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol+1;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-(elementwidth-mm.x)/(elementwidth-om.x)/2;
			else if (col==ocol+1) foldprogress=.5+(mm.x)/(elementwidth-om.x)/2;
			else if (col<ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=prevcol;
			foldc2=ocol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='l') {
		int adjacentcol=ocol-1; //edge is between ocol and adjacentcol
		int nextcol=ocol;
		while (nextcol<signature->numvfolds && foldinfo[orow][nextcol+1].pages.n!=0) nextcol++;
		int prevcol=adjacentcol;
		while (prevcol>0) prevcol--; //it's ok to fold over onto blank areas

		if (prevcol<0 || adjacentcol-prevcol+1<nextcol-ocol+1
				|| (adjacentcol>=0 && foldinfo[orow][adjacentcol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=ocol;

			 //find the fold progress
			if (col==ocol) foldprogress=.5-mm.x/om.x/2;
			else if (col==ocol-1) foldprogress=.5+(elementwidth-mm.x)/om.x/2;
			else if (col>ocol) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldc1=ocol;
			foldc2=nextcol;
			foldr1=orow;
			foldr2=orow;
			while (foldr1>0 && foldinfo[foldr1-1][ocol].pages.n!=0) foldr1--;
			while (foldr2<signature->numhfolds && foldinfo[foldr2+1][ocol].pages.n!=0) foldr2++;
		}
		needtodraw=1;

	} else if (folddirection=='t') {
		int adjacentrow=orow+1; //edge is between ocol and adjacentcol
		int prevrow=orow;
		while (prevrow>0 && foldinfo[prevrow-1][ocol].pages.n!=0) prevrow--;
		int nextrow=adjacentrow;
		while (nextrow<signature->numhfolds) nextrow++; //it's ok to fold over onto blank areas

		if (nextrow>signature->numhfolds || nextrow-adjacentrow+1<orow-prevrow+1
				|| (adjacentrow<=signature->numhfolds && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow+1;

			 //find the fold progress
			if (row==orow) foldprogress=.5-(elementheight-mm.y)/(elementheight-om.y)/2;
			else if (row==orow+1) foldprogress=.5+(mm.y)/(elementheight-om.y)/2;
			else if (row<orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=prevrow;
			foldr2=orow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;

	} else if (folddirection=='b') {
		int adjacentrow=orow-1; //edge is between orow and adjacentrow
		int nextrow=orow;
		while (nextrow<signature->numhfolds && foldinfo[nextrow+1][ocol].pages.n!=0) nextrow++;
		int prevrow=adjacentrow;
		while (prevrow>0) prevrow--; //it's ok to fold over onto blank areas

		if (prevrow<0 || adjacentrow-prevrow+1<nextrow-orow+1
				|| (adjacentrow>=0 && foldinfo[adjacentrow][ocol].pages.n==0)) {
			 //can't do the fold
			folddirection='x';
		} else {
			 //we can do the fold
			foldindex=orow;

			 //find the fold progress
			if (row==orow) foldprogress=.5-mm.y/om.y/2;
			else if (row==orow-1) foldprogress=.5+(elementheight-mm.y)/om.y/2;
			else if (row>orow) foldprogress=0;
			else foldprogress=1;
			if (foldprogress>1) foldprogress=1;
			if (foldprogress<0) foldprogress=0;

			 //need to find upper and lower affected elements
			foldr1=orow;
			foldr2=nextrow;
			foldc1=ocol;
			foldc2=ocol;
			while (foldc1>0 && foldinfo[orow][foldc1-1].pages.n!=0) foldc1--;
			while (foldc2<signature->numvfolds && foldinfo[orow][foldc2+1].pages.n!=0) foldc2++;
		}
		needtodraw=1;
	}

	DBG cerr <<"folding progress: "<<foldprogress<<",  om="<<om.x<<','<<om.y<<"  mm="<<mm.x<<','<<mm.y<<endl;


	return 0;
}

//! Adjust foldr1,foldr2,foldc1,foldc2 to reflect which cells get moved for whichfold.
/*! whichfold==0 means what is affected for the 1st fold, as if you were going from totally unfolded
 * to after the 1st fold.
 */
void SignatureInterface::remapAffectedCells(int whichfold)
{
	if (whichfold<0 || whichfold>=signature->folds.n) return;

	FoldedPageInfo **finfo=NULL;
	if (whichfold==foldlevel) {
		 //we can use the current foldinfo
		finfo=foldinfo;
	} else {
		 //we need to use a temporary foldinfo
		finfo=new FoldedPageInfo*[signature->numhfolds+2];
		int r;
		for (r=0; r<signature->numhfolds+1; r++) {
			finfo[r]=new FoldedPageInfo[signature->numvfolds+2];
			for (int c=0; c<signature->numvfolds+1; c++) {
				finfo[r][c].pages.push(r);
				finfo[r][c].pages.push(c);
			}
		}
		finfo[r]=NULL; //terminating NULL, so we don't need to remember sig->n

		signature->applyFold(finfo,whichfold);
	}

	// *** figure out cells based on direction and index
	int dir  =signature->folds.e[whichfold]->direction;
	int index=signature->folds.e[whichfold]->whichfold;
	int fr1,fr2,fc1,fc2;

	 //find the cells that must be moved..
	 //First find the whole block that might move, then shrink it down to include only active faces
	if (dir=='l') {
		fc1=index; //the only known so far
		fr1=0;
		fr2=signature->numhfolds;
		fc2=signature->numvfolds;
		while (fr1<fr2 && foldinfo[fr1][fc1].pages.n==0) fr1++;
		while (fr2>fr1 && foldinfo[fr2][fc1].pages.n==0) fr2--;
		while (fc2>fc1 && foldinfo[fr1][fc2].pages.n==0) fc2--;
	} else if (dir=='r') {
		fc2=index-1;
		fr1=0;
		fr2=signature->numhfolds;
		fc1=0;
		while (fr1<fr2 && foldinfo[fr1][fc2].pages.n==0) fr1++;
		while (fr2>fr1 && foldinfo[fr2][fc2].pages.n==0) fr2--;
		while (fc1<fc2 && foldinfo[fr1][fc1].pages.n==0) fc1++;
	} else if (dir=='b') {
		fr1=index;
		fc1=0;
		fc2=signature->numvfolds;
		fr2=signature->numhfolds;
		while (fc1<fc2 && foldinfo[fr1][fc1].pages.n==0) fc1++;
		while (fc2>fc1 && foldinfo[fr1][fc2].pages.n==0) fc2--;
		while (fr2>fr1 && foldinfo[fr2][fc1].pages.n==0) fr2--;
	} else if (dir=='t') {
		fr2=index-1;
		fr1=0;
		fc1=0;
		fc2=signature->numvfolds;
		while (fc1<fc2 && foldinfo[fr2][fc1].pages.n==0) fc1++;
		while (fc2>fc1 && foldinfo[fr2][fc2].pages.n==0) fc2--;
		while (fr1<fr2 && foldinfo[fr1][fc1].pages.n==0) fr1++;
	}

	if (finfo!=foldinfo) {
		for (int c=0; finfo[c]; c++) delete[] finfo[c];
		delete[] finfo;
	}

	foldr1=fr1;
	foldr2=fr2;
	foldc1=fc1;
	foldc2=fc2;
}

static const char *masktostr(int m)
{
	if (m==15) return _("all");
	if (m==1) return _("top");
	if (m==2) return _("right");
	if (m==4) return _("bottom");
	if (m==8) return _("left");
	return "?";
}

Laxkit::ShortcutHandler *SignatureInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("SignatureInterface");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("SignatureInterface");

	sc->Add(SIA_Decorations,     'd',0,0,          "Decorations",    _("Toggle decorations"),NULL,0);
	sc->Add(SIA_Thumbs,          'p',0,0,          "Thumbs",         _("Toggle showing of page thumbnails"),NULL,0);
	sc->Add(SIA_Center,          ' ',0,0,          "Center",         _("Center view"),NULL,0);
	sc->AddShortcut(LAX_Up,0,0, SIA_Center);
	sc->Add(SIA_CenterStacks,    LAX_Down,0,0,     "CenterStacks",   _("Center on stack arrangement area"),NULL,0);
	sc->Add(SIA_NextFold,        LAX_Left,0,0,     "NextFold",       _("Select next fold"),NULL,0);
	sc->Add(SIA_PreviousFold,    LAX_Right,0,0,    "PreviousFold",   _("Select previous fold"),NULL,0);
	sc->Add(SIA_InsetMask,       'i',ControlMask,0,"InsetMask",      _("Toggle which inset to change"),NULL,0);
	sc->Add(SIA_InsetInc,        'i',0,0,          "InsetInc",       _("Increment inset"),NULL,0);
	sc->Add(SIA_InsetDec,        'I',ShiftMask,0,  "InsetDec",       _("Decrement inset"),NULL,0);
	sc->Add(SIA_GapInc,          'g',0,0,          "GapInc",         _("Increment gap"),NULL,0);
	sc->Add(SIA_GapDec,          'G',ShiftMask,0,  "GapDec",         _("Decrement gap"),NULL,0);
	sc->Add(SIA_TileXInc,        'x',0,0,          "TileXInc",       _("Increase number of tiles horizontally"),NULL,0);
	sc->Add(SIA_TileXDec,        'X',ShiftMask,0,  "TileXDec",       _("Decrease number of tiles horizontally"),NULL,0);
	sc->Add(SIA_TileYInc,        'y',0,0,          "TileYInc",       _("Increase number of tiles vertically"),NULL,0);
	sc->Add(SIA_TileYDec,        'Y',ShiftMask,0,  "TileYDec",       _("Decrease number of tiles vertically"),NULL,0);
	sc->Add(SIA_NumFoldsVInc,    'v',0,0,          "NumFoldsVInc",   _("Increase number of vertical folds"),NULL,0);
	sc->Add(SIA_NumFoldsVDec,    'V',ShiftMask,0,  "NumFoldsVDec",   _("Decrease number of vertical folds"),NULL,0);
	sc->Add(SIA_NumFoldsHInc,    'h',0,0,          "NumFoldsHInc",   _("Increase number of horizontal folds"),NULL,0);
	sc->Add(SIA_NumFoldsHDec,    'H',ShiftMask,0,  "NumFoldsHDec",   _("Decrease number of horizontal folds"),NULL,0);
	sc->Add(SIA_BindingEdge,     'b',0,0,          "BindingEdge",    _("Next binding edge"),NULL,0);
	sc->Add(SIA_BindingEdgeR,    'B',ShiftMask,0,  "BindingEdgeR",   _("Previous binding edge"),NULL,0);
	sc->Add(SIA_PageOrientation, 'u',0,0,          "PageOrientation",_("Change page orientation"),NULL,0);
	sc->Add(SIA_TrimMask,        't',ControlMask,0,"TrimMask",       _("Toggle which trim to change"),NULL,0);
	sc->Add(SIA_TrimInc,         't',0,0,          "TrimInc",        _("Increase trim value"),NULL,0);
	sc->Add(SIA_TrimDec,         'T',ShiftMask,0,  "TrimDec",        _("Decrease trim value"),NULL,0);
	sc->Add(SIA_MarginMask,      'm',ControlMask,0,"MarginMask",     _("Toggle which margin to change"),NULL,0);
	sc->Add(SIA_MarginInc,       'm',0,0,          "MarginInc",      _("Increase margin value"),NULL,0);
	sc->Add(SIA_MarginDec,       'M',ShiftMask,0,  "MarginDec",      _("Decrease margin value"),NULL,0);

	manager->AddArea("SignatureInterface",sc);
	return sc;
}

int SignatureInterface::PerformAction(int action)
{
	if (action==SIA_Decorations) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (action==SIA_Thumbs) {
		showthumbs=!showthumbs;
		needtodraw=1;
		return 0;

	} else if (action==SIA_CenterStacks) {
		int nums=0,numi=1, t;
		SignatureInstance *s=sigimp->GetSignature(0,0);
		SignatureInstance *s2;
		while (s) {
			nums++;
			s2=s;
			t=0;
			while (s2) { t++; s2=s2->next_insert; }
			if (t>numi) numi=t;
			s=s->next_stack;
		}

		double w,h;
		GetDimensions(w,h);
		double textheight=dp->textheight()/dp->Getmag();
		double blockh=3*textheight;
		//double blockw=3*textheight+textheight+dp->textextent("00 sheet",-1,NULL,NULL)/dp->Getmag();

		dp->CenterPoint(flatpoint(w/2,-blockh*numi/2));
		needtodraw=1;
		return 0;
		
	} else if (action==SIA_Center) {
		double w,h;
		GetDimensions(w,h);
		viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (action==SIA_NextFold) {
		foldlevel--;
		if (foldlevel<0) foldlevel=0;

		 //we must remap the folds to reflect the new fold level
		signature->resetFoldinfo(NULL);
		for (int c=0; c<foldlevel; c++) {
			applyFold(signature->folds.e[c]);
		}
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (action==SIA_PreviousFold) {
		foldlevel++;
		if (foldlevel>=signature->folds.n) foldlevel=signature->folds.n;

		 //we must remap the folds to reflect the new fold level
		signature->resetFoldinfo(NULL);
		for (int c=0; c<foldlevel; c++) {
			applyFold(signature->folds.e[c]);
		}
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (action==SIA_InsetMask) {
		if (insetmask==15) insetmask=1;
		else { insetmask<<=1; if (insetmask>15) insetmask=15; }
		char str[100];
		sprintf(str,_("Sets %s inset"),masktostr(insetmask));
		PostMessage(str);
		return 0;

	} else if (action==SIA_InsetInc) {
		double step=siginstance->partition->totalheight*.01;
		if (insetmask&1) offsetHandle(SP_Inset_Top,    flatpoint(0,-step));
		if (insetmask&4) offsetHandle(SP_Inset_Bottom, flatpoint(0,step));
		if (insetmask&8) offsetHandle(SP_Inset_Left,   flatpoint(step,0));
		if (insetmask&2) offsetHandle(SP_Inset_Right,  flatpoint(-step,0));
		needtodraw=1;
		return 0;

	} else if (action==SIA_InsetDec) {
		double step=siginstance->partition->totalheight*.01;
		if (insetmask&1) offsetHandle(SP_Inset_Top,    flatpoint(0,step));
		if (insetmask&4) offsetHandle(SP_Inset_Bottom, flatpoint(0,-step));
		if (insetmask&8) offsetHandle(SP_Inset_Left,   flatpoint(-step,0));
		if (insetmask&2) offsetHandle(SP_Inset_Right,  flatpoint(step,0));
		needtodraw=1;
		return 0;

	} else if (action==SIA_GapInc) {
		double step=siginstance->partition->totalheight*.01;
		offsetHandle(SP_Tile_Gap_X, flatpoint(step,0));
		offsetHandle(SP_Tile_Gap_Y, flatpoint(0,step));
		return 0;

	} else if (action==SIA_GapDec) {
		double step=-siginstance->partition->totalheight*.01;
		offsetHandle(SP_Tile_Gap_X, flatpoint(step,0));
		offsetHandle(SP_Tile_Gap_Y, flatpoint(0,step));
		needtodraw=1;
		return 0;

	} else if (action==SIA_TileXInc) {
		adjustControl(SP_Tile_X_top, 1);
		return 0;

	} else if (action==SIA_TileXDec) {
		adjustControl(SP_Tile_X_top, -1);
		return 0;

	} else if (action==SIA_TileYInc) {
		adjustControl(SP_Tile_Y_left, 1);
		return 0;

	} else if (action==SIA_TileYDec) {
		adjustControl(SP_Tile_Y_left, -1);
		return 0;

	} else if (action==SIA_NumFoldsVInc) {
		adjustControl(SP_V_Folds_top, 1);
		return 0;

	} else if (action==SIA_NumFoldsVDec) {
		adjustControl(SP_V_Folds_top, -1);
		return 0;

	} else if (action==SIA_NumFoldsHInc) {
		adjustControl(SP_H_Folds_left, 1);
		return 0;

	} else if (action==SIA_NumFoldsHDec) {
		adjustControl(SP_H_Folds_left, -1);
		return 0;

	} else if (action==SIA_BindingEdge) {
		if (signature->binding=='l') signature->binding='t';
		else if (signature->binding=='t') signature->binding='r';
		else if (signature->binding=='r') signature->binding='b';
		else if (signature->binding=='b') signature->binding='l';
		needtodraw=1;
		return 0;

	} else if (action==SIA_BindingEdgeR) {
		if (signature->binding=='l') signature->binding='b';
		else if (signature->binding=='b') signature->binding='r';
		else if (signature->binding=='r') signature->binding='t';
		else if (signature->binding=='t') signature->binding='l';
		needtodraw=1;
		return 0;

	} else if (action==SIA_PageOrientation) {
		if (signature->up=='l') signature->up='t';
		else if (signature->up=='t') signature->up='r';
		else if (signature->up=='r') signature->up='b';
		else if (signature->up=='b') signature->up='l';
		needtodraw=1;
		return 0;

	} else if (action==SIA_TrimMask) {
		if (trimmask==15) trimmask=1;
		else { trimmask<<=1; if (trimmask>15) trimmask=15; }
		char str[100];
		sprintf(str,_("Set %s trim"),masktostr(trimmask));
		PostMessage(str);
		return 0;

	} else if (action==SIA_TrimInc) {
		double step=signature->PageHeight()*.01;
		if (trimmask&1) offsetHandle(SP_Trim_Top,    flatpoint(0,-step));
		if (trimmask&4) offsetHandle(SP_Trim_Bottom, flatpoint(0,step));
		if (trimmask&8) offsetHandle(SP_Trim_Left,   flatpoint(step,0));
		if (trimmask&2) offsetHandle(SP_Trim_Right,  flatpoint(-step,0));
		return 0;

	} else if (action==SIA_TrimDec) {
		double step=-signature->PageHeight()*.01;
		if (trimmask&1) offsetHandle(SP_Trim_Top,    flatpoint(0,-step));
		if (trimmask&4) offsetHandle(SP_Trim_Bottom, flatpoint(0,step));
		if (trimmask&8) offsetHandle(SP_Trim_Left,   flatpoint(step,0));
		if (trimmask&2) offsetHandle(SP_Trim_Right,  flatpoint(-step,0));
		return 0;

	} else if (action==SIA_MarginMask) {
		if (marginmask==15) marginmask=1;
		else { marginmask<<=1; if (marginmask>15) marginmask=15; }
		char str[100];
		sprintf(str,_("Set %s margin"),masktostr(marginmask));
		PostMessage(str);
		return 0;

	} else if (action==SIA_MarginInc) {
		double step=signature->PageHeight()*.01;
		if (marginmask&1) offsetHandle(SP_Margin_Top,    flatpoint(0,-step));
		if (marginmask&4) offsetHandle(SP_Margin_Bottom, flatpoint(0,step));
		if (marginmask&8) offsetHandle(SP_Margin_Left,   flatpoint(step,0));
		if (marginmask&2) offsetHandle(SP_Margin_Right,  flatpoint(-step,0));
		return 0;

	} else if (action==SIA_MarginDec) {
		double step=-signature->PageHeight()*.01;
		if (marginmask&1) offsetHandle(SP_Margin_Top,    flatpoint(0,-step));
		if (marginmask&4) offsetHandle(SP_Margin_Bottom, flatpoint(0,step));
		if (marginmask&8) offsetHandle(SP_Margin_Left,   flatpoint(step,0));
		if (marginmask&2) offsetHandle(SP_Margin_Right,  flatpoint(-step,0));
		return 0;
	}

	return 1;
}

int SignatureInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" SignatureInterface got ch:"<<(int)ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (showsplash) { showsplash=0; needtodraw=1; }

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1;
}

int SignatureInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
//	if (ch==LAX_Shift) {
//		return 0;
//	}
	return 1;
}


int SignatureInterface::UseThisSignature(Signature *sig)
{
	cerr <<" *** INCOMPLETE CODING: SignatureInterface::UseThisSignature(Document *doc)"<<endl;
	// ***
	return 1;
}



// ---------------------------ImpositionInterface functions:

int SignatureInterface::SetTotalDimensions(double width, double height)
{
	PaperStyle *p=new PaperStyle("Custom",width,height,0,300,NULL);
	SetPaper(p);
	p->dec_count();
	return 0;
}

/*! Return paper size for current signature instance.
 */
int SignatureInterface::GetDimensions(double &width, double &height)
{
	width =siginstance->partition->paper->w();
	height=siginstance->partition->paper->h();
	//sigimp->GetDimensions(0, &width,&height);
	return 0;
}

/*! Set paper of current signature instance.
 */
int SignatureInterface::SetPaper(PaperStyle *paper)
{
	siginstance->SetPaper(paper,0);
	//sigimp->SetPaperSize(paper);
	return 0;
	// *** should set paper size of every siginstance? or those of same size as top one and resize for page size of others?
}

/*! Use the given doc, but still use the current imposition, updated with number of pages from doc.
 */
int SignatureInterface::UseThisDocument(Document *doc)
{
	if (doc!=document) {
		if (document) document->dec_count();
		document=doc;
		if (document) document->inc_count();
	}
	sigimp->NumPages(document->pages.n);
	needtodraw=1;
	return 0;
}

/*! Return 1 for cannot use it.
 * Otherwise, the imposition is duplicated. If imp->doc exists, use it for this->document.
 */
int SignatureInterface::UseThisImposition(Imposition *imp)
{
	SignatureImposition *simp=dynamic_cast<SignatureImposition*>(imp);
	if (!simp) return 1;

	if (sigimp) sigimp->dec_count();
	sigimp=(SignatureImposition*)imp->duplicate();
	siginstance=sigimp->GetSignature(0,0);
	signature=siginstance->pattern;
	foldinfo=signature->foldinfo;
	signature->applyFold(NULL,-1);
	checkFoldLevel(1);

	if (sigimp->NumPages()==0) {
		sigimp->NumPages(1);
	}

	foldlevel=0; //how many of the folds are active in display. must be <= sig->folds.n
	foldinfo=signature->foldinfo;
	hasfinal=signature->HasFinal();
	signature->resetFoldinfo(NULL);
	if (simp->doc && simp->doc!=document) {
		simp->doc->inc_count();
		if (document) document->dec_count();
		document=sigimp->doc;
	}

	if (document) sigimp->NumPages(document->pages.n);
	ShowThisPaperSpread(0);
	return 0;
}


int SignatureInterface::ShowThisPaperSpread(int index)
{
	if (index<0) index=0;
	if (index>=sigimp->NumPapers()) index=sigimp->NumPapers();
	
	currentPaperSpread=index;
	SignatureInstance *sig=sigimp->InstanceFromPaper(currentPaperSpread, NULL,NULL, &sigpaper, &pageoffset,&midpageoffset, &siggroup);

	if (sig!=siginstance) {
		siginstance=sig;
		signature=siginstance->pattern;
		foldinfo=signature->foldinfo;
		foldlevel=0;
		signature->applyFold(NULL,-1);
		checkFoldLevel(1);
		signature->resetFoldinfo(NULL);
		//hasfinal=signature->HasFinal();
	}

    remapHandles(0);
	needtodraw=1;
	return 0;
}



} // namespace Laidout

