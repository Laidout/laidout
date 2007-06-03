//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//


#include "palettes.h"
#include "laidout.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

PalettePane::PalettePane(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,Window nowner,const char *nsend)
	: PaletteWindow(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{}

const char *PalettePane::PaletteDir()
{
	return laidout->palette_dir;
}


//! Send a anXWindow::sendthis message to laidout->lastview. 
/*! 
 * \todo *** there is every possibility that laidout->lastview is no longer valid?
 */
int PalettePane::send()
{
	if (!laidout->lastview || !laidout->lastview->window || 
			!sendthis || !palette || curcolor<0) return 0;
	XEvent e;
	e.xclient.type=ClientMessage;
	e.xclient.display=app->dpy;
	e.xclient.window=laidout->lastview->window;
	e.xclient.message_type=sendthis;
	e.xclient.format=32;
	e.xclient.data.l[0]=palette->defaultmaxcolor;
	int c;
	for (c=0; c<palette->colors.e[curcolor]->numcolors; c++) {
		DBG cout <<"send palette color "<<c<<": "<< palette->colors.e[curcolor]->channels[c];
		if (c<5) e.xclient.data.l[c+1]=palette->colors.e[curcolor]->channels[c];
	}
	for (c++; c<5; c++) e.xclient.data.l[c]=palette->defaultmaxcolor;
	XSendEvent(app->dpy,laidout->lastview->window,False,0,&e);
	return 1;
}

