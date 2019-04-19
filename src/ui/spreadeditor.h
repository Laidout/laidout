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
// Copyright (C) 2004-2015 by Tom Lechner
//
#ifndef SPREADEDITOR_H
#define SPREADEDITOR_H

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/pathinterface.h>
#include "../core/document.h"
#include "../core/project.h"
#include "../core/drawdata.h"
#include "../core/spreadview.h"


namespace Laidout {


//----------------------- SpreadInterface --------------------------------------
class SpreadEditor;

enum SpreadInterfaceActions {
	SIA_ToggleSelect,
	SIA_SelectSimilar,
	SIA_Center,
	SIA_LabelPos,
	SIA_ToggleColor,
	SIA_ToggleColorR,
	SIA_ToggleMark,
	SIA_ToggleMarkR,
	SIA_Thumbnails,
	SIA_ToggleArrange,
	SIA_RefreshArrange,

	SIA_First=           1000,
	SIA_PageLabels       ,
	SIA_InsertDummyPage  ,
	SIA_AddPageBefore    , 
	SIA_AddPageAfter     ,
	SIA_DeletePages      ,
	SIA_HidePages        ,
	SIA_DetachPages      ,
	SIA_ExportPages      ,
	SIA_NewView          ,
	SIA_SaveView         ,
	SIA_DeleteView       ,
	SIA_RenameView       ,

	SIA_MaybeMovePages   ,
	SIA_MoveSpreads,
	SIA_DragRect,

	SIA_ArrangeTypeMin=2000,
	SIA_ArrangeTypeMax=2000+ArrangetypeMax-ArrangetypeMin,
	SIA_MAX
};

class SpreadInterface : public LaxInterfaces::anInterface, virtual public LaxFiles::DumpUtility
{
 protected:
	int maxmarkertype;

	SpreadView *view;
	int drawthumbnails;

	int firsttime;
	int reversebuttons;
	flatpoint lbdown, lastmove;

	int curpage, dragpage;
	Laxkit::NumStack<int> curpages;
	Laxkit::PtrStack<LittleSpread> curspreads;
	LittleSpread *curspread;

	Laxkit::PtrStack<Page> pagestorender;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

 public:
	Document *doc;
	Project *project;
	unsigned int style;
	unsigned long controlcolor;

	SpreadInterface(Laxkit::Displayer *ndp,Project *proj,Document *docum);
	virtual ~SpreadInterface();
	virtual const char *IconId() { return "Spread"; }
	virtual const char *Name();
	virtual const char *whattype() { return "SpreadInterface"; }
	virtual const char *whatdatatype() { return "LittleSpread"; }
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual int rLBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int rLBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int rMBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int rMBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
//	//virtual int DrawData(Laxkit::anObject *ndata,int info=0);
//	//virtual int UseThis(Laxkit::anObject *newdata,unsigned int); // assumes not use local
//	//virtual void deletedata();
	virtual int UseThisDoc(Document *ndoc);
	virtual int SwitchView(int i);
	virtual int InterfaceOn();
//	//virtual int InterfaceOff();
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual void clearSelection();
	virtual void SortCurpages();
	//virtual int Modified();
	virtual void UpdateMarkers(bool all);
	virtual void CheckSpreads(int startpage,int endpage);
	virtual void GetSpreads();
	virtual void ArrangeSpreads(int how=-1);
	virtual LittleSpread *findSpread(int x,int y,int *pagestacki, int *thread);
	virtual void Center(int w=1);

	virtual void Reset();
	virtual void ApplyChanges();
	virtual void SwapPages(int previouspos, int newpos);
	virtual void SlidePages(int previouspos, int newpos,int thread);
	virtual int ChangeMarks(int newmark);
	virtual int DeselectPage(int page,LittleSpread *spread);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

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
	SpreadInterface *spreadtool;
	SpreadEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx, int yy, int ww, int hh, int brder,
						Project *project, Document *ndoc);
	virtual ~SpreadEditor();
	virtual int init();
	virtual const char *whattype() { return "SpreadEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int UseThisDoc(Document *ndoc);
	virtual int SelectTool(int id);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} // namespace Laidout

#endif

