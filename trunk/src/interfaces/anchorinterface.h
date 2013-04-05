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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef INTERFACES_ANCHORINTERFACE_H
#define INTERFACES_ANCHORINTERFACE_H

#include <lax/refptrstack.h>
#include "aligninterface.h"
#include "../guides.h"
#include "../viewwindow.h"

//#include "../laidout.h"


namespace Laidout {





//------------------------------------- AnchorInterface --------------------------------------

class AnchorInterface : public AlignInterface
{
  protected:

	Laxkit::RefPtrStack<PointAnchor> anchors;

	virtual int scan(int x,int y);
  public:

	AnchorInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	AnchorInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~AnchorInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Anchor"; }
	virtual const char *Name();
	virtual const char *whattype() { return "AnchorInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype);
	virtual int AddAnchor(flatpoint p,const char *name);
	virtual int AddAnchors(VObjContext *context);

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
	virtual int Refresh();
};



} //namespace Laidout

#endif

