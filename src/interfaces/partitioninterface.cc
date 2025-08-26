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
// Copyright (C) 2004-2007,2011 by Tom Lechner
//


#include "../laidout.h"
#include "partitioninterface.h"
#include "../language.h"
// #include "../ui/viewwindow.h"
// #include "../ui/papersizewindow.h"
// #include "../core/drawdata.h"
// #include "../dataobjects/printermarks.h"

// #include <lax/inputdialog.h>
// #include <lax/strmanip.h>
// #include <lax/laxutils.h>
// #include <lax/transformmath.h>
// #include <lax/colors.h>
#include <lax/interfaces/interfacemanager.h>

#include <lax/debug.h>


using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {


enum PartitionActions {
	PARTITION_None = 0,
	PARTITION_InsetTop,
	PARTITION_InsetRight,
	PARTITION_InsetBottom,
	PARTITION_InsetLeft,
	PARTITION_Cut,
	PARTITION_Top,
	PARTITION_Left,
	PARTITION_Bottom,
	PARTITION_Right,
	PARTITION_From,
	PARTITION_To,

	PARTITION_NewCut,
	PARTITION_MoveCut,

	PARTITION_ACTION_MIN,
	PARTITION_Tiles,
	PARTITION_Insets,
	PARTITION_Decorations,
	PARTITION_ToggleLabels,
	PARTITION_NoWorkAndTurn,
	PARTITION_WorkAndTurn,
	PARTITION_WorkAndTurnBF,
	PARTITION_WorkAndTumble,
	PARTITION_WorkAndTumbleBG,
	PARTITION_MarkWaste,
	PARTITION_MarkActive,
	PARTITION_ACTION_MAX,

	PARTITION_Paper,

	PARTITION_MAX
};


//------------------------------------- PaperCut --------------------------------------


//------------------------------------- Region --------------------------------------

/*! \class Region
 * A division of a PaperPartition.
 *
 * See also PartitionInterface.
 */

Region::Region()
{
	maxx = 8.5;
	maxy = 11;
}

Region::~Region()
{}


void Region::dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
    att.dump_out(f, indent);
}

Laxkit::Attribute* Region::dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context)
{
	DBGE("IMPLEMENT ME!!!")
	return att;
}

void Region::dump_in_atts(Laxkit::Attribute*att, int flag, Laxkit::DumpContext *context)
{
	DBGE("IMPLEMENT ME!!!")
}

Value *Region::duplicate()
{
	Region *dup = new Region();
	dup->maxx = maxx;
	dup->maxy = maxy;
	dup->label = label;
	return dup;
}

ObjectDef *Region::makeObjectDef()
{
	DBGE("IMPLEMENT ME!!!")
	return nullptr;
}

/*! Add insets for the first 4 cuts. If any insets exist already,
 * then update values.
 * Returns the number of new cuts.
 */
int Region::AddInsets(double top, double right, double bottom, double left)
{
	int n = 0;
	PaperCut *cut = nullptr;
	if (cuts.n > 0 && cuts.e[0]->cut_type == PaperCut::CUT_InsetTop) cut = cuts.e[0];
	if (!cut) {
		n++;
		cut = new PaperCut();
		cut->cut_type = PaperCut::CUT_InsetTop;
		cuts.push(cut,1,0);
	}
	cut->from = BBoxPoint(0,1, false) + flatvector(0, -top);
	cut->to   = BBoxPoint(1,1, false) + flatvector(0, -top);

	cut = nullptr;
	if (cuts.n > 1 && cuts.e[1]->cut_type == PaperCut::CUT_InsetRight) cut = cuts.e[1];
	if (!cut) {
		n++;
		cut = new PaperCut();
		cut->cut_type = PaperCut::CUT_InsetRight;
		cuts.push(cut,1,1);
	}
	cut->from = BBoxPoint(1,1, false) + flatvector(-left, 0);
	cut->to   = BBoxPoint(1,0, false) + flatvector(-left, 0);

	cut = nullptr;
	if (cuts.n > 2 && cuts.e[2]->cut_type == PaperCut::CUT_InsetBottom) cut = cuts.e[2];
	if (!cut) {
		n++;
		cut = new PaperCut();
		cut->cut_type = PaperCut::CUT_InsetBottom;
		cuts.push(cut,1,0);
	}
	cut->from = BBoxPoint(0,1, false) + flatvector(0, top);
	cut->to   = BBoxPoint(0,0, false) + flatvector(0, top);

	cut = nullptr;
	if (cuts.n > 3 && cuts.e[3]->cut_type == PaperCut::CUT_InsetLeft) cut = cuts.e[3];
	if (!cut) {
		n++;
		cut = new PaperCut();
		cut->cut_type = PaperCut::CUT_InsetLeft;
		cuts.push(cut,1,0);
	}
	cut->from = BBoxPoint(0,0, false) + flatvector(left, 0);
	cut->to   = BBoxPoint(0,1, false) + flatvector(left, 0);

	return n;
}

