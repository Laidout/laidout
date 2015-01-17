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

#include <lax/interfaces/objectinterface.h>
#include <lax/refptrstack.h>

#include "../laidout.h"
#include "../interfaces/actionarea.h"


namespace Laidout {



enum NUpControlType {
	NUP_None=0,
	NUP_Major_Arrow,
	NUP_Minor_Arrow,
	NUP_Major_Number,
	NUP_Minor_Number,
	NUP_Major_Tip,
	NUP_Minor_Tip,
	NUP_VAlign,
	NUP_HAlign,
	NUP_Type,
	NUP_Ok,
	NUP_Panel,
	NUP_Activate,

	 //menu ids
	NUP_Noflow=100,
	NUP_Grid,
	NUP_Sized_Grid,
	NUP_Flowed,
	NUP_Random,
	NUP_Unclump,
	NUP_Unoverlap,

	NUP_MAX
};

#define NUP_Has_Ok   (1<<0)
#define NUP_Has_Type (1<<1)
#define NUP_No_Align (1<<2)

#define NUP_LtoR  1
#define NUP_RtoL  2
#define NUP_BtoT  3
#define NUP_TtoB  4

//---------------------------------- NUpInfo -----------------------------------------
class NUpInfo : public Laxkit::DoubleBBox, public Laxkit::anObject, public LaxFiles::DumpUtility
{
  public:
	char *name;
	int direction; //lrtb, lrbt, rltb, rlbt, ...
	int flowtype; //whether allow variable sizes of cells, or each row/col same size, or flow as will fit
	double valign,halign; //how to center within rows and columns
	int rows, cols; //number of rows and columns in one n-up block, -1==infinity, 0==as many as will flow in
	double defaultgap;

	double scale; //transform for where the arrows are, works on screen space?
	flatpoint uioffset;
	
	NUpInfo();
	virtual ~NUpInfo();

	virtual void dump_out(FILE*, int, int, Laxkit::anObject*);
	virtual void dump_in_atts(LaxFiles::Attribute*, int, Laxkit::anObject*);
};


//------------------------------------- NUpInterface --------------------------------------

class NUpInterface : public LaxInterfaces::ObjectInterface
{
  protected:
	unsigned long color_arrow, color_num;

	ActionArea *major, *minor;
	ActionArea *majornum, *minornum;
	ActionArea *valignblock, *halignblock;
	ActionArea *okcontrol, *typecontrol;

	class ControlInfo //one per object
	{
	  public:
		flatpoint p;
		flatpoint v;
		double amountx,amounty;
		int flags;
		flatpoint new_center;
		flatpoint original_center;
		LaxInterfaces::SomeData *original_transform;

		ControlInfo();
		~ControlInfo();
		void SetOriginal(LaxInterfaces::SomeData *o);
	};
	Laxkit::PtrStack<ControlInfo> objcontrols;
	int needtoresetlayout;

	NUpInfo *nupinfo;
	int tempdir;
	int temparrowdir;
	Laxkit::PtrStack<ActionArea> controls;

	Laxkit::NumStack<double> rowpos;
	Laxkit::NumStack<double> colpos;
	Laxkit::RefPtrStack<LaxInterfaces::SomeData> groupareas;

	int showdecs;
	int firsttime;
	int overoverlay;
	int active;

	virtual int scanNup(int x,int y);
	virtual int hscan(int x,int y);
	virtual int vscan(int x,int y);
	virtual void createControls();
	virtual void remapControls(int tempdir=-1);
	virtual const char *controlTooltip(int action);
	virtual const char *flowtypeMessage(int set);
	virtual int Apply(int updateorig);
	virtual void ApplyUnclump();
	virtual void ApplyGrid();
	virtual void ApplySizedGrid();
	virtual void ApplyRandom();
	virtual int Reset();

	virtual int PerformAction(int action);

	virtual void WidthHeight(LaxInterfaces::ObjectContext *oc,flatvector x,flatvector y, double *width, double *height, flatpoint *cc);
  public:
	unsigned long nup_style;//options for interface

	NUpInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	NUpInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~NUpInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "NUp"; }
	virtual const char *Name();
	virtual const char *whattype() { return "NUpInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual void drawHandle(ActionArea *area, unsigned int color, flatpoint offset);
	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int validateInfo();

	virtual int FreeSelection();
	virtual int AddToSelection(Laxkit::PtrStack<LaxInterfaces::ObjectContext> &objs);
	virtual int AddToSelection(LaxInterfaces::Selection *objs);
};


} //namespace Laidout

#endif

