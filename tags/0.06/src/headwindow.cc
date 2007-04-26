//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include "headwindow.h"
#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"
#include "helpwindow.h"
#include "commandwindow.h"
#include "buttonbox.h"
#include "palettes.h"

#include <lax/laxutils.h>
#include <lax/filedialog.h>

#include <X11/cursorfont.h>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;

//---------------------------- HeadWindow Pane Generators -----------------------

/*! \defgroup mainwindows Main Pane Windows For HeadWindows
 *
 * These become the panes of a HeadWindow.
 */

////---------------------- newPaletteWindowFunc
/*! \ingroup mainwindows
 * \brief ButtonBox window generator for use in HeadWindow.
 */
anXWindow *newPaletteWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	Window owner=None;
	if (laidout->lastview) owner=laidout->lastview->window;
	PaletteWindow *palette=new PalettePane(parnt,ntitle,style, 0,0,0,0,1, NULL,owner,"change color");
	return palette;
}

////---------------------- newCommandWindowFunc
/*! \ingroup mainwindows
 * \brief ButtonBox window generator for use in HeadWindow.
 */
anXWindow *newButtonBoxFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	ButtonBox *buttons=new ButtonBox(parnt,ntitle,style, 0,0,0,0,1);
	return buttons;
}

////---------------------- newCommandWindowFunc
/*! \ingroup mainwindows
 * \brief CommandWindow window generator for use in HeadWindow.
 */
anXWindow *newCommandWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	CommandWindow *command=new CommandWindow(parnt,ntitle,style, 0,0,0,0,1);
	return command;
}

////---------------------- newViewWindowFunc
/*! \ingroup mainwindows
 * \brief ViewWindow window generator for use in HeadWindow.
 */
anXWindow *newViewWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	return new ViewWindow(parnt,ntitle,style, 0,0,0,0,1, NULL);
}

//------------------------ newSpreadEditorFunc
/*! \ingroup mainwindows
 * \brief SpreadEditro window generator for use in HeadWindow.
 */
anXWindow *newSpreadEditorFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	return new SpreadEditor(parnt,ntitle,style, 0,0,0,0,1, NULL,NULL);
}

//------------------------ newHelpWindowFunc
/*! \ingroup mainwindows
 * \brief SpreadEditor window generator for use in HeadWindow.
 */
anXWindow *newHelpWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
{
	HelpWindow *help=new HelpWindow(1);
	XFree(help->win_sizehints);
	help->win_parent=parnt;
	return help;
}

//---------------------------- newHeadWindow() -----------------------
/*! \ingroup mainwindows
 * \brief Create a new head split window, and fill it with a which window.
 *
 * Current main windows are:
 * <pre>
 *  ViewWindow
 *  SpreadEditor
 *  HelpWindow
 *  CommandWindow
 *  PaletteWindow (will be modified later)
 *
 *   TODO:
 *  Icons/Menus with optional dialogs ***
 *  Directory Window? FileOpener? ImageFileOpener->to drag n drop images? ***not imp
 *  PlainTextEditor ***not imp
 *  StoryEditor *** not imp
 *  StyleManager ***not imp
 *  ObjectTreeEditor ***not imp
 * </pre>
 *
 * \todo *** might want to remove doc, or have some more general way to start what is in which.
 *    perhaps have app keep track of most recently accessed doc? or have option to keep a new
 *    head empty
 * \todo *** WindowGone if was only one window, then remove the headwindow, if that was last one, then
 *    either quit or spring up default panel
 * \todo *** this will have to be modified later to more easily add windows through plugins...
 */
anXWindow *newHeadWindow(Document *doc,const char *which)
{
	if (doc==NULL) {
		doc=laidout->curdoc;
		if (!doc) {
			if (!laidout->project) return NULL;
			if (!laidout->project->docs.n) return NULL;
			doc=laidout->project->docs.e[0];
		}
	}
	HeadWindow *head=new HeadWindow(NULL,"head",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE, 0,0,500,500,0);

	 // put a new which in it. default to view
	if (which) head->Add(which);
	else head->Add(new ViewWindow(doc));//***

	return head;
}

