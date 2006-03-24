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
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


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
//};

HeadWindow::HeadWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: SplitWindow(parnt,ntitle,nstyle|SPLIT_WITH_SAME,xx,yy,ww,hh,brder)
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
}

HeadWindow::~HeadWindow()
{
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
		XEvent e;
		e.xclient.type=ClientMessage;
		e.xclient.display=app->dpy;
		e.xclient.message_type=XInternAtom(app->dpy,"docTreeChange",False);
		e.xclient.format=32;
	  
		int yes=0;
		for (int c=0; c<windows.n; c++) {
			//if (callfrom==windows.e[c]->win) continue; //*** this hardly has effect as most are children of top
			view=dynamic_cast<ViewWindow *>(windows.e[c]->win);
			if (view) yes=1;
			else if (s=dynamic_cast<SpreadEditor *>(windows.e[c]->win), s) yes=1;

			if (yes){
				e.xclient.window=windows.e[c]->win->window;
				XSendEvent(app->dpy,windows.e[c]->win->window,False,0,&e);
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