/*! Returns number of cuts removed. This should be 4 if insets existed, else 0.
 */
int Region::RemoveInsets()
{
	int n = 0;
	for (int c = cuts.n-1; c >= 0; c--) {
		if (cuts.e[c]->IsInset()) {
			cuts.remove(c);
			n++;
		}
	}
	return n;
}

bool Region::HasInsets()
{
	for (int c = cuts.n-1; c >= 0; c--) {
		if (cuts.e[c]->IsInset()) return true;
	}
	return false;
}

bool Region::HasTiles()
{
	// ***
	return false;
}


//------------------------------------- PartitionInterfaceSettings --------------------------------------

// static object so that it is easily shared between all tool instances
Laxkit::SingletonKeeper PartitionInterface::settingsObject;

PartitionInterfaceSettings::PartitionInterfaceSettings()
{}

PartitionInterfaceSettings::~PartitionInterfaceSettings()
{
	if (font) font->dec_count();
}


//------------------------------------- PartitionInterface --------------------------------------
	
/*! \class PartitionInterface 
 * \brief Interface to cut a rectangle into smaller rectangles.
 *
 */
/*! \var PaperBoxData *PartitionInterface::paperboxdata
 * \brief Temporary pointer to aid viewport refreshing of PaperBoxData objects.
 */



PartitionInterface::PartitionInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp)
{
	showdecs    = 0;
	// show_labels = true;
	// show_indices = false;

	// snapto      = MediaBox;
	// search_snap = true;
	// snap_px_threshhold = 5 * InterfaceManager::GetDefault()->ScreenLine();
	// snap_running_angle = 0;

	// font        = nullptr;

	// papergroup   = nullptr;
	// paperboxdata = nullptr;
	// doc          = nullptr;

	sc = nullptr;

	settings = dynamic_cast<PartitionInterfaceSettings*>(settingsObject.GetObject());
	if (!settings) {
		settings = new PartitionInterfaceSettings();
		settingsObject.SetObject(settings, false);
		settings->dec_count();
	}
}

PartitionInterface::PartitionInterface()
	: PartitionInterface(nullptr, -1, nullptr)
{}

PartitionInterface::~PartitionInterface()
{
	// DBG cerr <<"PartitionInterface destructor.."<<endl;

	if (paper_style) paper_style->dec_count();
	if (main_region) main_region->dec_count();

	if (sc) sc->dec_count();
}

const char *PartitionInterface::Name()
{ return _("Partition"); }


/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PartitionInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	rx = x;
	ry = y;

	if (!menu) menu = new MenuInfo(_("Partition"));
	else menu->AddSep(_("Partition"));

	if (!main_region) {
		menu->AddSep(_("New region"));
		BuildGroupedPaperMenu(menu, PARTITION_Paper, true, false);

	} else {
		menu->AddToggleItem(_("Insets"), PARTITION_Insets, 0, current_region->HasInsets());
		menu->AddToggleItem(_("Tiles"),  PARTITION_Tiles,  0, current_region->HasTiles());

		menu->AddSep(_("Work and turn"));
		menu->AddToggleItem(_("None"),               PARTITION_NoWorkAndTurn  , 0, current_region->work_and_turn == SIGT_None);
		menu->AddToggleItem(_("Work and turn"),      PARTITION_WorkAndTurn    , 0, current_region->work_and_turn == SIGT_Work_and_Turn);
		menu->AddToggleItem(_("Work and turn bf"),   PARTITION_WorkAndTurnBF  , 0, current_region->work_and_turn == SIGT_Work_and_Turn_BF);
		menu->AddToggleItem(_("Work and tumble"),    PARTITION_WorkAndTumble  , 0, current_region->work_and_turn == SIGT_Work_and_Tumble);
		menu->AddToggleItem(_("Work and tumble bf"), PARTITION_WorkAndTumbleBG, 0, current_region->work_and_turn == SIGT_Work_and_Tumble_BF);

		menu->AddSep(_("Paper"));
		menu->AddItem(_("Paper size"));
		menu->SubMenu();
		BuildGroupedPaperMenu(menu, PARTITION_Paper, true, false);
		menu->EndSubMenu();
	}


	// menu->AddToggleItem(_("Snap"),         PARTITION_ToggleSnap,     0, search_snap);
	// menu->AddToggleItem(_("Show labels"),  PARTITION_ToggleLabels,   0, show_labels);
	// menu->AddToggleItem(_("Show indices"), PARTITION_ToggleIndices,  0, show_indices);
	// menu->AddToggleItem(_("Edit margin"), PARTITION_EditMargins,  0, edit_margins);	
	// menu->AddSep();

	// if (papergroup) {
	// 	menu->AddItem(_("Add Registration Mark"),PARTITION_RegistrationMark);
	// 	menu->AddItem(_("Add Gray Bars"),PARTITION_GrayBars);
	// 	menu->AddItem(_("Add Cut Marks"),PARTITION_CutMarks);
	// 	menu->AddSep();
	// 	menu->AddItem(_("Create Net from group"), PARTITION_CreateImposition);
	// 	menu->AddSep();
	// }

	// laidout->resourcemanager->ResourceMenu("PaperGroup", false, menu, 0, PARTITION_Menu_Papergroup, papergroup);

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int PartitionInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item
		int info = s->info4;

		if (info == PARTITION_Paper) {
			// selected different paper size
			DBGE("IMPLEMENT ME!!!")
		
		} else if (i > PARTITION_ACTION_MIN && i < PARTITION_ACTION_MAX) {
			return PerformAction(i);
		}

	// } else if (!strcmp(mes,"")) {
	}
	return 1;
}

