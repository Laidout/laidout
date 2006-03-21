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

//int HeadWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
//{//***
//	DBG cout <<"HeadWindow got message: "<<mes<<endl;
//	if (!strcmp(mes,"paper size")) {
//	} else if (!strcmp(mes,"paper name")) { 
//	}
//	return 1;
//}


