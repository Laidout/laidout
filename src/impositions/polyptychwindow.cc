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
// Copyright (C) 2012 by Tom Lechner
//

#include "../language.h"
#include "../headwindow.h"
#include "polyptychwindow.h"
#include "polyptych/src/hedronwindow.h"
#include "../laidout.h"

#include <lax/button.h>
#include <lax/units.h>

#ifndef LAIDOUT_NOGL

using namespace Laxkit;
using namespace Polyptych;

#include <iostream>
using namespace std;
#define DBG



namespace Laidout {


//----------------------------------------- PolyhedronWindow --------------------------------------
class PolyhedronWindow : public HedronWindow
{
  public:
	PolyhedronWindow(anXWindow *parent);
	int changePaper(int towhich,int index);
};

PolyhedronWindow::PolyhedronWindow(anXWindow *parent)
  : HedronWindow(parent, "Hedron","Hedron",0, 0,0,0,0,0, NULL)
{
	int i=0;
	PaperStyle *p=laidout->papersizes.e[i];

	default_paper.id=i;
	default_paper.width =p->width;
	default_paper.height=p->height;
	makestr(default_paper.name,p->name);
	makestr(default_paper.units,"in");
	//makestr(default_paper.units,p->defaultunits);
}

//! Change paper index to a different type of paper.
/*! towhich==-2 for previous paper, -1 for next, or >=0 for absolute paper num
 *
 * Return 1 for successful change. 0 for no change.
 */
int PolyhedronWindow::changePaper(int towhich,int index)
{
	//DBG cerr << "change paper"<<endl;
	if (index<0 || index>=papers.n) return 0;

	int i=papers.e[index]->id;
	if (towhich==-2) i--;
	else if (towhich==-1) i++;
	if (i<0) i=laidout->papersizes.n-1;
	else if (i>laidout->papersizes.n-1) i=0;

	PaperStyle *p=laidout->papersizes.e[i];

	UnitManager *unitm=GetUnitManager();

	papers.e[index]->id=i;
	papers.e[index]->width =unitm->Convert(p->width, "in",p->defaultunits,NULL);
	papers.e[index]->height=unitm->Convert(p->height,"in",p->defaultunits,NULL);
	makestr(papers.e[index]->name,p->name);
	makestr(papers.e[index]->units,p->defaultunits);
	remapPaperOverlays();

	needtodraw=1;
	return 1;
}


//----------------------------------------- PolyptychWindow --------------------------------------

/*! \class PolyptychWindow
 * \brief Class to help edit polyhedra with Polyptych.
 */

PolyptychWindow::PolyptychWindow(NetImposition *imp, anXWindow *parnt,unsigned long owner,const char *sendmes)
  : RowFrame(parnt,"Polyptych","Polyptych",ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
	         0,0,500,500,0, NULL,owner,sendmes,
	         10)
{
	PolyhedronWindow *hw=new PolyhedronWindow(this);
	hwindow=hw;
	hw->GetShortcuts();
	AddWin(hw,1, 1,0,3000,50,0, 1,0,3000,50,0, -1);

	if (imp) setImposition(imp);
}

PolyptychWindow::~PolyptychWindow()
{
}

int PolyptychWindow::init()
{
	WinFrameBox *wfb=dynamic_cast<WinFrameBox*>(wholelist.e[0]);
	anXWindow *last=(wfb?wfb->win():NULL);
	Button *tbut=NULL;

	AddNull();

	//-------Cancel
	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	AddHSpacer(0,0,5000,50); //gap to push ok and cancel to opposite sides

	//--------Ok
	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last->CloseControlLoop();
	Sync(1);
	return 0;
}


int PolyptychWindow::Event(const EventData *data,const char *mes)
{
	//DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"ok")) {
		int status=sendNewImposition();
		if (status!=0) return 0;

		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"cancel")) {
		EventData *e=new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;
	}

	return RowFrame::Event(data,mes);
}

/*! Return 0 for success, nonzero for error.
 */
int PolyptychWindow::setImposition(NetImposition *imp)
{
	if (!imp) return 1;

	if (!(imp->abstractnet && dynamic_cast<Polyhedron*>(imp->abstractnet))) return 2;
	
	HedronWindow *hw=dynamic_cast<HedronWindow*>(hwindow);
	////DBG if (!hw) cerr<<" ***** no child window Hedron"<<endl;

	 //install hedron
	hw->installPolyhedron(dynamic_cast<Polyhedron*>(imp->abstractnet));

	 //add papers
	if (imp->papergroup) {
		PaperBound paper;
		for (int c=0; c<imp->papergroup->papers.n; c++) {
			paper.width=imp->papergroup->papers.e[c]->box->paperstyle->w();
			paper.height=imp->papergroup->papers.e[c]->box->paperstyle->h();
			paper.matrix.m(imp->papergroup->papers.e[c]->m());
			makestr(paper.name, imp->papergroup->papers.e[c]->box->paperstyle->name);
			hw->AddPaper(&paper);
		}
	}

	 //add nets
	for (int c=0; c<imp->nets.n; c++) {
		if (!imp->nets.e[c]->active) continue;
		hw->AddNet(imp->nets.e[c]);
	}

	return 0;
}

NetImposition *PolyptychWindow::getImposition()
{
	NetImposition *imp=new NetImposition;
	PaperGroup *papers=new PaperGroup;

	HedronWindow *hw=dynamic_cast<HedronWindow*>(findChildWindowByName("Hedron"));
	//DBG if (!hw) cerr<<" ***** no child window Hedron"<<endl;

	 //install hedron
	imp->abstractnet=hw->poly;
	imp->abstractnet->inc_count();

	 //add papers
	if (hw->papers.n) {
		for (int c=0; c<hw->papers.n; c++) {
			papers->AddPaper(hw->papers.e[c]->name,
					hw->papers.e[c]->width,hw->papers.e[c]->height,
					hw->papers.e[c]->matrix.m()
					);
		}
	}

	 //add nets
	Net *net;
	for (int c=0; c<hw->nets.n; c++) {
		net=hw->nets.e[c]->duplicate();
		imp->nets.push(net);
		net->dec_count();
	}

	if (!imp->numActiveNets()) {
		delete imp;
		return NULL;
	}
	
	return imp;	
}

int PolyptychWindow::sendNewImposition()
{
	NetImposition *imp=getImposition();
	if (!imp) return 1;

	RefCountedEventData *data=new RefCountedEventData(imp);
	imp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);

	return 0;
}


#endif //LAIDOUT_NOGL


} // namespace Laidout

