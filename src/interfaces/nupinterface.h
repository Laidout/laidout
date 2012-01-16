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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef INTERFACES_NUPINTERFACE_H
#define INTERFACES_NUPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/refptrstack.h>

#include "../laidout.h"
#include "../interfaces/actionarea.h"


#define NUP_None         0
#define NUP_Major_Arrow  1
#define NUP_Minor_Arrow  2
#define NUP_Major_Number 3
#define NUP_Minor_Number 4
#define NUP_Major_Tip    5
#define NUP_Minor_Tip    6
#define NUP_Type         7
#define NUP_Ok           8
#define NUP_Panel        9

#define NUP_Grid         100
#define NUP_Sized_Grid   101
#define NUP_Flowed       102
#define NUP_Random       103
#define NUP_Unclump      104
#define NUP_Unoverlap    105

#define NUP_Has_Ok   (1<<0)
#define NUP_Has_Type (1<<1)


//---------------------------------- NUpInfo -----------------------------------------
class NUpInfo : public Laxkit::DoubleBBox, public Laxkit::RefCounted
{
  public:
	char *name;
	int direction; //lrtb, lrbt, rltb, rlbt, ...
	int flowtype; //whether allow variable sizes of cells, or each row/col same size, or flow as will fit
	double rowcenter, colcenter; //how to center within rows and columns
	int rows, cols; //number of rows and columns in one n-up block, -1==infinity, 0==as many as will flow in

	double scale; //transform for where the arrows are, works on screen space?
	flatpoint uioffset;
	
	NUpInfo();
	virtual ~NUpInfo();
};


//------------------------------------- NUpInterface --------------------------------------

class NUpInterface : public LaxInterfaces::anInterface
{
  protected:
	Laxkit::ButtonDownInfo buttondown;

	unsigned long color_arrow, color_num;

	ActionArea *major, *minor;
	ActionArea *majornum, *minornum;
	ActionArea *okcontrol, *typecontrol;

	NUpInfo *nupinfo;
	int tempdir;
	Laxkit::PtrStack<ActionArea> controls;

	Laxkit::RefPtrStack<LaxInterfaces::SomeData> objects;

	int showdecs;
	int firsttime;
	int overoverlay;

	virtual int scan(int x,int y);
	virtual void createControls();
	virtual void remapControls();
  public:
	unsigned long nup_style;//options for interface

	NUpInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
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
	virtual void drawHandle(ActionArea *area, flatpoint offset);
	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
};



#endif

