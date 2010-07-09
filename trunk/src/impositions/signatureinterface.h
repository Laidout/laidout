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
// Copyright (C) 2010 by Tom Lechner
//
#ifndef INTERFACES_SIGNATUREINTERFACE_H
#define INTERFACES_SIGNATUREINTERFACE_H

#include <lax/interfaces/aninterface.h>

#include "../laidout.h"
#include "signatures.h"



//------------------------------------- SignatureInterface --------------------------------------

class FoldedPageInfo
{
 public:
	int currentrow, currentcol;
	int positive_y, positive_x;
	int paperheight;
	int foldedheight;
};

class SignatureInterface : public LaxInterfaces::InterfaceWithDp
{
 protected:
	int showdecs;
	Signature *signature;
	Document *doc;

	double patternwidth, patternheight;

	int foldlevel; //how hany folds are actively displayed
	FoldedPageInfo **foldinfo;
	void reallocateFoldinfo();

	virtual int scan(int x,int y,int *row,int *col);
 public:
	SignatureInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	SignatureInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~SignatureInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual const char *whattype() { return "SignatureInterface"; }
	virtual const char *whatdatatype() { return "Signature"; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	
	virtual int UseThisDocument(Document *doc);
//!	virtual int UseThisImposition(Signature *sig);
};



#endif

