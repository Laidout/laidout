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
// Copyright (C) 2012 by Tom Lechner
//

#include "../language.h"
#include "../headwindow.h"
#include "polyptychwindow.h"
#include "polyptych/src/hedronwindow.h"
#include <lax/button.h>

#ifndef LAIDOUT_NOGL

using namespace Laxkit;
using namespace Polyptych;

#include <iostream>
using namespace std;
#define DBG


/*! \class PolyptychWindow
 * \brief Class to help edit polyhedra with Polyptych.
 */

PolyptychWindow::PolyptychWindow(NetImposition *imp, anXWindow *parnt,unsigned long owner,const char *sendmes)
  : RowFrame(parnt,"Polyptych","Polyptych",ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
	         0,0,500,500,0, NULL,owner,sendmes,
	         10)
{
	HedronWindow *hw=new Polyptych::HedronWindow(this, "Hedron","Hedron",0,
												 0,0,0,0,0,
												 imp?dynamic_cast<Polyhedron*>(imp->abstractnet):NULL);
	AddWin(hw,1, 1,0,3000,50,0, 1,0,3000,50,0, -1);
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
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

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

NetImposition *PolyptychWindow::getImposition()
{
	return NULL;
	//***
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



