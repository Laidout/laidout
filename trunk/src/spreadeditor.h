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
#ifndef SPREADEDITOR_H
#define SPREADEDITOR_H

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/pathinterface.h>
#include "document.h"
#include "project.h"
#include "drawdata.h"

//----------------------- LittleSpread --------------------------------------

class LittleSpread : public LaxInterfaces::SomeData
{
 public:
	int what;
	Spread *spread; // holds the outline, etc..
	LaxInterfaces::PathsData *connection;
	int lowestpage,highestpage;
	LittleSpread *prev,*next;
	LittleSpread(Spread *sprd, LittleSpread *prv);
	virtual ~LittleSpread();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void mapConnection();
	virtual void FindBBox();
};

//----------------------- SpreadInterface --------------------------------------
class SpreadEditor;

class SpreadInterface : public LaxInterfaces::InterfaceWithDp, public LaxFiles::DumpUtility
{
 protected:
	int maxmarkertype;
	int centerlabels;
	char drawthumbnails;
	int arrangetype,arrangestate;
	int mx,my,firsttime;
	int reversebuttons;
	int curpage, dragpage;
	Laxkit::NumStack<int> curpages;
	LittleSpread *curspread;
	//SpreadView *view;
	//char dataislocal; 
	Laxkit::PtrStack<LittleSpread> spreads;
	int temppagen,*temppagemap;
	//Laxkit::PtrStack<TextBlock> notes;
	int reversemap(int i);
 public:
	Document *doc;
	Project *project;
	unsigned int style;
	unsigned long controlcolor;
	SpreadInterface(Laxkit::Displayer *ndp,Project *proj,Document *docum);
	virtual ~SpreadInterface();
	virtual int rLBDown(int x,int y,unsigned int state,int count);
	virtual int rLBUp(int x,int y,unsigned int state);
	virtual int rMBDown(int x,int y,unsigned int state,int count);
	virtual int rMBUp(int x,int y,unsigned int state);
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MBDown(int x,int y,unsigned int state,int count);
	virtual int MBUp(int x,int y,unsigned int state);
//	//virtual int RBDown(int x,int y,unsigned int state,int count);
//	//virtual int RBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state);
	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int Refresh();
//	//virtual int DrawData(Laxkit::anObject *ndata,int info=0);
//	//virtual int UseThis(Laxkit::anObject *newdata,unsigned int); // assumes not use local
//	//virtual void deletedata();
	virtual int UseThisDoc(Document *ndoc);
	virtual int InterfaceOn();
//	//virtual int InterfaceOff();
	virtual const char *whattype() { return "SpreadInterface"; }
	virtual const char *whatdatatype() { return "LittleSpread"; }
	virtual void Clear(LaxInterfaces::SomeData *d);

	virtual void CheckSpreads(int startpage,int endpage);
	virtual void GetSpreads();
	virtual void ArrangeSpreads(int how=-1);
	virtual int findPage(int x,int y);
	virtual int findSpread(int x,int y,int *page=NULL);
	virtual void Center(int w=1);
	virtual void drawLabel(int x,int y,Page *page, int outlinestatus);

	virtual void Reset();
	virtual void ApplyChanges();
	virtual void SwapPages(int previouspos, int newpos);
	virtual void SlidePages(int previouspos, int newpos);
	virtual int ChangeMarks(int newmark);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	friend class SpreadEditor;
};

//----------------------- SpreadEditor --------------------------------------

class SpreadEditor : public LaxInterfaces::ViewerWindow
{
 protected:
 public:
	Document *doc;
	Project *project;
	Laxkit::anXWindow *rulercornerbutton;
	SpreadEditor(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
						int xx, int yy, int ww, int hh, int brder,
						Project *project, Document *ndoc);
	virtual ~SpreadEditor();
	virtual int init();
	virtual const char *whattype() { return "SpreadEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int UseThisDoc(Document *ndoc);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

#endif

