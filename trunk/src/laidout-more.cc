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
/********************** laidout-more **********************/

// defines some stuff not stack dependent, so doesn't need lax/lists.cc

#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"
#include "headwindow.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

//! Tell all ViewWindow, SpreadEditor, and other main windows that the doc has changed.
/*! Puts the callfrom->window in data.l[0]. 
 * 
 * \todo *** this function is ill-conceived!! could extend it to somehow say what has changed?
 * like if object is added or removed, or simply moved to diff position... or pages added...
 * or....
 */
void LaidoutApp::notifyDocTreeChanged(Laxkit::anXWindow *callfrom)//callfrom=NULL
{
	ViewWindow *view;
	SpreadEditor *s;
	XEvent e;
	e.xclient.type=ClientMessage;
	e.xclient.display=app->dpy;
	e.xclient.message_type=XInternAtom(dpy,"docTreeChange",False);
	e.xclient.format=32;
	e.xclient.data.l[0]=(callfrom?callfrom->window:0);
  
	int yes=0;
	for (int c=0; c<topwindows.n; c++) {
		if (callfrom==topwindows.e[c]) continue; //*** this hardly has effect as most are children of top
		view=dynamic_cast<ViewWindow *>(topwindows.e[c]);
		if (view) yes=1;
		else if (s=dynamic_cast<SpreadEditor *>(topwindows.e[c]), s) yes=1;
		else if (dynamic_cast<HeadWindow *>(topwindows.e[c])) yes=1;

		if (yes){
			e.xclient.window=topwindows.e[c]->window;
			XSendEvent(dpy,topwindows.e[c]->window,False,0,&e);
			DBG cout <<"---sending docTreeChange to "<<topwindows.e[c]->win_title<<endl;
			yes=0;
		}
	}
}