//! Create a new head window based on att, that has not been added to the reigning anXApp.
/*! \todo *** should return NULL if the att was invalid
 *  \todo *** for plugins that have new pane types, there must be mechanism to
 *    add those types to new and existing headwindows
 */
anXWindow *newHeadWindow(LaxFiles::Attribute *att)
{
	HeadWindow *head=new HeadWindow(NULL,"head",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE, 0,0,500,500,0);
	head->dump_in_atts(att,0);
	return head;
}

//------------------------------- HeadWindow ---------------------------------------
/*! \class HeadWindow
 * \brief Top level windows to hold other stuff such as a ViewWindow.
 *
 * When coding new pane types, currently the built in ones must be manually added
 * in the constructor.
 * 
 * <pre>
 * *** menu: (todo for splitwindow probably)
 *  hide tabs for stacked panes
 *  show tabs for stacked panes
 *  mark for swap
 *  swap with marked
 *  float
 * </pre>
 */  

 //for SplitWindow::mode
#define NORMAL          0
#define MOVE_EDGES      1
#define VERTICAL_SPLIT  2
#define HORIZ_SPLIT     3
#define MAXIMIZED       4
#define SWAPWITH        50
#define DROPTO          51


Laxkit::PlainWinBox *HeadWindow::markedpane=NULL;
HeadWindow *HeadWindow::markedhead=NULL;

//! Pass SPLIT_WITH_SAME|SPLIT_BEVEL|SPLIT_DRAG_MAPPED to SplitWindow.
/*! Adds the main window type generating functions.
 *
 * \todo Currently, every new HeadWindow instance gets its own set of window generator functions.
 *   This should be changed to a HeadWindow static list, allowing all instances to share one
 *   list, which can easily be added to by plugins..
 */
HeadWindow::HeadWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: SplitWindow(parnt,ntitle,
				nstyle|SPLIT_WITH_SAME|SPLIT_BEVEL|SPLIT_DRAG_MAPPED,
				xx,yy,ww,hh,brder)
{
	//*** should fill with funcs for all known default main windows:
	//  ViewWindow
	//  SpreadEditor
	//  Buttons
	//  FileDialog?
	//  ArrangementEditor
	//  StyleManager
	//  ObjectTreeEditor
	
	win_xatts.background_pixel=app->coloravg(app->color_bg,0,.33);
	space=4;

	tooltip("With mouse in the gutter:\n"
			"  control-left click Splits\n"
			"  shift-left click Joins\n"
			"  right click brings up menu"
		);

	lastview=lastedit=NULL;
	
	 // add the window generator funcs
	AddWindowType("ViewWindow","View Window",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newViewWindowFunc,1);
	AddWindowType("SpreadEditor","Spread Editor",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newSpreadEditorFunc,0);
	AddWindowType("HelpWindow","Help Window",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newHelpWindowFunc,0);
	AddWindowType("CommandWindow","Command Prompt",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newCommandWindowFunc,0);
	AddWindowType("ButtonBox","Buttons",
			ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE|BOXSEL_STRETCHX|BOXSEL_ROWS|BOXSEL_BOTTOM,
			newButtonBoxFunc,0);
	AddWindowType("PaletteWindow","Palette",PALW_DBCLK_TO_LOAD|ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newPaletteWindowFunc,0);
}

//! Empty virtual destructor.
HeadWindow::~HeadWindow()
{}

//! Redefined to use a global mark, rather than per SplitWindow.
/*! If c==-1 then mark curbox. Else if c is out of range, then do not change
 * markedpane or markedhead. Else set them to the pane of that index.
 */
