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
// Copyright (C) 2005-2007,2009-2011 by Tom Lechner
//
#ifndef GROUPINTERFACE_H
#define GROUPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/somedata.h>



namespace Laidout {



//----------------------------- GroupInterface -----------------------

class GroupInterface : public LaxInterfaces::ObjectInterface
{
  protected:
	int rx,ry;
	virtual int PerformAction(int action);
  public:
	void TransformSelection(const double *N, int s=-1, int e=-1);// *****

	GroupInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~GroupInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual const char *whattype() { return "ObjectInterface"; }
	virtual const char *whatdatatype() { return "Group"; }
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual int UseThis(anObject *newdata,unsigned int);
	virtual int draws(const char *atype);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual int LBDown(int x, int y,unsigned int state, int count,const Laxkit::LaxMouse *mouse);
	virtual int GrabSelection(unsigned int state);
	virtual int ToggleGroup();
};


} //namespace Laidout

#endif

