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
// Copyright (C) 2004-2015 by Tom Lechner
//

#include <lax/messagebar.h>
#include <lax/button.h>
#include <lax/version.h>

#include "laidout.h"
#include "about.h"
#include "headwindow.h"
#include "version.h"
#include "language.h"

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
	win_style|=ANXWIN_ESCAPABLE;

	splash=NULL;
	if (laidout->prefs.splash_image_file) {
		splash=load_image_with_loaders(laidout->prefs.splash_image_file,
									   NULL,0,0,NULL,
									   0,-1,NULL);
	} else splash=NULL;
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
				splash->w(),0,0,50,0,
				splash->h(),0,0,50,0,
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
			"2004-2015\n"
			"\n"
			"so far coded\n"
			"by Tom Lechner,\n"
			"\n"
			"Translations:\n"
			"French: Nabyl Bennouri\n"));
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

//	int redo=0;
//	if (win_h>(int)(.9*screen->height)) { 
//		win_h=(int)(.9*screen->height);
//		redo=1;
//	}
//	if (win_w>(int)(.9*screen->width)) { 
//		win_w=(int)(.9*screen->width);
//		redo=1;
//	}
	return 0;
}

/*! Pops up a box with the  logo, author(s), version, and Laxkit version.
 */
int AboutWindow::init()
{
	
//------done in preinit now to avoid moveresize not resizing:
//	m[1]=BOX_SHOULD_WRAP;
//	m[7]=BOX_SHOULD_WRAP; //<-- this triggers a wrap in rowcol-figureDims
//	//WrapToExtent: 
//	arrangeBoxes(1);
//	win_w=m[1];
//	win_h=m[7];

//	//*** is this necessary??
//	if (!xlib_win_sizehints) {
//		xlib_win_sizehints=XAllocSizeHints();
//	}
//	xlib_win_sizehints->x=win_x;
//	xlib_win_sizehints->y=win_y;
//	xlib_win_sizehints->width=win_w;
//	xlib_win_sizehints->height=win_h;
//	xlib_win_sizehints->flags=USPosition|USSize;
//	      
//	MoveResize(win_x,win_y,win_w,win_h);
	
	MessageBox::init();

	return 0;
}

void AboutWindow::Refresh()
{
	if (splash) {
		image_out(splash,this, wholelist.e[0]->x(),wholelist.e[0]->y()); // *** needs to scale
	}
	needtodraw=0;
}

/*! Esc  dismiss the window.
 */
int AboutWindow::CharInput(unsigned int ch,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

int AboutWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	if (win_parent && dynamic_cast<HeadWindow*>(win_parent))
		dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
	return MessageBox::Event(e,mes);
}




} // namespace Laidout

