//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2005-2007,2009-2013 by Tom Lechner
//
#ifndef GROUPINTERFACE_H
#define GROUPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/somedata.h>
#include "../calculator/values.h"
#include "../viewwindow.h"



namespace Laidout {



//----------------------------- GroupInterface -----------------------

enum GroupInterfaceActions {
	GIA_Align = LaxInterfaces::OIA_MAX,
	GIA_Distribute,
	GIA_Edit_Filter_Nodes,
	GIA_Filter_Editor,
	GIA_Remove_Filter,
	GIA_Refresh_Filter,

	GIA_Clone,
	GIA_CloneB,
	GIA_Duplicate,
	GIA_DuplicateB,
	GIA_RegistrationMark,
	GIA_GrayBars,
	GIA_CutMarks,
	GIA_Parent,
	GIA_Unparent,

	//popup controls
	GIA_No_Popup,
	GIA_Link,
	GIA_Parent_Link,
	GIA_Constraints,
	GIA_Zone,
	GIA_Chains,

	GIA_Clip_First_On_Second,
	GIA_Clip_Second_On_First,
	GIA_Extract_Clip,
	GIA_Remove_Clip,
	GIA_Edit_Clip,

	GIA_Jump_To_Link,
	GIA_Sever_Link,

	GIA_Parent_Align,
	GIA_Parent_Matrix,
	GIA_Jump_To_Parent,
	GIA_Reparent,


	GIA_MAX
};


class GroupInterface : public LaxInterfaces::ObjectInterface, public Value
{
  protected:
	int rx,ry;
	GroupInterfaceActions popupcontrols;
	VObjContext reparent_temp;

	virtual int PerformAction(int action);
	virtual const char *hoverMessage(int p);
	virtual int AlternateScan(flatpoint sp, flatpoint p, double xmag,double ymag, double onepix);
	virtual int GetMode();
	virtual void DrawReparentArrows();

  public:
	void TransformSelection(const double *N, int s=-1, int e=-1);// *****

	GroupInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~GroupInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *whattype() { return "ObjectInterface"; }
	virtual const char *whatdatatype() { return "Group"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual int UseThis(anObject *newdata,unsigned int);
	virtual int draws(const char *atype);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	virtual int LBDown(int x, int y,unsigned int state, int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int GrabSelection(unsigned int state);
	virtual int ToggleGroup();
	virtual int GroupObjects();
    virtual int UngroupObjects();

	//from value
	virtual int type() { return VALUE_Fields; }
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} //namespace Laidout

#endif