int HeadWindow::Mark(int c)
{
	if (c==-1) {
		markedpane=curbox;
		markedhead=this;
		DBG cout <<"----head marking curbox in window "<<window<<endl;
	} else {
		if (c<0 || c>windows.n) return 1;
		markedpane=windows.e[c];
		markedhead=this;
		DBG cout <<"----head marking box "<<c<<" in window "<<window<<endl;
	}
	return 0;
}

//! Redefined to use a global mark, rather than per SplitWindow.
/*! Returns 0 for success, nonzero error.
 */
int HeadWindow::SwapWithMarked()
{
	//DBG cout <<" --SwapWithMarked: "<<
	if (curbox==NULL || markedpane==NULL || markedhead==NULL || curbox==markedpane) return 0;

	 // make sure markedhead is toplevel
	if (laidout->isTopWindow(markedhead)) {
		 // make sure markedpane is still pane of markedhead
		if (markedhead->FindBoxIndex(markedpane)>=0) {
			 // must potentially reparent!!!
			if (markedhead!=this) {
				if (curbox->win) app->reparent(curbox->win,markedhead);
				if (markedpane->win) app->reparent(markedpane->win,this);
			}
			anXWindow *w=curbox->win;
			curbox->win=markedpane->win;
			markedpane->win=w;
			curbox->sync(space/2);
			markedpane->sync(space/2);
			return 0;
		}
	} 
	
	markedpane=NULL;
	markedhead=NULL;
	return 1;
}

/*! Return 0 if window taken, else nonzero error.
 *
 * Note that which is ignored. Works only on curbox.
 */
int HeadWindow::Change(anXWindow *towhat,anXWindow *which)
{
	if (!curbox) return 0;

	 //Get doc from curbox. If win has no doc, then get first
	 //doc found in head. If none in head, then first in project (***should be last accessed?)
	Document *doc=findAnyDoc();

	 //change the window
	if (SplitWindow::Change(towhat,which)!=0) return 1;
	if (!doc) return 0;

	 // make sure same doc from old curbox is used for view and spread
	ViewWindow *v=dynamic_cast<ViewWindow *>(curbox->win);
	if (v) {
		((LaidoutViewport *)(v->viewport))->UseThisDoc(doc);
		v->doc=doc;
	} else {
		SpreadEditor *s=dynamic_cast<SpreadEditor *>(curbox->win);
		if (s) s->UseThisDoc(doc);
	}

	return 0;
}

//! Return doc associated with curbox, or with any pane in this, or first doc of laidout->project.
Document *HeadWindow::findAnyDoc()
{
	ViewWindow *v;
	SpreadEditor *s;
	if (curbox) {
		v=dynamic_cast<ViewWindow *>(curbox->win);
		if (v && v->doc) return v->doc;
		s=dynamic_cast<SpreadEditor *>(curbox->win);
		if (s && s->doc) return s->doc;
	}
	for (int c=0; c<windows.n; c++) {
		if (!windows.e[c]->win) continue;
		v=dynamic_cast<ViewWindow *>(windows.e[c]->win);
		if (v)
			if (v->doc) return v->doc;
			else continue;
		s=dynamic_cast<SpreadEditor *>(windows.e[c]->win);
		if (s)
			if (s->doc) return s->doc;
			else continue;
	}
	if (laidout->project && laidout->project->docs.n) return laidout->project->docs.e[0];
	return NULL;
}
	
/*! \todo ***** should be able to duplicate views/spreadeditor/etc..
 */
