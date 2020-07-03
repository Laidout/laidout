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


#include "../language.h"
#include "paperinterface.h"
#include "../ui/viewwindow.h"
#include "../core/drawdata.h"
#include "../dataobjects/printermarks.h"

#include <lax/inputdialog.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/colors.h>
#include <lax/interfaces/interfacemanager.h>

//template implementation:
#include <lax/lists.cc>

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
	static int num=1;
	char *str;
	int c;
	while (1) {
		str=new char[strlen(_("Paper Group %d"))+15];
		sprintf(str,_("Paper Group %d"),num++);
		for (c=0; c<laidout->project->papergroups.n; c++) {
			if (laidout->project->papergroups.e[c]->Name) {
				if (!strcmp(str,laidout->project->papergroups.e[c]->Name)) break;
			} else if (!strcmp(str,laidout->project->papergroups.e[c]->name)) break;
		}
		if (c==laidout->project->papergroups.n) return str;
		delete[] str;
	}
	return nullptr;
}

//------------------------------------- PaperInterface --------------------------------------
	
/*! \class PaperInterface 
 * \brief Interface to arrange an arbitrary spread to print on one or more sheets of paper.
 *
 * This can be used to position a single paper over real space, or set to allow any
 * number of papers. Also can be used to set rectangles for crop, bleed, and trim.
 */
