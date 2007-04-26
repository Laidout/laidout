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


**************** NOT FUNCTIONAL YET
 
#include "plaintextwindow.h"

using namespace Laxkit;


//------------------------------ PlainText ------------------------------

/*! \class PlainText
 * \brief Holds plain text, which is text with no formatting.
 *
 * These are for holding random notes and scripts.
 *
 * Ultimately, they might contain something like Latex code that an EPS
 * grabber might run to get formulas.... Big todo!!
 *
 * \todo allow 8bit text OR utf8
 */


//------------------------------ PlainTextWindow -------------------------------
/*! \class PlainTextWindow
 * \brief Editor for plain text
 *
 * <pre>
 *  [  the edit box       ]
 *  [owner/filename][apply][name [v]] <-- click name to retrieve other plain text objects
 *  [run (internally)][run with... (externally)]
 * </pre>
 */


/*! Increments the count of newtext.
 */
PlainTextWindow::PlainTextWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder, PlainText *newtext)
	: RowFrame(parnt,ntitle,nstyle, xx,yy,ww,hh,brder)
{
	textobj=newtext;
	if (textobj) textobj->inc_count();
}

PlainTextWindow::~PlainTextWindow()
{
	if (textobj) textobj->dec_count();
}

int PlainTextWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	//DBG cout <<"plaintext message: "<<mes<<endl;
	if (!strcmp(mes,"text change")) {
		***update textobj from edit
	} else if (!strcmp(mes,"***")) { 
	}

int PlainTextWindow::init()
{
	
	GoodEditWW *editbox;
	editbox=new GoodEditWW(***textobj->thetext);
	
	Sync(1);
	return 0;
}

/*! Return 0 for success, nonzero for error.
 *
 * Increments the count of txt.
 */
int PlainTextWindow::UseThis(PlainText *txt)
{
	if (txt==textobj) return 0;
	if (textobj) textobj->dec_count();
	textobj=newtext;
	if (textobj) textobj->inc_count();
	***update edit from textobj
	return 0;
}

