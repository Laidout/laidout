//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********************** laidout-more **********************/

// defines some stuff not stack dependent, so doesn't need lax/lists.cc

#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

//! Tell all ViewWindow, SpreadEditor, and other main windows that the doc has changed.
/*! \todo *** when finally i put all this in splitwindows, then this will have
 * to be changed accordingly
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
  
	int yes=0;
	for (int c=0; c<topwindows.n; c++) {
		if (callfrom==topwindows.e[c]) continue; //*** this hardly has effect as most are children of top
		view=dynamic_cast<ViewWindow *>(topwindows.e[c]);
		if (view) yes=1;
		else if (s=dynamic_cast<SpreadEditor *>(topwindows.e[c]), s) yes=1;

		if (yes){
			e.xclient.window=topwindows.e[c]->window;
			XSendEvent(dpy,topwindows.e[c]->window,False,0,&e);
			DBG cout <<"---sending docTreeChange to "<<topwindows.e[c]->win_title<<endl;
			yes=0;
		}
	}
}

