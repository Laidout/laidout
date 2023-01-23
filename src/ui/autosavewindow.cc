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
// Copyright (C) 2016 by Tom Lechner
//


#include "autosavewindow.h"
#include "../laidout.h"
#include "../language.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


namespace Laidout {

/*! \class AutosaveWindow
 *
 * Settings window for autosaving.
 */
AutosaveWindow::AutosaveWindow(Laxkit::anXWindow *parnt)
		: RowFrame(parnt,"autosave",_("Autosave settings..."),
					ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ANXWIN_REMEMBER|ANXWIN_CENTER,
					0,0,0,0,0, NULL,0,NULL,
					laidout->defaultlaxfont->textheight()/3)
{
}

AutosaveWindow::~AutosaveWindow()
{
}

int AutosaveWindow::preinit()
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

int AutosaveWindow::init()
{
	int th=app->defaultlaxfont->textheight();
	int linpheight=th*1.3;
	int pad=th/3;
	int lpad=th/4;
	Button *tbut=NULL;
	anXWindow *last=NULL;
	//LineInput *linp=NULL;
	CheckBox *check=NULL;
	char scratch[200]; 


	 //--------------[ ] Autosave
	last=check=new CheckBox(this,"autosave",NULL,CHECK_LEFT, 0,0,0,0,0, 
									last,object_id,"autosave", _("Autosave"), pad,pad);
	check->State(laidout->prefs.autosave ? LAX_ON : LAX_OFF);
	check->tooltip(_("Whether to autosave or not."));
	AddWin(check,1, check->win_w,0,30000,50,0, check->win_h,0,0,50,0, -1);
	AddNull();

	 //-------------  Time between autosaves:___5_minute__  ...ms millisecond s sec seconds second m minute min hr hour h
	sprintf(scratch, _("%.10g"), laidout->prefs.autosave_time);
	last=new LineInput(this,"autosave_time",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						NULL,object_id,"autosave_time",
			            _("Minutes between autosaves"), scratch,0,
			            0,0, pad,pad, lpad,lpad);
	last->tooltip(_("0 means don't autosave"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();

	 //-------------  Naming for autosaves: __%f-autosave#____
	last=new LineInput(this,"autosave_path",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						NULL,object_id,"autosave_path",
			            _("Autosave file name"), laidout->prefs.autosave_path,0,
			            0,0, pad,pad, lpad,lpad);
	//last->tooltip(_("%f = filename\n%b = basename without extension\n%e = extension\n# = autosave number"));
	last->tooltip(_("%f = filename\n%b = basename without extension\n%e = extension"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();

//	 //-------------  Number of different autosaves: ___1___
//	sprintf(scratch, "%d", laidout->prefs.autosave_num);
//	last=new LineInput(this,"autosave_num",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
//						NULL,object_id,"autosave_num",
//			            _("Number of autosave files"), scratch,0,
//			            0,0, pad,pad, lpad,lpad);
//	//last->tooltip(_("Don't have more than this number of autosave files. 0 means no limit.\nRequires a '#' be in the autosave file name."));
//	last->tooltip(_("Don't have more than this number of autosave files. 0 means no limit."));
//	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
//	AddNull();



	//------------------------------ final ok -------------------------------------------------------

	if (!win_parent) {
		 //ok and cancel only when is toplevel...

		AddVSpacer(th/2,0,0,50,-1);
		AddNull();

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
		
		 // [ ok ]
		last=tbut=new Button(this,"ok",NULL,0,0,0,0,0,1, last,object_id,"Ok", BUTTON_OK);
		tbut->State(LAX_OFF);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

		 // [ cancel ]
		last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL,0,0,0,0,1, last,object_id,"Cancel");
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
	}


	
	last->CloseControlLoop();
	Sync(1);
	return 0;
}

int AutosaveWindow::Event(const EventData *data,const char *mes)
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
int AutosaveWindow::send()
{
	CheckBox *autosave = dynamic_cast<CheckBox  *>(findChildWindowByName("autosave")); 
	//LineEdit *num     = dynamic_cast<LineInput *>(findChildWindowByName("autosave_num"))->GetLineEdit();
	LineEdit *time    = dynamic_cast<LineInput *>(findChildWindowByName("autosave_time"))->GetLineEdit();
	LineEdit *path    = dynamic_cast<LineInput *>(findChildWindowByName("autosave_path"))->GetLineEdit();

	char scratch[100];
	char rcpath[strlen(laidout->config_dir)+20];
	sprintf(rcpath,"%s/laidoutrc",laidout->config_dir);

	bool modified=false;

	//autosave
	bool newv = (autosave->State()==LAX_ON);
	if (laidout->prefs.autosave != newv) {
		laidout->prefs.autosave = newv;
		laidout->prefs.UpdatePreference("autosave", laidout->prefs.autosave ? "yes" : "no", rcpath);
		modified=true;
	}

	//autosave_path
	if (path->GetCText() && strcmp(path->GetCText(), laidout->prefs.autosave_path)) {
		makestr(laidout->prefs.autosave_path, path->GetCText()); 
		laidout->prefs.UpdatePreference("autosave_path", laidout->prefs.autosave_path, rcpath);
		modified=true;
	}

//	//autosave_num
//	int n=num->GetLong(NULL);
//	if (n<0) n=0;
//	if (n!=laidout->prefs.autosave_num) {
//		laidout->prefs.autosave_num = n;
//		sprintf(scratch, "%d", laidout->prefs.autosave_num);
//		laidout->prefs.UpdatePreference("autosave_num", scratch, rcpath);
//		modified=true;
//	}

	//autosave_time
	double d=time->GetDouble(NULL);
	if (d != laidout->prefs.autosave_time) {
		laidout->prefs.autosave_time = d;
		sprintf(scratch, "%.10g", laidout->prefs.autosave_time);
		laidout->prefs.UpdatePreference("autosave_time", scratch, rcpath); 
		modified=true;
	}
	
	if (modified) laidout->UpdateAutosave();

	return 1;
}


} //namespace Laidout


