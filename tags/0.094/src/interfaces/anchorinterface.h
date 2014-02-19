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
#ifndef INTERFACES_ANCHORINTERFACE_H
#define INTERFACES_ANCHORINTERFACE_H

#include <lax/interfaces/somedataref.h>
#include <lax/refptrstack.h>
#include "../guides.h"
#include "../viewwindow.h"
#include "selection.h"

//#include "../laidout.h"


namespace Laidout {





//------------------------------------- AnchorInterface --------------------------------------

enum AnchorPlaces
{
	ANCHOR_Parents=1,
	ANCHOR_Page_Area,
	ANCHOR_Margin_Area,
	ANCHOR_Paper,
	ANCHOR_Selection,
	ANCHOR_Other_Objects,
	ANCHOR_Guides,

	ANCHOR_Anchor,
	ANCHOR_Region,
	ANCHOR_Regions,
	ANCHOR_Offset,
	ANCHOR_Contstraint,
	ANCHOR_Rules,
	ANCHOR_Object,
	ANCHOR_MAX
};

class AnchorInterface : public LaxInterfaces::anInterface
{
  protected:

	class Anchors
	{
	  public:
		PointAnchor *anchor;
		flatpoint real_point;
		int anchorsource;
		int on;
		Anchors(PointAnchor *aa, flatpoint real, int source, int oon);
		~Anchors();
	};

	Selection *selection;
	VObjContext *cur_oc;
	LaxInterfaces::SomeDataRef *proxy;
	PointAnchor temptarget;
	AlignmentRule *current_rule;
	flatpoint current_target;
	flatpoint offset_dir;
	flatpoint current_offset;

	unsigned int anchor_regions;
	bool show_region_selector;
	bool firsttime;
	Laxkit::PtrStack<Anchors> anchors;
	Laxkit::MenuInfo regions;
	int hover_item, hover_anchor;
	int active_anchor, active_i1, active_i2;
	int active_match;
	int active_type, last_type;
	bool newpoint;

	virtual int scan(int x,int y, int *anchor);
	virtual void DrawSelectedIndicator(LaxInterfaces::ObjectContext *oc);
	virtual void RefreshMenu();
	virtual void DrawText(flatpoint p,const char *s);
	virtual int RemoveAnchors(LaxInterfaces::ObjectContext *oc);
	virtual int UpdateAnchors(int region);
	virtual void UpdateSelectionAnchors();
	virtual bool RegionActive(int region);
	virtual int NumInvariants();
	virtual int findAnchor(int id);

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:

	AnchorInterface(LaxInterfaces::anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~AnchorInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Anchor"; }
	virtual const char *Name();
	virtual const char *whattype() { return "AnchorInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype);
	virtual int AddAnchor(flatpoint p,int p_type, const char *name, int source, int id, Laxkit::anObject *owner);
	virtual int AddAnchors(VObjContext *context, int source);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	virtual int SetCurrentObject(LaxInterfaces::ObjectContext *oc);
	virtual int AddToSelection(LaxInterfaces::ObjectContext *oc, int where=-1);
	virtual int AddToSelection(Laxkit::PtrStack<LaxInterfaces::ObjectContext> &selection);
};



} //namespace Laidout

#endif

