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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include "../language.h"
#include "paperinterface.h"
#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


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
			if (!strcmp(str,laidout->project->papergroups.e[c]->Name)) break;
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
	: InterfaceWithDp(nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	curbox=maybebox=NULL;
	papergroup=NULL;
	paperboxdata=NULL;
	showdecs=0;
	doc=NULL;

	papergroup=NULL;

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
}

PaperInterface::PaperInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	snapto=MediaBox;
	editwhat=MediaBox;
	drawwhat=MediaBox;
	curbox=maybebox=NULL;
	papergroup=NULL;
	paperboxdata=NULL;
	showdecs=0;
	doc=NULL;

	papergroup=NULL;

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
}

PaperInterface::~PaperInterface()
{
	DBG cerr <<"PaperInterface destructor.."<<endl;

	if (maybebox) maybebox->dec_count();
	if (papergroup) papergroup->dec_count();
	if (doc) doc->dec_count();
}

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PaperInterface::ContextMenu(int x,int y)
{
	MenuInfo *menu=new MenuInfo(_("Paper Interface"));
	if (papergroup && curboxes.n) {

		menu->AddItem(_("Paper Size"),999);
		menu->SubMenu(_("Paper Size"));
		for (int c=0; c<laidout->papersizes.n; c++) {
			menu->AddItem(laidout->papersizes.e[c]->name,c,
					LAX_ISTOGGLE
					| (!strcmp(curboxes.e[0]->box->paperstyle->name,laidout->papersizes.e[c]->name)
					  ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
		//int landscape=curboxes.e[0]->box->paperstyle->flags&1;
		//menu->AddItem(_("Portrait"), 1000, LAX_OFF|MENU_ISTOGGLE|(landscape?0:MENU_CHECKED));
		//menu->AddItem(_("Landscape"),1001, LAX_OFF|MENU_ISTOGGLE|(landscape?MENU_CHECKED:0));
		menu->AddItem(_("Print with paper group"),1002);
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
			menu->AddItem(nme,2000+c,
						  LAX_ISTOGGLE | LAX_OFF | (papergroup==pg ? LAX_CHECKED : 0));
		}
		menu->EndSubMenu();
	}
	menu->AddItem(_("New paper group..."),3000);
	if (papergroup) menu->AddItem(_("Rename current paper group..."),3001);

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int PaperInterface::MenuEvent(XClientMessageEvent *e)
{
	int i=e->data.l[1];
	if (i==1000) {
		 //portrait
		return 0;
	} else if (i==1001) {
		 //landscape
		return 0;
	} else if (i==1002) {
		 //print with the active paper group
		curwindow->win_parent->ClientEvent(NULL,"print");//***hack Hack HACK
		return 0;
	} else if (i>=0 && i<1000) {
		 //paper size
		if (i>=laidout->papersizes.n-1) return 1;
		PaperStyle *newpaper=(PaperStyle *)laidout->papersizes.e[i]->duplicate();
		for (int c=0; c<curboxes.n; c++) {
			curboxes.e[c]->box->Set(newpaper);
			curboxes.e[c]->setbounds(&curboxes.e[c]->box->media);
		}
		newpaper->dec_count();
		needtodraw=1;
		return 0;
	} else if (i>=2000 && i<3000) {
		//***is selecting a new papergroup from laidout->project->papergroups	
		i-=2000;
		if (i<0 || i>laidout->project->papergroups.n) return 0;

		Clear(NULL);
		papergroup=laidout->project->papergroups.e[i];
		papergroup->inc_count();
		LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
		if (lvp) lvp->UseThisPaperGroup(papergroup);
		needtodraw=1;
		return 0;

	} else if (i==3000) {
		 //New paper group...
		if (papergroup) papergroup->dec_count();
		papergroup=new PaperGroup;
		papergroup->Name=new_paper_group_name();
		laidout->project->papergroups.push(papergroup);
		
		LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
		if (lvp) lvp->UseThisPaperGroup(papergroup);
		needtodraw=1;
		return 0;
	} else if (i==3001) {
		//***Rename paper group...
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
	if (lvp) lvp->UseThisPaperGroup(papergroup);
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

	
//----This interface, like Spread Editor, keeps everything to itself, so doesn't need:
//int PaperInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask=0)
//{
//	return 0;
//}
//
//int PaperInterface::DrawData(Laxkit::anObject *ndata,
//			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0)
//{***
//}

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
	XSetLineAttributes(dp->GetDpy(),dp->GetGC(),w,LineSolid,CapButt,JoinMiter);
	//dp->PushAndNewTransform(data->m());

	PaperBox *box=data->box;
	flatpoint p[4];
	if ((what&MediaBox) && (box->which&MediaBox)) {
		p[0]=flatpoint(box->media.minx,box->media.miny);
		p[1]=flatpoint(box->media.minx,box->media.maxy);
		p[2]=flatpoint(box->media.maxx,box->media.maxy);
		p[3]=flatpoint(box->media.maxx,box->media.miny);

		XSetFillStyle(dp->GetDpy(),dp->GetGC(),FillSolid);
		XSetFillRule(dp->GetDpy(),dp->GetGC(),WindingRule);

		 //draw black shadow
		//dp->NewFG(255,0,0);
		//dp->NewBG(~0);
		if (shadow) {
			dp->NewFG(0,0,0);
			dp->PushAxes();
			dp->ShiftScreen(shadow,shadow);
			dp->drawlines(1,1,1,4,p);
			dp->PopAxes();
		}
		
		 //draw white fill or plain outline
		if (fill||shadow) {
			dp->NewFG(data->red>>8,data->green>>8,data->blue>>8);
			dp->NewBG(~0);
			dp->drawlines(1,1,2,4,p);
		} else {
			dp->NewFG(data->red>>8,data->green>>8,data->blue>>8);
			dp->drawlines(1,1,0,4,p);
		}

		 //draw orientation arrow
		if (arrow) {
			flatpoint p1=dp->realtoscreen((p[0]+p[3])/2),
					  p2=dp->realtoscreen((p[1]+p[2])/2),
					  v=p2-p1,
					  v2=transpose(v);
			p1=p1+.1*v;
			p2=p2-.1*v;
			dp->drawline(p1,p2);
			dp->drawline(p2,p2-v2*.2-v*.2);
			dp->drawline(p2,p2+v2*.2-v*.2);
		}

	}
	if ((what&ArtBox) && (box->which&ArtBox)) {
		p[0]=dp->realtoscreen(box->art.minx,box->art.miny);
		p[1]=dp->realtoscreen(box->art.minx,box->art.maxy);
		p[2]=dp->realtoscreen(box->art.maxx,box->art.maxy);
		p[3]=dp->realtoscreen(box->art.maxx,box->art.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&TrimBox) && (box->which&TrimBox)) {
		p[0]=dp->realtoscreen(box->trim.minx,box->trim.miny);
		p[1]=dp->realtoscreen(box->trim.minx,box->trim.maxy);
		p[2]=dp->realtoscreen(box->trim.maxx,box->trim.maxy);
		p[3]=dp->realtoscreen(box->trim.maxx,box->trim.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&PrintableBox) && (box->which&PrintableBox)) {
		p[0]=dp->realtoscreen(box->printable.minx,box->printable.miny);
		p[1]=dp->realtoscreen(box->printable.minx,box->printable.maxy);
		p[2]=dp->realtoscreen(box->printable.maxx,box->printable.maxy);
		p[3]=dp->realtoscreen(box->printable.maxx,box->printable.miny);
		dp->drawlines(1,0,0,4,p);
	}
	if ((what&BleedBox) && (box->which&BleedBox)) {
		p[0]=dp->realtoscreen(box->bleed.minx,box->bleed.miny);
		p[1]=dp->realtoscreen(box->bleed.minx,box->bleed.maxy);
		p[2]=dp->realtoscreen(box->bleed.maxx,box->bleed.maxy);
		p[3]=dp->realtoscreen(box->bleed.maxx,box->bleed.miny);
		dp->drawlines(1,0,0,4,p);
	}
	//dp->PopAxes(); //spread axes
}

void PaperInterface::DrawGroup(PaperGroup *group,char shadow,char fill,char arrow)
{
	 //draw shadow under whole group
	if (shadow) {
		for (int c=0; c<group->papers.n; c++) {
			dp->PushAndNewTransform(group->papers.e[c]->m());
			DrawPaper(group->papers.e[c],MediaBox, fill,5,0);
			dp->PopAxes(); 
		}
	}
	for (int c=0; c<group->papers.n; c++) {
		dp->PushAndNewTransform(group->papers.e[c]->m());
		DrawPaper(group->papers.e[c],drawwhat, fill,0,arrow);
		dp->PopAxes(); 
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
		double m[6];
		transform_copy(m,dp->m());
		dp->PopAxes(); //remove transform to viewport context
		PaperBox *box;
		flatpoint p[4];
		if (maybebox) {
			box=maybebox->box;
			dp->PushAndNewTransform(maybebox->m());
			XSetLineAttributes(dp->GetDpy(),dp->GetGC(),0,LineDoubleDash,CapButt,JoinMiter);
			p[0]=dp->realtoscreen(box->media.minx,box->media.miny);
			p[1]=dp->realtoscreen(box->media.minx,box->media.maxy);
			p[2]=dp->realtoscreen(box->media.maxx,box->media.maxy);
			p[3]=dp->realtoscreen(box->media.maxx,box->media.miny);
			dp->drawlines(1,0,0,4,p);
			dp->PopAxes(); 
		}

		if (!papergroup || !papergroup->papers.n) return 0;

		DrawGroup(papergroup,0,showdecs==1?1:0,showdecs==2?1:0);

		dp->PushAndNewTransform(m); //reinstall transform to viewport context
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
		box=new PaperBox((PaperStyle *)doc->docstyle->imposition->paper->paperstyle->duplicate());
		box->paperstyle->dec_count();
		del=1;
	} else {
		box=new PaperBox((PaperStyle *)laidout->papersizes.e[0]->duplicate());
		box->paperstyle->dec_count();
		del=1;
	}

	maybebox=new PaperBoxData(box); //incs count of box
	maybebox->red=maybebox->blue=65535;
	maybebox->green=0;
	if (boxdata) {
		maybebox->xaxis(boxdata->xaxis());
		maybebox->yaxis(boxdata->yaxis());
	}
	maybebox->origin(p-flatpoint(norm(maybebox->xaxis())*(maybebox->maxx-maybebox->minx)/2,
								 norm(maybebox->yaxis())*(maybebox->maxy-maybebox->miny)/2));
	if (del) box->dec_count();
}

/*! Add maybe box if shift is down.
 */
int PaperInterface::LBDown(int x,int y,unsigned int state,int count)
{
	buttondown|=LEFTBUTTON;

	mx=x; my=y;
	flatpoint fp=dp->screentoreal(x,y);
	int over=scan(x,y);
	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
		//add a new box
		if (!maybebox) createMaybebox(dp->screentoreal(x,y));
		if (!papergroup) {
			papergroup=new PaperGroup;
			papergroup->Name=new_paper_group_name();
			laidout->project->papergroups.push(papergroup);
			
			LaidoutViewport *lvp=dynamic_cast<LaidoutViewport *>(curwindow);
			if (lvp) lvp->UseThisPaperGroup(papergroup);
		}
		over=papergroup->papers.push(maybebox);
		maybebox->dec_count();
		maybebox=NULL;
		needtodraw=1;
		//return 0; -- do not return, continue to let box be added..
	}

	if (over>=0) {
		if (curbox) curbox->dec_count();
		curbox=papergroup->papers.e[over];
		curbox->inc_count();
		if ((state&LAX_STATE_MASK)!=ShiftMask) curboxes.flush();
		curboxes.pushnodup(curbox,0);
		needtodraw=1;
	}
	lbdown=dp->screentoreal(x,y);

	return 0;
}

int PaperInterface::LBUp(int x,int y,unsigned int state)
{
	if (!(buttondown&LEFTBUTTON)) return 1;
	buttondown&=~LEFTBUTTON;

	//***
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

//int PaperInterface::MBDown(int x,int y,unsigned int state,int count)
//int PaperInterface::MBUp(int x,int y,unsigned int state)
//int PaperInterface::RBDown(int x,int y,unsigned int state,int count)
//int PaperInterface::RBUp(int x,int y,unsigned int state)
//int PaperInterface::But4(int x,int y,unsigned int state);
//int PaperInterface::But5(int x,int y,unsigned int state);

int PaperInterface::MouseMove(int x,int y,unsigned int state)
{
	int over=scan(x,y);

	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown) {
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
			  d=fp-dp->screentoreal(mx,my);

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

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int PaperInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	if (ch==LAX_Shift) {
		if (maybebox) return 0;
		int x,y;
		if (mouseposition(viewport,&x,&y,NULL,NULL,NULL)!=0) return 0;
		if (scan(x,y)>=0) return 0;
		mx=x; my=y;
		createMaybebox(flatpoint(dp->screentoreal(x,y)));
		needtodraw=1;
		return 0;
	} else if (ch==LAX_Left && (state&LAX_STATE_MASK)==0) {
		//*** select previous paper
		return 0;
	} else if (ch==LAX_Right && (state&LAX_STATE_MASK)==0) {
		//*** select next paper
		return 0;
	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) {
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
	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
		if (!papergroup) return 1;
		needtodraw=1;
		int n=curboxes.n;
		curboxes.flush();
		if (curbox) { curbox->dec_count(); curbox=NULL; }
		if (n) return 0;
		for (int c=0; c<papergroup->papers.n; c++) 
			curboxes.push(papergroup->papers.e[c],0);
		return 0;
	} else if (ch=='r' && (state&LAX_STATE_MASK)==0) {
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
	} else if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;
	} else if (ch=='9' && (state&LAX_STATE_MASK)==0) {
		 //rotate by 90 degree increments
		if (curboxes.n) {
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
		}
	}
	return 1;
}

int PaperInterface::CharRelease(unsigned int ch,unsigned int state)
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


//} // namespace Laidout

