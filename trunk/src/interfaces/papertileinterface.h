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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef INTERFACES_PAPERTILE_H
#define INTERFACES_PAPERTILE_H

#include <lax/interfaces/aninterface.h>



class PaperTileInterface : public LaxInterfaces::InterfaceWithDp
{
 protected:
	Laxkit::PtrStack<PaperBoxData> papers;
	PaperBoxData *curbox;
 public:
	PaperTileInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	PaperTileInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~PaperTileInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual const char *whattype() { return "PaperTileInterface"; }
	virtual const char *whatdatatype() { return "PaperBoxData"; }

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(SomeData *d);
	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int MBDown(int x,int y,unsigned int state,int count);
	virtual int RBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MBUp(int x,int y,unsigned int state);
	virtual int RBUp(int x,int y,unsigned int state);
	virtual int But4(int x,int y,unsigned int state);
	virtual int But5(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch,unsigned int state);
	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int Refresh();

	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int DrawData(Laxkit::anObject *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1);
};



#endif

