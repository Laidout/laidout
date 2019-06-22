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
// Copyright (C) 2019 by Tom Lechner
//


#include <lax/button.h>
#include <lax/utf8string.h>
#include <lax/messagebar.h>

#include "pluginwindow.h"
#include "../laidout.h"
#include "../language.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {

/*! \class PluginWindow
 *
 * Settings window for autosaving.
 */
PluginWindow::PluginWindow(Laxkit::anXWindow *parnt)
		: RowFrame(parnt,"plugins",_("Plugins"),
					ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ANXWIN_REMEMBER|ANXWIN_CENTER,
					0,0,0,0,0, nullptr,0,nullptr,
					laidout->defaultlaxfont->textheight()/3)
{
}

PluginWindow::~PluginWindow()
{
}

int PluginWindow::preinit()
{
	anXWindow::preinit();

	if (win_w<=0) win_w=400;
	if (win_h<=0) win_h=laidout->defaultlaxfont->textheight()*10;

//	if (win_w<=0 || win_h<=0) {
//		arrangeBoxes(1);
//		win_w=w();
//		win_h=h();
//	} 
	
	return 0;
}

int PluginWindow::init()
{
	int th = win_themestyle->normal->textheight();
	int linpheight = th*1.3;
	//int pad = th/3;
	//int lpad = th/4;
	Button *tbut = nullptr;
	anXWindow *last = nullptr;
	//LineInput *linp = nullptr;
	//CheckBox *check = nullptr;
	//char scratch[200]; 


//	 //--------------[ ] Enabled
//	last = check = new CheckBox(this,"enabled",nullptr,CHECK_LEFT, 0,0,0,0,0, 
//									last,object_id,"enabled", nullptr, pad,pad);
//	check->State(plugin->active ? LAX_ON : LAX_OFF);
//	check->tooltip(_("Whether plugin is currently loaded."));
//	AddWin(check,1, check->win_w,0,30000,50,0, check->win_h,0,0,50,0, -1);
//	AddNull();


	//  [ active ]     Text, clickable website?
	//  [ Remove from disk ]
	
//	last = tbut = new Button(this,"load",nullptr,0, 0,0,0,0,1, last,object_id,"Load", 0, _("Load..."));
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddNull();
	AddVSpacer(th,0,0,50);
	AddNull();


	Utf8String str;

	//note: laidout's plugin list needs to be expanded to list also unloaded but available, or broken plugins, not just active ones.
	for (int c=0; c<laidout->plugins.n; c++) {
		PluginBase *plugin = laidout->plugins.e[c];

		str = "";
		str = str + "Name:        " + plugin->PluginName() + "\n"
		 + "Version:     " + plugin->Version()     + "\n"
		 + "Description:\n" + plugin->Description() + "\n"
		 + "Author:      " + plugin->Author()      + "\n"
		 + "ReleaseDate: " + plugin->ReleaseDate() + "\n"
		 + "License:     " + plugin->License()     + "\n\n" ;

		MessageBar *bar = new MessageBar(this,plugin->PluginName(),NULL,MB_LEFT | MB_MOVE, 0,0,0,0,0, str.c_str());
        AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);
        AddNull();
	}

	//------------------------------ final ok -------------------------------------------------------

	if (!win_parent) {
		 //ok and cancel only when is toplevel...

		AddVSpacer(th/2,0,0,50,-1);
		AddNull();

		AddWin(nullptr,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
		
		 // [ ok ]
		last = tbut = new Button(this,"ok",nullptr,0,0,0,0,0,1, last,object_id,"Ok", BUTTON_OK, _("Done"));
		tbut->State(LAX_OFF);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

		 //// [ cancel ]
		//last = tbut = new Button(this,"cancel",nullptr,BUTTON_CANCEL,0,0,0,0,1, last,object_id,"Cancel");
		//AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

		AddWin(nullptr,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
	}


	
	last->CloseControlLoop();
	Sync(1);
	WrapToExtent();
	return 0;
}

int PluginWindow::Event(const EventData *data,const char *mes)
{

	if (!strcmp(mes,"Ok")) {
		if (send()==0) return 0;
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

//	} else if (!strcmp(mes,"autosave")) { 
//		CheckBox *box;
//		box=dynamic_cast<CheckBox *>(findChildWindowByName("autosave"));
//		if (!box) return 0;
//		if (box->State()==LAX_ON) box->State(LAX_OFF);
//		else box->State(LAX_ON);
//		return 0;

	}

	return 0;
}

/*! Validate inputs and send if valid.
 *
 * Return 1 for ok, 0 for invalid and don't destroy dialog yet.
 */
int PluginWindow::send()
{

	return 1;
}


} //namespace Laidout


