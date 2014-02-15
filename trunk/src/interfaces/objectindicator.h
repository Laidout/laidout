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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef INTERFACES_OBJECTINDICATOR_H
#define INTERFACES_OBJECTINDICATOR_H

#include <lax/interfaces/aninterface.h>
#include <lax/refptrstack.h>

#include "../laidout.h"
#include "viewwindow.h"


namespace Laidout {






//------------------------------------- ObjectIndicator --------------------------------------

class ObjectIndicator : public LaxInterfaces::anInterface
{
  protected:
	Laxkit::ButtonDownInfo buttondown;

	VObjContext *context;
	DrawableObject *hover_object;
	int last_hover;
	unsigned long color_arrow, color_num;

	int showdecs;
	int firsttime;

	virtual int scan(int x,int y);
  public:
	unsigned long shape_style;//options for interface

	ObjectIndicator(int nid=0,Laxkit::Displayer *ndp=NULL);
	ObjectIndicator(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~ObjectIndicator();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "ObjectIndicator"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ObjectIndicator"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
};



} //namespace Laidout

#endif

