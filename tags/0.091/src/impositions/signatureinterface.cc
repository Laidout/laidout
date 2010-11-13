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
#include "signatureinterface.h"
#include "../viewwindow.h"
#include "../headwindow.h"
#include "../utils.h"
#include "../version.h"
#include "../filetypes/scribus.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/filedialog.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 

//size of the fold indicators on left of screen
#define INDICATOR_SIZE 10



#define SP_None              0

#define SP_Tile_X_top        1
#define SP_Tile_X_bottom     2
#define SP_Tile_Y_left       3
#define SP_Tile_Y_right      4
#define SP_Tile_Gap_X        5
#define SP_Tile_Gap_Y        6

#define SP_Inset_Top         7
#define SP_Inset_Bottom      8
#define SP_Inset_Left        9
#define SP_Inset_Right       10

#define SP_H_Folds_left      11
#define SP_H_Folds_right     12
#define SP_V_Folds_top       13
#define SP_V_Folds_bottom    14

#define SP_Trim_Top          15
#define SP_Trim_Bottom       16
#define SP_Trim_Left         17
#define SP_Trim_Right        18

#define SP_Margin_Top        19
#define SP_Margin_Bottom     20
#define SP_Margin_Left       21
#define SP_Margin_Right      22

#define SP_Binding           23

#define SP_Sheets_Per_Sig    24
#define SP_Num_Pages         25
#define SP_Num_Sigs          26

 //these three currently ignored for the most part:
#define SP_Up                27
#define SP_X                 28
#define SP_Y                 29

#define SP_FOLDS             100

// *************************************
//------------------------------------- ActionArea --------------------------------------
//typedef void RenderActionAreaFunc(Laxkit::Displayer *disp,ActionArea *overlay);

enum ActionAreaType {
	AREA_Handle,
	AREA_Slider,
	AREA_Button,
	AREA_Display_Only,
	AREA_Mode,
	AREA_H_Pan,
	AREA_V_Pan,
	AREA_Menu_Trigger
};

/*! \class ActionArea
 * \brief This can define areas on screen for various purposes.
 */


ActionArea::ActionArea(int what,int ntype,const char *txt,const char *ntip,int r,int v,unsigned long col,int cat)
{
	text=newstr(txt);
	tip=newstr(ntip);
	outline=NULL;
	npoints=0;
	visible=v;
	real=r;
	color=col;
	action=what;
	type=ntype;
	hidden=0;
	category=cat;
}

ActionArea::~ActionArea()
{
	if (outline) delete[] outline;
	if (tip) delete[] tip;
	if (text) delete[] text;
}

//! Change the position of the area, where pos==offset+hotspot.
/*! If which&1, adjust x. If which&2, adjust y.
 */
void ActionArea::Position(double x,double y,int which)
{
	if (which&1) offset.x=x-hotspot.x;
	if (which&2) offset.y=y-hotspot.y;
}

//! Return if point is within the outline (If outline is not NULL), or within the ActionArea DoubleBBox bounds otherwise.
/*! No point transformation is done. If real==1, it is assumed x,y are real coordinates, and otherwise it is
 * assumed they are screen coordinates.
 */
int ActionArea::PointIn(double x,double y)
{
	x-=offset.x; y-=offset.y;
	if (npoints) return point_is_in(flatpoint(x,y),outline,npoints);
	return x>=minx && x<=maxx && y>=miny && y<=maxy;
}

//! Create outline points.
/*! If takethem!=0, then make outline=pts, and that array will be delete[]'d in the destructor.
 * Otherise, the points are copied into a new array.
 *
 * If pts==NULL, but n>0, then allocate (or reallocate) the array.
 * It is presumed that the calling code will then adjust the points.
 *
 * outline will be reallocated only if n>npoints.
 *
 * Please note that this function does not set the min/max bounds. You can use FindBBox for that.
 */
flatpoint *ActionArea::Points(flatpoint *pts, int n, int takethem)
{
	if (n<=0) return NULL;

	if (!pts) {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		if (!outline) outline=new flatpoint[n];

	} else if (takethem) {
		delete[] outline;
		outline=pts;

	} else {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		outline=new flatpoint[n];
		memcpy(outline,pts,sizeof(flatpoint));
	}

	npoints=n;

	return outline;
}

//! Make the bounds be the actual bounds of outline.
/*! If there are no points in outline, then do nothing.
 */
void ActionArea::FindBBox()
{
	if (!npoints) return;

	clear();
	for (int c=0; c<npoints; c++) addtobounds(outline[c]);
}

// *************************************

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

SignatureInterface::SignatureInterface(int nid,Displayer *ndp,Signature *sig, PaperStyle *p)
	: InterfaceWithDp(nid,ndp) 
{
	showdecs=0;
	papersize=NULL;
	insetmask=15; trimmask=15; marginmask=15;
	firsttime=1;
	showsplash=0;

	if (sig) {
		signature=sig->duplicate();
	} else {
		signature=new Signature;
		reallocateFoldinfo();
	}
	if (p) signature->SetPaper(p);

	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	hasfinal=0;
	foldinfo=signature->foldinfo;
	checkFoldLevel(1);
	signature->resetFoldinfo(NULL);

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	if (!p && !sig) {
		signature->totalheight=5;
		signature->totalwidth=5;
	}

	color_inset=rgbcolorf(.5,0,0);
	color_margin=rgbcolorf(.75,.75,.75);
	color_trim=rgbcolorf(1.,0,0);
	color_binding=rgbcolorf(0,1,0);

	onoverlay=0;
	overoverlay=0;
	arrowscale=1;
	activetilex=activetiley=0;
	//remapHandles();
}

SignatureInterface::SignatureInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	showdecs=0;
	signature=new Signature;
	foldinfo=signature->foldinfo;
	papersize=NULL;

	foldr1=foldc1=foldr2=foldc2=-1;
	folddirection=0;
	lbdown_row=lbdown_col=-1;

	onoverlay=0;

	signature->totalheight=5;
	signature->totalwidth=5;
	
	foldlevel=0; //how many of the folds are active in display. must be < sig->folds.n
	hasfinal=0;

	reallocateFoldinfo();
	checkFoldLevel(1);
}

SignatureInterface::~SignatureInterface()
{
	DBG cerr <<"SignatureInterface destructor.."<<endl;

	if (signature) signature->dec_count();
	if (papersize) papersize->dec_count();
}

//! Reallocate foldinfo, usually after adding fold lines.
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
		viewport->postmessage(str);
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
 *
 * Returns foldlevel.
 */
int SignatureInterface::checkFoldLevel(int update)
{
	hasfinal=signature->checkFoldLevel(foldinfo,&finalr,&finalc);
	return foldlevel;
}