/*! \var PaperBoxData *PaperInterface::paperboxdata
 * \brief Temporary pointer to aid viewport refreshing of PaperBoxData objects.
 */

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	snapto      = MediaBox;
	editwhat    = MediaBox;
	drawwhat    = MediaBox;
	showdecs    = 0;
	show_labels = true;
	search_snap = true;
	snap_px_threshhold = 5 * InterfaceManager::GetDefault()->ScreenLine();
	font        = nullptr;

	curbox = maybebox = nullptr;

	papergroup   = nullptr;
	paperboxdata = nullptr;
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

	PAPERI_Select,
	PAPERI_Decorations,
	PAPERI_Delete, //curboxes
	PAPERI_Rectify,
	PAPERI_Rotate,
	PAPERI_RotateCC,

	PAPERI_first_pagesize   = 1000,
	PAPERI_first_papergroup = 2000,

	PAPERI_MAX = 5000
};

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PaperInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	rx = x;
	ry = y;

	if (!menu) menu=new MenuInfo(_("Paper Interface"));
	else menu->AddSep(_("Papers"));

	menu->AddToggleItem(_("Snap"),        nullptr, PAPERI_ToggleSnap,   0, search_snap);
	menu->AddToggleItem(_("Show labels"), nullptr, PAPERI_ToggleLabels, 0, show_labels);
	menu->AddSep();

	if (papergroup) {
		menu->AddItem(_("Add Registration Mark"),PAPERI_RegistrationMark);
		menu->AddItem(_("Add Gray Bars"),PAPERI_GrayBars);
		menu->AddItem(_("Add Cut Marks"),PAPERI_CutMarks);
		menu->AddSep();
		menu->AddItem(_("Reset paper scaling"),PAPERI_ResetScaling);
	}

	if (papergroup && curboxes.n) {

		menu->AddItem(_("Reset paper angle"),PAPERI_ResetAngle);
		menu->AddItem(_("Paper Size"),PAPERI_PaperSize);
		menu->SubMenu(_("Paper Size"));
		for (int c=0; c<laidout->papersizes.n; c++) {
			menu->AddItem(laidout->papersizes.e[c]->name,PAPERI_first_pagesize+c,
					LAX_ISTOGGLE
					| (!strcmp(curboxes.e[0]->box->paperstyle->name,laidout->papersizes.e[c]->name)
					  ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
		//int landscape=curboxes.e[0]->box->paperstyle->flags&1;
		//menu->AddItem(_("Portrait"), PAPERI_Portrait, LAX_OFF|MENU_ISTOGGLE|(landscape?0:MENU_CHECKED));
		//menu->AddItem(_("Landscape"),PAPERI_Landscape, LAX_OFF|MENU_ISTOGGLE|(landscape?MENU_CHECKED:0));
		menu->AddItem(_("Print with paper group"),PAPERI_Print);
		menu->AddSep();	
	}

	if (laidout->project->papergroups.n) {		
		menu->AddItem(_("Paper Group"));
		menu->SubMenu(_("Paper group"));
		const char *nme;
		PaperGroup *pg;
		for (int c=0; c<laidout->project->papergroups.n; c++) {
			pg=laidout->project->papergroups.e[c];
			nme=pg->Name;
			if (!nme) nme=pg->name;
			if (!nme) nme=_("(unnamed)");
			menu->AddItem(nme, PAPERI_first_papergroup+c,
						  LAX_ISTOGGLE | LAX_OFF | (papergroup==pg ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
	}
	menu->AddItem(_("New paper group..."),PAPERI_NewPaperGroup);
	if (papergroup) menu->AddItem(_("Rename current paper group..."),PAPERI_RenamePaperGroup);
	if (papergroup) menu->AddItem(_("Delete current paper group..."),PAPERI_DeletePaperGroup);

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
		int i=s->info2; //id of menu item

		if (     i == PAPERI_ToggleSnap
			  || i == PAPERI_ToggleLabels) {
			PerformAction(i);
			return 0;

//		} else if (i==PAPERI_Portrait) {
//			 //portrait
//			return 0;
//
//		} else if (i==PAPERI_Landscape) {
//			 //landscape
//			return 0;

		} else if (i==PAPERI_Print) {
			 //print with the active paper group
			curwindow->win_parent->Event(nullptr,"print");//***hack Hack HACK
			return 0;

		} else if (i==PAPERI_ResetScaling) {
			if (!papergroup) return 0;
			PaperBoxData *data=papergroup->papers.e[0];
			double s=1/data->xaxis().norm();
			for (int c=0; c<papergroup->papers.n; c++) {
				data=papergroup->papers.e[c];
				data->Scale(s);
			}
			needtodraw=1;
			return 0;

		} else if (i==PAPERI_ResetAngle) {
			if (!curboxes.n) return 0;

			PaperBoxData *data;
			double s=norm(curboxes.e[0]->xaxis());
			for (int c=0; c<curboxes.n; c++) {
				data=curboxes.e[c];
				data->xaxis(flatpoint(s,0));
				data->yaxis(flatpoint(0,s));
			}
			needtodraw=1;
			return 0;

		} else if (i>=PAPERI_first_pagesize && i<PAPERI_first_pagesize+1000) {
			 //paper size
			i-=PAPERI_first_pagesize;
			if (i>=laidout->papersizes.n-1) return 1;
			PaperStyle *newpaper=(PaperStyle *)laidout->papersizes.e[i]->duplicate();
			for (int c=0; c<curboxes.n; c++) {
				curboxes.e[c]->box->Set(newpaper);
				curboxes.e[c]->setbounds(&curboxes.e[c]->box->media);
			}
			newpaper->dec_count();
			needtodraw=1;
			return 0;

		} else if (i>=PAPERI_first_papergroup && i<PAPERI_first_papergroup+1000) {
			//***is selecting a new papergroup from laidout->project->papergroups	
			i-=PAPERI_first_papergroup;
			if (i<0 || i>laidout->project->papergroups.n) return 0;

			Clear(nullptr);
			papergroup=laidout->project->papergroups.e[i];
			papergroup->inc_count();
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;

		} else if (i==PAPERI_NewPaperGroup) {
			 //New paper group...
			if (papergroup) papergroup->dec_count();
			papergroup=new PaperGroup;
			papergroup->name=new_paper_group_name();
			laidout->project->papergroups.push(papergroup);
			
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;

		} else if (i==PAPERI_RenamePaperGroup) {
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

		} else if (i==PAPERI_RegistrationMark) {
			if (!papergroup) return 0;
			SomeData *obj= RegistrationMark(18,1);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			papergroup->objs.push(obj);
			obj->dec_count();
			return 0;

		} else if (i==PAPERI_GrayBars) {
			if (!papergroup) return 0;
			SomeData *obj= BWColorBars(18,LAX_COLOR_GRAY);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			papergroup->objs.push(obj);
			obj->dec_count();
			return 0;
		}
		return 0;
	}
	return 1;
}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int PaperInterface::UseThisDocument(Document *ndoc)
{
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();
	return 0;
}

/*! PaperGroup or PaperBoxData.
 */
int PaperInterface::draws(const char *atype)
{
	return !strcmp(atype,"PaperBoxData") || !strcmp(atype,"PaperGroup");
}


//! Return a new PaperInterface if dup=nullptr, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PaperInterface::duplicate(anInterface *dup)//dup=nullptr
{
	if (dup==nullptr) dup=new PaperInterface(id,nullptr);
	else if (!dynamic_cast<PaperInterface *>(dup)) return nullptr;
	
	return anInterface::duplicate(dup);
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
	PaperGroup *pg=dynamic_cast<PaperGroup *>(ndata);
	if (!pg && ndata) return 0; //was a non-null object, but not a papergroup
	Clear(nullptr);
	
	papergroup=pg;
	if (papergroup) papergroup->inc_count();
	needtodraw=1;

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
	int ntd            = needtodraw;
	BoxTypes tdrawwhat = drawwhat;
	drawwhat           = AllBoxes;
	showdecs           = 1;
	show_labels        = false;
	needtodraw         = 1;
	Displayer *olddp   = dp;
	dp                 = tdp;
	DrawPaper(data, ~0, 1, 5, 0);
	show_labels  = labels;
	dp           = olddp;
	drawwhat     = tdrawwhat;
	needtodraw   = ntd;
	showdecs     = td;
	paperboxdata = nullptr;
	return 1;
}

/*! If fill!=0, then fill the media box with the paper color.
 * If shadow!=0, then put a black drop shadow under a filled media box,
 * at an offset pixel length shadow.
 */
void PaperInterface::DrawPaper(PaperBoxData *data,int what,char fill,int shadow,char arrow)
{
	if (!data) return;

	int w = InterfaceManager::GetDefault()->ScreenLine(); //1
	if (data==curbox || curboxes.findindex(data)>=0) w=2;
	dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
	dp->LineWidthScreen(w);
	//dp->PushAndNewTransform(data->m());

	//double sshadow = shadow*dp->Getmag(); <- this one scales properly
	double sshadow = shadow;

	PaperBox *box=data->box;
	flatpoint p[4];

	if ((what&MediaBox) && (box->which&MediaBox)) {
		p[0]=flatpoint(box->media.minx,box->media.miny);
		p[1]=flatpoint(box->media.minx,box->media.maxy);
		p[2]=flatpoint(box->media.maxx,box->media.maxy);
		p[3]=flatpoint(box->media.maxx,box->media.miny);

		dp->FillAttributes(FillSolid,WindingRule);

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
		if (fill || shadow) {
			dp->NewFG(&data->outlinecolor);
			dp->NewBG(~0);
			dp->drawlines(p,4,1,2);
		} else {
			dp->NewFG(&data->outlinecolor);
			dp->drawlines(p,4,1,0);
		}

		 //draw orientation arrow
		if (arrow) {
			flatpoint p1=dp->realtoscreen((p[0]+p[3])/2),
					  p2=dp->realtoscreen((p[1]+p[2])/2),
					  v=p2-p1,
					  v2=transpose(v);
			p1=p1+.1*v;
			p2=p2-.1*v;
			dp->DrawScreen();
			dp->LineWidthScreen(w);
			dp->drawline(p1,p2);
			dp->drawline(p2,p2-v2*.2-v*.2);
			dp->drawline(p2,p2+v2*.2-v*.2);
			dp->DrawReal();
			dp->LineWidthScreen(w);
		}

		if (show_labels) {
			if (!font) { font = laidout->defaultlaxfont; font->inc_count(); }
			dp->DrawScreen();
			dp->font(font);
			double th = dp->textheight();
			dp->NewFG(128,128,128);
			flatpoint ang = dp->realtoscreen(p[3]) - dp->realtoscreen(p[0]);
			flatpoint center = dp->realtoscreen((p[1] + p[2])/2);
			double y = center.y;
			dp->textout(-atan2(ang.y, ang.x), center.x,y, box->paperstyle->name,-1,LAX_HCENTER|LAX_BOTTOM);
			y -= th;
			if (!data->label.IsEmpty()) dp->textout(atan2(ang.y, ang.x), center.x,y, data->label.c_str(),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->DrawReal();
		}
	}

	dp->DrawScreen();
	dp->LineWidthScreen(w);

	if ((what&ArtBox) && (box->which&ArtBox)) {
		p[0]=dp->realtoscreen(box->art.minx,box->art.miny);
		p[1]=dp->realtoscreen(box->art.minx,box->art.maxy);
		p[2]=dp->realtoscreen(box->art.maxx,box->art.maxy);
		p[3]=dp->realtoscreen(box->art.maxx,box->art.miny);
		dp->drawlines(p,4,1,0);
	}
	if ((what&TrimBox) && (box->which&TrimBox)) {
		p[0]=dp->realtoscreen(box->trim.minx,box->trim.miny);
		p[1]=dp->realtoscreen(box->trim.minx,box->trim.maxy);
		p[2]=dp->realtoscreen(box->trim.maxx,box->trim.maxy);
		p[3]=dp->realtoscreen(box->trim.maxx,box->trim.miny);
		dp->drawlines(p,4,1,0);
	}
	if ((what&PrintableBox) && (box->which&PrintableBox)) {
		p[0]=dp->realtoscreen(box->printable.minx,box->printable.miny);
		p[1]=dp->realtoscreen(box->printable.minx,box->printable.maxy);
		p[2]=dp->realtoscreen(box->printable.maxx,box->printable.maxy);
		p[3]=dp->realtoscreen(box->printable.maxx,box->printable.miny);
		dp->drawlines(p,4,1,0);
	}
	if ((what&BleedBox) && (box->which&BleedBox)) {
		p[0]=dp->realtoscreen(box->bleed.minx,box->bleed.miny);
		p[1]=dp->realtoscreen(box->bleed.minx,box->bleed.maxy);
		p[2]=dp->realtoscreen(box->bleed.maxx,box->bleed.maxy);
		p[3]=dp->realtoscreen(box->bleed.maxx,box->bleed.miny);
		dp->drawlines(p,4,1,0);
	}

	dp->DrawReal();
	//dp->PopAxes(); //spread axes
}

/*! If which&1 draw paper outline. If which&2, draw paper objects.
 */
void PaperInterface::DrawGroup(PaperGroup *group, char shadow, char fill, char arrow, int which)
{
	if (which&1) {
		 //draw shadow under whole group
		if (shadow) {
			for (int c=0; c<group->papers.n; c++) {
				dp->PushAndNewTransform(group->papers.e[c]->m());
				DrawPaper(group->papers.e[c],MediaBox, fill,laidout->prefs.pagedropshadow,0);
				dp->PopAxes(); 
			}
		}

		 //draw paper
		for (int c=0; c<group->papers.n; c++) {
			dp->PushAndNewTransform(group->papers.e[c]->m());
			DrawPaper(group->papers.e[c],drawwhat, fill,0,arrow);
			dp->PopAxes(); 
		}
	}

	 //draw papergroup objects
	if (which&2) {
		if (group->objs.n()) {
			for (int c=0; c<group->objs.n(); c++) {
				Laidout::DrawData(dp,group->objs.e(c),nullptr,nullptr,0);
			}
		}
	}
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
		PaperBox *box;
		flatpoint p[4];
		if (maybebox) {
			box=maybebox->box;
			dp->PushAndNewTransform(maybebox->m());
			dp->LineAttributes(-1,LineDoubleDash,CapButt,JoinMiter);
			dp->LineWidthScreen(1);
			p[0]=flatpoint(box->media.minx,box->media.miny);
			p[1]=flatpoint(box->media.minx,box->media.maxy);
			p[2]=flatpoint(box->media.maxx,box->media.maxy);
			p[3]=flatpoint(box->media.maxx,box->media.miny);
			dp->drawlines(p,4,1,0);
			dp->PopAxes(); 
		}

		if (!papergroup || !papergroup->papers.n) return 0;

		DrawGroup(papergroup,0,showdecs==1?1:0,showdecs==2?1:0);
	}

	return 1;
}

//! Return the papergroup->papers element index underneath x,y, or -1.
int PaperInterface::scan(int x,int y)
{
	if (!papergroup || !papergroup->papers.n) return -1;
	flatpoint fp=dp->screentoreal(x,y);

	//if (curbox && curbox->pointin(fp)) return papergroup->papers.findindex(curbox);

	for (int c=papergroup->papers.n-1; c>=0; c--) {
		if (papergroup->papers.e[c]->pointin(fp)) return c;
	}
	return -1;
}

//! Make a maybebox at real point p, if one does not exist.
void PaperInterface::CreateMaybebox(flatpoint p)
{
	if (maybebox) return;

	 // find a paper size, not so easy!
	PaperBox *box=nullptr;
	PaperBoxData *boxdata=nullptr;
	int del=0;
	if (curbox) {
		box=curbox->box;
		boxdata=curbox;
	} else if (papergroup && papergroup->papers.n) {
		box=papergroup->papers.e[0]->box;
		boxdata=papergroup->papers.e[0];
	} else if (doc) {
		box=new PaperBox((PaperStyle *)doc->imposition->paper->paperstyle->duplicate(), true);
		del=1;
	} else {
		box=new PaperBox((PaperStyle *)laidout->papersizes.e[0]->duplicate(), true);
		del=1;
	}

	maybebox=new PaperBoxData(box); //incs count of box
	maybebox->outlinecolor.rgbf(1.0, 0.0, 1.0);
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

/*! Add maybe box if shift is down.
 */
int PaperInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	DBG flatpoint fp;
	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"1 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	int over = scan(x, y);

	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"2 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
		//add a new box
		if (!maybebox) CreateMaybebox(dp->screentoreal(x,y));

		DBG fp=dp->screentoreal(x,y);
		DBG cerr <<"3 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		if (!papergroup) {
			papergroup=new PaperGroup;
			papergroup->Name=new_paper_group_name();
			laidout->project->papergroups.push(papergroup);
			//papergroup->dec_count();
			
			DBG fp=dp->screentoreal(x,y);
			DBG cerr <<"4 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);

			DBG fp=dp->screentoreal(x,y);
			DBG cerr <<"5 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		}
		over=papergroup->papers.push(maybebox);

		maybebox->dec_count();
		maybebox=nullptr;
		needtodraw=1;
		//return 0; -- do not return, continue to let box be added..
	}

	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"6 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
	if (over>=0) {
		if (curbox) curbox->dec_count();
		curbox=papergroup->papers.e[over];
		curbox->inc_count();
		if ((state&LAX_STATE_MASK)==0) curboxes.flush();
		curboxes.pushnodup(curbox,0);
		needtodraw=1;
	}
	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"7 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	lbdown=dp->screentoreal(x,y);
	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"8 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	return 0;
}

int PaperInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	buttondown.up(d->id,LEFTBUTTON);

	DBG flatpoint fp=dp->screentoreal(x,y);
	DBG cerr <<"9 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	//***
	//if (curbox) { curbox->dec_count(); curbox=nullptr; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

/*! Adjust curboxes so that they snap to other non-curboxes.
 * Return number of boxes changed.
 */
int PaperInterface::SnapBoxes()
{
	if (curboxes.n == papergroup->papers.n) return 0; //all boxes are selected.. todo: maybe we should snap to viewport spread/pages?

	double threshhold = snap_px_threshhold / dp->Getmag();

	NumStack<double> xx,yy;
	for (int c=0; c<curboxes.n; c++) {
		//build points that we need to check for snapping
		flatpoint p = curboxes.e[c]->BBoxPoint(0,0,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = curboxes.e[c]->BBoxPoint(0,1,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = curboxes.e[c]->BBoxPoint(1,1,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = curboxes.e[c]->BBoxPoint(1,0,true);
		xx.pushnodup(p.x);
		yy.pushnodup(p.y);
		p = curboxes.e[c]->BBoxPoint(.5,.5,true);
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
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->origin(curboxes.e[c]->origin() + dp);
		}
		return curboxes.n;
	}
	return 0;
}


int PaperInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	DBG flatpoint fpp = dp->screentoreal(x, y);
	DBG cerr <<"mm *****ARG**** "<<fpp.x<<","<<fpp.y<<endl;

	int over = scan(x, y);

	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (!(state & ShiftMask)) return 1;

		//update maybebox when shift key down
		if (over >= 0) { //don't show maybebox when we hover over an existing box
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
	flatpoint d  = fp - dp->screentoreal(mx, my);

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
		for (int c=0; c<curboxes.n; c++) {
			data=curboxes.e[c];
			leftd=transform_point_inverse(data->m(),lbdown);
	  		lp=data->origin() + leftd.x*data->xaxis() + leftd.y*data->yaxis(); 
			double angle=x-mx;
			data->xaxis(rotate(data->xaxis(),angle,1));
			data->yaxis(rotate(data->yaxis(),angle,1));
			d=lp-(data->origin()+data->xaxis()*leftd.x+data->yaxis()*leftd.y);
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
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("PaperInterface");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("PaperInterface");

	sc->Add(PAPERI_Select,      'a',0,0,         "Select",  _("Select or deselect all"),nullptr,0);
	sc->Add(PAPERI_Decorations, 'd',0,0,         "Decs",    _("Toggle decorations"),nullptr,0);
	sc->Add(PAPERI_Delete,      LAX_Bksp,0,0,    "Delete",  _("Delete selected"),nullptr,0);
	sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);
	sc->Add(PAPERI_Rectify,     'o',0,0,         "Rectify", _("Make the axes horizontal and vertical"),nullptr,0);
	sc->Add(PAPERI_Rotate,      'r',0,0,         "Rotate",  _("Rotate selected by 90 degrees"),nullptr,0);
	sc->Add(PAPERI_RotateCC,    'R',ShiftMask,0, "RotateCC",_("Rotate selected by 90 degrees in the other direction"),nullptr,0);


	manager->AddArea("PaperInterface",sc);
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

	} else if (action==PAPERI_ToggleSnap) {
		search_snap = !search_snap;
		PostMessage(search_snap ? _("Snap on (hold shift to not snap)") : _("Snap off (hold shift to snap)"));
		return 0;

	} else if (action==PAPERI_ToggleLabels) {
		show_labels = !show_labels;
		PostMessage(show_labels ? _("Show labels") : _("Don't show labels"));
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
			x = curboxes.e[c]->m(0);
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

//	} else if (action==PAPERI_RotateCC) {
//		 //rotate by 90 degree increments
//		if (!curboxes.n) return 0;
//		double x,y;
//		for (int c=0; c<curboxes.n; c++) {
//			 //rotate x axis
//			x=curboxes.e[c]->m(0);
//			y=curboxes.e[c]->m(1);
//			curboxes.e[c]->m(0,y);
//			curboxes.e[c]->m(1,-x);
//			 //rotate y axis
//			x=curboxes.e[c]->m(2);
//			y=curboxes.e[c]->m(3);
//			curboxes.e[c]->m(2,y);
//			curboxes.e[c]->m(3,-x);
//		}
//		needtodraw=1;
//		return 0;
	}

	return 1;
}

int PaperInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state & LAX_STATE_MASK, 0);
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

