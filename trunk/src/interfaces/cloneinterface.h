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
// Copyright (C) 2013 by Tom Lechner
//



namespace Laidout {

//------------------------------------- CloneInterface --------------------------------------

class CloneInterface : public LaxInterfaces::ObjectInterface
{
  protected:


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

	CloneInfo *cloneinfo;
	int tempdir;
	int temparrowdir;
	Laxkit::PtrStack<ActionArea> controls;

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
	unsigned long clone_style;//options for interface

	CloneInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	CloneInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~CloneInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Clone"; }
	virtual const char *Name();
	virtual const char *whattype() { return "CloneInterface"; }
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
};


} //namespace Laidout

