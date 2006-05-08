//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
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

//! Create a new head window based on att.
/*! \todo *** should return NULL if the att was invalid
 */
anXWindow *newHeadWindow(LaxFiles::Attribute *att)
{
	HeadWindow *head=new HeadWindow(NULL,"head",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE, 0,0,500,500,0);
	head->dump_in_atts(att);
	return head;
}

//------------------------------- HeadWindow ---------------------------------------
/*! \class HeadWindow
 * \brief Top level windows to hold other stuff such as a ViewWindow.
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
//class HeadWindow : public Laxkit::SplitWindow, public LaxFiles::DumpUtility
//{
// public:
//	Laxkit::anXWindow *lastview, *lastedit;
// 	HeadWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
// 		int xx,int yy,int ww,int hh,int brder);
// 	virtual const char *whattype() { return "HeadWindow"; }
//	virtual ~HeadWindow();
//	virtual int init();
//	virtual MenuInfo *GetMenu();
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//	virtual Laxkit::anXWindow *NewWindow(const char *wtype);
//	virtual void WindowGone(Laxkit::anXWindow *win);
//	virtual int Curbox(int c);
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

//! Constructor.
HeadWindow::HeadWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: SplitWindow(parnt,ntitle,nstyle|SPLIT_WITH_SAME|SPLIT_BEVEL,xx,yy,ww,hh,brder)
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
}

//! Empty destructor.
HeadWindow::~HeadWindow()
{
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
		fprintf(f,"%s  xywh %d %d %d %d\n",
					spc,windows.e[c]->x,windows.e[c]->y,windows.e[c]->w,windows.e[c]->h);
		if (windows.e[c]->win) {
			fprintf(f,"%s  window %s\n",spc,windows.e[c]->win->whattype());
			wind=dynamic_cast<DumpUtility *>(windows.e[c]->win);
			if (wind) wind->dump_out(f,indent+4,what);
		}
	}
}

//! Set up the window according to windows listed in att.
/*! \todo *** as time goes on, must ensure that header can deal with new
 * types of windows...
 */
void HeadWindow::dump_in_atts(Attribute *att)
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
		} else if (!strcmp(name,"pane")) {
			box=new PlainWinBox(NULL,0,0,0,0);
			for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"xywh")) {
					c3=IntListAttribute(value,i,4);
					if (c3>0) box->x=i[0];
					if (c3>1) box->y=i[1];
					if (c3>2) box->w=i[2];
					if (c3>3) box->h=i[3];
				} else if (!strcmp(name,"window")) {
					win=NewWindow(value);
					if (win) {
						wind=dynamic_cast<DumpUtility *>(win);
						if (wind) wind->dump_in_atts(att->attributes.e[c]);
						box->win=win;
					} else {
						DBG cout <<"**** *** warning: window func not found for "<<(value?value:"(unknown)")<<endl;
					}
				}
			}
			box->sync(1);
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
		if (windows.e[c]->win==win) windows.e[c]->win=NULL;
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
 */
MenuInfo *HeadWindow::GetMenu()
{
	//*** must make something like:
	// Split
	// Join
	// Split...
	// Change to >
	// Float
	// Stack on >
	// ------
	// (other things defined by curbox?...
	// 
	return SplitWindow::GetMenu();
}

/*! \todo *** handling of docTreeChange message is rather silly. there must be
 * a more responsible way to make sure that all windows are synchronized properly
 * to the project/document/data.
 */
int HeadWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
	DBG cout <<"HeadWindow got message: "<<mes<<endl;
	if (!strcmp(mes,"docTreeChange")) {
		ViewWindow *view;
		SpreadEditor *s;
		XEvent ee;
		ee.xclient.type=ClientMessage;
		ee.xclient.display=app->dpy;
		ee.xclient.message_type=XInternAtom(app->dpy,"docTreeChange",False);
		ee.xclient.format=32;
		ee.xclient.data.l[0]=e->data.l[0];
		ee.xclient.data.l[1]=e->data.l[1];
		ee.xclient.data.l[2]=e->data.l[2];
		ee.xclient.data.l[3]=e->data.l[3];
		ee.xclient.data.l[4]=e->data.l[4];
	  
		int yes=0;
		for (int c=0; c<windows.n; c++) {
			//if (callfrom==windows.e[c]->win) continue; //*** this hardly has effect as most are children of top
			view=dynamic_cast<ViewWindow *>(windows.e[c]->win);
			if (view) yes=1;
			else if (s=dynamic_cast<SpreadEditor *>(windows.e[c]->win), s) yes=1;

			if (yes){
				ee.xclient.window=windows.e[c]->win->window;
				XSendEvent(app->dpy,windows.e[c]->win->window,False,0,&ee);
				DBG cout <<"---sending docTreeChange to "<<windows.e[c]->win->win_title<<endl;
				yes=0;
			}
		}
	} //else if (!strcmp(mes,"paper name")) { 
	//}
	return SplitWindow::ClientEvent(e,mes);
}

//! Create split panes with names like SplitPane12, where the number is getUniqueNumber().
/*! New spread editors will be created with the same document of the most recent view.
 */
anXWindow *HeadWindow::NewWindow(const char *wtype)
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
	if (!strcmp(win->whattype(),"ViewWindow")) lastview=curbox->win;

	return cc;
}


