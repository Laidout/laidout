//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2025 by Tom Lechner
//
#ifndef _LO_STREAMINTERFACE_H
#define _LO_STREAMINTERFACE_H

#include <lax/interfaces/textstreaminterface.h>


namespace Laidout { 


//--------------------------- PathIntersectionsInterface -------------------------------------

class StreamInterface : public TextStreamInterface
{
  protected:
	virtual bool AttachStream();

  public:
	// enum PathIntersectionsActions {
	// 	INTERSECTIONS_None = 0,
	// 	INTERSECTIONS_Something,
	// 	INTERSECTIONS_MAX
	// };

	unsigned int interface_flags;
	double threshhold;

	StreamInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~StreamInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "TextStream"; }
	virtual const char *Name();
	virtual const char *whattype() { return "StreamInterface"; }
	virtual const char *whatdatatype();
	// virtual ObjectContext *Context(); 
	// virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	// virtual int Event(const Laxkit::EventData *data, const char *mes);
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int PerformAction(int action);
	//virtual void deletedata();
	//virtual PathIntersectionsData *newData();

	// virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	// virtual int UseThisObject(ObjectContext *oc);
	// virtual int InterfaceOn();
	// virtual int InterfaceOff();
	// virtual void Clear(SomeData *d);
	//virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	// virtual int Refresh();
	// virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	// virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	// virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	// virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
};

} // namespace Laidout

#endif