#define SIGM_Portrait        2000
#define SIGM_Landscape       2001
#define SIGM_SaveAsResource  2002
#define SIGM_FinalFromPaper  2003
#define SIGM_CustomPaper     2004

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *SignatureInterface::ContextMenu(int x,int y, int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Signature Interface"));

	int landscape=0;
	const char *paper="";
	if (signature->paperbox) {
		landscape=signature->paperbox->landscape();
		paper=signature->paperbox->name;
	}

	menu->AddItem(_("Portrait"),  SIGM_Portrait,  LAX_ISTOGGLE|(landscape?0:LAX_CHECKED));
	menu->AddItem(_("Landscape"), SIGM_Landscape, LAX_ISTOGGLE|(landscape?LAX_CHECKED:0));
	menu->AddSep(_("Paper"));

	menu->AddItem(_("Paper Size"),999);
	menu->SubMenu(_("Paper Size"));
	for (int c=0; c<laidout->papersizes.n; c++) {
		if (!strcmp(laidout->papersizes.e[c]->name,"Custom")) continue; // *** 
		if (!strcmp(laidout->papersizes.e[c]->name,"Whatever")) continue;

		menu->AddItem(laidout->papersizes.e[c]->name,c,
				LAX_ISTOGGLE
				| (!strcmp(paper,laidout->papersizes.e[c]->name) ? LAX_CHECKED : 0));
	}
	menu->EndSubMenu();
	//***menu->AddItem(_("Custom..."),SIGM_CustomPaper);
	menu->AddItem(_("Paper Size from Final Size"),SIGM_FinalFromPaper);

	if (hasfinal) {
		menu->AddSep();
		menu->AddItem(_("Save as resource..."),SIGM_SaveAsResource);
	}
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

		} else if (i==SIGM_FinalFromPaper) {
			signature->SetPaperFromFinalSize(signature->paperbox->w(),signature->paperbox->h());
			remapHandles();
			needtodraw=1;

			return 0;

		} else if (i==SIGM_CustomPaper) {
			cerr <<" *** need to implement edit custom paper size!!"<<endl;
			return 0;

		} else if (i==SIGM_Landscape) {
			if (signature->paperbox && !signature->paperbox->landscape()) {
				signature->paperbox->landscape(1);
				double t=signature->totalheight;
				signature->totalheight=signature->totalwidth;
				signature->totalwidth=t;
				remapHandles();
				needtodraw=1;
			}
			return 0;

		} else if (i==SIGM_Portrait) {
			if (signature->paperbox && signature->paperbox->landscape()) {
				signature->paperbox->landscape(0);
				double t=signature->totalheight;
				signature->totalheight=signature->totalwidth;
				signature->totalwidth=t;
				remapHandles();
				needtodraw=1;
			}
			return 0;

		} else if (i<999) {
			 //selecting new paper size
			if (i>=0 && i<laidout->papersizes.n) {
				if (!strcmp(laidout->papersizes.e[i]->name,"Custom")) {
					cerr <<" *** need to implement edit custom paper size!!"<<endl;
					return 0;
				}
				signature->SetPaper(laidout->papersizes.e[i]);
				remapHandles();
				needtodraw=1;
			}
			return 0;
		}
		return 1;

	} else if (!strcmp(mes,"saveAsPopup")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;

		char *error=NULL;
		FILE *f=open_file_for_writing(s->str,1,&error);
		if (!f) {
			if (error) viewport->postmessage(error);
			return 0;
		}

		fprintf(f,"#Laidout %s Imposition\n",LAIDOUT_VERSION);
		fprintf(f,"type SignatureImposition\n\n");
		if (signature->paperbox) {
			fprintf(f,"paper\n");
			signature->paperbox->dump_out(f,2,0,NULL);
		}

		signature->dump_out(f,0,0,NULL);

		fclose(f);

		viewport->postmessage(_("Saved."));

		return 0;
	}

	return 1;
}

