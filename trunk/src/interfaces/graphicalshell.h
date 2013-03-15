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
#ifndef INTERFACES_GRAPHICALSHELL_H
#define INTERFACES_GRAPHICALSHELL_H

#include <lax/interfaces/aninterface.h>
#include <lax/lineedit.h>
#include <lax/menuinfo.h>

#include "../laidout.h"


namespace Laidout {



//------------------------------------- GraphicalShell --------------------------------------


class GraphicalShell : public LaxInterfaces::anInterface
{
 protected:
	int showdecs;
	Document *doc;
	LaidoutCalculator calculator;

	int currenthistory;
	Laxkit::PtrStack<char> history;

	int tablevel;
	int numresults;
	Laxkit::PtrStack<Laxkit::MenuItem> completion;

	char *searchterm;
	Laxkit::RefPtrStack<ObjectDef> areas; //definitions
	ValueHash context; //inside of which objects, all available names
	ObjectDef contextdef;
	int active;

	Laxkit::ScreenColor boxcolor;
	int showerror;
	int pad;
	Laxkit::DoubleBBox box;
	Laxkit::LineEdit *le;
	int placement_gravity;
	int showcompletion;
	Laxkit::MenuInfo tree;

	virtual int scan(int x,int y);
	virtual int Setup();
	void base_init();

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
 public:
	GraphicalShell(int nid=0,Laxkit::Displayer *ndp=NULL);
	GraphicalShell(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~GraphicalShell();
	virtual anInterface *duplicate(anInterface *dup);
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual const char *IconId() { return "GraphicalShell"; }
	virtual const char *Name();
	virtual const char *whattype() { return "GraphicalShell"; }
	virtual const char *whatdatatype() { return "NULL"; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void ViewportResized();

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual void RefreshTree(Laxkit::MenuInfo *menu, int x,int &y);
	virtual void DrawName(Laxkit::MenuItem *mii, int &x,int y);

	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int UseThisDocument(Document *doc);

	virtual int ChangeContext(const char *name, Value *value);
	virtual int Update();
	virtual int UpdateCompletion();
	virtual void UpdateFromTab(Laxkit::MenuInfo *menu, int &i);
	virtual void TextFromItem(Laxkit::MenuItem *mii,char *&str);
	virtual void EscapeBrowsing();
	
};


} //namespace Laidout


#endif

