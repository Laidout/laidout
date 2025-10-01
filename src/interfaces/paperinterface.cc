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


#include "paperinterface.h"
#include "../language.h"
#include "../ui/viewwindow.h"
#include "../ui/papersizewindow.h"
#include "../core/drawdata.h"
#include "../dataobjects/printermarks.h"

#include <lax/inputdialog.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/colors.h>
#include <lax/interfaces/interfacemanager.h>


using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



//------------------------------------ Local functions ---------------------------------

//! Return a new char[] with a name like "Paper Group 3". The number is incremented with each call.
/*! This will not create a name already used by a papergroup.
 */
char *new_paper_group_name()
{
	ResourceType *groups = laidout->resourcemanager->FindType("PaperGroup");
	char *name = newstr(_("Paper Group"));
	if (!groups || groups->NumResources() == 0) return name;
	groups->MakeNameUnique(name);
	return name;
}

//------------------------------------- PaperInterface --------------------------------------
	
/*! \class PaperInterface 
 * \brief Interface to arrange an arbitrary spread to print on one or more sheets of paper.
 *
 * This can be used to position a single paper over real space, or set to allow any
 * number of papers. Also can be used to set rectangles for crop, bleed, and trim.
 */

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	snapto      = MediaBox;
	editwhat    = MediaBox;
	drawwhat    = AllBoxes; //MediaBox;
	showdecs    = 0;
	show_labels = true;
	show_indices = false;
	sync_physical_size = true;
	edit_back_indices = false;
	edit_margins = false;
	search_snap = true;
	snap_px_threshhold = 5 * InterfaceManager::GetDefault()->ScreenLine();
	snap_running_angle = 0;
	font        = nullptr;
	default_outline_color.rgbf(1., 0., 1.);
	default_fill.rgbf(1.,1.,1.);

	curbox = maybebox = nullptr;

	papergroup   = nullptr;
	doc          = nullptr;

	sc = nullptr;
}

PaperInterface::PaperInterface(int nid,Displayer *ndp)
	: PaperInterface(nullptr, nid, ndp)
{
}

PaperInterface::~PaperInterface()
{
	DBG cerr <<"PaperInterface destructor.."<<endl;

	if (maybebox) maybebox->dec_count();
	if (curbox) { curbox->dec_count(); curbox=nullptr; }
	if (papergroup) papergroup->dec_count();
	if (doc) doc->dec_count();
	if (sc) sc->dec_count();
	if (font) font->dec_count();
}

const char *PaperInterface::Name()
{ return _("Paper Group Tool"); }

enum PaperInterfaceActions {
	PAPERI_PaperSize = 1,
	PAPERI_ToggleLandscape,
	PAPERI_Landscape,
	PAPERI_Portrait,
	PAPERI_NewPaperGroup,
	PAPERI_RenamePaperGroup,
	PAPERI_DeletePaperGroup, //from resource list
	PAPERI_Print,
	PAPERI_RegistrationMark,
	PAPERI_GrayBars,
	PAPERI_CutMarks,
	PAPERI_ResetScaling,
	PAPERI_ResetAngle,
	PAPERI_ToggleSnap,
	PAPERI_ToggleLabels,
	PAPERI_ToggleIndices,
	PAPERI_ToggleBackIndices,
	PAPERI_ToggleSyncSizes,
	PAPERI_Swap_Orientation,
	PAPERI_CreateImposition,
	PAPERI_DupPapers,
	PAPERI_EditMargins,
	PAPERI_EditTrim,

	PAPERI_Select,
	PAPERI_Decorations,
	PAPERI_Delete, //curboxes
	PAPERI_Rectify,
	PAPERI_Rotate,
	PAPERI_RotateCC,

	PAPERI_first_pagesize   = 1000,
	PAPERI_first_papergroup = 2000,
	PAPERI_Menu_Papergroup  = 1,

	PAPERI_Trim_Top    = 1000000,
	PAPERI_Trim_Right  = 1000001,
	PAPERI_Trim_Bottom = 1000002,
	PAPERI_Trim_Left   = 1000003,

	PAPERI_Margin_Top    = 2000000,
	PAPERI_Margin_Right  = 2000001,
	PAPERI_Margin_Bottom = 2000002,
	PAPERI_Margin_Left   = 2000003,

	PAPERI_Art_Top    = 3000000,
	PAPERI_Art_Right  = 3000001,
	PAPERI_Art_Bottom = 3000002,
	PAPERI_Art_Left   = 3000003,

	PAPERI_Bleed_Top    = 4000000,
	PAPERI_Bleed_Right  = 4000001,
	PAPERI_Bleed_Bottom = 4000002,
	PAPERI_Bleed_Left   = 4000003,

	PAPERI_Printable_Top    = 5000000,
	PAPERI_Printable_Right  = 5000001,
	PAPERI_Printable_Bottom = 5000002,
	PAPERI_Printable_Left   = 5000003,

	PAPERI_MAX = 5000
};