// /*! incs count of ndoc if ndoc is not already the current document.
//  *
//  * Return 0 for success, nonzero for fail.
//  */
// int PartitionInterface::UseThisDocument(Document *ndoc)
// {
// 	if (ndoc==doc) return 0;
// 	if (doc) doc->dec_count();
// 	doc=ndoc;
// 	if (ndoc) ndoc->inc_count();
// 	return 0;
// }



//! Return a new PartitionInterface if dup=nullptr, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PartitionInterface::duplicate(anInterface *dup)//dup=nullptr
{
	if (dup == nullptr) dup = new PartitionInterface(nullptr, id,nullptr);
	else if (!dynamic_cast<PartitionInterface *>(dup)) return nullptr;
	
	return anInterface::duplicate(dup);
}


int PartitionInterface::InterfaceOn()
{
	// DBG cerr <<"PartitionInterfaceOn()"<<endl;
	// LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
	// if (lvp && papergroup) lvp->UseThisPaperGroup(papergroup);
	showdecs = 1;
	needtodraw = 1;
	return 0;
}

int PartitionInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs = 0;

	// LaidoutViewport *lvp = dynamic_cast<LaidoutViewport *>(curwindow);
	// if (lvp) lvp->UseThisPaperGroup(nullptr);

	needtodraw = 1;
	// DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

//! Use a PaperGroup. Does NOT update viewport.
int PartitionInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	// PaperGroup *pg=dynamic_cast<PaperGroup *>(ndata);
	// if (!pg && ndata) return 0; //was a non-null object, but not a papergroup
	// Clear(nullptr);
	
	// papergroup=pg;
	// if (papergroup) papergroup->inc_count();
	// needtodraw=1;

	return 1;
}

/*! Clean maybebox, curbox, curboxes, papergroup.
 * Does NOT update viewport.
 */
void PartitionInterface::Clear(SomeData *d)
{
	// if (papergroup) { papergroup->dec_count(); papergroup = nullptr; }
}

	
// /*! This will be called by viewports, and the papers will be displayed opaque with
//  * drop shadow.
//  */
// int PartitionInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
// 			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
// {
// 	PaperBoxData *data = dynamic_cast<PaperBoxData *>(tdata);
// 	if (!data) return 1;
// 	int td             = showdecs;
// 	bool labels        = show_labels;
// 	bool indices       = show_indices;
// 	int ntd            = needtodraw;
// 	BoxTypes tdrawwhat = drawwhat;
// 	drawwhat           = AllBoxes;
// 	showdecs           = 1;
// 	show_labels        = false;
// 	show_indices       = false;
// 	needtodraw         = 1;
// 	Displayer *olddp   = dp;
// 	dp                 = tdp;
// 	DrawPaper(data, ~0, 1, 5, 0);
// 	show_labels  = labels;
// 	show_indices = indices;
// 	dp           = olddp;
// 	drawwhat     = tdrawwhat;
// 	needtodraw   = ntd;
// 	showdecs     = td;
// 	paperboxdata = nullptr;
// 	return 1;
// }

/*! If fill!=0, then fill the media box with the paper color.
 * If shadow!=0, then put a black drop shadow under a filled media box,
 * at an offset pixel length shadow.
 */
