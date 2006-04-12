//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include "headwindow.h"
#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"
#include "helpwindow.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

//---------------------------- HeadWindow Pane Generators -----------------------

/*! \defgroup mainwindows Main Pane Windows For HeadWindows
 *
 * These become the panes of a HeadWindow.
 */

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
	return new HelpWindow();
	//return new HelpWindow(parnt,ntitle,style, 0,0,0,0,1, NULL,NULL);
}

//---------------------------- newHeadWindow() -----------------------
/*! \ingroup mainwindows
 * \brief Create a new head split window, and fill it with available main windows.
 *
 * Current main windows are:
 * <pre>
 *  ViewWindow
 *  SpreadEditor
 *  HelpWindow
 *
 *   TODO:
 *  Icons/Menus with optional dialogs ***
 *  Directory Window? FileOpener? ImageFileOpener->to drag n drop images? ***not imp
 *  PlainTextEditor ***not imp
 *  StoryEditor *** not imp
 *  InterpreterConsole *** not imp, future:python, other?
 *  StyleManager ***not imp
 *  ObjectTreeEditor ***not imp
 * </pre>
 *
 * \todo *** might want to remove doc, or have some more general way to start what is in which
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
	
	 // add the window generator funcs
	head->AddWindowType("ViewWindow","View Window",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newViewWindowFunc,1);
	head->AddWindowType("SpreadEditor","Spread Editor",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newSpreadEditorFunc,0);
	head->AddWindowType("HelpWindow","Help Window",ANXWIN_LOCAL_ACTIVE|ANXWIN_DELETEABLE,newHelpWindowFunc,0);

	 // put a new which in it. default to view
	if (which) head->Add(which);
	else head->Add(new ViewWindow(doc));//***

	return head;
}

//------------------------------- HeadWindow ---------------------------------------
/*! \class HeadWindow
 * \brief Top level windows to hold other stuff such as a ViewWindow.
 */  
//class HeadWindow : public Laxkit::SplitWindow
//{
// public:
// 	HeadWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
// 		int xx,int yy,int ww,int hh,int brder);
// 	virtual const char *whattype() { return "HeadWindow"; }
//	virtual ~HeadWindow();
//	virtual int init();
//	virtual MenuInfo *GetMenu();
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//	virtual Laxkit::anXWindow *NewWindow(const char *wtype);
//	virtual void WindowGone(Laxkit::anXWindow *win);
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
}

HeadWindow::~HeadWindow()
{
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