const char *HoverMessage(int msg)
{
	switch(msg) {
		case PAPERI_Trim_Top:      return _("Trim Top");
		case PAPERI_Trim_Right:    return _("Trim Right");
		case PAPERI_Trim_Bottom:   return _("Trim Bottom");
		case PAPERI_Trim_Left:     return _("Trim Left");
		case PAPERI_Margin_Top:    return _("Margin Top");
		case PAPERI_Margin_Right:  return _("Margin Right");
		case PAPERI_Margin_Bottom: return _("Margin Bottom");
		case PAPERI_Margin_Left:   return _("Margin Left");
	}
	return nullptr;
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PaperInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	rx = x;
	ry = y;

	if (!menu) menu=new MenuInfo(_("Paper Interface"));
	else menu->AddSep(_("Papers"));

	if (allow_margin_edit) menu->AddToggleItem(_("Edit margin"), PAPERI_EditMargins, 0, edit_margins);
	if (allow_trim_edit)   menu->AddToggleItem(_("Edit trim"),   PAPERI_EditTrim,    0, edit_trim);
	menu->AddSep();
	menu->AddToggleItem(_("Snap"),         PAPERI_ToggleSnap,     0, search_snap);
	menu->AddToggleItem(_("Show labels"),  PAPERI_ToggleLabels,   0, show_labels);
	menu->AddToggleItem(_("Show indices"), PAPERI_ToggleIndices,  0, show_indices);
	menu->AddToggleItem(_("Sync physical sizes"), PAPERI_ToggleSyncSizes,0, sync_physical_size);
	menu->AddSep();

	if (papergroup) {
		menu->AddItem(_("Add Registration Mark"),PAPERI_RegistrationMark);
		menu->AddItem(_("Add Gray Bars"),PAPERI_GrayBars);
		menu->AddItem(_("Add Cut Marks"),PAPERI_CutMarks);
		menu->AddSep();
		if (full_menu) {
			menu->AddItem(_("Create Net from group"), PAPERI_CreateImposition);
			menu->AddSep();
		}
	}

	if (papergroup && curboxes.n) {

		menu->AddItem(_("Reset paper scaling"),PAPERI_ResetScaling);
		menu->AddItem(_("Reset paper angle"),PAPERI_ResetAngle);

		menu->AddItem(_("Swap orientation"),PAPERI_ToggleLandscape);
		menu->AddItem(_("Paper Size"),PAPERI_PaperSize);
		menu->SubMenu(_("Paper Size"));
		for (int c=0; c<laidout->papersizes.n; c++) {
			if (!strcasecmp(laidout->papersizes.e[c]->name, "Whatever")) continue;
			menu->AddToggleItem(laidout->papersizes.e[c]->name, PAPERI_first_pagesize+c,
					0, strcasecmp(curboxes.e[0]->box->paperstyle->name, laidout->papersizes.e[c]->name) == 0);
		}
		menu->EndSubMenu();
		if (full_menu) {
			menu->AddItem(_("Print with paper group"),PAPERI_Print);
			menu->AddSep();	
		}
	}

	if (full_menu) {
		laidout->resourcemanager->ResourceMenu("PaperGroup", false, menu, 0, PAPERI_Menu_Papergroup, papergroup);

		menu->AddItem(_("New paper group..."),PAPERI_NewPaperGroup);
		if (papergroup) menu->AddItem(_("Rename current paper group..."),PAPERI_RenamePaperGroup);
		if (papergroup) menu->AddItem(_("Delete current paper group..."),PAPERI_DeletePaperGroup);
	}

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int PaperInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"newname")) {
		if (!papergroup) return 0;
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (isblank(s->str)) return 0;
		if (papergroup->name) makestr(papergroup->name,s->str);
		else if (papergroup->Name) makestr(papergroup->Name,s->str);

		return 0;

	} else if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item
		int info = s->info4;

		if (info == PAPERI_Menu_Papergroup) {
			Resource *resource = laidout->resourcemanager->FindResourceFromRID(i, "PaperGroup");
			if (!resource) return 0;

			Clear(nullptr);
			papergroup = dynamic_cast<PaperGroup*>(resource->GetObject());
			papergroup->inc_count();
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;
		}

		if (     i == PAPERI_ToggleSnap
			  || i == PAPERI_ToggleLabels
			  || i == PAPERI_ToggleIndices
			  || i == PAPERI_ToggleBackIndices
			  || i == PAPERI_ToggleSyncSizes
			  || i == PAPERI_ResetScaling
			  || i == PAPERI_ResetAngle
			  || i == PAPERI_CreateImposition
			  || i == PAPERI_EditMargins
			  || i == PAPERI_EditTrim
			  || i == PAPERI_ToggleLandscape
			  || i == PAPERI_DeletePaperGroup
		   ) {
			PerformAction(i);
			return 0;

		// } else if (i==PAPERI_Portrait) {
		// 	 //portrait
		// 	return 0;

		// } else if (i==PAPERI_Landscape) {
		// 	 //landscape
		// 	return 0;
		
		} else if (i==PAPERI_Print) {
			 //print with the active paper group
			curwindow->win_parent->Event(nullptr,"print");//***hack Hack HACK
			return 0;

		} else if (i >= PAPERI_first_pagesize && i < PAPERI_first_pagesize + 1000) {
			// paper size
			i -= PAPERI_first_pagesize;
			if (i >= laidout->papersizes.n-1) return 1;
			if (!strcasecmp(laidout->papersizes.e[i]->name, "Custom")) {
				//popup paper size window
				app->rundialog(new PaperSizeWindow(nullptr, "Paper", "Paper", 0, object_id,"custompaper", curboxes.e[0]->box->paperstyle, 
												false, true, false, false)); //mod in place, dpi, color

			} else {
				PaperStyle *newpaper = (PaperStyle *)laidout->papersizes.e[i]; //->duplicate();
				for (int c = 0; c < curboxes.n; c++) {
					PaperStyle *paper = dynamic_cast<PaperStyle*>(newpaper->duplicate());
					curboxes.e[c]->box->Set(paper);
					curboxes.e[c]->setbounds(&curboxes.e[c]->box->media);
					paper->dec_count();
				}
				needtodraw = 1;
			}
			return 0;

		} else if (i == PAPERI_NewPaperGroup) {
			// New paper group...
			if (papergroup) papergroup->dec_count();
			papergroup = new PaperGroup;
			papergroup->name = new_paper_group_name();
			laidout->resourcemanager->AddResource("PaperGroup", papergroup, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
			//papergroup->dec_count();
			
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;

		} else if (i == PAPERI_RenamePaperGroup) {
			//***Rename paper group...
			if (!papergroup) return 0;
			InputDialog *i=new InputDialog(nullptr,_("New paper group name"),_("New paper group name"),ANXWIN_CENTER,
									 0,0,0,0,0,
									 nullptr,object_id,"newname",
									 papergroup->name?papergroup->name:papergroup->Name, _("Name:"),
									 _("Ok"),BUTTON_OK,
									 _("Cancel"),BUTTON_CANCEL);
			app->rundialog(i);
			return 0;

		} else if (i == PAPERI_RegistrationMark) {
			if (!papergroup) return 0;
			SomeData *obj= RegistrationMark(18,1);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			papergroup->objs.push(obj);
			obj->dec_count();
			return 0;

		} else if (i == PAPERI_GrayBars) {
			if (!papergroup) return 0;
			SomeData *obj= BWColorBars(18,LAX_COLOR_GRAY);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			papergroup->objs.push(obj);
			obj->dec_count();
			return 0;
		}
		return 0;

	} else if (!strcmp(mes,"custompaper")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		if (!s || !s->object) return 0;
		PaperStyle *newpaper = dynamic_cast<PaperStyle*>(s->object);
		if (!newpaper) return 0;
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->box->Set(newpaper);
			curboxes.e[c]->setbounds(&curboxes.e[c]->box->media);
		}
		needtodraw = 1;
		return 0;
	
	} else if (strstr(mes, "trim_") == mes) {
		mes += 5;
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		char *endptr = nullptr;
		double d = strtod(s->str, &endptr);
		if (curbox && endptr != s->str) {
			PaperBox *box = curbox->box;
			if (*mes == 'l') {
				box->trim.minx = d;
			} else if (*mes == 'r') {
				box->trim.maxx = curbox->w() - d;
			} else if (*mes == 't') {
				box->trim.maxy = curbox->h() - d;
			} else if (*mes == 'b') {
				box->trim.miny = d;
			}
			needtodraw = 1;
		}

	} else if (strstr(mes, "margin_") == mes) {
		mes += 7;
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		char *endptr = nullptr;
		double d = strtod(s->str, &endptr);
		if (curbox && endptr != s->str) {
			PaperBox *box = curbox->box;
			if (*mes == 'l') {
				box->margin.minx = d;
			} else if (*mes == 'r') {
				box->margin.maxx = curbox->w() - d;
			} else if (*mes == 't') {
				box->margin.maxy = curbox->h() - d;
			} else if (*mes == 'b') {
				box->margin.miny = d;
			}
			needtodraw = 1;
		}
	}
	return 1;
}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int PaperInterface::UseThisDocument(Document *ndoc)
{
	if (ndoc == doc) return 0;
	if (doc) doc->dec_count();
	doc = ndoc;
	if (ndoc) ndoc->inc_count();
	AdjustBoxInsets();
	return 0;
}

/*! PaperGroup or PaperBoxData.
 */
