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

#include "../laidout.h"

#define NUP_None       0
#define NUP_Grid       1
#define NUP_SizedGrid  2
#define NUP_Flowed     3
#define NUP_Random     4
#define NUP_Unclump    5
#define NUP_Unoverlap  6

//---------------------------------- NUpInfo -----------------------------------------
class NUpInfo : public Laxkit::DoubleBBox
{
  public:
	int direction; //lrtb, lrbt, rltb, rlbt, ...
	int cellstyle; //whether allow variable sizes of cells, or each row/col same size
	double rowcenter, colcenter; //how to center within rows and columns
	int rows, cols; //number of rows and columns in one n-up block, -1==infinity, 0==as many as will flow in

	DoubleBBox final_grid_bounds;
	double final_grid_offset[6];

	double ui_offset[6]; //transform for where the arrows are, works on screen space?
	
	NUpInfo();
	virtual ~NUpInfo();
};


//------------------------------------- NUpInterface --------------------------------------

class NUpInterface : public LaxInterfaces::anInterface
{
  protected:
	Laxkit::ButtonDownInfo buttondown;

	NUpInfo *nupinfo;
	Laxkit::PtrStack<ActionArea> controls;

	RefPtrStack<SomeData> objects;
	SomeData *bounds;

	int showdecs;
	int firsttime;

	DoubleBBox mainarrow,  minorarrow;
	DoubleBBox mainnumber, minornumber;

	virtual int scan(int x,int y);
	virtual void createControls();
  public:
	NUpInterface(int nid=0,Laxkit::Displayer *ndp=NULL,Document *ndoc=NULL);
	NUpInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~NUpInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "NUp"; }
	virtual const char *Name();
	virtual const char *whattype() { return "NUpInterface"; }
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
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int UseThisDocument(Document *doc);
};



#endif

