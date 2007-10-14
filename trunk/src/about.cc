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

#include "about.h"
#include <lax/mesbar.h>
#include <lax/textbutton.h>

#include <lax/version.h>
#include "headwindow.h"
#include "version.h"
#include "language.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


//------------------------ AboutWindow -------------------------
//
/*! \class AboutWindow
 * \brief Show a little box with the logo, author(s), version, and Laxkit version.
 */  

AboutWindow::AboutWindow()
	: MessageBox(NULL,"About",ANXWIN_CENTER, 0,0,500,600,0, NULL,None,NULL, NULL)
{
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
	m[1]=BOX_SHOULD_WRAP;
	m[7]=BOX_SHOULD_WRAP; //<-- this triggers a wrap in rowcol-figureDims

	char *about=newstr(_(
			"[insert splash logo here!]\n"
			"\n"
			"Laidout Version "));
	appendstr(about,LAIDOUT_VERSION);
	appendstr(about,_(
			"\nusing Laxkit version " LAXKIT_VERSION "\n"
			"2004-2007\n"
			"\n"
			"so far coded entirely\n"
			"by Tom Lechner,\n"
			"\n"
			"Translations:\n"
			"French: Nabyl Bennouri"));
	MessageBar *mesbar=new MessageBar(this,"aboutmesbar",MB_CENTER|MB_TOP|MB_MOVE, 0,0,0,0,0,about);
	delete[] about;
			
	AddWin(mesbar,	mesbar->win_w,mesbar->win_w,0,50,
					mesbar->win_h,mesbar->win_h,0,50);
	AddNull();
	AddButton(TBUT_OK);
	
	//WrapToExtent: 
	arrangeBoxes(1);
	win_w=m[1];
	win_h=m[7];

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
	
//	m[1]=BOX_SHOULD_WRAP;
//	m[7]=BOX_SHOULD_WRAP; //<-- this triggers a wrap in rowcol-figureDims
//	//WrapToExtent: 
//	arrangeBoxes(1);
//	win_w=m[1];
//	win_h=m[7];

	if (!win_sizehints) {
		win_sizehints=XAllocSizeHints();
	}
	win_sizehints->x=win_x;
	win_sizehints->y=win_y;
	win_sizehints->width=win_w;
	win_sizehints->height=win_h;
	win_sizehints->flags=USPosition|USSize;
	      
	MoveResize(win_x,win_y,win_w,win_h);
	
	MessageBox::init();

	return 0;
}

/*! Esc  dismiss the window.
 */
int AboutWindow::CharInput(unsigned int ch,unsigned int state)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

