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


#include <lax/multilineedit.h>
#include <lax/mesbar.h>
#include <lax/textbutton.h>
#include <lax/iconbutton.h>
#include "plaintextwindow.h"
#include "language.h"
#include "laidout.h"


#define DBG
#include <iostream>
using namespace std;

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

PlainText::PlainText()
{
	thetext=NULL;
	name=NULL;
	owner=NULL;
	lastmodtime=0;
}

PlainText::~PlainText()
{
	if (thetext) delete[] thetext;
	if (name) delete[] name;
	if (owner) dynamic_cast<RefCounted *>(owner)->dec_count();
}

//class DynamicObject : LaxInterfaces::SomeDataRef
//{
// public:
//	 LaxInterfaces::SomeData *ref;
//};

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
 		int xx,int yy,int ww,int hh,int brder,
		PlainText *newtext)
	: RowFrame(parnt,ntitle,ANXWIN_REMEMBER|ROWFRAME_ROWS|nstyle, xx,yy,ww,hh,brder, NULL,None,NULL)
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
	DBG cerr <<"plaintext message: "<<mes<<endl;
	if (!strcmp(mes,"text change")) {
		//***update textobj from edit
	} else if (!strcmp(mes,"***")) { 
	}
	return 1;
}

int PlainTextWindow::init()
{
	anXWindow *last=NULL;

	MultiLineEdit *editbox;
	last=editbox=new MultiLineEdit(this,"plain-text-edit",0, 0,0,0,0,1, NULL,window,"ptedit",
							  0,textobj?textobj->thetext:NULL);
	AddWin(editbox, 100,95,2000,50, 100,95,20000,50);
	AddNull();

	cout <<"*** need to fix PlainTextWindow ownership text"<<endl;

	const char *txt=NULL;
	if (textobj) {
		if (textobj->owner) txt=textobj->owner->whattype();
		else txt="unowned";
	} else txt="(no text object)";
	AddWin(new MessageBar(this,"textowner",MB_MOVE, 0,0,0,0,1, txt));

	IconButton *ibut=NULL;
	last=ibut=new IconButton(this,"open",IBUT_ICON_ONLY, 0,0,0,0,1, NULL,window,"open",-1,
			laidout->icons.GetIcon("Open"),_("Open"));
	ibut->tooltip(_("Open a file from disk"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	last=ibut=new IconButton(this,"save",IBUT_ICON_ONLY, 0,0,0,0,1, NULL,window,"saveDoc",-1,
			laidout->icons.GetIcon("Save"),_("Save"));
	ibut->tooltip(_("Save the current text"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	last=new TextButton(this,"apply",0, 0,0,0,0,1, last,window,"apply", _("Apply"));
	AddWin(last);

	last=new TextButton(this,"run",0, 0,0,0,0,1, last,window,"run", _("Run..."));
	AddWin(last);

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
	textobj=txt;
	if (textobj) textobj->inc_count();

	//***update edit from textobj

	return 0;
}

