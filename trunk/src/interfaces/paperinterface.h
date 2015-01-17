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
// Copyright (C) 2004-2007,2011 by Tom Lechner
//
#ifndef INTERFACES_PAPERTILE_H
#define INTERFACES_PAPERTILE_H

#include <lax/interfaces/aninterface.h>

#include "../laidout.h"


namespace Laidout {


//-------------------------- misc PaperGroup utils --------------------------
char *new_paper_group_name();

//------------------------------------- PaperInterface --------------------------------------

#define PAPERTILE_ONE_ONLY   (1<<0)

class PaperInterface : public LaxInterfaces::anInterface
{
 protected:
	int showdecs;
	PaperGroup *papergroup;
	PaperBoxData *paperboxdata;
	Laxkit::PtrStack<PaperBoxData> curboxes;
	PaperBoxData *curbox, *maybebox;
	BoxTypes editwhat, drawwhat, snapto;
	virtual int scan(int x,int y);
	virtual void createMaybebox(flatpoint p);
	Document *doc;
	int mx,my;
	int rx,ry;
	flatpoint lbdown;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
 public:
	PaperInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	PaperInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~PaperInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual const char *IconId() { return "Paper"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PaperInterface"; }
	virtual const char *whatdatatype() { return "PaperGroup"; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual void DrawPaper(PaperBoxData *data,int what,char fill,int shadow,char arrow);
	virtual void DrawGroup(PaperGroup *group,char shadow,char fill,char arrow);
	virtual int DrawDataDp(Laxkit::Displayer *tdp,LaxInterfaces::SomeData *tdata,
					Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1);

	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	//virtual int DrawData(Laxkit::anObject *ndata,
	//		Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	//virtual int DrawDataDp(Laxkit::Displayer *tdp,LaxInterfaces::SomeData *tdata,
	//		Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1);
	
	virtual int UseThisDocument(Document *doc);
};


} //namespace Laidout


#endif