int PaperInterface::draws(const char *atype)
{
	return !strcmp(atype,"PaperBoxData") || !strcmp(atype,"PaperGroup");
}


//! Return a new PaperInterface if dup=nullptr, or anInterface::duplicateInterface(dup) otherwise.
/*! 
 */
anInterface *PaperInterface::duplicateInterface(anInterface *dup)//dup=nullptr
{
	if (dup==nullptr) dup = new PaperInterface(id, nullptr);
	else if (!dynamic_cast<PaperInterface *>(dup)) return nullptr;

	return anInterface::duplicateInterface(dup);
}


int PaperInterface::InterfaceOn()
{
	DBG cerr <<"paperinterfaceOn()"<<endl;
	LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
	if (lvp && papergroup) lvp->UseThisPaperGroup(papergroup);
	showdecs=2;
	needtodraw=1;
	return 0;
}

int PaperInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs=0;

	LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
	if (lvp) lvp->UseThisPaperGroup(nullptr);

	if (maybebox) { maybebox->dec_count(); maybebox=nullptr; }

	needtodraw=1;
	DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

//! Use a PaperGroup. Does NOT update viewport.
int PaperInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	PaperGroup *pg = dynamic_cast<PaperGroup *>(ndata);
	if (!pg && ndata) return 0; //was a non-null object, but not a papergroup
	Clear(nullptr);
	
	papergroup = pg;
	if (papergroup) papergroup->inc_count();
	AdjustBoxInsets();
	needtodraw = 1;

	return 1;
}

/*! Clean maybebox, curbox, curboxes, papergroup.
 * Does NOT update viewport.
 */
void PaperInterface::Clear(SomeData *d)
{
	if (maybebox) { maybebox->dec_count(); maybebox=nullptr; }
	if (curbox) { curbox->dec_count(); curbox=nullptr; }
	curboxes.flush();
	if (papergroup) { papergroup->dec_count(); papergroup=nullptr; }
}

	
/*! This will be called by viewports, and the papers will be displayed opaque with
 * drop shadow.
 */
int PaperInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
{
	PaperBoxData *data = dynamic_cast<PaperBoxData *>(tdata);
	if (!data) return 1;
	int td             = showdecs;
	bool labels        = show_labels;
	bool indices       = show_indices;
	int ntd            = needtodraw;
	int tdrawwhat      = drawwhat;
	drawwhat           = AllBoxes;
	showdecs           = 1;
	show_labels        = false;
	show_indices       = false;
	needtodraw         = 1;
	Displayer *olddp   = dp;
	dp                 = tdp;

	DrawPaper(data, ~0, 1, 5, 0);

	show_labels  = labels;
	show_indices = indices;
	dp           = olddp;
	drawwhat     = tdrawwhat;
	needtodraw   = ntd;
	showdecs     = td;
	return 1;
}

/*! If fill!=0, then fill the media box with the paper color.
 * If shadow!=0, then put a black drop shadow under a filled media box,
 * at an offset pixel length shadow.
 */
