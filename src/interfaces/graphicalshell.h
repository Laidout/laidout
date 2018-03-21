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

	char *searchterm;
	char *searchexpression;

	int showcompletion;
	int showdecs;
	Document *doc;
	LaidoutCalculator calculator;

	Laxkit::ScreenColor boxcolor;
	int pad;
	Laxkit::DoubleBBox box;
	Laxkit::LineEdit *le;
	int placement_gravity;


	int num_lines_above; //number of lines of matches to show, -1 means fill to top
	int num_lines_input; //default -1, to use just 1, but autoexpand when necessary
	int current_column; //-1 means inside edit box. >0 means default to that column on key up
	int current_item; //-1 means inside edit box
	int hover_column;
	int hover_item;

	class ColumnInfo
	{
	  public:
		double x;
		double width;
		int num_matches;
		int offset;
		Laxkit::MenuInfo items;
		ColumnInfo() { x=0; width=-1; num_matches=-1; offset=0; }
	};
	ColumnInfo columns[3];

	void ClearSearch();
	void UpdateMatches();

	bool needtomap;

	 //column 1, context matches
	Laxkit::MenuInfo tree; //tree of all context
	ValueHash context; //inside of which objects, all available names
	ObjectDef *searcharea;
	char *searcharea_str;

	 //column 2, expression history

	 //column 3, past values
	ValueHash past_values; //column 3


	int showerror;
	char *error_message;
	int error_message_type;
	virtual void ClearError();
	virtual void MakeHoverInWindow();
	virtual int PreviousColumn(int cc);
	virtual int NextColumn(int cc);


	virtual void AddTreeToCompletion(Laxkit::MenuInfo *menu);
	virtual void UpdateSearchTerm(const char *str,int pos, int firsttime);
	virtual ObjectDef *GetContextDef(const char *expr);
	virtual const char *GetItemText(int column,int item);
	virtual int scan(int x,int y, int *column, int *item);
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
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void ViewportResized();

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);	    
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual void RefreshTree(Laxkit::MenuInfo *menu, int x,int &y, int col,int &item);
	virtual void DrawName(Laxkit::MenuItem *mii, int &x,int y);

	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	virtual int UseThisDocument(Document *doc);

	//virtual int ChangeContext(const char *name, Value *value);
	virtual int UpdateContext();
	virtual int InitAreas();
	virtual void UpdateFromItem();
	virtual void TextFromItem(Laxkit::MenuItem *mii,char *&str);
	virtual void EscapeBrowsing();
	
};


} //namespace Laidout


#endif

