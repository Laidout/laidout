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
// Copyright (C) 2004-2007,2011 by Tom Lechner
//


#include "../language.h"
#include "paperinterface.h"
#include "../viewwindow.h"
#include "../drawdata.h"
#include "../dataobjects/printermarks.h"
#include <lax/inputdialog.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/colors.h>

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
	return NULL;
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

PaperInterface::PaperInterface(int nid,Displayer *ndp)
	: anInterface(nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	showdecs=0;

	curbox=maybebox=NULL;

	papergroup=NULL;
	paperboxdata=NULL;
	doc=NULL;

	sc=NULL;
}

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	showdecs=0;

	curbox=maybebox=NULL;

	papergroup=NULL;
	paperboxdata=NULL;
	doc=NULL;

	sc=NULL;
}

PaperInterface::~PaperInterface()
{
	DBG cerr <<"PaperInterface destructor.."<<endl;

	if (maybebox) maybebox->dec_count();
	if (papergroup) papergroup->dec_count();
	if (doc) doc->dec_count();
	if (sc) sc->dec_count();
}

const char *PaperInterface::Name()
{ return _("Paper Group Tool"); }

#define PAPERM_PaperSize        1
#define PAPERM_Landscape        2 
#define PAPERM_Portrait         3 
#define PAPERM_NewPaperGroup    4 
#define PAPERM_RenamePaperGroup 5
#define PAPERM_DeletePaperGroup 6 
#define PAPERM_Print            7 
#define PAPERM_RegistrationMark 8
#define PAPERM_GrayBars         9 
#define PAPERM_CutMarks         10 
#define PAPERM_ResetScaling     11
#define PAPERM_ResetAngle       12

