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

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/aninterface.h>

#include "../laidout.h"
#include "signatures.h"



//------------------------------------- SignatureInterface --------------------------------------

class SignatureInterface : public LaxInterfaces::InterfaceWithDp
{
 protected:
	int showdecs;

	int lbdown_row, lbdown_col;

	 //to keep track of current partial fold:
	int foldr1, foldc1, foldr2, foldc2;
	int folddirection;
	int foldunder;
	int foldindex;
	double foldprogress;

	int finalr,finalc; //cell location of totally folded pages
	int onoverlay; //nonzero if mouse clicked down on and is over an overlay
	int hasfinal; //whether the pattern has been totally folded yet or not

	double totalwidth, totalheight;

	int foldlevel; //how hany folds are actively displayed
	FoldedPageInfo **foldinfo;
	void reallocateFoldinfo();
	void resetFoldinfo();
	void applyFold(char folddir, int index, int under);

	virtual int scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col);
	virtual int scanhandle(int x,int y);
	virtual int checkFoldLevel(int update);
	virtual void getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h);

	void dumpFoldinfo();
 public:
	Signature *signature;
	PaperStyle *papersize;

	SignatureInterface(int nid=0,Laxkit::Displayer *ndp=NULL,Signature *sig=NULL, PaperStyle *p=NULL);
	SignatureInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~SignatureInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual const char *whattype() { return "SignatureInterface"; }
	virtual const char *whatdatatype() { return "Signature"; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y, int deviceid);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	
//!	virtual int UseThisImposition(Signature *sig);
};


//------------------------------------- SignatureEditor --------------------------------------

class SignatureEditor : public LaxInterfaces::ViewerWindow
{
 protected:
 public:
	PaperStyle *p;
	Signature *sig;
	SignatureEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						Laxkit::anXWindow *nowner, const char *mes,
						Signature *sig, PaperStyle *p);
	virtual ~SignatureEditor();
	virtual int init();
	virtual const char *whattype() { return "SignatureEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


#endif