int HeadWindow::splitthewindow(anXWindow *fillwindow)
{
	DBG cout <<"********SPLITTING THE WINDOW"<<endl;
	 //Get doc from curbox. If win has no doc, then get first
	 //doc found in head. If none in head, then first in project (***should be last accessed?)
	Document *doc=findAnyDoc();
	
	anXWindow *winold=curbox->win;
	if (SplitWindow::splitthewindow(fillwindow)!=0) return 1;
	anXWindow *win=windows.e[windows.n-1]->win;
	if (!win) return 0;

	 // make sure same doc from old box is used for view and spread
	ViewWindow *v=dynamic_cast<ViewWindow *>(win);
	if (v) {
		((LaidoutViewport *)(v->viewport))->UseThisDoc(doc);
		v->doc=doc;
		ViewWindow *vold=dynamic_cast<ViewWindow *>(winold);
		 // duplicate view and page of old
		if (vold) {
			int pg, m=((LaidoutViewport *)(vold->viewport))->ViewMode(&pg);
			((LaidoutViewport *)(v->viewport))->SetViewMode(m,pg);
		}
	} else {
		SpreadEditor *s=dynamic_cast<SpreadEditor *>(win);
		if (s) s->UseThisDoc(doc);
	}
	return 0;
}


//! Dump out what's in the window, and the borders.
/*! \todo *** stacked panes
 */
void HeadWindow::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	//anXWindow *win;
	LaxFiles::DumpUtility *wind;
	fprintf(f,"%sxywh %d %d %d %d\n",
				spc,win_x,win_y,win_w,win_h);
	for (int c=0; c<windows.n; c++) {
		fprintf(f,"%spane\n",spc);
		fprintf(f,"%s  xyxy %d %d %d %d\n",
					spc,windows.e[c]->x1,windows.e[c]->y1,windows.e[c]->x2,windows.e[c]->y2);
		if (windows.e[c]->win) {
			fprintf(f,"%s  window %s\n",spc,windows.e[c]->win->whattype());
			wind=dynamic_cast<DumpUtility *>(windows.e[c]->win);
			if (wind) wind->dump_out(f,indent+4,what);
		}
	}
}

//! Set up the window according to windows listed in att.
/*! 
 * \todo *** as time goes on, must ensure that header can deal with new types of windows...
 */
void HeadWindow::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	char *name,*value;
	int c,c2,c3;
	PlainWinBox *box;
	anXWindow *win;
	LaxFiles::DumpUtility *wind;
	int i[4];
	windows.flush();
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"xywh")) {
			c2=IntListAttribute(value,i,4);
			if (c2>0) win_x=i[0];
			if (c2>1) win_y=i[1];
			if (c2>2) win_w=i[2];
			if (c2>3) win_h=i[3];

			if (c2>3 && win_w>0 && win_h>0) {
				if (!win_sizehints) win_sizehints=XAllocSizeHints();
				if (win_sizehints && !win_parent) {
					DBG cout <<"doingwin_sizehintsfor"<<(win_title?win_title:"untitled")<<endl;
					//*** The initial x and y become the upper left corner of the window
					//manager decorations. ***how to figure out how much room those decorations take,
					//so as to place things on the screen accurately? like full screen view?
					win_sizehints->x=win_x;
					win_sizehints->y=win_y;
					win_sizehints->width=win_w;
					win_sizehints->height=win_h;
					win_sizehints->flags=USPosition|USSize;
				}
			}
		} else if (!strcmp(name,"pane")) {
			box=new PlainWinBox(NULL,0,0,0,0);
			for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"xyxy")) {
					c3=IntListAttribute(value,i,4);
					if (c3>0) box->x1=i[0];
					if (c3>1) box->y1=i[1];
					if (c3>2) box->x2=i[2];
					if (c3>3) box->y2=i[3];
				} else if (!strcmp(name,"window")) {
					win=NewWindow(value);
					if (win) {
						wind=dynamic_cast<DumpUtility *>(win);
						if (wind) wind->dump_in_atts(att->attributes.e[c]->attributes.e[c2],flag);
						box->win=win;
					} else {
						DBG cout <<"**** *** warning: window func not found for "<<(value?value:"(unknown)")<<endl;
					}
				}
			}
			box->sync(space/2,1);
			windows.push(box);
		}
	}
	//***must check that window is actually reasonably onscreen 
}


//! Remove references to win.
void HeadWindow::WindowGone(Laxkit::anXWindow *win)
{
	if (!win) return;
	for (int c=0; c<windows.n; c++) {
		if (windows.e[c]->win==win) {
			windows.e[c]->win=NULL;
			if (windows.n==1) app->destroywindow(this);
		}
	}
	needtodraw=1;
}

