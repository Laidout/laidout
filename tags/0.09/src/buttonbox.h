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
#ifndef BUTTONBOX_H
#define BUTTONBOX_H


#include <lax/tabframe.h>


class ButtonBox : public Laxkit::TabFrame
{
 public:
	//ButtonBox();
 	ButtonBox(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
	virtual ~ButtonBox();
	virtual const char *whattype() { return "ButtonBox"; }
	virtual int RBDown(int x,int y,unsigned int state,int count);
	virtual int RBUp(int x,int y,unsigned int state);
};


#endif