void PaperInterface::DrawPaper(PaperBoxData *data, int what, bool fill, int shadow, bool arrow)
{
	if (!data) return;

	int w = InterfaceManager::GetDefault()->ScreenLine(); //1
	if (data==curbox || curboxes.findindex(data)>=0) w=2;
	dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
	dp->LineWidthScreen(w);
	//dp->PushAndNewTransform(data->m());

	//double sshadow = shadow*dp->Getmag(); <- this one scales properly
	double sshadow = shadow;

	PaperBox *box = data->box;
	flatpoint p[4];

	if ((what & MediaBox) && (box->which & MediaBox)) {
		p[0] = flatpoint(box->media.minx,box->media.miny);
		p[1] = flatpoint(box->media.minx,box->media.maxy);
		p[2] = flatpoint(box->media.maxx,box->media.maxy);
		p[3] = flatpoint(box->media.maxx,box->media.miny);

		dp->FillAttributes(LAXFILL_Solid, LAXFILL_Nonzero);

		 //draw black shadow
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
		
		 //draw white fill or plain outline
		if (fill || shadow) { // fill and outline
			dp->NewFG(&data->outlinecolor);
			dp->NewBG(&data->color);
			dp->drawlines(p,4,1,2);
		} else { // outline only
			dp->NewFG(&data->outlinecolor);
			dp->drawlines(p,4,1,0);
		}

		 //draw orientation arrow
		if (arrow) {
			flatpoint p1 = dp->realtoscreen((p[0] + p[3]) / 2);
			flatpoint p2 = dp->realtoscreen((p[1] + p[2]) / 2);
			flatpoint v  = p2 - p1;
			flatpoint v2 = transpose(v);
			p1 = p1+.1*v;
			p2 = p2-.1*v;
			dp->DrawScreen();
			dp->LineWidthScreen(w);
			dp->drawline(p1,p2);
			dp->drawline(p2,p2-v2*.2-v*.2);
			dp->drawline(p2,p2+v2*.2-v*.2);
			dp->DrawReal();
			dp->LineWidthScreen(w);
		}

		flatpoint ang = dp->realtoscreen(p[3]) - dp->realtoscreen(p[0]);

		if (show_labels) {
			if (!font) { font = laidout->defaultlaxfont; font->inc_count(); }
			dp->DrawScreen();
			dp->font(font, UIScale() * font->textheight());
			double th = dp->textheight();
			cerr << "show labels with height: "<<th<<endl;
			dp->NewFG(128,128,128);
			flatpoint center = dp->realtoscreen((p[1] + p[2])/2);
			double y = center.y+th;
			// write paper name
			dp->textout(-atan2(ang.y, ang.x), center.x,y, box->paperstyle->name,-1,LAX_HCENTER|LAX_BOTTOM);
			y += th;
			// write out dimensions
			double scale = data->xaxis().norm();
			if (fabs(scale-1.0) > 1e-5) {
				char str[20];
				sprintf(str, "%d%%", int(scale*100));
				dp->textout(-atan2(ang.y, ang.x), center.x,y, str,-1, LAX_HCENTER|LAX_BOTTOM);
				y += th;
			}
			//write paper label
			if (!data->label.IsEmpty()) dp->textout(-atan2(ang.y, ang.x), center.x,y, data->label.c_str(),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->DrawReal();
		}

		if (show_indices) {
			if (!font) { font = laidout->defaultlaxfont; font->inc_count(); }
			int index = papergroup->papers.findindex(data);
			if (index >= 0) {
				double th = dp->textheight();
				dp->DrawScreen();
				dp->font(font);
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
	}

	// dp->DrawScreen();
	dp->LineWidthScreen(w);


	// if ((what & PrintableBox) && (box->which & PrintableBox)) DrawBox(box->printable, data, ScreenColor(.5, 0., 0., 1.), printable_offset, PAPERI_Printable_Top);
	// if ((what & ArtBox)       && (box->which & ArtBox))       DrawBox(box->art,       data, ScreenColor(.5, 0., 0., 1.), art_offset,       PAPERI_Art_Top);
	// if ((what & BleedBox)     && (box->which & BleedBox))     DrawBox(box->bleed,     data, ScreenColor(.5, 0., 0., 1.), bleed_offset,     PAPERI_Bleed_Top);
	if ((what & MarginBox)    && (box->which & MarginBox))    DrawBox(box->margin,    data, ScreenColor(.5, .5, .5, 1.), margin_offset,    PAPERI_Margin_Top);
	if ((what & TrimBox)      && (box->which & TrimBox))      DrawBox(box->trim,      data, ScreenColor(.5, 0., 0., 1.), trim_offset,      PAPERI_Trim_Top);

	dp->DrawReal();
	//dp->PopAxes(); //spread axes
}

void PaperInterface::DrawBox(const Laxkit::DoubleBBox &box, PaperBoxData *boxd, const Laxkit::ScreenColor &color, double arrow_offset, int hoveri)
{
	dp->NewFG(color);
	dp->LineWidthScreen(ScreenLine());

	flatpoint p[4];
	p[0] = flatvector(box.minx, box.miny); // ..left
	p[1] = flatvector(box.minx, box.maxy); // ..top
	p[2] = flatvector(box.maxx, box.maxy); // ..right
	p[3] = flatvector(box.maxx, box.miny); // ..bottom
	if (fabs(box.minx) > 1e-4) dp->drawline(p[0], p[1]);
	if (fabs(box.miny) > 1e-4) dp->drawline(p[0], p[3]);
	if (fabs(box.maxx - boxd->w()) > 1e-4) dp->drawline(p[2], p[3]);
	if (fabs(box.maxy - boxd->h()) > 1e-4) dp->drawline(p[2], p[1]);
	// dp->drawlines(p,4,1,0);

	// draw arrows
	if (showdecs && curbox == boxd) {
		double r = arrow_threshold * ScreenLine() / dp->Getmag();
		dp->drawthing((1-arrow_offset) * p[1] +    arrow_offset  * p[2] + flatpoint(0, r), r,r, hover_item == hoveri + 0 ? 1 : 0, THING_Arrow_Down);  //top
		dp->drawthing(   arrow_offset  * p[3] + (1-arrow_offset) * p[0] + flatpoint(0,-r), r,r, hover_item == hoveri + 2 ? 1 : 0, THING_Arrow_Up);    //bottom
		dp->drawthing(   arrow_offset  * p[2] + (1-arrow_offset) * p[3] + flatpoint( r,0), r,r, hover_item == hoveri + 1 ? 1 : 0, THING_Arrow_Left);  //right
		dp->drawthing((1-arrow_offset) * p[0] +    arrow_offset  * p[1] + flatpoint(-r,0), r,r, hover_item == hoveri + 3 ? 1 : 0, THING_Arrow_Right); //left
	}
}

/*! If which&1 draw paper outline. If which&2, draw paper objects.
 */
void PaperInterface::DrawGroup(PaperGroup *group, bool shadow, bool fill, bool arrow, int which,
								bool with_decs)
{
	int slabels = show_labels;
	int sindices = show_indices;
	if (!with_decs) {
		show_labels = false;
		show_indices = false;
	}

	if (which & 1) {
		// draw shadow under whole group
		if (shadow) {
			for (int c=0; c<group->papers.n; c++) {
				dp->PushAndNewTransform(group->papers.e[c]->m());
				DrawPaper(group->papers.e[c],MediaBox, fill,laidout->prefs.pagedropshadow,0);
				dp->PopAxes(); 
			}
		}

		// draw paper
		for (int c=0; c<group->papers.n; c++) {
			dp->PushAndNewTransform(group->papers.e[c]->m());
			DrawPaper(group->papers.e[c], drawwhat, fill,0,arrow);
			dp->PopAxes(); 
		}
	}

	// draw papergroup objects
	if (which & 2) {
		if (group->objs.n()) {
			for (int c=0; c<group->objs.n(); c++) {
				Laidout::DrawData(dp,group->objs.e(c),nullptr,nullptr,0);
			}
		}
	}

	show_labels = slabels;
	show_indices = sindices;
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int PaperInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	//showdecs:
	// 0 do not draw
	// 1 draw with fill
	// 2 draw without fill
	if (showdecs) {
		dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
		PaperBox *box;
		flatpoint p[4];
		if (maybebox) {
			box=maybebox->box;
			dp->PushAndNewTransform(maybebox->m());
			dp->LineWidthScreen(1);
			dp->drawthing(box->media.BBoxPoint(.5,.5), box->media.boxwidth()/20, box->media.boxwidth()/20, 0, THING_Plus);
			dp->LineAttributes(-1,LineDoubleDash,CapButt,JoinMiter);
			dp->Dashes(5);
			p[0]=flatpoint(box->media.minx,box->media.miny);
			p[1]=flatpoint(box->media.minx,box->media.maxy);
			p[2]=flatpoint(box->media.maxx,box->media.maxy);
			p[3]=flatpoint(box->media.maxx,box->media.miny);
			dp->drawlines(p,4,1,0);
			dp->PopAxes(); 
		}

		if (!papergroup || !papergroup->papers.n) return 0;

		DrawGroup(papergroup, 0, //shadow
							  showdecs==1?1:0, //fill
							  showdecs==2?1:0, //arrow
							  3, // which
							  true //with_decs
							  );
	}

	return 1;
}

int ScanBox(const flatpoint &pp, const DoubleBBox &box, double r, double offset, int i_offset, flatpoint *p)
{
	p[0] = flatvector(box.minx, box.miny);
	p[1] = flatvector(box.minx, box.maxy);
	p[2] = flatvector(box.maxx, box.maxy);
	p[3] = flatvector(box.maxx, box.miny);

	// dp->drawthing((1-arrow_offset) * p[1] +    arrow_offset  * p[2] + flatpoint(0, r), r,r, hover_item == hoveri + 0 ? 1 : 0, THING_Arrow_Down);  //top
	// dp->drawthing(   arrow_offset  * p[3] + (1-arrow_offset) * p[0] + flatpoint(0,-r), r,r, hover_item == hoveri + 2 ? 1 : 0, THING_Arrow_Up);    //bottom
	// dp->drawthing(   arrow_offset  * p[2] + (1-arrow_offset) * p[3] + flatpoint( r,0), r,r, hover_item == hoveri + 1 ? 1 : 0, THING_Arrow_Left);  //right
	// dp->drawthing((1-arrow_offset) * p[0] +    arrow_offset  * p[1] + flatpoint(-r,0), r,r, hover_item == hoveri + 3 ? 1 : 0, THING_Arrow_Right); //left

	flatpoint h = offset * p[2] + (1-offset) * p[1] + flatpoint(0, r);
	if (pp.x >= h.x-r && pp.x <= h.x+r && pp.y >= h.y-r && pp.y <= h.y+r) return i_offset + 0; // top

	h = offset * p[3] + (1-offset) * p[0] + flatpoint(0,-r);
	if (pp.x >= h.x-r && pp.x <= h.x+r && pp.y >= h.y-r && pp.y <= h.y+r) return i_offset + 2; // bottom

	h = offset * p[2] + (1-offset) * p[3] + flatpoint( r,0);
	if (pp.x >= h.x-r && pp.x <= h.x+r && pp.y >= h.y-r && pp.y <= h.y+r) return i_offset + 1; // right

	h = offset * p[1] + (1-offset) * p[0] + flatpoint(-r,0);
		if (pp.x >= h.x-r && pp.x <= h.x+r && pp.y >= h.y-r && pp.y <= h.y+r) return i_offset + 3; // left

	return -1;
}

void AdjustBox(DoubleBBox &box, const flatpoint &d, int i_offset, int hover_item, PaperBoxData *curbox)
{
	if (hover_item == i_offset + 0) { //top
		box.maxy += d.y;
		if (box.maxy > curbox->h()) box.maxy = curbox->h();
		else if (box.maxy <= box.miny) box.maxy = box.miny;

	} else if (hover_item == i_offset + 2) { //bottom
		box.miny += d.y;
		if (box.miny < 0) box.miny = 0;
		else if (box.miny >= box.maxy) box.miny = box.maxy;

	} else if (hover_item == i_offset + 3) { //left
		box.minx += d.x;
		if (box.minx < 0) box.minx = 0;
		else if (box.minx >= box.maxx) box.minx = box.maxx;

	} else if (hover_item == i_offset + 1) { //right
		box.maxx += d.x;
		if (box.maxx > curbox->w()) box.maxx = curbox->w();
		else if (box.maxx <= box.minx) box.maxx = box.minx;
	}
}

//! Return the papergroup->papers element index underneath x,y, or -1.
int PaperInterface::scan(int x,int y)
{
	if (!papergroup || !papergroup->papers.n) return -1;
	flatpoint fp = dp->screentoreal(x,y);

	if (curbox) {
		flatpoint pp = curbox->transformPointInverse(fp);
		flatpoint p[4];
		double r = arrow_threshold * ScreenLine() / Getmag() / curbox->xaxis().norm();
		
		if (allow_margin_edit && edit_margins) {
			int i = ScanBox(pp, curbox->box->margin, r, margin_offset, PAPERI_Margin_Top, p);
			if (i >= 0) return i;
		}

		if (allow_trim_edit && edit_trim) {
			int i = ScanBox(pp, curbox->box->trim, r, trim_offset, PAPERI_Trim_Top, p);
			if (i >= 0) return i;
		}
	}

	for (int c = papergroup->papers.n-1; c >= 0; c--) {
		if (papergroup->papers.e[c]->pointin(fp)) return c;
	}
	return -1;
}

//! Make a maybebox at real point p, if one does not exist.
void PaperInterface::CreateMaybebox(flatpoint p)
{
	if (maybebox) return;

	 // find a paper size, not so easy!
	PaperBox *box = nullptr;
	PaperBoxData *boxdata = nullptr;
	int del = 0;

	if (curbox) {
		box = dynamic_cast<PaperBox*>(curbox->box->duplicate());
		del = 1;
		boxdata = curbox;
	} else if (papergroup && papergroup->papers.n) {
		box = dynamic_cast<PaperBox*>(papergroup->papers.e[0]->box->duplicate());
		boxdata = papergroup->papers.e[0];
		del = 1;
	} else if (doc) {
		box = new PaperBox((PaperStyle *)doc->imposition->GetDefaultPaper()->duplicateValue(), true);
		del = 1;
	} else {
		box = new PaperBox((PaperStyle *)laidout->papersizes.e[0]->duplicateValue(), true);
		del = 1;
	}

	maybebox = new PaperBoxData(box); //incs count of box
	maybebox->outlinecolor = default_outline_color; //.rgbf(1.0, 0.0, 1.0);
	maybebox->color = default_fill;
	if (boxdata) {
		maybebox->xaxis(boxdata->xaxis());
		maybebox->yaxis(boxdata->yaxis());
	}
	maybebox->origin(flatpoint(0,0));
	maybebox->origin(p-transform_point(maybebox->m(),(maybebox->maxx+maybebox->minx)/2, norm(maybebox->yaxis())*(maybebox->maxy+maybebox->miny)/2));
	if (del) box->dec_count();

	p = dp->realtoscreen(p);
	rx = p.x;
	ry = p.y;
}

void PaperInterface::EnsureBox(PaperBoxData *data, BoxTypes type, DoubleBBox &box)
{
	if (data && data->box) {
	 	if (!(data->box->Has(type))) {
	 		data->box->which |= type;
	 		box.minx = box.miny = 0;
	 		box.maxx = data->w();
	 		box.maxy = data->h();
	 	}
	}
}

/*! Add maybe box if shift is down.
 */
int PaperInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	maybe_flush = false;

	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	// DBG flatpoint fp;
	// DBG fp=dp->screentoreal(x,y);
	// DBG cerr <<"1 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	int over = scan(x, y);
	hover_item = -1;

	// DBG fp=dp->screentoreal(x,y);
	// DBG cerr <<"2 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
	if ((state&LAX_STATE_MASK) == ShiftMask && over < 0) {
		//add a new box
		if (!maybebox) CreateMaybebox(dp->screentoreal(x,y));

		// DBG fp=dp->screentoreal(x,y);
		// DBG cerr <<"3 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		if (!papergroup) {
			papergroup = new PaperGroup;
			papergroup->Name = new_paper_group_name();
			laidout->resourcemanager->AddResource("PaperGroup", papergroup, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
			//papergroup->dec_count();
			
			// DBG fp=dp->screentoreal(x,y);
			// DBG cerr <<"4 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);

			// DBG fp=dp->screentoreal(x,y);
			// DBG cerr <<"5 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		}
		over = papergroup->papers.push(maybebox);

		maybebox->dec_count();
		maybebox = nullptr;
		needtodraw = 1;
		//return 0; -- do not return, continue to let box be added..
	}

	// DBG fp = dp->screentoreal(x,y);
	// DBG cerr <<"6 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
	if (over >= 0 && over < papergroup->papers.n) {
		if (curbox) curbox->dec_count();
		curbox = papergroup->papers.e[over];
		curbox->inc_count();

		if (edit_margins) EnsureBox(curbox, MarginBox, curbox->box->margin);
		if (edit_trim)    EnsureBox(curbox, TrimBox,   curbox->box->trim);

		if ((state & LAX_STATE_MASK) == 0) {
			if (curboxes.findindex(curbox) < 0)
				curboxes.flush();
			else maybe_flush = true;
		}

		curboxes.pushnodup(curbox,0);
		flatpoint xx = curbox->xaxis();
		snap_running_angle = atan2(xx.y, xx.x);
		needtodraw = 1;

	} else if (over > 0) { // misc handles
		buttondown.moveinfo(d->id, LEFTBUTTON, state, over);
		hover_item = over;

	} else {
		curboxes.flush();
		if (curbox) { curbox->dec_count(); curbox = nullptr; }
		needtodraw = 1;
	}

	// DBG fp=dp->screentoreal(x,y);
	// DBG cerr <<"7 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	lbdown = dp->screentoreal(x,y);
	// DBG fp=dp->screentoreal(x,y);
	// DBG cerr <<"8 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	return 0;
}

int PaperInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	
	int dragged = buttondown.up(d->id,LEFTBUTTON);
	if (dragged < DraggedThreshhold()) {
		if (    (hover_item >= PAPERI_Trim_Top   && hover_item <= PAPERI_Trim_Left)
			 || (hover_item >= PAPERI_Margin_Top && hover_item <= PAPERI_Margin_Left)
		   ) {
			flatpoint p;
			double startvalue = 0;
			const char *message = nullptr;
			const char *label = nullptr;
			PaperBox *box = curbox->box;

			if (hover_item == PAPERI_Trim_Top) {
				// p = flatpoint((1-trim_offset)*curbox->w(), box->trim.maxy);
				startvalue = curbox->h() - box->trim.maxy;
				message = "trim_top";
				label = _("Trim top");
			
			} else if (hover_item == PAPERI_Trim_Right) {
				// p = flatpoint(box->trim.minx, (1-trim_offset)*curbox->h());
				startvalue = box->trim.minx;
				message = "trim_left";
				label = _("Trim left");
			
			} else if (hover_item == PAPERI_Trim_Left) {
				// p = flatpoint(box->trim.maxx, (1-trim_offset)*curbox->h());
				startvalue = curbox->w() - box->trim.maxx;
				message = "trim_right";
				label = _("Trim right");
			
			} else if (hover_item == PAPERI_Trim_Bottom) {
				// p = flatpoint((1-trim_offset)*curbox->w(), box->trim.miny);
				startvalue = box->trim.miny;
				message = "trim_bottom";
				label = _("Trim bottom");
			
			} else if (hover_item == PAPERI_Margin_Top) {
				// p = flatpoint((1-margin_offset)*curbox->w(), box->margin.maxy);
				startvalue = curbox->h() - box->margin.maxy;
				message = "margin_top";
				label = _("Margin top");
			
			} else if (hover_item == PAPERI_Margin_Right) {
				// p = flatpoint(box->margin.minx, (1-margin_offset)*curbox->h());
				startvalue = box->margin.minx;
				message = "margin_left";
				label = _("Margin left");
			
			} else if (hover_item == PAPERI_Margin_Left) {
				// p = flatpoint(box->margin.maxx, (1-margin_offset)*curbox->h());
				startvalue = curbox->w() - box->margin.maxx;
				message = "margin_right";
				label = _("Margin right");
			
			} else if (hover_item == PAPERI_Margin_Bottom) {
				// p = flatpoint((1-margin_offset)*curbox->w(), box->margin.miny);
				startvalue = box->margin.miny;
				message = "margin_bottom";
				label = _("Margin bottom");
			}

			// p = dp->realtoscreen(curbox->transformPoint(p));
			p.set(x,y);

			double th = curwindow->win_themestyle->normal->textheight();
			DoubleBBox bounds(p.x - 5*th, p.x + 5*th, p.y - th, p.y + th);
			char scratch[30];
			sprintf(scratch, "%g", startvalue);
			viewport->SetupInputBox(object_id, label, scratch, message, bounds);

		} else if (maybe_flush) {
			maybe_flush = false;
			curboxes.flush();
			curboxes.pushnodup(curbox, 0);
		}
		needtodraw = 1;
	}

	return 0;
}

/*! Adjust curboxes so that they snap to other non-curboxes.
 * Return number of boxes changed.
 */
int PaperInterface::SnapBoxes()
{
	if (!papergroup) return 0;
	if (!maybebox && curboxes.n == papergroup->papers.n) return 0; //all boxes are selected.. todo: maybe we should snap to viewport spread/pages?

	double threshhold = snap_px_threshhold / dp->Getmag();

	NumStack<double> xx,yy;
	PaperBoxData *box = nullptr;
	for (int c=-1; c<curboxes.n; c++) {
		if (c == -1) {
			if (!maybebox) continue;
			box = maybebox;
		} else {
			if (maybebox) continue;
			box = curboxes.e[c];
		}
		//build points that we need to check for snapping
		flatpoint p = box->BBoxPoint(0,0,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = box->BBoxPoint(0,1,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = box->BBoxPoint(1,1,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = box->BBoxPoint(1,0,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = box->BBoxPoint(.5,.5,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
	}

	flatpoint d, p;
	flatpoint dp;
	double dd;
	bool do_x = true;
	bool do_y = true;
	flatpoint pgp[5];
	for (int c=0; c<papergroup->papers.n; c++) {
		if (curboxes.Contains(papergroup->papers.e[c])) continue;

		pgp[0] = papergroup->papers.e[c]->BBoxPoint(0,1,true);
		pgp[1] = papergroup->papers.e[c]->BBoxPoint(1,1,true);
		pgp[2] = papergroup->papers.e[c]->BBoxPoint(1,0,true);
		pgp[3] = papergroup->papers.e[c]->BBoxPoint(0,0,true);
		pgp[4] = papergroup->papers.e[c]->BBoxPoint(.5,.5,true);

		for (int c2=0; c2<xx.n && (do_x || do_y); c2++) {
			for (int c3 = 0; c3 < 5; c3++) {
				p = pgp[c3];
				if (do_x) {
					dd = fabs(xx.e[c2] - p.x);
					if (dd < threshhold) {
						do_x = false;
						dp.x = p.x - xx.e[c2];
					}
				}
				if (do_y) {
					dd = fabs(yy.e[c2] - p.y);
					if (dd < threshhold) {
						do_y = false;
						dp.y = p.y - yy.e[c2];
					}
				}
			}
		}
	}

	if (do_x == false || do_y == false) {
		//we matched a snap, so adjust transforms
		if (maybebox) {
			maybebox->origin(maybebox->origin() + dp);
		} else {
			for (int c=0; c<curboxes.n; c++) {
				curboxes.e[c]->origin(curboxes.e[c]->origin() + dp);
			}
		}
		return curboxes.n;
	}
	return 0;
}


int PaperInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	// DBG flatpoint fpp = dp->screentoreal(x, y);
	// DBG cerr <<"mm *****ARG**** "<<fpp.x<<","<<fpp.y<<endl;

	int over = scan(x, y);

	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (!(state & ShiftMask)) {
			if (over != hover_item) {
				hover_item = over;
				PostMessage(HoverMessage(hover_item));
				needtodraw = 1;
			}
			return 1;
		}

		//update maybebox when shift key down
		if (over >= 0) { //don't show maybebox when we hover over something
			if (maybebox) {
				maybebox->dec_count();
				maybebox   = nullptr;
				needtodraw = 1;
			}
			return 0;
		}
		if (!maybebox) {
			CreateMaybebox(dp->screentoreal(x,y));
		} else {
			maybebox->origin(maybebox->origin() + dp->screentoreal(x, y) - dp->screentoreal(rx, ry));
			SnapBoxes();
			rx = x;
			ry = y;
		}
		needtodraw = 1;
		return 0;
	}

	if (curboxes.n == 0) return 1;

	int mx, my;
	buttondown.move(mouse->id, x,y, &mx,&my);

	// if curboxes.n>0, this implies papergroup!=nullptr

	flatpoint fp = dp->screentoreal(x, y);
	flatpoint p0 = dp->screentoreal(mx, my);

	if (curbox && hover_item > 10000) {
		flatpoint d = curbox->transformPointInverse(fp) - curbox->transformPointInverse(p0);
		PaperBox *box = curbox->box;

		if (hover_item >= PAPERI_Trim_Top && hover_item <= PAPERI_Trim_Left)
			AdjustBox(box->trim, d, PAPERI_Trim_Top, hover_item, curbox);
		else if (hover_item >= PAPERI_Margin_Top && hover_item <= PAPERI_Margin_Left)
			AdjustBox(box->margin, d, PAPERI_Margin_Top, hover_item, curbox);

		needtodraw = 1;
		return 0;
	}

	flatpoint d  = fp - p0;

	// plain or + moves curboxes (or the box given by editwhat)
	if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->origin(curboxes.e[c]->origin()+d);
		}
		if ((search_snap && (state&LAX_STATE_MASK)==0)
				|| (!search_snap && (state&LAX_STATE_MASK)==ShiftMask)) {
			SnapBoxes();
		}
		needtodraw=1;
		return 0;
	}

	 //^ scales
	if ((state&LAX_STATE_MASK)==ControlMask) {
		PaperBoxData *data;
		for (int c=0; c<papergroup->papers.n; c++) {
			data = papergroup->papers.e[c];
			if (!sync_physical_size && curboxes.findindex(data) < 0) continue;

			if (x>mx) {
				if (data->xaxis()*data->xaxis()<dp->upperbound*dp->upperbound) {
					data->xaxis(data->xaxis()*1.05);
					data->yaxis(data->yaxis()*1.05);
					if (curboxes.findindex(data)>=0) {
						data->origin(lbdown+(data->origin()-lbdown)*1.05);
					}
				}
			} else if (x<mx) {
				if (data->xaxis()*data->xaxis()>dp->lowerbound*dp->lowerbound) {
					data->xaxis(data->xaxis()/1.05);
					data->yaxis(data->yaxis()/1.05);
					if (curboxes.findindex(data)>=0) {
						data->origin(lbdown+(data->origin()-lbdown)/1.05);
					}
				}
			}
		}
		needtodraw=1;
		return 0;
	}

	 //+^ rotates
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
		SomeData *data;
		flatpoint lp,leftd;
		double angle = (x-mx) * M_PI/180;
		if (search_snap) {
			flatpoint xx = curbox->xaxis();
			double old_angle = atan2(xx.y, xx.x);
			snap_running_angle += angle;
			double snap_to = 15*M_PI/180;
			angle = snap_to * int((snap_running_angle + snap_to/2)/ snap_to);
			angle = angle - old_angle;

		}
		for (int c=0; c<curboxes.n; c++) {
			data = curboxes.e[c];
			leftd = transform_point_inverse(data->m(),lbdown);
	  		lp = data->origin() + leftd.x*data->xaxis() + leftd.y*data->yaxis(); 
			data->xaxis(rotate(data->xaxis(),angle));
			data->yaxis(rotate(data->yaxis(),angle));
			d = lp-(data->origin()+data->xaxis()*leftd.x+data->yaxis()*leftd.y);
			data->origin(data->origin()+d);
		}
		needtodraw=1;
		return 0;
	}

	return 0;
}

Laxkit::ShortcutHandler *PaperInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc = new ShortcutHandler(whattype());

	sc->Add(PAPERI_DupPapers,   ' ',0,1,         "DupPapers",_("Duplicate selected papers"),nullptr,0);
	sc->Add(PAPERI_Select,      'a',0,0,         "Select",   _("Select or deselect all"),nullptr,0);
	sc->Add(PAPERI_Decorations, 'd',0,0,         "Decs",     _("Toggle decorations"),nullptr,0);
	sc->Add(PAPERI_Delete,      LAX_Bksp,0,0,    "Delete",   _("Delete selected"),nullptr,0);
	sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);
	sc->Add(PAPERI_Rectify,     'o',0,0,         "Rectify",  _("Make the axes horizontal and vertical"),nullptr,0);
	sc->Add(PAPERI_Rotate,      'r',0,0,         "Rotate",   _("Rotate selected by 90 degrees"),nullptr,0);
	sc->Add(PAPERI_RotateCC,    'R',ShiftMask,0, "RotateCC", _("Rotate selected by 90 degrees in the other direction"),nullptr,0);
	sc->Add(PAPERI_ToggleLandscape, 'l',0,0, "ToggleLandscape", _("Toggle landscape"),nullptr,0);
	sc->AddAction(PAPERI_Portrait, "Portrait", _("Portrait"), nullptr, 0, 0);
	sc->AddAction(PAPERI_Landscape, "Landscape", _("Landscape"), nullptr, 0, 0);



	manager->AddArea(whattype(),sc);
	return sc;
}

