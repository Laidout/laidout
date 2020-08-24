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
// Copyright (C) 2020 by Tom Lechner
//


#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/tabframe.h>
#include <lax/button.h>
#include <lax/colorbox.h>
#include <lax/units.h>

#include "FindWindow.h"
#include "../language.h"
#include "../core/utils.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


//--------------------------------- FindWindow ------------------------------------
/*! \class FindWindow
 * \brief Window that lets you search for objects.
 */  

FindWindow::FindWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							unsigned long owner, const char *msg)
		: RowFrame(parnt,nname,ntitle,nstyle | ROWFRAME_HORIZONTAL | ROWFRAME_LEFT | ANXWIN_REMEMBER,
					0,0,0,0,0, nullptr,owner,msg,
					10)
{
	InstallColors(THEME_Panel);
	
	if (!win_parent) win_style |= ANXWIN_ESCAPABLE;

	howmany = nullptr;
	pattern = nullptr;
	founditems = nullptr;

	// continuous_update = false;
}

FindWindow::~FindWindow()
{
}

int FindWindow::preinit()
{
	anXWindow::preinit();
	if (win_w <= 0 || win_h <= 0) flags |= BOX_WRAP_TO_EXTENT; //WrapToExtent();
	// if (win_w <= 0) win_w = 500;
	// if (win_h <= 0) win_h = 600;
	return 0;
}


