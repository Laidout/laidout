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

/*! \enum TreeChangeType
 * \ingroup misc
 * \brief Type for what in the doc tree has changed. See TreeChangeEvent.
 */

/*! \class TreeChangeEvent
 * \brief Event class to tell various windows what has been altered.
 *
 * Windows will receive this event after the changes occur, but potentially also after
 * the window wants to access a possibly altered tree. This should not 
 * crash the program because of reference counting, but the windows must verify all the
 * relevant references when they receive such an event.
 *
 * \todo why is enum TreeChangeType not turning into a doxygen doc??
 * TreeChangeType:
 * <pre>
 *	TreeDocGone,
 *	TreePagesAdded,
 *	TreePagesDeleted,
 *	TreePagesMoved,
 *	TreeObjectRepositioned,
 *	TreeObjectReorder,
 *	TreeObjectDiffPage,
 *	TreeObjectDeleted,
 *	TreeObjectAdded
 * </pre>
 */


//! Tell all ViewWindow, SpreadEditor, and other main windows that the doc has changed.
/*! Sends a TreeChangeEvent to all SpreadEditor and ViewWindow panes in each top level HeadWindow.
 *
 * \todo *** should probably replace s and e with a FieldPlace: doc,page,obj,obj,...
 */
void LaidoutApp::notifyDocTreeChanged(Laxkit::anXWindow *callfrom,TreeChangeType change,int s,int e)
{
	//DBG cout<<"notifyDocTreeChanged sending.."<<endl;
	HeadWindow *h;
	PlainWinBox *pwb;
	ViewWindow *view;
	SpreadEditor *se;
	anXWindow *w;
	TreeChangeEvent *edata,*te=new TreeChangeEvent;
	te->send_message=XInternAtom(laidout->dpy,"docTreeChange",False);
	te->changer=callfrom;
	te->changetype=change;
	te->start=s;
	te->end=e;
	 
	int c2,yes=0;
	for (int c=0; c<topwindows.n; c++) {
		if (callfrom==topwindows.e[c]) continue; // this hardly has effect as most are children of top
		h=dynamic_cast<HeadWindow *>(topwindows.e[c]);
		if (!h) continue;
		
		for (c2=0; c2<h->numwindows(); c2++) {
			pwb=h->windowe(c2);
			w=pwb->win;
			if (!w || callfrom==w) continue;
			view=dynamic_cast<ViewWindow *>(w);
			if (view) yes=1;
			else if (se=dynamic_cast<SpreadEditor *>(w), se) yes=1;

			 //construct events for the panes
			if (yes){
				edata=new TreeChangeEvent();
				*edata=*te;
				edata->send_towindow=w->window;
				app->SendMessage(edata);
				//DBG cout <<"---(notifyDocTreeChanged) sending docTreeChange to "<<
				//DBG 	w->win_title<< "("<<w->whattype()<<")"<<endl;
				yes=0;
			}
		}
	}
	delete te;
	//DBG cout <<"eo notifyDocTreeChanged"<<endl;
	return;
}

