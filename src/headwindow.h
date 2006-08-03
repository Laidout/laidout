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
#ifndef HEADWINDOW_H
#define HEADWINDOW_H

#include <lax/splitwindow.h>
#include <lax/attributes.h>
#include "document.h"

class HeadWindow : public Laxkit::SplitWindow, public LaxFiles::DumpUtility
{
 protected:
	static Laxkit::PlainWinBox *markedpane;
	static HeadWindow *markedhead;
	virtual int splitthewindow(anXWindow *fillwindow=NULL);
 public:
	Laxkit::anXWindow *lastview, *lastedit;
 	HeadWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
 	virtual const char *whattype() { return "HeadWindow"; }
	virtual ~HeadWindow();
	virtual int init();
	virtual int LBUp(int x,int y,unsigned int state); 
	virtual int MouseMove(int x,int y,unsigned int state);
	
	virtual int Mark(int c);
	virtual int SwapWithMarked();
	virtual Laxkit::MenuInfo *GetMenu();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);
	virtual Laxkit::anXWindow *NewWindow(const char *wtype);
	virtual int Curbox(int c);
	virtual int Change(anXWindow *towhat,anXWindow *which=NULL);
	
	virtual void WindowGone(Laxkit::anXWindow *win);
	virtual int numwindows() { return windows.n; }
	virtual Laxkit::PlainWinBox *windowe(int i) { if (i>=0 && i<windows.n) return windows.e[i]; return NULL; }
	virtual int HasOnlyThis(Document *doc);
	virtual Document *HeadWindow::findAnyDoc();
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
};

Laxkit::anXWindow *newHeadWindow(Document *doc=NULL,const char *which=NULL);
Laxkit::anXWindow *newHeadWindow(LaxFiles::Attribute *att);

#endif