int FindWindow::init()
{
	//   ____pattern_____ .* Aa all (is regex, case sensitive)
	//   [next] [prev] [all]
	//   [ 5 found: (found object thumbnail list) ]
	//   search:
	//     name desc other-meta
	//     broken
	//     by type
	//     scriptable pre-flighty conditions
	//     current view only | in current selection | everywhere
	//   foreach: scriptable action
	// 

	int         textheight = win_themestyle->normal->textheight();
	int         linpheight = textheight + 12;
	Button *    tbut = nullptr;
	anXWindow * last = nullptr;
	LineInput * linp;

	padinset = textheight / 2;


	 // -------------- pattern input --------------------

	last = pattern = new LineInput(this,"pattern",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"pattern",
			            _("Find:"), nullptr,0,
			            100,0,1,1,3,3);
	pattern->tooltip(
			regex 
			  ? _("Pattern as a regular expressions.")
			  : _("Space separated list of patterns. Put a minus in front to ignore.")
			);
	pattern->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_OFF, true);
	AddWin(pattern,1, paperx->win_w,0,0,50,0, linpheight,0,0,50,0, -1);


	 // -------------- regex
	last = tbut = new Button(this, "regex", nullptr, IBUT_FLAT | BUTTON_TOGGLE, 0,0,0,0,1, last,object_id,"regex",
						 0, //id
						 ".*", //label
						 nullptr,nullptr); //img filename, img
	tbut->tooltip(_("Toggle using regular expressions"));
	tbut->State(regex ? LAX_ON : LAX_OFF);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	 // -------------- caseless
	last = tbut = new Button(this, "caseless", nullptr,IBUT_FLAT | BUTTON_TOGGLE, 0,0,0,0,1, last,object_id,"caseless",
						 0, //id
						 "Aa", //label
						 nullptr,nullptr); //img filename, img
	tbut->State(caseless ? LAX_ON : LAX_OFF);
	tbut->tooltip(_("Toggle caseles searching"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	 // ----- where: all, current view, selection
	SliderPopup *popup;
	last = popup = new SliderPopup(this,"where",nullptr,SLIDER_POP_ONLY, 0,0, 0,0, 1, last,object_id,"where");
	popup->AddItem(_("Anywhere"),  FIND_Anywhere);
	popup->AddItem(_("In view"),   FIND_InView);
	popup->AddItem(_("Selection"), FIND_InSelection);
	//if (c2 >= 0) popup->Select(c2);
	AddWin(popup, 1, 200, 100, 50, 50, 0, linpheight, 0, 0, 50, 0, -1);

	AddNull();
	
	
	 // -------------- Find prev
	last = tbut = new Button(this, "prev", nullptr,0, 0,0,0,0,1, last,object_id,"prev",
						 0, //id
						 _("Prev"), //label
						 nullptr,nullptr); //img filename, img
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last = tbut = new Button(this, "next", nullptr,0, 0,0,0,0,1, last,object_id,"next",
						 0, //id
						 _("Next"), //label
						 nullptr,nullptr); //img filename, img
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	last = tbut = new Button(this, "all", nullptr,0, 0,0,0,0,1, last,object_id,"findall",
						 0, //id
						 _("All"), //label
						 nullptr,nullptr); //img filename, img
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	AddNull();


	//--------------- Found ---------------------------
	MessageBar *howmany = new MessageBar(this,"foundmsg",nullptr, MB_MOVE, 0,0,0,0,0, _("Found:"));
	AddWin(howmany,1, 40,0,5000,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	last = founditems = new IconSelector(this, "founditems", nullptr, BOXSEL_ROWS | BOXSEL_CENTER | BOXSEL_SPACE | BOXSEL_FLAT,
						0,0,0,0, 1,
						last,object_id,"founditems",
						textheight/4,textheight/4);
	founditems->labelstyle = LAX_ICON_ONLY;
	AddWin(founditems,1, 100,0,5000,50,0, linpheight * 2,0,5000,50,0, -1);

	AddNull();
	

	//------------------------------ final ok -------------------------------------------------------
	
	if (!win_parent) {
		 // [ Done ]   [ cancel ]
		last = tbut = new Button(this,"ok",nullptr,0, 0,0,0,0,1, last,object_id,"Ok",
							 BUTTON_OK,
							 _("Done"),
							 nullptr,nullptr);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

//		last = tbut = new Button(this,"cancel",nullptr,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	}


	WrapToExtent();
	last->CloseControlLoop();
	Sync(1);
	return 0;
}


void FindWindow::SearchNext()
{
	// ***
}

void FindWindow::SearchPrevious()
{
	// ***
}

void FindWindow::SearchAll()
{
	// ***
}

void FindWindow::UpdateFound()
{
	if (!howmany) return;
	Utf8String str;
	str.Sprintf(_("Found (%d):"), selection->n());
	howmany->SetText(str.c_str());

	int dim = 50 * UIScale();
	founditems.Flush();
	for (int c=0; c<selection.n(); c++) {
		SomeData *obj = selection.e(c)->obj;
		LaxImage *img = GeneratePreview(obj, dim,dim, true);
		founditems.AddBox(obj->Id(), img, c);
	}
	founditems->sync();
}

int FindWindow::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;


	const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(data);

	if (!strcmp(mes,"pattern")) { 
		// int i = s->info1;
		// if (send_on_change) send(false);
		return 0;

	} else if (!strcmp(mes,"where")) {
		where = s->info1;
		return 0;

	} else if (!strcmp(mes,"prev")) {
		SearchPrevious();
		return 0;

	} else if (!strcmp(mes,"next")) {
		SearchNext();
		return 0;

	} else if (!strcmp(mes,"findall")) {
		SearchAll();
		return 0;

	} else if (!strcmp(mes,"regex")) {
		int l = s->info1;
		regex = (l == LAX_ON);
		return 0;

	} else if (!strcmp(mes,"caseless")) {
		int l = s->info1;
		caseless = (l == LAX_ON);
		return 0;


	} else if (!strcmp(mes,"Ok")) {
		send(true);
		if (win_parent) app->destroywindow(win_parent); // *** assuming parent is a headwindow? wut's going on here
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;
	}
	return 1;
}


void FindWindow::send(bool also_delete)
{
	DBG cerr << "Send in FindWindow"<<endl;

	if (!win_owner || isblank(win_sendthis)) return;

	SimpleMessage *msg = new SimpleMessage(papertype);
	app->SendMessage(msg, win_owner, win_sendthis, object_id);
}


} // namespace Laidout