//! Returns SplitWindow::init().
int HeadWindow::init()
{
	return SplitWindow::init();
//	if (!window) return 1;
//
//	//default SplitWindow just adds new box if windows.n==0
//	//assume there's one alread?
//	
//	return 0;
}


/*! \todo *** work on this..
 * <pre>
 *  *** must make something like:
 *   Split
 *   Join
 *   Split...
 *   Change to >
 *   Float
 *   Stack on >
 *   ------
 *   (other things defined by curbox?...
 * </pre>
 */
MenuInfo *HeadWindow::GetMenu()
{
	DBG cout <<"*************GetMenu: mode="<<mode<<endl;
	MenuInfo *menu=new MenuInfo();
	
 //make sure this always agrees with SplitWindow::mode!!
	
	 //straight from Laxkit, do not change item ids:
	if (mode!=MAXIMIZED) {
		menu->AddItem("Split",1);
		menu->AddItem("Join",2);
	}
	if (winfuncs.n) {
		menu->AddItem("Change to");
		menu->SubMenu();
		for (int c=0; c<winfuncs.n; c++) {
			menu->AddItem(winfuncs.e[c]->desc,101+c);
		}
		menu->AddItem("(Blank)",100);
		menu->EndSubMenu();
	}
	if (mode!=MAXIMIZED) {
		menu->AddItem("Mark",3);
		menu->AddItem("Swap with marked",4);
		menu->AddItem("Swap with...",53);
		
		 //laidout additions:
		menu->AddItem("Drop To...",51);
		menu->AddItem("Float",52);
	}

	 //straight from Laxkit, do not change item ids:
	if (mode==MAXIMIZED) menu->AddItem("Un-Maximize",5);
	else menu->AddItem("Maximize",5);
	
	 //laidout additions:
	menu->AddSep();
	menu->AddItem("New Window",50);
	return menu;
}

/*! Propagate TreeChangeEvent events
 */
int HeadWindow::DataEvent(Laxkit::EventData *data,const char *mes)
{
	DBG cout <<"HeadWindow got message: "<<mes<<endl;
	if (!strcmp(mes,"docTreeChange")) {
		TreeChangeEvent *edata,*te=dynamic_cast<TreeChangeEvent *>(data);
		if (!te) return 1;
		ViewWindow *view;
		SpreadEditor *s;
		
		 // Relay docTreeChange to each pane 
		int yes=0;
		for (int c=0; c<windows.n; c++) {
			view=dynamic_cast<ViewWindow *>(windows.e[c]->win);
			if (view) yes=1;
			else if (s=dynamic_cast<SpreadEditor *>(windows.e[c]->win), s) yes=1;

		 	 //construct events for the panes
			if (yes){
				edata=new TreeChangeEvent();
				*edata=*te;
				edata->send_towindow=windows.e[c]->win->window;
				app->SendMessage(edata);
				DBG cout <<"---sending docTreeChange to "<<windows.e[c]->win->win_title<<endl;
				yes=0;
			}
		}
		delete data;
		return 0;
	} else if (!strcmp(mes,"open document")) {
		StrsEventData *s=dynamic_cast<StrsEventData *>(data);
		if (!s || !s->n) return 1;

		//**** this is really hacky if doc already open...
		int nw;
		Document *d;
		for (int c=0; c<s->n; c++) {
			nw=laidout->numTopWindows();
			d=laidout->LoadDocument(s->strs[c]);
			if (!d) {
				//DBG cout <<"*** fail to open "<<s->strs[c]<<endl;
			} else {
				if (nw==laidout->numTopWindows()) {
					app->addwindow(newHeadWindow(d));
				}
			}
		}
		delete data;
		return 0;
	}
	return 1;
}