void SignatureInterface::createHandles()
{
	unsigned long c;

	//ActionArea categories:
	//  0 single instance
	//  1 in pattern area
	//  2 in page area
	//  3 in gap area

	c=color_inset;
	controls.push(new ActionArea(SP_Inset_Top        , AREA_Handle, NULL, _("Inset top"),1,1,c,0));
	controls.push(new ActionArea(SP_Inset_Bottom     , AREA_Handle, NULL, _("Inset bottom"),1,1,c,0));
	controls.push(new ActionArea(SP_Inset_Left       , AREA_Handle, NULL, _("Inset left"),1,1,c,0));
	controls.push(new ActionArea(SP_Inset_Right      , AREA_Handle, NULL, _("Inset right"),1,1,c,0));

	c=color_trim;
	controls.push(new ActionArea(SP_Trim_Top         , AREA_Handle, NULL, _("Trim top of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Trim_Bottom      , AREA_Handle, NULL, _("Trim bottom of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Trim_Left        , AREA_Handle, NULL, _("Trim left of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Trim_Right       , AREA_Handle, NULL, _("Trim right of page"),1,1,c,2));

	c=coloravg(color_margin,0,.3333);
	controls.push(new ActionArea(SP_Margin_Top       , AREA_Handle, NULL, _("Margin top of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Margin_Bottom    , AREA_Handle, NULL, _("Margin bottom of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Margin_Left      , AREA_Handle, NULL, _("Margin left of page"),1,1,c,2));
	controls.push(new ActionArea(SP_Margin_Right     , AREA_Handle, NULL, _("Margin right of page"),1,1,c,2));

	c=color_binding;
	controls.push(new ActionArea(SP_Binding          , AREA_Handle, NULL, _("Binding edge, drag to place"),1,0,c,2));

	c=color_inset;
	controls.push(new ActionArea(SP_Tile_X_top       , AREA_Slider, NULL, _("Tile horizontally, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_X_bottom    , AREA_Slider, NULL, _("Tile horizontally, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_Y_left      , AREA_Slider, NULL, _("Tile vertically, wheel or drag changes"),1,0,c,0));
	controls.push(new ActionArea(SP_Tile_Y_right     , AREA_Slider, NULL, _("Tile vertically, wheel or drag changes"),1,0,c,0));

	controls.push(new ActionArea(SP_Tile_Gap_X       , AREA_Handle, NULL, _("Vertical gap between tiles"),1,0,c,3));
	controls.push(new ActionArea(SP_Tile_Gap_Y       , AREA_Handle, NULL, _("Horizontal gap between tiles"),1,0,c,3));

	c=color_margin;
	controls.push(new ActionArea(SP_H_Folds_left     , AREA_Slider, NULL, _("Number of horizontal folds"),1,0,c,1));
	controls.push(new ActionArea(SP_H_Folds_right    , AREA_Slider, NULL, _("Number of horizontal folds"),1,0,c,1));
	controls.push(new ActionArea(SP_V_Folds_top      , AREA_Slider, NULL, _("Number of vertical folds"),1,0,c,1));
	controls.push(new ActionArea(SP_V_Folds_bottom   , AREA_Slider, NULL, _("Number of vertical folds"),1,0,c,1));

	controls.push(new ActionArea(SP_Sheets_Per_Sig   , AREA_Slider, NULL, _("Wheel or drag changes"),1,0,0,0));
	controls.push(new ActionArea(SP_Num_Pages        , AREA_Slider, NULL, _("Wheel or drag changes"),1,0,0,0));
	controls.push(new ActionArea(SP_Num_Sigs         , AREA_Slider, NULL, _("Wheel or drag changes"),1,0,0,0));

}

ActionArea *SignatureInterface::control(int what)
{
	for (int c=0; c<controls.n; c++) if (what==controls.e[c]->action) return controls.e[c];
	return NULL;
}

/*! which==0 means do all. which&1 do singletons, which&2 pattern area (category 1), which&4 page area (category 2)
 */
void SignatureInterface::remapHandles(int which)
{
	if (controls.n==0) createHandles();

	ActionArea *area;
	flatpoint *p;
	int n;
	double ww=signature->totalwidth, hh=signature->totalheight;
	double w=signature->PageWidth(0), h=signature->PageHeight(0);
	double www=signature->PatternWidth(), hhh=signature->PatternHeight();

	arrowscale=2*INDICATOR_SIZE/dp->Getmag();

	if (which==0 || which&1) {
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
		p=draw_thing_coordinates(THING_Double_Arrow_Horizontal, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale/2);
		area->Position(0,0,3);

		area=control(SP_Tile_Gap_Y);
		p=draw_thing_coordinates(THING_Double_Arrow_Vertical, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale/2);
		area->Position(0,0,3);

		 //------inset
		area=control(SP_Inset_Top);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(ww/2-arrowscale/2,hh-signature->insettop);

		area=control(SP_Inset_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(ww/2-arrowscale/2,signature->insetbottom-arrowscale);

		area=control(SP_Inset_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(signature->insetleft-arrowscale,hh/2-arrowscale/2);

		area=control(SP_Inset_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(ww-signature->insetright,hh/2-arrowscale/2);
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
		 //----trim
		area=control(SP_Trim_Top);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(w/3-arrowscale/2,h-signature->trimtop);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(w/3-arrowscale/2,signature->trimbottom-arrowscale);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(signature->trimleft-arrowscale,h/3-arrowscale/2);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Trim_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(w-signature->trimright,h/3-arrowscale/2);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		 //------margins
		area=control(SP_Margin_Top);
		p=draw_thing_coordinates(THING_Arrow_Down, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,0);
		area->offset=flatpoint(w*2/3-arrowscale/2,h-signature->margintop);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Bottom);
		p=draw_thing_coordinates(THING_Arrow_Up, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale/2,arrowscale);
		area->offset=flatpoint(w*2/3-arrowscale/2,signature->marginbottom-arrowscale);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Left);
		p=draw_thing_coordinates(THING_Arrow_Right, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(arrowscale,arrowscale/2);
		area->offset=flatpoint(signature->marginleft-arrowscale,h*2/3-arrowscale/2);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		area=control(SP_Margin_Right);
		p=draw_thing_coordinates(THING_Arrow_Left, NULL,0, &n, arrowscale);
		area->Points(p,n,1);
		area->hotspot=flatpoint(0,arrowscale/2);
		area->offset=flatpoint(w-signature->marginright,h*2/3-arrowscale/2);
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);

		 //--------binding
		area=control(SP_Binding);
		//area->Points(NULL,4,1); //doesn't actually need points, it is checked for very manually
		area->hidden=!(hasfinal && foldlevel==signature->folds.n);
	} //page area items

	 //----- sheets and pages
	if (which==0 || which&1) {
		area=control(SP_Sheets_Per_Sig);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,-hh*.1);  p[1]=flatpoint(ww,-hh*.1); p[2]=flatpoint(ww,-hh*.2); p[3]=flatpoint(0,-hh*.2);
		area->FindBBox();

		area=control(SP_Num_Pages);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(0,-hh*.2);  p[1]=flatpoint(ww/2,-hh*.2); p[2]=flatpoint(ww/2,-hh*.3); p[3]=flatpoint(0,-hh*.3);
		area->FindBBox();

		area=control(SP_Num_Sigs);
		p=area->Points(NULL,4,0);
		p[0]=flatpoint(ww/2,-hh*.2);  p[1]=flatpoint(ww,-hh*.2); p[2]=flatpoint(ww,-hh*.3); p[3]=flatpoint(ww/2,-hh*.3);
		area->FindBBox();
	}
}

/*! Return 1 for cannot use it.
 * Otherwise, the imposition is duplicated.
 */
int SignatureInterface::UseThisImposition(SignatureImposition *sigimp)
{
	if (!sigimp || !sigimp->signature) return 1;

	if (signature) signature->dec_count();
	signature=sigimp->signature->duplicate();
	foldlevel=0; //how many of the folds are active in display. must be <= sig->folds.n
	hasfinal=0;
	foldinfo=signature->foldinfo;
	checkFoldLevel(1);
	signature->resetFoldinfo(NULL);

	return 0;
}

int SignatureInterface::UseThisSignature(Signature *sig)
{
	// ***
	return 1;
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
	if (dup==NULL) dup=new SignatureInterface(id,NULL);
	else if (!dynamic_cast<SignatureInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int SignatureInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
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

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();

	double x,y,w,h;
	static char str[150];

	 //----------------draw whole outline
	dp->NewFG(1.,0.,1.); //purple for paper outline, like custom papergroup border color
	w=signature->totalwidth;
	h=signature->totalheight;
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->drawline(0,0, w,0);
	dp->drawline(w,0, w,h);
	dp->drawline(w,h, 0,h);
	dp->drawline(0,h, 0,0);
	
	 //----------------draw inset
	dp->NewFG(color_inset); //dark red for inset
	if (signature->insetleft) dp->drawline(signature->insetleft,0, signature->insetleft,h);
	if (signature->insetright) dp->drawline(w-signature->insetright,0, w-signature->insetright,h);
	if (signature->insettop) dp->drawline(0,h-signature->insettop, w,h-signature->insettop);
	if (signature->insetbottom) dp->drawline(0,signature->insetbottom, w,signature->insetbottom);


	 //------------------draw fold pattern in each tile
	double ew=patternwidth/(signature->numvfolds+1);
	double eh=patternheight/(signature->numhfolds+1);
	w=patternwidth;
	h=patternheight;

	flatpoint pts[4],fp;
	int facedown=0;
	int hasface;
	int rrr,ccc;
	int ff,tt;
	double xx,yy;
	int xflip, yflip;

	x=signature->insetleft;
	for (int tx=0; tx<signature->tilex; tx++) {
	  y=signature->insetbottom;
	  for (int ty=0; ty<signature->tiley; ty++) {
		 //fill in light gray for elements with no current faces
		 //or draw orientation arrow and number for existing faces
		for (int rr=0; rr<signature->numhfolds+1; rr++) {
		  for (int cc=0; cc<signature->numvfolds+1; cc++) {
			hasface=foldinfo[rr][cc].pages.n;

			 //first draw filled face, grayed if no current faces
			dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
			pts[0]=flatpoint(x+cc*ew,y+rr*eh);
			pts[1]=pts[0]+flatpoint(ew,0);
			pts[2]=pts[0]+flatpoint(ew,eh);
			pts[3]=pts[0]+flatpoint(0,eh);

			if (hasface) dp->NewFG(1.,1.,1.);
			else dp->NewFG(.75,.75,.75);

			dp->drawlines(pts,4,1,1);

			//if (hasface && foldlevel==0) {
			if (hasface) {
				rrr=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-2];
				ccc=foldinfo[rr][cc].pages[foldinfo[rr][cc].pages.n-1];

				if (foldinfo[rr][cc].finalindexfront>=0) {
					 //there are faces in this spot, draw arrow and page number
					dp->NewFG(.75,.75,.75);
					if (hasfinal) {
						xflip=foldinfo[rrr][ccc].x_flipped^foldinfo[rrr][ccc].finalxflip;
						yflip=foldinfo[rrr][ccc].y_flipped^foldinfo[rrr][ccc].finalyflip;
					} else {
						xflip=foldinfo[rrr][ccc].x_flipped;
						yflip=foldinfo[rrr][ccc].y_flipped;
					}
					facedown=((xflip && !yflip) || (!xflip && yflip));

					//if (facedown) dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
					//else dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
					dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

					pts[0]=flatpoint(x+(cc+.5)*ew,y+(rr+.25+.5*(yflip?1:0))*eh);
					dp->drawarrow(pts[0],flatpoint(0,yflip?-1:1)*eh/4, 0,eh/2,1);
					fp=dp->realtoscreen(pts[0]);

					 //show range of pages at this position
					ff=foldinfo[rrr][ccc].finalindexfront;
					tt=foldinfo[rrr][ccc].finalindexback;
					if (!signature->autoaddsheets) {
						if (ff>tt) {
							tt*=signature->sheetspersignature;
							ff=tt+2*signature->sheetspersignature-1;
						} else {
							ff*=signature->sheetspersignature;
							tt=ff+2*signature->sheetspersignature-1;
						}
					}
					sprintf(str,"%d-%d",ff,tt);
					dp->textout(fp.x,fp.y, str,-1, LAX_CENTER);
				}
			}

			 //draw markings for final page binding edge, up, trim, margin
			if (hasfinal && foldlevel==signature->folds.n && rr==finalr && cc==finalc) {
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);

				xx=x+cc*ew;
				yy=y+rr*eh;

				 //draw gray margin edge
				dp->NewFG(color_margin);
				dp->drawline(xx,yy+signature->marginbottom,   xx+ew,yy+signature->marginbottom);			
				dp->drawline(xx,yy+eh-signature->margintop,   xx+ew,yy+eh-signature->margintop);			
				dp->drawline(xx+signature->marginleft,yy,     xx+signature->marginleft,yy+eh);			
				dp->drawline(xx+ew-signature->marginright,yy, xx+ew-signature->marginright,yy+eh);			

				 //draw red trim edge
				dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
				dp->NewFG(color_trim);
				if (signature->trimbottom>0) dp->drawline(xx,yy+signature->trimbottom, xx+ew,yy+signature->trimbottom);			
				if (signature->trimtop>0)    dp->drawline(xx,yy+eh-signature->trimtop, xx+ew,yy+eh-signature->trimtop);			
				if (signature->trimleft>0)   dp->drawline(xx+signature->trimleft,yy, xx+signature->trimleft,yy+eh);			
				if (signature->trimright>0)  dp->drawline(xx+ew-signature->trimright,yy, xx+ew-signature->trimright,yy+eh);			

				 //draw green binding edge
				dp->LineAttributes(2,LineSolid, CapButt, JoinMiter);
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
		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
		dp->drawline(x,    y, x+w,  y);
		dp->drawline(x+w,  y, x+w,y+h);
		dp->drawline(x+w,y+h, x  ,y+h);
		dp->drawline(x,  y+h, x,y);

		 //draw all fold lines
		dp->NewFG(.5,.5,.5);
		dp->LineAttributes(1,LineOnOffDash, CapButt, JoinMiter);
		for (int c=0; c<signature->numvfolds; c++) { //verticals
			dp->drawline(x+(c+1)*ew,y, x+(c+1)*ew,y+h);
		}

		for (int c=0; c<signature->numhfolds; c++) { //horizontals
			dp->drawline(x,y+(c+1)*eh, x+w,y+(c+1)*eh);
		}
		
		y+=patternheight+signature->tilegapy;
	  } //tx
	  x+=patternwidth+signature->tilegapx;
	} //ty

	 //draw in progress folding
	int device=0;
	DBG cerr <<"----------------any "<<buttondown.any(0,LEFTBUTTON,&device)<<endl;
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

		dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
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
			dp->LineAttributes(2, LineOnOffDash, CapButt, JoinMiter);
			if (folddirection=='l' || folddirection=='r') axis.y=-axis.y;
			else axis.x=-axis.x;
		} else {
			dp->NewFG(.9,.9,.9);
			dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
		}

		x=signature->insetleft;
		flatpoint pts[4];
		for (int tx=0; tx<signature->tilex; tx++) {
		  y=signature->insetbottom;
		  for (int ty=0; ty<signature->tiley; ty++) {

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
			
			y+=patternheight+signature->tilegapy;
		  }
		  x+=patternwidth+signature->tilegapx;
		}
	}

	 //draw fold indicator overlays on left side of screen
	dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);

	int thing;
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

	 //write out final page dimensions
	dp->NewFG(0.,0.,0.);
	sprintf(str,_("%d pages per pattern, Final size: %g x %g %s"),
				signature->PagesPerPattern(),
				signature->PageWidth(1)*laidout->unitmultiplier,
				signature->PageHeight(1)*laidout->unitmultiplier,
				laidout->unitname);
	dp->textout(0,0, str,-1, LAX_LEFT|LAX_TOP);



	 //-----------------draw control handles
	ActionArea *area;
	double d;
	Signature *s=signature;
	flatpoint dv;
	for (int c=controls.n-1; c>=0; c--) {
		area=controls.e[c];
		if (area->hidden) continue;

		if (area->action==SP_Sheets_Per_Sig) {
			 //draw sheet per signature indicator
			dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
			dp->NewFG(.5,0.,0.); //dark red for inset
			y=area->maxy;
			dp->drawline(0,y, signature->totalwidth,y);
			y-=signature->totalheight*.01;
			for (int c=1; c<5 && c<signature->sheetspersignature; c++) {
				dp->drawline(0,y, signature->totalwidth,y);
				y-=signature->totalheight*.01;
			}
			if (signature->autoaddsheets) sprintf(str,_("Many sheets in a single signature"));
			else if (signature->sheetspersignature==1) sprintf(str,_("1 sheet per signature"));
			else sprintf(str,_("%d sheets per signature"),signature->sheetspersignature);
			pts[0]=dp->realtoscreen(signature->totalwidth/2,y);
			dp->textout(pts[0].x,pts[0].y, str,-1, LAX_HCENTER|LAX_TOP);

		} else {
			if (area->outline && area->visible) {
				dp->LineAttributes(1, LineSolid, CapButt, JoinMiter);
				dv.x=dv.y=0;
				if (area->category==2) { //page specific
					dv.x=s->insetleft  +activetilex*(s->PatternWidth() +s->tilegapx) + finalc*s->PageWidth(0);
					dv.y=s->insetbottom+activetiley*(s->PatternHeight()+s->tilegapy) + finalr*s->PageHeight(0);
				}
				drawHandle(area,dv);

			} else if (area->action==SP_Tile_Gap_X) {
				if (overoverlay==SP_Tile_Gap_X) {
					dp->LineAttributes(5, LineSolid, CapButt, JoinMiter);
					for (int c2=0; c2<signature->tilex-1; c2++) {
						d=s->insetleft+(c2+1)*(s->tilegapx+s->PatternWidth()) - s->tilegapx/2;
						dp->drawline(d,0, d,s->totalheight);
						//----------
						//area->Position(d,-arrowscale, 3);
						//drawHandle(area,flatpoint());
						//area->Position(d,s->totalheight+arrowscale, 3);
						//drawHandle(area,flatpoint());
					}
				}

			} else if (area->action==SP_Tile_Gap_Y) {
				if (overoverlay==SP_Tile_Gap_Y) {
					dp->LineAttributes(5, LineSolid, CapButt, JoinMiter);
					for (int c2=0; c2<signature->tiley-1; c2++) {
						d=s->insetbottom+(c2+1)*(s->tilegapy+s->PatternHeight()) - s->tilegapy/2;
						dp->drawline(0,d, s->totalwidth,d);
						//area->Position(-arrowscale,d, 3);
						//drawHandle(area,flatpoint());
						//area->Position(s->totalheight+arrowscale,d, 3);
						//drawHandle(area,flatpoint());
					}
				}
			}
		}
	}
	dp->DrawReal();

	if (showsplash) {
		dp->DrawScreen();
		dp->NewFG(0,0,0);
		dp->textout(viewport->win_w/2,viewport->win_h/2,
				_("Laidout (impose only)\nBy Tom Lechner"), //***should probably show version number
				-1,LAX_CENTER);
		dp->DrawReal();
	}

	return 0;
}

void SignatureInterface::drawHandle(ActionArea *area, flatpoint offset)
{
	if (area->real) dp->DrawReal(); else dp->DrawScreen();
	dp->NewFG(area->color);
	dp->PushAxes();
	dp->ShiftReal(offset.x,offset.y);
	dp->ShiftReal(area->offset.x,area->offset.y);
	dp->drawlines(area->outline,area->npoints,1,area->action==overoverlay);
	//if (area->action==overoverlay) *** draw numeric value next to handle
	dp->PopAxes();
	dp->DrawReal();
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


//! Scan for handles to control variables, not for row and column.
/*! Returns which handle screen point is in, or none.
 */
int SignatureInterface::scanHandle(int x,int y)
{
	flatpoint fp=screentoreal(x,y);
	flatpoint tp;
	Signature *s=signature;

	for (int c=0; c<controls.n; c++) {
		if (controls.e[c]->hidden) continue;

		if (controls.e[c]->category==0) {
			 //normal, single instance handles
			if ((controls.e[c]->real && controls.e[c]->PointIn(fp.x,fp.y))
					|| (!controls.e[c]->real && controls.e[c]->PointIn(x,y))) {
				return controls.e[c]->action;
			}

		} else if (controls.e[c]->category==1) {
			 //fold lines, in the pattern area
			for (int x=0; x<signature->tilex; x++) {
			  for (int y=0; y<signature->tiley; y++) {
				tp=fp-flatpoint(s->insetleft,s->insetbottom);
				tp-=flatpoint(x*(s->tilegapx+s->PatternWidth()), y*(s->tilegapy+s->PatternHeight()));

				if (controls.e[c]->PointIn(tp.x,tp.y)) return controls.e[c]->action;
			  }
			}

		} else if (controls.e[c]->category==2) {
			 //in a final page area
			double w=s->PageWidth(0);
			double h=s->PageHeight(0);
			tp.x=fp.x-(s->insetleft  +activetilex*(s->PatternWidth() +s->tilegapx) + finalc*w);
			tp.y=fp.y-(s->insetbottom+activetiley*(s->PatternHeight()+s->tilegapy) + finalr*h);

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
				for (int c=0; c<signature->tilex-1; c++) {
					 //first check for inside gap itself
					d=s->insetleft+(c+1)*(s->tilegapx+s->PatternWidth())-s->tilegapx/2;
					if (s->tilegapx>zone) zone=s->tilegapx;
					if (fp.y>0 && fp.y<s->totalheight && fp.x<=d+zone/2 && fp.x>=d-zone/2) return SP_Tile_Gap_X;
				}

			} else { //tile gap y
				for (int c=0; c<signature->tiley-1; c++) {
					 //first check for inside gap itself
					d=s->insetbottom+(c+1)*(s->tilegapy+s->PatternHeight())-s->tilegapy/2;
					if (s->tilegapy>zone) zone=s->tilegapy;
					if (fp.x>0 && fp.x<s->totalwidth && fp.y<=d+zone/2 && fp.y>=d-zone/2) return SP_Tile_Gap_Y;
				}
			}
		}
	}

	 //check for gross areas for insets 
	if (fp.x>0 && fp.x<signature->totalwidth) {
		if (fp.y>0 && fp.y<signature->insetbottom) return SP_Inset_Bottom;
		if (fp.y>s->totalheight-s->insettop && fp.y<s->totalheight) return SP_Inset_Top;
	}
	if (fp.y>0 && fp.y<signature->totalheight) {
		if (fp.x>0 && fp.x<signature->insetleft) return SP_Inset_Left;
		if (fp.x>s->totalwidth-s->insetright && fp.x<s->totalwidth) return SP_Inset_Right;
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

	fp.x-=signature->insetleft;
	fp.y-=signature->insetbottom;

	double patternheight=signature->PatternHeight();
	double patternwidth =signature->PatternWidth();
	double elementheight=patternheight/(signature->numhfolds+1);
	double elementwidth =patternwidth /(signature->numvfolds+1);

	int tilex,tiley;
	tilex=floorl(fp.x/(patternwidth +signature->tilegapx));
	tiley=floorl(fp.y/(patternheight+signature->tilegapy));
	if (tile_col) *tile_col=tilex;
	if (tile_row) *tile_row=tiley;

	fp.x-=tilex*(patternwidth +signature->tilegapx);
	fp.y-=tiley*(patternheight+signature->tilegapy);

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

	flatpoint fp=screentoreal(x,y);

	if (state&LAX_STATE_MASK) return 1;

	int handle=scanHandle(x,y);
	adjustControl(handle,-1);

	return 0; //this will always absorb plain wheel
}

//! Respond to spinning controls.
int SignatureInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	flatpoint fp=screentoreal(x,y);

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
	if (handle==SP_None) return 1;

	if (handle==SP_Sheets_Per_Sig) {
		if (dir==-1) {
			signature->sheetspersignature--;
			if (signature->sheetspersignature<=0) {
				signature->autoaddsheets=1;
				signature->sheetspersignature=0;
			}
			needtodraw=1;
			return 0;

		} else {
			signature->sheetspersignature++;
			signature->autoaddsheets=0;
			needtodraw=1;
			return 0;
		}

	} else if (handle==SP_Tile_X_top || handle==SP_Tile_X_bottom) {
		if (dir==-1) {
			signature->tilex--;
			if (signature->tilex<=1) signature->tilex=1;
		} else {
			signature->tilex++;
		}
		remapHandles();
		needtodraw=1;
		return 0;

	} else if (handle==SP_Tile_Y_left || handle==SP_Tile_Y_right) {
		if (dir==-1) {
			signature->tiley--;
			if (signature->tiley<=1) signature->tiley=1;
		} else {
			signature->tiley++;
		}
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
		remapHandles();
		needtodraw=1;
		return 0;
	}

	return 1;
}

int SignatureInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (showsplash) { showsplash=0; needtodraw=1; }

	int row,col,tilerow,tilecol;
	int over=scan(x,y, &row,&col, NULL,NULL, &tilerow,&tilecol);
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
		if (a->type==AREA_Handle) return 0;
		onoverlay=SP_None;
	}

	 //check for on something to fold
	if (row<0 || row>signature->numhfolds || col<0 || col>signature->numvfolds
		  || foldinfo[row][col].pages.n==0
		  || tilerow<0 || tilecol<0
		  || tilerow>signature->tiley || tilecol>signature->tilex) {
		lbdown_row=lbdown_col=-1;
	} else {
		if (tilerow>=0 && tilerow<signature->tiley && tilerow!=activetiley) { needtodraw=1; activetiley=tilerow; }
		if (tilecol>=0 && tilecol<signature->tilex && tilecol!=activetilex) { needtodraw=1; activetilex=tilecol; }

		lbdown_row=row;
		lbdown_col=col;
		folddirection=0;
		foldprogress=0;
	}


	return 0;
}

int SignatureInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!(buttondown.isdown(d->id,LEFTBUTTON))) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (onoverlay) {
		if (onoverlay>=SP_FOLDS) {
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
		}
		
		return 0;
	}

	if (folddirection && folddirection!='x' && foldprogress>.9) {
		 //apply the fold, and add to the signature...

		applyFold(folddirection,foldindex,foldunder);

		if (foldlevel<signature->folds.n) {
			 //we have tried to fold when there are already further folds, so we must remove any
			 //after the current foldlevel.
			while (foldlevel<signature->folds.n) signature->folds.remove();
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
 */
int SignatureInterface::offsetHandle(int which, flatpoint d)
{ 
	ActionArea *area=control(which);

	if (area->type!=AREA_Handle) return 1;

	area->offset+=d;
	Signature *s=signature;
	char scratch[100];

	if (which==SP_Inset_Top) {
		 //adjust signature
		signature->insettop-=d.y;
		if (signature->insettop<0) {
			signature->insettop=0;
		} else if (s->insettop>s->totalheight - ((s->tiley-1)*s->tilegapy+s->insetbottom)) {
			s->insettop= s->totalheight - ((s->tiley-1)*s->tilegapy+s->insetbottom);
		}

		 //adjust handle to match signature
		d=area->Position();
		if (d.x<0) area->Position(0,0,1);
		else if (d.x>signature->totalwidth) area->Position(signature->totalwidth,0,1);
		area->Position(0,signature->totalheight-signature->insettop,2);

		sprintf(scratch,_("Top Inset")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->insettop);
		viewport->postmessage(scratch);

		remapHandles(2|4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Inset_Bottom  ) {
		 //adjust signature
		signature->insetbottom+=d.y;
		if (signature->insetbottom<0) {
			signature->insetbottom=0;
		} else if (s->insetbottom>s->totalheight - ((s->tiley-1)*s->tilegapy+s->insettop)) {
			s->insetbottom= s->totalheight - ((s->tiley-1)*s->tilegapy+s->insettop);
		}

		 //adjust handle to match signature
		d=area->Position();
		if (d.x<0) area->Position(0,0,1);
		else if (d.x>signature->totalwidth) area->Position(signature->totalwidth,0,1);
		area->Position(0,signature->insetbottom,2);

		sprintf(scratch,_("Bottom Inset")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->insetbottom);
		viewport->postmessage(scratch);

		remapHandles(2|4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Inset_Left    ) {
		signature->insetleft+=d.x;
		if (signature->insetleft<0) {
			signature->insetleft=0;
		} else if (s->insetleft>s->totalwidth - ((s->tilex-1)*s->tilegapx+s->insetright)) {
			s->insetleft= s->totalwidth - ((s->tilex-1)*s->tilegapx+s->insetright);
		}

		 //adjust handle to match signature
		d=area->Position();
		if (d.y<0) area->Position(0,0,2);
		else if (d.y>signature->totalheight) area->Position(0,signature->totalheight,2);
		area->Position(signature->insetleft,0,1);

		sprintf(scratch,_("Left Inset")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->insetleft);
		viewport->postmessage(scratch);

		remapHandles(2|4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Inset_Right   ) {
		signature->insetright-=d.x;
		if (signature->insetright<0) {
			signature->insetright=0;
		} else if (s->insetright>s->totalwidth - ((s->tilex-1)*s->tilegapx+s->insetleft)) {
			s->insetright= s->totalwidth - ((s->tilex-1)*s->tilegapx+s->insetleft);
		}

		 //adjust handle to match signature
		d=area->Position();
		if (d.y<0) area->Position(0,0,2);
		else if (d.y>signature->totalheight) area->Position(0,signature->totalheight,2);
		area->Position(signature->totalwidth-signature->insetright,0,1);

		sprintf(scratch,_("Right Inset")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->insetright);
		viewport->postmessage(scratch);

		remapHandles(2|4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Tile_Gap_X    ) {
		s->tilegapx+=d.x;

		if ((s->tilex-1)*s->tilegapx > s->totalwidth-s->insetleft-s->insetright+s->tilex*s->PatternWidth()) {
			if (s->tilex==1) s->tilegapx=0;
			else s->tilegapx=(s->totalwidth-s->insetleft-s->insetright+s->tilex*s->PatternWidth())/(s->tilex-1);
		} else if (s->tilegapx<0) s->tilegapx=0;

		sprintf(scratch,_("Tile gap")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->tilegapx);
		viewport->postmessage(scratch);

		remapHandles(2|4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Tile_Gap_Y    ) {
		s->tilegapy+=d.y;

		if ((s->tiley-1)*s->tilegapy > s->totalheight-s->insettop-s->insetbottom+s->tiley*s->PatternHeight()) {
			if (s->tiley==1) s->tilegapy=0;
			else s->tilegapy=(s->totalheight-s->insettop-s->insetbottom+s->tiley*s->PatternHeight())/(s->tiley-1);
		} else if (s->tilegapy<0) s->tilegapy=0;

		sprintf(scratch,_("Tile gap")); //to make fewer things to translate
		sprintf(scratch+strlen(scratch),_(" %.10g"),s->tilegapy);
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

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
		viewport->postmessage(scratch);

		remapHandles(4);
		needtodraw=1;
		return 0;

	} else if (which==SP_Binding       ) {
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
	return anInterface::RBDown(x,y,state,count,d);
}

int SignatureInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	int row,col,tilerow,tilecol;
	flatpoint mm;
	int over=scan(x,y, &row,&col, &mm.x,&mm.y, &tilerow,&tilecol);
	//fp now holds coordinates relative to the element cell

	DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<"  mm="<<mm.x<<','<<mm.y<<"  tile r,c:"<<tilerow<<','<<tilecol;
	DBG if (row>=0 && row<signature->numhfolds+1 && col>=0 && col<signature->numvfolds+1)
	DBG    cerr <<"  xflip: "<<foldinfo[row][col].x_flipped<<"  yflip:"<<foldinfo[row][col].y_flipped
	DBG         <<"  pages:"<<foldinfo[row][col].pages.n<<endl;

	int mx,my;
	int lx,ly;
	if (!buttondown.any()) {
		int handle=scanHandle(x,y);
		DBG cerr <<"found handle "<<handle<<endl;
		if (overoverlay!=handle) needtodraw=1;
		if (handle==SP_None) {
			overoverlay=0;
			viewport->postmessage(" ");
		} else {
			overoverlay=handle;
			viewport->postmessage(control(handle)->tip);
		}
		return 0;
	}
	buttondown.move(mouse->id,x,y, &lx,&ly);

	if (onoverlay!=SP_None) {
		if (onoverlay<SP_FOLDS) {
			flatpoint d=dp->screentoreal(x,y)-dp->screentoreal(lx,ly);
			if ((state&LAX_STATE_MASK)==ShiftMask) d*=.1;
			if ((state&LAX_STATE_MASK)==ControlMask) d*=.01;
			if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) d*=.001;

			if (onoverlay==SP_Binding) {
				d=dp->screentoreal(x,y);
				Signature *s=signature;
				d.x-=s->insetleft  +activetilex*(s->PatternWidth() +s->tilegapx) + (finalc+.5)*s->PageWidth(0);
				d.y-=s->insetbottom+activetiley*(s->PatternHeight()+s->tilegapy) + (finalr+.5)*s->PageHeight(0);
				d.x/=s->PageWidth(0);
				d.y/=s->PageHeight(0);
				if (d.y>d.x) {
					if (d.y>-d.x) { if (s->binding!='t') needtodraw=1; s->binding='t'; }
					else  { if (s->binding!='l') needtodraw=1; s->binding='l'; }
				} else {
					if (d.y>-d.x) { if (s->binding!='r') needtodraw=1; s->binding='r'; }
					else  { if (s->binding!='b') needtodraw=1; s->binding='b'; }
				}
			} else offsetHandle(onoverlay,d);
			return 0;

		} else if (onoverlay>=SP_FOLDS) {
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

int SignatureInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" SignatureInterface got ch:"<<(int)ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (showsplash) { showsplash=0; needtodraw=1; }

	if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		remapHandles();
		needtodraw=1;
		return 0;

		//--------------select how many folds to display
	} else if (ch>='0' && ch <='9') {
		// ***
		return 0;

	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		int h=signature->totalheight;
		int w=signature->totalwidth;
		viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
		remapHandles();
		needtodraw=1;
		return 0;
		
		//--------------change inset
	} else if (ch=='i') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (insetmask==15) insetmask=1;
			else { insetmask<<=1; if (insetmask>15) insetmask=15; }
			char str[100];
			sprintf(str,_("Sets %s inset"),masktostr(insetmask));
			viewport->postmessage(str);
			return 0;
		}
		double step=signature->totalheight*.01;
		if (insetmask&1) offsetHandle(SP_Inset_Top,    flatpoint(0,-step));
		if (insetmask&4) offsetHandle(SP_Inset_Bottom, flatpoint(0,step));
		if (insetmask&8) offsetHandle(SP_Inset_Left,   flatpoint(step,0));
		if (insetmask&2) offsetHandle(SP_Inset_Right,  flatpoint(-step,0));
		needtodraw=1;
		return 0;

	} else if (ch=='I') {
		double step=signature->totalheight*.01;
		if (insetmask&1) offsetHandle(SP_Inset_Top,    flatpoint(0,step));
		if (insetmask&4) offsetHandle(SP_Inset_Bottom, flatpoint(0,-step));
		if (insetmask&8) offsetHandle(SP_Inset_Left,   flatpoint(-step,0));
		if (insetmask&2) offsetHandle(SP_Inset_Right,  flatpoint(step,0));
		needtodraw=1;
		return 0;

		//--------------change tile gap
	} else if (ch=='g') {
		double step=signature->totalheight*.01;
		offsetHandle(SP_Tile_Gap_X, flatpoint(step,0));
		offsetHandle(SP_Tile_Gap_Y, flatpoint(0,step));
		return 0;

	} else if (ch=='G') {
		double step=-signature->totalheight*.01;
		offsetHandle(SP_Tile_Gap_X, flatpoint(step,0));
		offsetHandle(SP_Tile_Gap_Y, flatpoint(0,step));
		needtodraw=1;
		return 0;

		//-------------tilex and tiley
	} else if (ch=='x') {
		adjustControl(SP_Tile_X_top, 1);
		return 0;

	} else if (ch=='X') {
		adjustControl(SP_Tile_X_top, -1);
		return 0;

	} else if (ch=='y') {
		adjustControl(SP_Tile_Y_left, 1);
		return 0;

	} else if (ch=='Y') {
		adjustControl(SP_Tile_Y_left, -1);
		return 0;

		//-----------numhfolds and numvfolds
	} else if (ch=='v') {
		adjustControl(SP_V_Folds_top, 1);
		return 0;

	} else if (ch=='V') {
		adjustControl(SP_V_Folds_top, -1);
		return 0;

	} else if (ch=='h') {
		adjustControl(SP_H_Folds_left, 1);
		return 0;

	} else if (ch=='H') {
		adjustControl(SP_H_Folds_left, -1);
		return 0;

		//---------------move binding edge
	} else if (ch=='b') {
		if (signature->binding=='l') signature->binding='t';
		else if (signature->binding=='t') signature->binding='r';
		else if (signature->binding=='r') signature->binding='b';
		else if (signature->binding=='b') signature->binding='l';
		needtodraw=1;
		return 0;

	} else if (ch=='B') {
		if (signature->binding=='l') signature->binding='b';
		else if (signature->binding=='b') signature->binding='r';
		else if (signature->binding=='r') signature->binding='t';
		else if (signature->binding=='t') signature->binding='l';
		needtodraw=1;
		return 0;

		//---------------page orientation
	} else if (ch=='u') { //up direction
		if (signature->up=='l') signature->up='t';
		else if (signature->up=='t') signature->up='r';
		else if (signature->up=='r') signature->up='b';
		else if (signature->up=='b') signature->up='l';
		needtodraw=1;
		return 0;

		//--------------change page trim
	} else if (ch=='t') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (trimmask==15) trimmask=1;
			else { trimmask<<=1; if (trimmask>15) trimmask=15; }
			char str[100];
			sprintf(str,_("Set %s trim"),masktostr(trimmask));
			viewport->postmessage(str);
			return 0;
		}
		double step=signature->PageHeight()*.01;
		if (trimmask&1) offsetHandle(SP_Trim_Top,    flatpoint(0,-step));
		if (trimmask&4) offsetHandle(SP_Trim_Bottom, flatpoint(0,step));
		if (trimmask&8) offsetHandle(SP_Trim_Left,   flatpoint(step,0));
		if (trimmask&2) offsetHandle(SP_Trim_Right,  flatpoint(-step,0));
		return 0;

	} else if (ch=='T') {
		double step=-signature->PageHeight()*.01;
		if (trimmask&1) offsetHandle(SP_Trim_Top,    flatpoint(0,-step));
		if (trimmask&4) offsetHandle(SP_Trim_Bottom, flatpoint(0,step));
		if (trimmask&8) offsetHandle(SP_Trim_Left,   flatpoint(step,0));
		if (trimmask&2) offsetHandle(SP_Trim_Right,  flatpoint(-step,0));
		return 0;

		//--------------change page margin
	} else if (ch=='m') {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (marginmask==15) marginmask=1;
			else { marginmask<<=1; if (marginmask>15) marginmask=15; }
			char str[100];
			sprintf(str,_("Set %s margin"),masktostr(marginmask));
			viewport->postmessage(str);
			return 0;
		}
		double step=signature->PageHeight()*.01;
		if (marginmask&1) offsetHandle(SP_Margin_Top,    flatpoint(0,-step));
		if (marginmask&4) offsetHandle(SP_Margin_Bottom, flatpoint(0,step));
		if (marginmask&8) offsetHandle(SP_Margin_Left,   flatpoint(step,0));
		if (marginmask&2) offsetHandle(SP_Margin_Right,  flatpoint(-step,0));
		return 0;

	} else if (ch=='M') {
		double step=-signature->PageHeight()*.01;
		if (marginmask&1) offsetHandle(SP_Margin_Top,    flatpoint(0,-step));
		if (marginmask&4) offsetHandle(SP_Margin_Bottom, flatpoint(0,step));
		if (marginmask&8) offsetHandle(SP_Margin_Left,   flatpoint(step,0));
		if (marginmask&2) offsetHandle(SP_Margin_Right,  flatpoint(-step,0));
		return 0;

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


//----------------------- SignatureEditor --------------------------------------

/*! \class SignatureEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for signature editing.
 *
 * This creates the window with a SignatureInterface.
 */


//! Make the window using project.
/*! Inc count of ndoc.
 */
SignatureEditor::SignatureEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						Laxkit::anXWindow *nowner, const char *mes,
						SignatureImposition *sigimp, PaperStyle *p,const char *imposearg)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_NO_SCROLLERS|VIEWPORT_NO_RULERS, 
					0,0,500,500, 0, NULL)
{
	SetOwner(nowner,mes);

	//signature=sig;
	//if (signature) signature->inc_count();

	if (!viewport) {
		viewport=new ViewportWindow(this,"signature-editor-viewport","signature-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);
		app->reparent(viewport,this);
		viewport->dec_count();
	}

	win_colors->bg=rgbcolor(200,200,200);
	viewport->dp->NewBG(200,200,200);

	needtodraw=1;
	tool=new SignatureInterface(1,viewport->dp,(sigimp?sigimp->signature:NULL),p);
	if (imposearg) tool->showsplash=1;

	AddTool(tool,1,1); // local, and select it
	// *** add signature and paper if any...
	

	 //**** this is a hack! Should instead be parsed into an export config with extra fields for additional
	 // 		imposing
	imposeout=NULL;
	imposeformat=NULL;
	if (imposearg) {
		//need to load a new document, which may be a non-laidout document.
		//If non-laidout, then create new singles, and import
		//add extra field for impose out

		Attribute att;
		DBG const char *in="",*out="";
		const char *prefer=NULL;
		NameValueToAttribute(&att,imposearg,'=',',');
		const char *name,*value;
		for (int c=0; c<att.attributes.n; c++) {
			name =att.attributes.e[c]->name;
			value=att.attributes.e[c]->value;
			if (!strcmp(name,"in")) {
				in=value;
				if (isScribusFile(value)) {
					if (addScribusDocument(value)==0) {
						//yikes!
						tool->signature->SetPaper(laidout->project->docs.e[0]->doc->imposition->papergroup->papers.e[0]->box->paperstyle);
					}
				}
				//***
				//file can be:
				//  laidout: load normally
				//  scribus: create singles with widthxheight pages, import
				//  pdf: if you know number of pages, create singles with that many pages,
				//     export temp podofo plan, call podofoimpose
			} else if (!strcmp(name,"out")) {
				makestr(imposeout,value);
				out=value;
			} else if (!strcmp(name,"prefer")) {
				prefer=value;
			} else if (!strcmp(name,"width")) {
				DoubleAttribute(value,&tool->signature->totalwidth,NULL);
			} else if (!strcmp(name,"height")) {
				DoubleAttribute(value,&tool->signature->totalheight,NULL);
			}
		}

		if (prefer) {
			int c;
			for (c=0; c<laidout->impositionpool.n; c++) {
				if (!strcasecmp(laidout->impositionpool.e[c]->name,prefer)) break;
			}
			if (c<laidout->impositionpool.n) {
				Imposition *imp=laidout->impositionpool.e[c]->Create();
				SignatureImposition *simp=dynamic_cast<SignatureImposition*>(imp);
				if (simp) {
					PaperStyle *p;
					p=(PaperStyle*)laidout->project->docs.e[0]->doc->imposition->papergroup->papers.e[0]->box->paperstyle->duplicate();
					//simp->SetPaper(paper);
					simp->SetPaperFromFinalSize(p->w(),p->h());
					tool->UseThisImposition(simp);
					simp->dec_count();
					p->dec_count();
				} else {
					delete imp;
				}
			}
		}

		DBG cerr <<"Impose only from "<<in<<" to "<<out<<endl;
	}

}

SignatureEditor::~SignatureEditor()
{ 
	if (imposeout) delete[] imposeout;
	if (imposeformat) delete[] imposeformat;
}

//! Passes off to SignatureInterface::dump_out().
void SignatureEditor::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_out(f,indent,what,context);
}

//! Passes off to SignatureInterface::dump_in_atts().
void SignatureEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	// *** ((SignatureInterface *)curtool)->dump_in_atts(att,flag,context);
}

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int SignatureEditor::init()
{
	ViewerWindow::init();
	viewport->dp->NewBG(200,200,200);

	anXWindow *last=NULL;
	Button *tbut;

	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

//	if (imposeout) {
//		AddNull();
//		LineInput *linp;
//		last=linp=new LineInput(NULL,"imp",_("Impose..."),0,
//									  0,0,0,0,0,
//									  last,object_id,"out",
//									  _("Out:"),imposeout,0,
//									  0,0, 5,3, 5,3);
//		linp->tooltip(_("The file to output the imposed file to"));
//		//AddWin(linp,1, 50,0,2000,50,0, 50,0,50,50,0, -1);
//		AddWin(linp,1, 50,0,2000,50,0, linp->win_h,0,0,0,0, -1);
//	}

	Sync(1);	

	int h=tool->signature->totalheight;
	int w=tool->signature->totalwidth;
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);

	return 0;
}

//! Send the current imposition to win_owner.
void SignatureEditor::send()
{
	SignatureImposition *sigimp=new SignatureImposition(tool->signature);
	sigimp->SetPaperSize(sigimp->signature->paperbox);
	RefCountedEventData *data=new RefCountedEventData(sigimp);

	if (imposeout) {
		//for impose-only mode
		//if imposeformat==scribus, continue...
		Document *doc=laidout->project->docs.e[0]->doc;
		doc->ReImpose(sigimp,0);
		exportImposedScribus(doc,imposeout);
	}

	sigimp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

/*! Responds to: "ok", "cancel"
 *
 */
int SignatureEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"SignatureEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"ok")) {
		send();
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("cancel",mes)) {
		EventData *e=new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	}
	return 1;
}

int SignatureEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

//	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
//		app->addwindow(new HelpWindow());
//		return 0;
	}
	return 1;
}



//} // namespace Laidout