int PaperInterface::PerformAction(int action)
{
	if (action==PAPERI_Select) {
		if (!papergroup) return 1;
		needtodraw=1;
		int n=curboxes.n;
		curboxes.flush();
		if (curbox) { curbox->dec_count(); curbox=nullptr; }
		if (n) return 0;
		for (int c=0; c<papergroup->papers.n; c++) 
			curboxes.push(papergroup->papers.e[c],0);
		return 0;

	} else if (action==PAPERI_Decorations) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

	} else if (action == PAPERI_ResetScaling) {
		if (!papergroup) return 0;
		// PaperBoxData *data = papergroup->papers.e[0];
		// double s = 1 / data->xaxis().norm();
		for (int c = 0; c < papergroup->papers.n; c++) {
			PaperBoxData *data = papergroup->papers.e[c];
			data->setScale(1,1);
		}
		needtodraw = 1;
		return 0;

	} else if (action == PAPERI_ResetAngle) {
		if (!curboxes.n) return 0;

		PaperBoxData *data;
		double s = norm(curboxes.e[0]->xaxis());
		for (int c = 0; c < curboxes.n; c++) {
			data = curboxes.e[c];
			data->xaxis(flatpoint(s, 0));
			data->yaxis(flatpoint(0, s));
		}
		needtodraw = 1;
		return 0;

	} else if (action==PAPERI_ToggleSnap) {
		search_snap = !search_snap;
		PostMessage(search_snap ? _("Snap on (hold shift to not snap)") : _("Snap off (hold shift to snap)"));
		return 0;

	} else if (action==PAPERI_ToggleLabels) {
		show_labels = !show_labels;
		PostMessage(show_labels ? _("Show labels") : _("Don't show labels"));
		needtodraw = 1;
		return 0;

	} else if (action==PAPERI_ToggleIndices) {
		show_indices = !show_indices;
		PostMessage(show_indices ? _("Show indices") : _("Don't show indices"));
		needtodraw = 1;
		return 0;

	} else if (action==PAPERI_ToggleBackIndices) {
		edit_back_indices = !edit_back_indices;
		PostMessage(edit_back_indices ? _("Edit back indices") : _("Edit front indices"));
		needtodraw = 1;
		return 0;

	} else if (action==PAPERI_ToggleSyncSizes) {
		sync_physical_size = !sync_physical_size;
		PostMessage(sync_physical_size ? _("Papers scale together") : _("Papers scale independently"));
		if (sync_physical_size) {
			if (papergroup && papergroup->papers.n > 0) {
				double scale = papergroup->papers.e[0]->xaxis().norm();
				for (int c=1; c<papergroup->papers.n; c++) {
					papergroup->papers.e[c]->setScale(scale, scale);
				}
			}
		}
		needtodraw = 1;
		return 0;

	} else if (action==PAPERI_Decorations) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

	} else if (action==PAPERI_Delete) {
		if (!curboxes.n || !papergroup) return 0;
		int c2;
		for (int c=0; c<curboxes.n; c++) {
			c2=papergroup->papers.findindex(curboxes.e[c]);
			if (c2<0) continue;
			papergroup->papers.remove(c2);
		}
		curboxes.flush();
		if (curbox) { curbox->dec_count(); curbox=nullptr; }
		needtodraw=1;
		return 0;

	} else if (action==PAPERI_Rectify) {
		if (papergroup && papergroup->papers.n) {
			flatpoint p;
			for (int c=0; c<papergroup->papers.n; c++) {
				papergroup->papers.e[c]->m(0,1);
				papergroup->papers.e[c]->m(1,0);
				papergroup->papers.e[c]->m(2,0);
				papergroup->papers.e[c]->m(3,1);
			}
			needtodraw=1;
			return 0;
		}

	} else if (action==PAPERI_Rotate || action==PAPERI_RotateCC) {
		 //rotate by 90 degree increments
		if (!curboxes.n) return 0;
		bool ccw = (action == PAPERI_RotateCC);
		double x,y;
		flatpoint invariant;
		for (int c=0; c<curboxes.n; c++) {
			invariant += curboxes.e[c]->BBoxPoint(.5,.5,true);
		}
		invariant /= curboxes.n;
		for (int c=0; c<curboxes.n; c++) {
			flatpoint i1 = curboxes.e[c]->transformPointInverse(invariant);
			 //rotate x axis
			x = curboxes.e[c]->m((int)0);
			y = curboxes.e[c]->m(1);
			curboxes.e[c]->m(0, ccw ? y : -y);
			curboxes.e[c]->m(1, ccw ? -x : x);
			// rotate y axis
			x = curboxes.e[c]->m(2);
			y = curboxes.e[c]->m(3);
			curboxes.e[c]->m(2, ccw ? y : -y);
			curboxes.e[c]->m(3, ccw ? -x : x);
			i1 = curboxes.e[c]->transformPoint(i1);
			curboxes.e[c]->origin(curboxes.e[c]->origin() + invariant-i1);
		}
		needtodraw=1;
		return 0;

	} else if (action == PAPERI_CreateImposition) {
		// Save paper group as a Net
		//  Name ___________
		//  [ ] Save as global asset (else save with resources of document)
		// [ok] [cancel]
		PostMessage("IMPLEMENT PAPERI_CreateImposition!!!!");

		//ValueHash props;
		//props.push("name", StringValue(papergroup ? new papergroup->Id() : nullptr), -1, true);
		//props.push("save_global", new BooleanValue(false), -1, true);
		//ValueWindow *win = new ValueWindow(props);
		//app->rundialog(win);

		return 0;

	}  else if (action == PAPERI_DupPapers) {
		if (!curboxes.n) return 0;
		flatpoint offset;
		if (!buttondown.any()) {
			offset.x = offset.y = 10 * ScreenLine() / dp->Getmag();
		}

		for (int c=0; c<curboxes.n; c++) {
			PaperBoxData *paper = curboxes.e[c];

			PaperBoxData *newpaper = dynamic_cast<PaperBoxData*>(paper->duplicateData(nullptr));

			newpaper->origin(newpaper->origin() + offset);
			papergroup->papers.push(newpaper);
		}

		PostMessage("Duplicated.");
		return 0;

	} else if (action == PAPERI_EditMargins) {
		if (!allow_margin_edit) {
			edit_margins = false;
		} else {
			edit_margins = !edit_margins;
		}
		if (edit_margins) {
			 drawwhat |= MarginBox;
 			 if (curbox && curbox->box) EnsureBox(curbox, MarginBox, curbox->box->margin);

			 PostMessage(_("Edit margins"));
		} else {
			drawwhat = (~MarginBox) & drawwhat;
			PostMessage(_("Don't edit margins"));
		}
		AdjustBoxInsets();
		needtodraw = 1;
		return 0;

	} else if (action == PAPERI_EditTrim) {
		if (!allow_trim_edit) {
			edit_trim = false;
		} else {
			edit_trim = !edit_trim;
		}
		if (edit_trim) {
			 drawwhat |= TrimBox;
			 if (curbox && curbox->box) EnsureBox(curbox, TrimBox, curbox->box->trim);
			 PostMessage(_("Edit trim"));
		} else {
			drawwhat = (~TrimBox) & drawwhat;
			PostMessage(_("Don't edit trim"));
		}
		AdjustBoxInsets();
		needtodraw = 1;
		return 0;

	} else if (action == PAPERI_ToggleLandscape) {
		if (!curboxes.n) return 0;
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->box->landscape(!curboxes.e[c]->box->landscape());
		}
		needtodraw = 1;
		PostMessage(_("Orientation toggled."));
		return 0;

	} else if (action == PAPERI_Landscape) {
		if (!curboxes.n) return 0;
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->box->landscape(true);
		}
		needtodraw = 1;
		PostMessage(_("Landscape orientation"));
		return 0;
	
	} else if (action == PAPERI_Portrait) {
		if (!curboxes.n) return 0;
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->box->landscape(false);
		}
		needtodraw = 1;
		PostMessage(_("Portrait orientation"));
		return 0;

	} else if (action == PAPERI_DeletePaperGroup) {
		if (!papergroup) return 0;
		laidout->resourcemanager->RemoveResource(papergroup, "PaperGroup");
		Clear(nullptr);
		LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
		if (lvp) lvp->UseThisPaperGroup(nullptr);
		needtodraw=1;
		return 0;
	}

	return 1;
}