void PartitionInterface::DrawRegion(Region *region, bool arrow)
{
	if (!region) return;

	double w = InterfaceManager::GetDefault()->ScreenLine();
	bool shadow = false;
	double sshadow = 5*w;

	dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
	dp->LineWidthScreen(w);

	flatpoint p[4];

	p[0] = flatpoint(region->minx, region->miny);
	p[1] = flatpoint(region->minx, region->maxy);
	p[2] = flatpoint(region->maxx, region->maxy);
	p[3] = flatpoint(region->maxx, region->miny);

	dp->FillAttributes(LAXFILL_Solid, LAXFILL_Nonzero);

	// draw black shadow
	//dp->NewFG(255,0,0);
	//dp->NewBG(~0);
	if (shadow) {
		dp->NewFG(0,0,0);
		dp->PushAxes();
		dp->ShiftScreen(sshadow,sshadow);
		dp->drawlines(p,4,1,1);
		dp->PopAxes();
		dp->LineWidthScreen(w);
	}
		
	// draw white fill or plain outline
	if (shadow) {
		dp->NewFG(settings->outline_color);
		dp->NewBG(~0);
		dp->drawlines(p,4,1,2);
	} else {
		dp->NewFG(settings->outline_color);
		dp->drawlines(p,4,1,0);
	}

	// draw orientation arrow
	// if (arrow) {
	// 	flatpoint p1 = dp->realtoscreen((p[0] + p[3]) / 2);
	// 	flatpoint p2 = dp->realtoscreen((p[1] + p[2]) / 2);
	// 	flatpoint v  = p2 - p1;
	// 	flatpoint v2 = transpose(v);
	// 	p1 = p1 + .1 * v;
	// 	p2 = p2 - .1 * v;
	// 	dp->DrawScreen();
	// 	dp->LineWidthScreen(w);
	// 	dp->drawline(p1, p2);
	// 	dp->drawline(p2, p2 - v2 * .2 - v * .2);
	// 	dp->drawline(p2, p2 + v2 * .2 - v * .2);
	// 	dp->DrawReal();
	// 	dp->LineWidthScreen(w);
	// }

	dp->NewFG(1.,0.,0.);
	dp->LineWidthScreen(ScreenLine());
	for (int c = 0 ; c < region->cuts.n; c++) {
		dp->drawline(region->cuts.e[c]->from, region->cuts.e[c]->to);
	}

	// draw inset handles
	if (region == current_region) {
		double r = settings->arrow_size * ScreenLine() / dp->Getmag();
		for (int c = 0 ; c < region->cuts.n; c++) {
			PaperCut *cut = region->cuts.e[c];
			if (cut->cut_type == PaperCut::CUT_InsetTop) {
				dp->drawthing(region->BBoxPoint(.5,1, false) + flatpoint(0,r), r,r, 0, THING_Arrow_Down);
			} else if (cut->cut_type == PaperCut::CUT_InsetRight) {
				dp->drawthing(region->BBoxPoint(1,.5, false) + flatpoint(r,0), r,r, 0, THING_Arrow_Left);
			} else if (cut->cut_type == PaperCut::CUT_InsetBottom) {
				dp->drawthing(region->BBoxPoint(.5,0, false) + flatpoint(0,-r), r,r, 0, THING_Arrow_Up);
			} else if (cut->cut_type == PaperCut::CUT_InsetLeft) {
				dp->drawthing(region->BBoxPoint(0,.5, false) + flatpoint(-r,0), r,r, 0, THING_Arrow_Right);
			}
		}
	}

	flatpoint ang = dp->realtoscreen(p[3]) - dp->realtoscreen(p[0]); // vector pointing positive x

	if (show_labels) {
		if (!settings->font) { settings->font = laidout->defaultlaxfont; settings->font->inc_count(); }
		dp->DrawScreen();
		dp->font(settings->font, UIScale() * settings->font->textheight());
		double th = dp->textheight();
		dp->NewFG(128,128,128);
		flatpoint center = dp->realtoscreen((p[1] + p[2])/2);
		double y = center.y+th;
		// write paper name
		dp->textout(-atan2(ang.y, ang.x), center.x,y, region->label.c_str(),-1,LAX_HCENTER|LAX_BOTTOM);
		y += th;
		// write out dimensions
		double scale = region->xaxis().norm();
		if (fabs(scale-1.0) > 1e-5) {
			char str[20];
			sprintf(str, "%d%%", int(scale*100));
			dp->textout(-atan2(ang.y, ang.x), center.x,y, str,-1, LAX_HCENTER|LAX_BOTTOM);
			y += th;
		}
		//write paper label
		if (!region->label.IsEmpty()) dp->textout(-atan2(ang.y, ang.x), center.x,y, region->label.c_str(),-1, LAX_HCENTER|LAX_BOTTOM);
		dp->DrawReal();
	}

	if (show_indices) {
		if (!settings->font) { settings->font = laidout->defaultlaxfont; settings->font->inc_count(); }
		int index = region->index;
		if (index >= 0) {
			double th = dp->textheight();
			dp->DrawScreen();
			dp->font(settings->font);
			flatpoint center = dp->realtoscreen((p[0] + p[2])/2);
			flatpoint v = dp->realtoscreen(p[0]) - dp->realtoscreen(p[1]);
			double h = v.norm() * .25;
			v.normalize();
			dp->fontsize(h);
			dp->NewFG(128,128,128);
			char str[20];
			sprintf(str, "%d", index+1);
			double angle = -atan2(ang.y, ang.x);
			dp->textout(angle, center.x,center.y, str,-1);
			dp->LineWidth(h*.1);
			dp->drawline(center + v * h *.6 - v.transpose() * h*.5, center + v * h *.6 + v.transpose() * h*.5); // underline
			dp->DrawReal();
			dp->fontsize(th);
		}
	}
	
	if (region->work_and_turn != SIGT_None) {
		dp->NewFG(.8, .8, 1.0);
		dp->LineWidthScreen(w);
		flatpoint f,b;
		if (region->work_and_turn == SIGT_Work_and_Turn || region->work_and_turn == SIGT_Work_and_Turn_BF) {
			dp->drawline(region->BBoxPoint(.5, 0, true), region->BBoxPoint(.5,1.0, true));
			if (region->work_and_turn == SIGT_Work_and_Turn) {
				f = region->BBoxPoint(.25, .5, true);
				b = region->BBoxPoint(.75, .5, true);
			} else {
				f = region->BBoxPoint(.75, .5, true);
				b = region->BBoxPoint(.25, .5, true);
			}
		} else {
			dp->drawline(region->BBoxPoint(0, .5, true), region->BBoxPoint(1.0, .5, true));
			if (region->work_and_turn == SIGT_Work_and_Tumble) {
				f = region->BBoxPoint(.5, .25, true);
				b = region->BBoxPoint(.5, .75, true);
			} else {
				f = region->BBoxPoint(.5, .75, true);
				b = region->BBoxPoint(.5, .25, true);
			}
		}
		if (!settings->font) { settings->font = laidout->defaultlaxfont; settings->font->inc_count(); }
		dp->font(settings->font, 1.5);
		dp->textout(0., f.x, f.y, _("FRONT"), -1, LAX_FLIP);
		dp->textout(0., b.x, b.y, _("BACK" ), -1, LAX_FLIP);
	}

	// dp->DrawScreen();
	// dp->LineWidthScreen(w);
	// dp->DrawReal();
}

int PartitionInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw = 0;
	if (!main_region) return 0;

	//showdecs:
	// 0 do not draw
	// 1 draw with fill
	// 2 draw without fill
	if (showdecs) {
		dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);

		// PaperBox *box;
		// flatpoint p[4];
		// if (maybebox) {
		// 	box=maybebox->box;
		// 	dp->PushAndNewTransform(maybebox->m());
		// 	dp->LineWidthScreen(1);
		// 	dp->drawthing(box->media.BBoxPoint(.5,.5), box->media.boxwidth()/20, box->media.boxwidth()/20, 0, THING_Plus);
		// 	dp->LineAttributes(-1,LineDoubleDash,CapButt,JoinMiter);
		// 	dp->Dashes(5);
		// 	p[0]=flatpoint(box->media.minx,box->media.miny);
		// 	p[1]=flatpoint(box->media.minx,box->media.maxy);
		// 	p[2]=flatpoint(box->media.maxx,box->media.maxy);
		// 	p[3]=flatpoint(box->media.maxx,box->media.miny);
		// 	dp->drawlines(p,4,1,0);
		// 	dp->PopAxes(); 
		// }

		
		DrawRegion(main_region, true);
	}

	if (show_hover_line) {
		dp->NewFG(1.,.5,.5);
		dp->LineWidthScreen(2*ScreenLine());
		dp->drawline(hover_from, hover_to);
	}

	return 0;
}

int PartitionInterface::scan(int x,int y, Region *start_here, int &edge_ret, flatpoint &p_ret, double &dist, Region *&region_ret)
{
	if (!start_here) start_here = main_region;
	if (!start_here) return PARTITION_None;

	flatpoint fp = dp->screentoreal(x,y);
	DBGL("  - "<<fp.x<<','<<fp.y);
	fp = start_here->transformPointInverse(fp);
	DBGL("  - "<<fp.x<<','<<fp.y);

	flatpoint p[4];
	p[0] = start_here->BBoxPoint(0,1, false);
	p[1] = start_here->BBoxPoint(1,1, false);
	p[2] = start_here->BBoxPoint(1,0, false);
	p[3] = start_here->BBoxPoint(0,0, false);

#define HOVER_TOP     (-100)
#define HOVER_RIGHT   (-101)
#define HOVER_BOTTOM  (-102)
#define HOVER_LEFT    (-103)

	// check region boundaries
	int edge = -1;
	flatpoint pp;
	const int box_edges[] = { HOVER_TOP, HOVER_RIGHT, HOVER_BOTTOM, HOVER_LEFT };
	for (int c = 0; c < 4; c++) {
		pp = nearest_to_segment(fp, p[c],p[(c+1)%4], true);
		double d = (pp-fp).norm();
		if (d < dist) {
			dist = d;
			edge = box_edges[c];
			p_ret = pp;
		}
	}

	// check explicit cuts
	for (int c = 0; c < start_here->cuts.n; c++) {
		pp = nearest_to_segment(fp, start_here->cuts.e[c]->from, start_here->cuts.e[c]->to, true);
		double d = (pp-fp).norm();
		if (d < dist) {
			dist = d;
			p_ret = pp;
			// edge = box_edges[c];
		}
	}

	edge_ret = edge;

	for (int c = 0; c < start_here->child_regions.n; c++) {
		scan(x,y, start_here->child_regions.e[c], edge_ret, p_ret, dist, region_ret);
	}

	DBGL("partition scan ("<<fp.x<<','<<fp.y<<") over edge: "<<edge);

	if (edge != -1) return PARTITION_NewCut;
	return PARTITION_None;
}