#define PAPERM_first_pagesize   1000
#define PAPERM_first_papergroup 2000

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PaperInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	rx=x,ry=y;

	if (!menu) menu=new MenuInfo(_("Paper Interface"));
	else menu->AddSep(_("Papers"));

	if (papergroup) {
		menu->AddItem(_("Add Registration Mark"),PAPERM_RegistrationMark);
		menu->AddItem(_("Add Gray Bars"),PAPERM_GrayBars);
		menu->AddItem(_("Add Cut Marks"),PAPERM_CutMarks);
		menu->AddSep();
		menu->AddItem(_("Reset paper scaling"),PAPERM_ResetScaling);
	}

	if (papergroup && curboxes.n) {

		menu->AddItem(_("Reset paper angle"),PAPERM_ResetAngle);
		menu->AddItem(_("Paper Size"),PAPERM_PaperSize);
		menu->SubMenu(_("Paper Size"));
		for (int c=0; c<laidout->papersizes.n; c++) {
			menu->AddItem(laidout->papersizes.e[c]->name,PAPERM_first_pagesize+c,
					LAX_ISTOGGLE
					| (!strcmp(curboxes.e[0]->box->paperstyle->name,laidout->papersizes.e[c]->name)
					  ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
		//int landscape=curboxes.e[0]->box->paperstyle->flags&1;
		//menu->AddItem(_("Portrait"), PAPERM_Portrait, LAX_OFF|MENU_ISTOGGLE|(landscape?0:MENU_CHECKED));
		//menu->AddItem(_("Landscape"),PAPERM_Landscape, LAX_OFF|MENU_ISTOGGLE|(landscape?MENU_CHECKED:0));
		menu->AddItem(_("Print with paper group"),PAPERM_Print);
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
			menu->AddItem(nme, PAPERM_first_papergroup+c,
						  LAX_ISTOGGLE | LAX_OFF | (papergroup==pg ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
	}
	menu->AddItem(_("New paper group..."),PAPERM_NewPaperGroup);
	if (papergroup) menu->AddItem(_("Rename current paper group..."),PAPERM_RenamePaperGroup);
	if (papergroup) menu->AddItem(_("Delete current paper group..."),PAPERM_DeletePaperGroup);

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
		if (i==PAPERM_Portrait) {
			 //portrait
			return 0;

		} else if (i==PAPERM_Landscape) {
			 //landscape
			return 0;

		} else if (i==PAPERM_Print) {
			 //print with the active paper group
			curwindow->win_parent->Event(NULL,"print");//***hack Hack HACK
			return 0;

		} else if (i==PAPERM_ResetScaling) {
			if (!papergroup) return 0;
			PaperBoxData *data=papergroup->papers.e[0];
			double s=1/data->xaxis().norm();
			for (int c=0; c<papergroup->papers.n; c++) {
				data=papergroup->papers.e[c];
				data->Scale(s);
			}
			needtodraw=1;
			return 0;

		} else if (i==PAPERM_ResetAngle) {
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

		} else if (i>=PAPERM_first_pagesize && i<PAPERM_first_pagesize+1000) {
			 //paper size
			i-=PAPERM_first_pagesize;
			if (i>=laidout->papersizes.n-1) return 1;
			PaperStyle *newpaper=(PaperStyle *)laidout->papersizes.e[i]->duplicate();
			for (int c=0; c<curboxes.n; c++) {
				curboxes.e[c]->box->Set(newpaper);
				curboxes.e[c]->setbounds(&curboxes.e[c]->box->media);
			}
			newpaper->dec_count();
			needtodraw=1;
			return 0;

		} else if (i>=PAPERM_first_papergroup && i<PAPERM_first_papergroup+1000) {
			//***is selecting a new papergroup from laidout->project->papergroups	
			i-=PAPERM_first_papergroup;
			if (i<0 || i>laidout->project->papergroups.n) return 0;

			Clear(NULL);
			papergroup=laidout->project->papergroups.e[i];
			papergroup->inc_count();
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;

		} else if (i==PAPERM_NewPaperGroup) {
			 //New paper group...
			if (papergroup) papergroup->dec_count();
			papergroup=new PaperGroup;
			papergroup->name=new_paper_group_name();
			laidout->project->papergroups.push(papergroup);
			
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
			needtodraw=1;
			return 0;

		} else if (i==PAPERM_RenamePaperGroup) {
			//***Rename paper group...
			if (!papergroup) return 0;
			InputDialog *i=new InputDialog(NULL,_("New paper group name"),_("New paper group name"),ANXWIN_CENTER,
									 0,0,0,0,0,
									 NULL,object_id,"newname",
									 papergroup->name?papergroup->name:papergroup->Name, _("Name:"),
									 _("Ok"),BUTTON_OK,
									 _("Cancel"),BUTTON_CANCEL);
			app->rundialog(i);
			return 0;

		} else if (i==PAPERM_RegistrationMark) {
			if (!papergroup) return 0;
			SomeData *obj= RegistrationMark(18,1);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			papergroup->objs.push(obj);
			obj->dec_count();
			return 0;

		} else if (i==PAPERM_GrayBars) {
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


//! Return a new PaperInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PaperInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PaperInterface(id,NULL);
	else if (!dynamic_cast<PaperInterface *>(dup)) return NULL;
	
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
	Clear(NULL);
	showdecs=0;

	LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
	if (lvp) lvp->UseThisPaperGroup(NULL);

	if (maybebox) { maybebox->dec_count(); maybebox=NULL; }

	needtodraw=1;
	DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

//! Use a PaperGroup. Does NOT update viewport.
int PaperInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	PaperGroup *pg=dynamic_cast<PaperGroup *>(ndata);
	if (!pg && ndata) return 0; //was a non-null object, but not a papergroup
	Clear(NULL);
	
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
	if (maybebox) { maybebox->dec_count(); maybebox=NULL; }
	if (curbox) { curbox->dec_count(); curbox=NULL; }
	curboxes.flush();
	if (papergroup) { papergroup->dec_count(); papergroup=NULL; }
}

	
/*! This will be called by viewports, and the papers will be displayed opaque with
 * drop shadow.
 */
int PaperInterface::DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
{
	PaperBoxData *data=dynamic_cast<PaperBoxData *>(tdata);
	if (!data) return 1;
	int td=showdecs,ntd=needtodraw;
	BoxTypes tdrawwhat=drawwhat;
	drawwhat=AllBoxes;
	showdecs=1;
	needtodraw=1;
	Displayer *olddp=dp;
	dp=tdp;
	DrawPaper(data,~0,1,5,0);
	dp=olddp;
	drawwhat=tdrawwhat;
	needtodraw=ntd;
	showdecs=td;
	paperboxdata=NULL;
	return 1;
}

/*! If fill!=0, then fill the media box with the paper color.
 * If shadow!=0, then put a black drop shadow under a filled media box,
 * at an offset pixel length shadow.
 */
void PaperInterface::DrawPaper(PaperBoxData *data,int what,char fill,int shadow,char arrow)
{
	if (!data) return;

	int w=1;
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
				Laidout::DrawData(dp,group->objs.e(c),NULL,NULL,0);
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

//! Make a maybebox, if one does not exist.
void PaperInterface::createMaybebox(flatpoint p)
{
	if (maybebox) return;

	 // find a paper size, not so easy!
	PaperBox *box=NULL;
	PaperBoxData *boxdata=NULL;
	int del=0;
	if (curbox) {
		box=curbox->box;
		boxdata=curbox;
	} else if (papergroup && papergroup->papers.n) {
		box=papergroup->papers.e[0]->box;
		boxdata=papergroup->papers.e[0];
	} else if (doc) {
		box=new PaperBox((PaperStyle *)doc->imposition->paper->paperstyle->duplicate());
		box->paperstyle->dec_count();
		del=1;
	} else {
		box=new PaperBox((PaperStyle *)laidout->papersizes.e[0]->duplicate());
		box->paperstyle->dec_count();
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
}

/*! Add maybe box if shift is down.
 */
int PaperInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	mx=x; my=y;
	DBG flatpoint fp;
	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"1 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	int over=scan(x,y);

	DBG fp=dp->screentoreal(x,y);
	DBG cerr <<"2 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
		//add a new box
		if (!maybebox) createMaybebox(dp->screentoreal(x,y));

		DBG fp=dp->screentoreal(x,y);
		DBG cerr <<"3 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		if (!papergroup) {
			papergroup=new PaperGroup;
			papergroup->Name=new_paper_group_name();
			laidout->project->papergroups.push(papergroup);
			
			DBG fp=dp->screentoreal(x,y);
			DBG cerr <<"4 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);

			DBG fp=dp->screentoreal(x,y);
			DBG cerr <<"5 *****ARG**** "<<fp.x<<","<<fp.y<<endl;
		}
		over=papergroup->papers.push(maybebox);

		maybebox->dec_count();
		maybebox=NULL;
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
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

int PaperInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	DBG flatpoint fpp=dp->screentoreal(x,y);
	DBG cerr <<"mm *****ARG**** "<<fpp.x<<","<<fpp.y<<endl;

	int over=scan(x,y);

	DBG cerr <<"over box: "<<over<<endl;

	buttondown.move(mouse->id,x,y);
	if (!buttondown.any()) {
		if (!(state&ShiftMask)) return 1;
		//*** activate maybebox
		if (over>=0) {
			if (maybebox) { 
				maybebox->dec_count();
				maybebox=NULL;
				needtodraw=1;
			}
			mx=x; my=y;
			return 0;
		}
		if (!maybebox) {
			createMaybebox(dp->screentoreal(x,y));
		} else {
			maybebox->origin(maybebox->origin()
						+dp->screentoreal(x,y)-dp->screentoreal(mx,my));
		}
		mx=x; my=y;
		needtodraw=1;
		return 0;
	}

	if (curboxes.n==0) return 1;

	 //if curboxes.n>0, this implies papergroup!=NULL

	flatpoint fp=dp->screentoreal(x,y),
			  d =fp-dp->screentoreal(mx,my);

	 //plain or + moves curboxes (or the box given by editwhat)
	if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
		// ***snapto

		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->origin(curboxes.e[c]->origin()+d);
		}
		needtodraw=1;
		mx=x; my=y;
		return 0;
	}

	 //^ scales
	if ((state&LAX_STATE_MASK)==ControlMask) {
		PaperBoxData *data;
		//flatpoint leftd=transform_point_inverse(data->m(),lbdown);
		for (int c=0; c<papergroup->papers.n; c++) {
			data=papergroup->papers.e[c];
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
		//oo=data->origin() + leftp.x*data->xaxis() + leftp.y*data->yaxis(); // where the point clicked down on is now
		////DBG cerr <<"  oo="<<oo.x<<','<<oo.y<<endl;
		//d=lp-oo;
		//data->origin(data->origin()+d);
		needtodraw=1;
		mx=x; my=y;
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
		mx=x; my=y;
		return 0;
	}

	return 0;
}

enum PaperInterfaceActions {
	PAPERI_Select,
	PAPERI_Decorations,
	PAPERI_Delete,
	PAPERI_Rectify,
	PAPERI_Rotate,
	PAPERI_RotateCC,
	PAPERI_MAX
};

Laxkit::ShortcutHandler *PaperInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("PaperInterface");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("PaperInterface");

	sc->Add(PAPERI_Select,      'a',0,0,         "Select",  _("Select or deselect all"),NULL,0);
	sc->Add(PAPERI_Decorations, 'd',0,0,         "Decs",    _("Toggle decorations"),NULL,0);
	sc->Add(PAPERI_Delete,      LAX_Bksp,0,0,    "Delete",  _("Delete selected"),NULL,0);
	sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);
	sc->Add(PAPERI_Rectify,     'o',0,0,         "Rectify", _("Make the axes horizontal and vertical"),NULL,0);
	sc->Add(PAPERI_Rotate,      'r',0,0,         "Rotate",  _("Rotate selected by 90 degrees"),NULL,0);
	sc->Add(PAPERI_RotateCC,    'R',ShiftMask,0, "RotateCC",_("Rotate selected by 90 degrees in the other direction"),NULL,0);


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
		if (curbox) { curbox->dec_count(); curbox=NULL; }
		if (n) return 0;
		for (int c=0; c<papergroup->papers.n; c++) 
			curboxes.push(papergroup->papers.e[c],0);
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
		if (curbox) { curbox->dec_count(); curbox=NULL; }
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

	} else if (action==PAPERI_Rotate) {
		 //rotate by 90 degree increments
		if (!curboxes.n) return 0;
		double x,y;
		for (int c=0; c<curboxes.n; c++) {
			 //rotate x axis
			x=curboxes.e[c]->m(0);
			y=curboxes.e[c]->m(1);
			curboxes.e[c]->m(0,-y);
			curboxes.e[c]->m(1,x);
			 //rotate y axis
			x=curboxes.e[c]->m(2);
			y=curboxes.e[c]->m(3);
			curboxes.e[c]->m(2,-y);
			curboxes.e[c]->m(3,x);
		}
		needtodraw=1;
		return 0;

	} else if (action==PAPERI_RotateCC) {
		 //rotate by 90 degree increments
		if (!curboxes.n) return 0;
		double x,y;
		for (int c=0; c<curboxes.n; c++) {
			 //rotate x axis
			x=curboxes.e[c]->m(0);
			y=curboxes.e[c]->m(1);
			curboxes.e[c]->m(0,y);
			curboxes.e[c]->m(1,-x);
			 //rotate y axis
			x=curboxes.e[c]->m(2);
			y=curboxes.e[c]->m(3);
			curboxes.e[c]->m(2,y);
			curboxes.e[c]->m(3,-x);
		}
		needtodraw=1;
		return 0;
	}

	return 1;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int PaperInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	
//	if (ch==' ') {
//		if (!papergroup) papergroup=new PaperGroup;
//		papergroup->AddPaper(8.5,11,-1,-1);
//		needtodraw=1;
//		return 0;
//	}

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	if (ch==LAX_Shift) {
		if (maybebox) return 0;
		int x,y;
		if (mouseposition(d->paired_mouse->id,viewport,&x,&y,NULL,NULL)!=0) return 0;
		if (scan(x,y)>=0) return 0;
		mx=x; my=y;
		createMaybebox(flatpoint(dp->screentoreal(x,y)));
		needtodraw=1;
		return 0;

	}
	return 1;
}

int PaperInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{//***
	if (ch==LAX_Shift) {
		if (!maybebox) return 1;
		maybebox->dec_count();
		maybebox=NULL;
		needtodraw=1;
		return 0;
	}
	return 1;
}


} // namespace Laidout

