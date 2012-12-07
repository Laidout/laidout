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
#ifndef BUTTONBOX_H
#define BUTTONBOX_H


#include <lax/tabframe.h>


namespace Laidout {



//------------------------------------ ButtonBox -----------------------------------------
class ButtonBox : public Laxkit::TabFrame
{
 public:
	//ButtonBox();
 	ButtonBox(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
	virtual ~ButtonBox();
	virtual const char *whattype() { return "ButtonBox"; }
	virtual const char *tooltip(int mouseid=0);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
};


} // namespace Laidout

#endif