/*! Return end point for cut. */
Laxkit::flatpoint Region::DetectNewCutBoundary(Laxkit::flatvector cut_from, Laxkit::flatvector dir, int ignore, int *on_cut_ret)
{
	Region *region = this;
	double max_len = (region->BBoxPoint(0,0,false) - region->BBoxPoint(1,1,false)).norm();
	flatvector p2 = cut_from + max_len * dir;

	double dist = 100000000;
	flatline l(cut_from, p2);
	flatpoint pp;
	double t;
	double on_cut = -1;
	flatpoint on_p;

	for (int c = 0; c < cuts.n; c++) {
		if (c == ignore) continue;
		PaperCut *cut = cuts.e[c];
		int result = segmentandline(cut->from, cut->to, l, pp, &t);
		double d = (pp - cut_from).norm();
		if (result && t > 0 && d < dist && d > .05) {
			dist = d;
			on_cut = c;
			on_p = pp;
		}
	}

	if (on_cut == -1) { // search edges
		flatpoint p[4];
		p[0] = BBoxPoint(0,1, false);
		p[1] = BBoxPoint(1,1, false);
		p[2] = BBoxPoint(1,0, false);
		p[3] = BBoxPoint(0,0, false);

		dist = 100000000;
		const int box_edges[] = { HOVER_TOP, HOVER_RIGHT, HOVER_BOTTOM, HOVER_LEFT };
		for (int c = 0; c < 4; c++) {
			if (box_edges[c] == ignore) continue;
			int result = segmentandline(p[c],p[(c+1)%4], l, pp, &t);
			double d = (pp - cut_from).norm();
			if (result && t > 0 && d < dist && d > .05) {
				dist = d;
				on_cut = box_edges[c];
				on_p = pp;
			}
		}
	}

	if (on_cut_ret) *on_cut_ret = on_cut;
	return on_p;
}

/*! Add maybe box if shift is down.
 */
int PartitionInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

	if (!main_region) {
		main_region = new Region();
		current_region = main_region;
		needtodraw = 1;
		return 0;
	}

	Region *over_region = nullptr;
	int edge = -1;
	double dist = 10000000;
	flatpoint p;
	int over = scan(x, y, main_region, edge, p, dist, over_region);

	if (over == PARTITION_NewCut) {
		hover_from = p; //dp->screentoreal(x,y);
		flatvector dir;
		if      (edge == HOVER_TOP)    dir.y = -1;
		else if (edge == HOVER_RIGHT)  dir.x = -1;
		else if (edge == HOVER_BOTTOM) dir.y =  1;
		else if (edge == HOVER_LEFT)   dir.x =  1;

		int on_cut = -1;
		hover_to = (over_region ? over_region : current_region)->DetectNewCutBoundary(hover_from, dir, edge, &on_cut);
		if (on_cut != -1) {
			show_hover_line = true;
		} else {
			show_hover_line = true;
		}
		buttondown.down(d->id,LEFTBUTTON,x,y, over);
	}

	return 0;
}

int PartitionInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int dragged = buttondown.up(d->id,LEFTBUTTON);
	if (dragged < DraggedThreshhold()) {
		// curboxes.flush();
		// curboxes.pushnodup(curbox, 0);
		// needtodraw = 1;
	}

	return 0;
}

/*! Adjust curboxes so that they snap to other non-curboxes.
 * Return number of boxes changed.
 */