void PaperInterface::AdjustBoxInsets()
{
	int n = 0;
	if (edit_trim) n++;
	if (edit_margins) n++;
	if (n == 0) return; // offsets irrelevant

	double diff = 1. / (n+1);
	double x = diff;
	if (edit_trim)    { trim_offset   = x; x += diff; }
	if (edit_margins) { margin_offset = x; x += diff; }
}

int PaperInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch == LAX_Esc) {
		if (edit_margins) {
			edit_margins = false;
			needtodraw = 1;
			return 0;
		}
		if (curboxes.n) {
			curboxes.flush();
			if (curbox) { curbox->dec_count(); curbox = nullptr; }
			return 0;
		}
	}

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state & LAX_STATE_MASK, buttondown.any() ? 1 : 0);
	if (action >= 0) {
		return PerformAction(action);
	}

	if (ch == LAX_Shift) {
		//turn on maybebox
		if (maybebox) return 0;
		if (buttondown.any()) return 0;
		int x, y;
		if (mouseposition(d->paired_mouse->id, viewport, &x, &y, nullptr, nullptr) != 0) return 0;
		if (scan(x, y) >= 0) return 0;
		CreateMaybebox(flatpoint(dp->screentoreal(x, y)));
		if (buttondown.isdown(d->paired_mouse->id, LEFTBUTTON))
			buttondown.down(d->paired_mouse->id, LEFTBUTTON, x,y, state);
		needtodraw = 1;
		return 0;
	}
	return 1;
}

int PaperInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch == LAX_Shift) {
		 //turn off maybebox
		if (!maybebox) return 1;
		maybebox->dec_count();
		maybebox   = nullptr;
		needtodraw = 1;
		return 0;
	}
	return 1;
}


} // namespace Laidout