int HeadWindow::event(XEvent *e)
{ 
	//DBG cout <<"SplitWindow::event:"<<event_name(e->type)<<endl;
	//DBG if (e->type==EnterNotify || e->type==LeaveNotify) { cout <<" crossing:"; printxcrossing(this,e); }

	if (e->type==LeaveNotify && mode!=SWAPWITH && mode!=DROPTO) {
		mousein=0;
		DBG cout <<"************************UNDEFINE CURSOR**************** mode="<<mode<<endl;
		if (!buttondown) XUndefineCursor(app->dpy,window);
	}
	return SplitWindow::event(e);
}

//! Intercept a menu item values 50 to 99.
/*! 
 * <pre>
 *  50: Create new HeadWindow
 *  51: Drop to...
 *  52: Float
 * </pre>
 */
int HeadWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"popupsplitmenu") && e->data.l[1]>=50 && e->data.l[1]<100) {
		if (e->data.l[1]==50) {
			const char *type=NULL;
			if (curbox && curbox->win) type=curbox->win->whattype();
			app->addwindow(newHeadWindow(NULL,type));
			return 0;
		} else if (e->data.l[1]==52) {
			 //Float
			 
			 //pop the window to new headwindow
			if (!curbox || !curbox->win || windows.n<=1) return 0;

			HeadWindow *head=new HeadWindow(NULL,"head",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE, 0,0,500,500,0);
			app->addwindow(head);
			head->Add(curbox->win);//this reparents win
			curbox->win=NULL;

			RemoveCurbox();
			return 0;
		} else if (e->data.l[1]==51) {
			if (mode!=0 || !curbox) return 0;
			DBG cout <<"  HeadWindow:: Drop To..."<<endl;
			
			XWindowAttributes atts;
			XGetWindowAttributes(app->dpy,window, &atts);
			if (atts.map_state!=IsViewable) return 0;

			if (XGrabPointer(app->dpy, window, False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
							 GrabModeAsync,GrabModeAsync,
							 None, None, CurrentTime)!=GrabSuccess) return 0;
			DBG cout <<"***********************GRAB***********************"<<endl;
			app->Tooltips(0);
			Cursor cursor=XCreateFontCursor(app->dpy,XC_sb_down_arrow);
			if (cursor) {
				DBG cout <<"***********************CURSOR***********************"<<endl;
				XDefineCursor(app->dpy,window,cursor);
				XFreeCursor(app->dpy,cursor);
			}
			markedhead=NULL;
			markedpane=NULL;
			mode=DROPTO;
			DBG cout <<"***************changing mode to DROPTO"<<endl;
			return 0;
		} else if (e->data.l[1]==53) {
			 // swap with...
			if (mode!=0 || !curbox) return 0;
			DBG cout <<"  HeadWindow:: Swap With..."<<endl;
			
			XWindowAttributes atts;
			XGetWindowAttributes(app->dpy,window, &atts);
			if (atts.map_state!=IsViewable) return 0;

			DBG cout <<"***********************GRAB***********************"<<endl;
			if (XGrabPointer(app->dpy, window, False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
							 GrabModeAsync,GrabModeAsync,
							 None, None, CurrentTime)!=GrabSuccess) return 0;
			Cursor cursor=XCreateFontCursor(app->dpy,XC_exchange);
			if (cursor) {
				DBG cout <<"***********************CURSOR***********************"<<endl;
				XDefineCursor(app->dpy,window,cursor);
				XFreeCursor(app->dpy,cursor);
			}
			app->Tooltips(0);
			markedhead=NULL;
			markedpane=NULL;
			mode=SWAPWITH;
			DBG cout <<"***************changing mode to SWAPWITH"<<endl;
			return 0;
		}
	}
	return SplitWindow::ClientEvent(e,mes);
}

//! Intercept and do nothing when grabbed for a "drop to..." to or "swap with...".
int HeadWindow::LBDown(int x,int y,unsigned int state,int count)
{
	DBG cout <<"***********HeadWindow::LBDown mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::LBDown(x,y,state,count);
	//markedpane=NULL;
	//markedhead=NULL;
	return 0;
}

