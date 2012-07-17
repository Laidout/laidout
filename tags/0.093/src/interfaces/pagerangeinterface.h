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
// Copyright (C) 2011 by Tom Lechner
//
#ifndef INTERFACES_PAGERANGE_H
#define INTERFACES_PAGERANGE_H

#include <lax/interfaces/aninterface.h>

#include "documentuser.h"
#include "../laidout.h"


//------------------------------------- PageRangeInterface --------------------------------------

class PageRangeInterface : public LaxInterfaces::anInterface, public DocumentUser
{
  protected:
	Document *doc;
	Laxkit::ButtonDownInfo buttondown;
	double xscale,yscale;
	flatpoint offset;
	int InstallDefaultRange();
	char *LabelPreview(int range,int first,int labeltype);
	char *LabelPreview(PageRange *rangeobj,int first,int labeltype);
	PageRange *findRangeWith(int i);

	unsigned long defaultfg,defaultbg;

	int currange;
	int hover_part;
	int hover_range;
	int hover_position;
	int hover_index;
	PageRange *temp_range_l, *temp_range_r;

	int showdecs;
	int firsttime;
	virtual int scan(int x,int y, int *position, int *range, int *part, int *index);
	virtual double position(int pagenumber);
	virtual void drawBox(PageRange *r, int includehover);
  public:
	PageRangeInterface(int nid=0,Laxkit::Displayer *ndp=NULL,Document *ndoc=NULL);
	PageRangeInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~PageRangeInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "PageRange"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PageRangeInterface"; }
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
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int UseThisDocument(Document *doc);
};



#endif

