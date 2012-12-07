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
// Copyright (C) 2004-2006,2010 by Tom Lechner
//
#ifndef HEADWINDOW_H
#define HEADWINDOW_H

#include <lax/splitwindow.h>
#include <lax/attributes.h>
#include "document.h"



namespace Laidout {


//------------------------------- HeadWindow ---------------------------------------

class HeadWindow : public Laxkit::SplitWindow
{
 protected:
	static Laxkit::PlainWinBox *markedpane;
	static HeadWindow *markedhead;
	virtual int splitthewindow(anXWindow *fillwindow=NULL);

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
 public:
	Laxkit::anXWindow *lastview, *lastedit;
 	HeadWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
 	virtual const char *whattype() { return "HeadWindow"; }
	virtual ~HeadWindow();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual void InitializeShortcuts();
	virtual int init();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d); 
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int FocusOff(const Laxkit::FocusChangeData *e);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	
	virtual int Mark(int c);
	virtual int SwapWithMarked();
	virtual Laxkit::MenuInfo *GetMenu();
	virtual Laxkit::anXWindow *NewWindow(const char *wtype,anXWindow *likethis=NULL);
	virtual int Curbox(int c);
	virtual int Change(anXWindow *towhat,int absorbcount,int which);
	
	virtual void WindowGone(Laxkit::anXWindow *win);
	virtual int numwindows() { return windows.n; }
	virtual Laxkit::PlainWinBox *windowe(int i) { if (i>=0 && i<windows.n) return windows.e[i]; return NULL; }
	virtual int HasOnlyThis(Document *doc);
	virtual Document *findAnyDoc();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

Laxkit::anXWindow *newHeadWindow(Document *doc=NULL,const char *which=NULL);
Laxkit::anXWindow *newHeadWindow(LaxFiles::Attribute *att);


} // namespace Laidout

#endif