int PartitionInterface::SnapBoxes()
{
	// if (!papergroup) return 0;
	// if (!maybebox && curboxes.n == papergroup->papers.n) return 0; //all boxes are selected.. todo: maybe we should snap to viewport spread/pages?

	// double threshhold = snap_px_threshhold / dp->Getmag();

	// NumStack<double> xx,yy;
	// PaperBoxData *box = nullptr;
	// for (int c=-1; c<curboxes.n; c++) {
	// 	if (c == -1) {
	// 		if (!maybebox) continue;
	// 		box = maybebox;
	// 	} else {
	// 		if (maybebox) continue;
	// 		box = curboxes.e[c];
	// 	}
	// 	//build points that we need to check for snapping
	// 	flatpoint p = box->BBoxPoint(0,0,true);
	// 	xx.pushnodup(p.x);
	// 	yy.pushnodup(p.y);
	// 	p = box->BBoxPoint(0,1,true);
	// 	xx.pushnodup(p.x);
	// 	yy.pushnodup(p.y);
	// 	p = box->BBoxPoint(1,1,true);
	// 	xx.pushnodup(p.x);
	// 	yy.pushnodup(p.y);
	// 	p = box->BBoxPoint(1,0,true);
	// 	xx.pushnodup(p.x);
	// 	yy.pushnodup(p.y);
	// 	p = box->BBoxPoint(.5,.5,true);
	// 	xx.pushnodup(p.x);
	// 	yy.pushnodup(p.y);
	// }

	// flatpoint d, p;
	// flatpoint dp;
	// double dd;
	// bool do_x = true;
	// bool do_y = true;
	// flatpoint pgp[5];
	// for (int c=0; c<papergroup->papers.n; c++) {
	// 	if (curboxes.Contains(papergroup->papers.e[c])) continue;

	// 	pgp[0] = papergroup->papers.e[c]->BBoxPoint(0,1,true);
	// 	pgp[1] = papergroup->papers.e[c]->BBoxPoint(1,1,true);
	// 	pgp[2] = papergroup->papers.e[c]->BBoxPoint(1,0,true);
	// 	pgp[3] = papergroup->papers.e[c]->BBoxPoint(0,0,true);
	// 	pgp[4] = papergroup->papers.e[c]->BBoxPoint(.5,.5,true);

	// 	for (int c2=0; c2<xx.n && (do_x || do_y); c2++) {
	// 		for (int c3 = 0; c3 < 5; c3++) {
	// 			p = pgp[c3];
	// 			if (do_x) {
	// 				dd = fabs(xx.e[c2] - p.x);
	// 				if (dd < threshhold) {
	// 					do_x = false;
	// 					dp.x = p.x - xx.e[c2];
	// 				}
	// 			}
	// 			if (do_y) {
	// 				dd = fabs(yy.e[c2] - p.y);
	// 				if (dd < threshhold) {
	// 					do_y = false;
	// 					dp.y = p.y - yy.e[c2];
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// if (do_x == false || do_y == false) {
	// 	//we matched a snap, so adjust transforms
	// 	if (maybebox) {
	// 		maybebox->origin(maybebox->origin() + dp);
	// 	} else {
	// 		for (int c=0; c<curboxes.n; c++) {
	// 			curboxes.e[c]->origin(curboxes.e[c]->origin() + dp);
	// 		}
	// 	}
	// 	return curboxes.n;
	// }
	return 0;
}


int PartitionInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	// DBG flatpoint fpp = dp->screentoreal(x, y);
	// DBGL("mm *****ARG**** "<<x<<","<<y<<"  "<<fpp.x<<","<<fpp.y);

	if (!buttondown.any()) {
		Region *over_region = nullptr;
		int edge = -1;
		double dist = 10000000;
		flatpoint p;
		
		int over = scan(x, y, main_region, edge, p, dist, over_region);
		DBGL("over box: "<<over<<"  edge: "<<edge);

		if (over == PARTITION_NewCut) {
			hover_from = p; //dp->screentoreal(x,y);
			flatvector dir;
			if      (edge == HOVER_TOP)    dir.y = -1;
			else if (edge == HOVER_RIGHT)  dir.x = -1;
			else if (edge == HOVER_BOTTOM) dir.y =  1;
			else if (edge == HOVER_LEFT)   dir.x =  1;

			int on_cut = -1;
			hover_to = (over_region ? over_region : current_region)->DetectNewCutBoundary(hover_from+.0001*dir, dir, edge, &on_cut);
			DBGL("  detect booundary, dir: "<<dir.x<<','<<dir.y<<"  cut: "<<on_cut);
			bool old_show_hover_line = show_hover_line;
			if (on_cut != -1) {
				show_hover_line = true;
			} else {
				show_hover_line = true;
			}
			if (edge != hover_cut || old_show_hover_line != show_hover_line || over_region != hover_region) {
				hover_cut = edge;
				hover_region = over_region;
			}
			needtodraw = 1;
		}

		return 0;
	}



	// if (curboxes.n == 0) return 1;

	int mx, my;
	buttondown.move(mouse->id, x,y, &mx,&my);

	// flatpoint fp = dp->screentoreal(x, y);
	// flatpoint d  = fp - dp->screentoreal(mx, my);

	// // plain or + moves curboxes (or the box given by editwhat)
	// if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
	// 	for (int c=0; c<curboxes.n; c++) {
	// 		curboxes.e[c]->origin(curboxes.e[c]->origin()+d);
	// 	}
	// 	if ((search_snap && (state&LAX_STATE_MASK)==0)
	// 			|| (!search_snap && (state&LAX_STATE_MASK)==ShiftMask)) {
	// 		SnapBoxes();
	// 	}
	// 	needtodraw=1;
	// 	return 0;
	// }

	return 0;
}