//! Intercept to do a "drop to..." to or "swap with...".
int HeadWindow::LBUp(int x,int y,unsigned int state)
{
	DBG cout <<"***********HeadWindow::LBUp mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::LBUp(x,y,state);
	
	XUngrabPointer(app->dpy, CurrentTime);
	app->Tooltips(1);
	DBG cout <<"***********************UN-GRAB***********************"<<endl;
	if (!laidout->isTopWindow(markedhead)) { 
		DBG if (!markedhead) cout <<"***********no marked head"<<endl;
		DBG else cout <<"***********marked head is not top"<<endl;
		DBG cout <<"***************changing mode to NORMAL"<<endl;
		mode=0;
		return 0; 
	}
	if (!markedpane) {
		DBG cout <<"***********no marked pane"<<endl;
		return 0;
	}
	
	if (mode==DROPTO) {
		if (markedpane!=curbox) {
			 // remove original pane and put it in markedhead/markedpane.
			 // If there was only one pane in *this, then remove this HeadWindow.
			 
			DBG cout <<"********Dropping... \"float\" curbox, then split markedpane according to mouse position"<<endl;
			int side=-1,x,y;
			mouseposition(markedhead,&x,&y,NULL);
			DBG cout <<"********** x,y: "<<x<<','<<y<<endl;
			//DBG cout <<"********"<<markedhead->win_x<<","<<markedhead->win_y<<"  "<<
			//*** this should probably go by corner to corner, not x=y and x=-y.
			x-=markedpane->x1+(markedpane->x2-markedpane->x1)/2;
			y-=markedpane->y1+(markedpane->y2-markedpane->y1)/2;
			DBG cout <<"********** x,y: "<<x<<','<<y<<endl;
			if (y>0 && y>x && y>-x) side=LAX_BOTTOM;
			else if (y<0 && y<x && y<-x) side=LAX_TOP;
			else if (x>0 && y<x && y>-x) side=LAX_RIGHT;
			else side=LAX_LEFT;
			DBG cout <<"*******side="<<side<<endl;
			
			anXWindow *win=curbox->win;
			curbox->win=NULL;
			DBG cout <<"***********markedhead->numwindows()="<<markedhead->numwindows()<<endl;
			markedhead->Split(markedhead->FindBoxIndex(markedpane), side, win);//this reparents win
			DBG cout <<"***********markedhead->numwindows()="<<markedhead->numwindows()<<endl;
			RemoveCurbox();
			if (windows.n==1) {
				app->destroywindow(this);
			}
			DBG cout <<"*************done Dropping"<<endl;
		}
	} else { //SWAPWITH
		DBG cout <<"**************** SWAPWITH"<<endl;
		SwapWithMarked();
	}
	
	mode=0;
	DBG cout <<"***************changing mode to NORMAL"<<endl;
	return 0;
}

//! Intercept for finding a drop to or swap with location, put in markedhead and markedpane.
/*! This stores the target head and pane in markedhead and markedpane.
 */
int HeadWindow::MouseMove(int x,int y,unsigned int state)
{//***
	DBG cout <<"***********HeadWindow::MouseMove  mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::MouseMove(x,y,state);
	
	//***based on mouseposition set markedhead and markedpane
	anXWindow *win;
	mouseposition(&x,&y,NULL,&win,NULL);
	if (!win) {
		markedpane=NULL;
		markedhead=NULL;
		return 0;
	}
	while (win->win_parent!=NULL) {
		DBG cout <<"  -----"<<win->win_title<<endl;
		win=win->win_parent;
	}
	
	markedhead=dynamic_cast<HeadWindow *>(win);
	if (!markedhead) {
		markedpane=NULL;
		return 0;
	}
	mouseposition(markedhead,&x,&y,NULL,NULL,NULL);
	x=markedhead->FindBox(x,y);
	if (x<0) markedhead=NULL;
	else markedhead->Mark(x);
	DBG cout <<"***************markedpane="<<x<<endl;
	
	return 0;
}

