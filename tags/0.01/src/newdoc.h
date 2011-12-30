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
#ifndef NEWDOC_H
#define NEWDOC_H

#include <lax/rowframe.h>
#include <lax/lineedit.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/menuselector.h>
#include <lax/strsliderpopup.h>
#include <lax/textbutton.h>
#include <lax/mesbar.h>
#include <lax/colorbox.h>

#include "laidout.h"
#include "papersizes.h"

class NewDocWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual void sendNewDoc();
 public:
	int curorientation;
	 // the names of each, so to change Left->Inside, Top->Inside (like calender), etc
	const char *marginl,*marginr,*margint,*marginb; 
	Disposition *disp;
	PaperType *papertype;
	
	Laxkit::PtrStack<PaperType> *papersizes;
	Laxkit::StrSliderPopup *dispsel;
	Laxkit::LineEdit *lineedit;
	Laxkit::LineInput *saveas,*paperx,*papery,*numpages;
	Laxkit::MessageBar *mesbar;
	Laxkit::CheckBox *defaultpage,*custompage;
 	NewDocWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder);
	virtual ~NewDocWindow();
	virtual int init();
//	virtual int Refresh();
//	virtual int CharInput(char ch,unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
};


#endif