Laxkit::ShortcutHandler *PartitionInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc = new ShortcutHandler(whattype());

	// sc->Add(PARTITION_DupPapers,   ' ',0,1,         "DupPapers",_("Duplicate selected papers"),nullptr,0);
	// sc->Add(PARTITION_Select,      'a',0,0,         "Select",   _("Select or deselect all"),nullptr,0);
	// sc->Add(PARTITION_Decorations, 'd',0,0,         "Decs",     _("Toggle decorations"),nullptr,0);
	// sc->Add(PARTITION_Delete,      LAX_Bksp,0,0,    "Delete",   _("Delete selected"),nullptr,0);
	// sc->AddShortcut(LAX_Del,0,0, PARTITION_Delete);
	// sc->Add(PARTITION_Rectify,     'o',0,0,         "Rectify",  _("Make the axes horizontal and vertical"),nullptr,0);
	// sc->Add(PARTITION_Rotate,      'r',0,0,         "Rotate",   _("Rotate selected by 90 degrees"),nullptr,0);
	// sc->Add(PARTITION_RotateCC,    'R',ShiftMask,0, "RotateCC", _("Rotate selected by 90 degrees in the other direction"),nullptr,0);
	// sc->Add(PARTITION_ToggleLandscape, 'l',0,0, "ToggleLandscape", _("Toggle landscape"),nullptr,0);
	// sc->AddAction(PARTITION_Portrait, "Portrait", _("Portrait"), nullptr, 0, 0);
	// sc->AddAction(PARTITION_Landscape, "Landscape", _("Landscape"), nullptr, 0, 0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int PartitionInterface::PerformAction(int action)
{
	if (action == PARTITION_Decorations) {
		showdecs = !showdecs;
		// showdecs++;
		// if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

	// } else if (action == PARTITION_ToggleSnap) {
	// 	search_snap = !search_snap;
	// 	PostMessage(search_snap ? _("Snap on (hold shift to not snap)") : _("Snap off (hold shift to snap)"));
	// 	return 0;

	} else if (action == PARTITION_ToggleLabels) {
		show_labels = !show_labels;
		PostMessage(show_labels ? _("Show labels") : _("Don't show labels"));
		needtodraw = 1;
		return 0;

	// } else if (action==PARTITION_ToggleIndices) {
	// 	show_indices = !show_indices;
	// 	PostMessage(show_indices ? _("Show indices") : _("Don't show indices"));
	// 	needtodraw = 1;
	// 	return 0;

	// } else if (action == PARTITION_ToggleLandscape) {
	// 	if (!curboxes.n) return 0;
	// 	for (int c=0; c<curboxes.n; c++) {
	// 		curboxes.e[c]->box->landscape(!curboxes.e[c]->box->landscape());
	// 	}
	// 	needtodraw = 1;
	// 	PostMessage(_("Orientation toggled."));
	// 	return 0;

	// } else if (action == PARTITION_Landscape) {
	// 	if (!curboxes.n) return 0;
	// 	for (int c=0; c<curboxes.n; c++) {
	// 		curboxes.e[c]->box->landscape(true);
	// 	}
	// 	needtodraw = 1;
	// 	PostMessage(_("Landscape orientation"));
	// 	return 0;
	
	// } else if (action == PARTITION_Portrait) {
	// 	if (!curboxes.n) return 0;
	// 	for (int c=0; c<curboxes.n; c++) {
	// 		curboxes.e[c]->box->landscape(false);
	// 	}
	// 	needtodraw = 1;
	// 	PostMessage(_("Portrait orientation"));
	// 	return 0;

	} else if (action == PARTITION_Insets) {
		if (!main_region) return 0;
		if (current_region->HasInsets()) {
			current_region->RemoveInsets();
			PostMessage(_("Insets removed."));
		} else {
			current_region->AddInsets(0,0,0,0);
			PostMessage(_("Insets added."));
		}

		needtodraw = 1;
		return 0;

	} else if (action == PARTITION_Tiles) {
		if (!main_region) return 0;
		return 0;

	} else if (action == PARTITION_NoWorkAndTurn) {
		if (!main_region) return 0;
		main_region->work_and_turn = SIGT_None;
		needtodraw = 1;
		return 0;

	} else if (action == PARTITION_WorkAndTurn) {
		if (!main_region) return 0;
		main_region->work_and_turn = SIGT_Work_and_Turn;
		PostMessage(_("Work and turn"));
		needtodraw = 1;
		return 0;

	} else if (action == PARTITION_WorkAndTurnBF) {
		if (!main_region) return 0;
		main_region->work_and_turn = SIGT_Work_and_Turn_BF;
		PostMessage(_("Work and turn, opposite front and back"));
		needtodraw = 1;
		return 0;

	} else if (action == PARTITION_WorkAndTumble) {
		if (!main_region) return 0;
		main_region->work_and_turn = SIGT_Work_and_Tumble;
		PostMessage(_("Work and tumble"));
		needtodraw = 1;
		return 0;

	} else if (action == PARTITION_WorkAndTumbleBG) {
		if (!main_region) return 0;
		main_region->work_and_turn = SIGT_Work_and_Tumble_BF;
		PostMessage(_("Work and tumble, opposite front and back"));
		needtodraw = 1;
		return 0;
	}

	return 1;
}

int PartitionInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	// DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch == LAX_Esc) {
		// if (curboxes.n) {
		// 	curboxes.flush();
		// 	if (curbox) { curbox->dec_count(); curbox = nullptr; }
		// 	return 0;
		// }
		return 1;
	}

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state & LAX_STATE_MASK, buttondown.any() ? 1 : 0);
	if (action >= 0) {
		return PerformAction(action);
	}

	return 1;
}

int PartitionInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}


} // namespace Laidout

