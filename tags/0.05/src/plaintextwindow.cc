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
 */
class PlainText : public Laxkit::anObject, public Laxkit::RefCounted
{
 public:
	int ownertype;
	union {
		Laxkit::anObject *owner;
		char *filename;
	} owner;
	clock_t lastmodtime;
	char *thetext;
};


//------------------------------ PlainTextWindow -------------------------------
/*! \class PlainTextWindow
 * \brief Editor for plain text
 *
 * ***NOTE: Directly descending from GoodEditWW is temporary.
 */
//class PlainTextWindow : public Laxkit::RowFrame
//{
// protected:
// public:
// 	PlainTextWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
// 		int xx,int yy,int ww,int hh,int brder);
// 	virtual const char *whattype() { return "PlainTextWindow"; }
// 	virtual int init();
//	virtual ~PlainTextWindow();
//	virtual int UseThis(PlainText *txt);
//};

PlainTextWindow::PlainTextWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder)
	: RowFrame(parnt,ntitle,nstyle, xx,yy,ww,hh,brder)
{
}

PlainTextWindow::~PlainTextWindow()
{
}

int PlainTextWindow::init()
{
	
	
	
	Sync(1);
	return 0;
}

