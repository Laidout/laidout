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
 public:
	Laxkit::anXWindow *lastview, *lastedit;
 	HeadWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
 	virtual const char *whattype() { return "HeadWindow"; }
	virtual ~HeadWindow();
	virtual int init();
	virtual Laxkit::MenuInfo *GetMenu();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual Laxkit::anXWindow *NewWindow(const char *wtype);
	virtual void WindowGone(Laxkit::anXWindow *win);
	virtual int Curbox(int c);
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

Laxkit::anXWindow *newHeadWindow(Document *doc=NULL,const char *which=NULL);

#endif