/*! Intercept Esc to revert anything to NORMAL mode.
 */
int HeadWindow::CharInput(unsigned int ch,unsigned int state)
{
	if (ch==LAX_Esc) {
		DBG cout <<"***************changing mode to NORMAL"<<endl;
		mode=NORMAL;
		XUndefineCursor(app->dpy,window);
		XUngrabPointer(app->dpy, CurrentTime);
		needtodraw=1;
		return 0;
	} else if (ch=='o' && (state&LAX_STATE_MASK)==ControlMask) {
		app->rundialog(new FileDialog(NULL,"Open Document",
					ANXWIN_CENTER|ANXWIN_DELETEABLE|FILES_FILES_ONLY|FILES_OPEN_MANY|FILES_PREVIEW,
					0,0,500,500,0, window,"open document"));
		return 0;
		
	}
	return SplitWindow::CharInput(ch,state);
}

//! Reset mode when focus somehow leaves the window during non-normal modes.
int HeadWindow::FocusOff(XFocusChangeEvent *e)
{ //***
	DBG cout <<"**********************HeadWindow::FocusOff"<<endl;
	if (e->detail==NotifyInferior || e->detail==NotifyAncestor || e->detail==NotifyNonlinear) {
		if (mode!=SWAPWITH && mode!=DROPTO && mode!=MAXIMIZED) {
			mode=NORMAL;
			DBG cout <<"***********************UN-GRAB***********************"<<endl;
			XUngrabPointer(app->dpy, CurrentTime);
			app->Tooltips(1);
			if (mode!=NORMAL && buttondown) {
				//****uh?
			}
		}
		return anXWindow::FocusOff(e);
	}		 
	return SplitWindow::FocusOff(e);
}

//! Create split panes with names like SplitPane12, where the number is getUniqueNumber().
/*! New spread editors will be created with the same document of the most recent view.
 */
anXWindow *HeadWindow::NewWindow(const char *wtype,anXWindow *likethis)
{
	if (!wtype) 
		if (defaultwinfunc<0) return NULL;
		else wtype=winfuncs.e[defaultwinfunc]->name;
		
	anXWindow *win=NULL;
	char blah[100];
	sprintf(blah,"SplitPane%lu",getUniqueNumber());
	for (int c=0; c<winfuncs.n; c++) {
		if (!strcmp(winfuncs.e[c]->name,wtype)) {
			win=winfuncs.e[c]->function(this,blah,winfuncs.e[c]->style);
			if (!win || !likethis) return win;

			cout <<"*** need to repair HeadWindow::NewWindow"<<endl;

			// need to dump_out_atts from likethis, then if wtype is same, dump in atts,
			// else just transfer the Document...

			return win;
		}
	}
	return NULL;
}

//! Intercept when curbox changes to keep track what was the most recently focused viewer.
int HeadWindow::Curbox(int c)
{
	int cc=SplitWindow::Curbox(c);
	if (!curbox || !curbox->win) return cc;
	
	anXWindow *win=curbox->win;
	if (win && !strcmp(win->whattype(),"ViewWindow")) lastview=curbox->win;

	return cc;
}

//! Return 1 for this's ViewWindows and SpreadEditors use only doc.
int HeadWindow::HasOnlyThis(Document *doc)
{
	ViewWindow *v;
	SpreadEditor *s;
	for (int c=0; c<windows.n; c++) {
		if (!windows.e[c]->win) continue;
		v=dynamic_cast<ViewWindow *>(windows.e[c]->win);
		if (v)
			if (v->doc!=doc) return 0;
			else continue;
		s=dynamic_cast<SpreadEditor *>(windows.e[c]->win);
		if (s)
			if (s->doc!=doc) return 0;
			else continue;
	}
	return 1;
}


