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
// Copyright (C) 2004-2019 by Tom Lechner
//

#include <lax/messagebar.h>
#include <lax/button.h>
#include <lax/version.h>

#include "../laidout.h"
#include "about.h"
#include "headwindow.h"
#include "../version.h"
#include "../language.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;



namespace Laidout {


//------------------------ AboutWindow -------------------------
//
/*! \class AboutWindow
 * \brief Show a little box with the logo, author(s), version, and Laxkit version.
 */  

AboutWindow::AboutWindow(Laxkit::anXWindow *parent)
	: MessageBox(parent,"About",_("About"),ANXWIN_CENTER, 0,0,500,600,0, NULL,0,"ok", NULL)
{
	win_style |= ANXWIN_ESCAPABLE;

	splash = NULL;
	if (laidout->prefs.splash_image_file) {
		splash = ImageLoader::LoadImage(laidout->prefs.splash_image_file,
									   NULL,0,0,NULL,
									   0,-1,NULL, false, 0);
	} else {
		IconManager *iconmanager=IconManager::GetDefault();
		splash = iconmanager->GetIcon("LaidoutSplash");
	}
}

AboutWindow::~AboutWindow()
{
	if (splash) splash->dec_count();
}

/*! The default MessageBox::init() sets m[1]=m[7]=BOX_SHOULD_WRAP, which is supposed 
 * to trigger a wrap to extent. However, if a window has a stretch of 2000, say
 * like the main messagebar, then that window is stretched
 * to that amount, which is silly. So, intercept this to be a more reasonable width.
 */
int AboutWindow::preinit()
{
	//Screen *screen=DefaultScreenOfDisplay(app->dpy);
	
	//m[1]=screen->width/2;
	w(BOX_SHOULD_WRAP);
	h(BOX_SHOULD_WRAP); //<-- this triggers a wrap in rowcol-figureDims


	if (splash) {
		AddWin(NULL,0,
				splash->w(),MAX(splash->w(), 50),0,50,0,
				splash->h(),splash->h(),0,50,0,
				0);
		AddNull(1);
	}

	char *about=NULL;
	if (!splash) about=newstr(_("[pretend there is a splash logo here!]\n"));

	appendstr(about,"\n");
	appendstr(about,_("Laidout Version "));
	appendstr(about,LAIDOUT_VERSION);
	appendstr(about,_(
			"\nusing Laxkit version " LAXKIT_VERSION "\n"
			"2004-2021\n"
			"\n"
			"by Tom Lechner,\n"
			"laidout.org\n"
			"\n"
			"Contributors:\n"
			"AkiSakurai\n"
			"Nabyl Bennouri\n"
			"Probonopd\n"
			));
	MessageBar *mesbar=new MessageBar(this,"aboutmesbar",NULL,MB_CENTER|MB_TOP|MB_MOVE, 0,0,0,0,0,about);
	delete[] about;
			
	AddWin(mesbar,1, mesbar->win_w,mesbar->win_w,0,50,0,
					mesbar->win_h,mesbar->win_h,0,50,0,
					-1);
	AddNull();
	if (!win_parent) AddButton(BUTTON_OK);
	
	//WrapToExtent:
	arrangeBoxes(1);
	win_w=w();
	win_h=h();

	return 0;
}

/*! Pops up a box with the  logo, author(s), version, and Laxkit version.
 */
int AboutWindow::init()
{
	MessageBox::init();

	return 0;
}

void AboutWindow::Refresh()
{
	if (arrangedstate!=1 && splash) {
		//force aspect for splash box
		wholelist[0]->pw(win_w);
		wholelist[0]->ph(win_w*splash->h()/(float)splash->w());
	}
	RowFrame::Refresh(); //window resize syncs are cached until this refresh

	Displayer *dp = MakeCurrent();

	if (splash) {
		dp->imageout_within(splash, wholelist.e[0]->x(),wholelist.e[0]->y(), wholelist.e[0]->w(),wholelist.e[0]->h(), nullptr, 0);
	}
	needtodraw=0;
}

/*! Esc  dismiss the window.
 */
int AboutWindow::CharInput(unsigned int ch,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc && (win_style & ANXWIN_ESCAPABLE)) {
		if (win_parent && dynamic_cast<HeadWindow*>(win_parent))
			dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

int AboutWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	return MessageBox::Event(e,mes);
}




} // namespace Laidout

