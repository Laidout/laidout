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
// Copyright (C) 2004-2010 by Tom Lechner
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

#define NEWDOC_EDIT

class NewDocWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual void sendNewDoc();
 public:
	int curorientation;
	 // the names of each, so to change Left->Inside, Top->Inside (like calender), etc
	const char *margintextl,*margintextr,*margintextt,*margintextb; 
	Imposition *imp;
	Document *doc;
	PaperStyle *papertype;
	
	Laxkit::PtrStack<PaperStyle> *papersizes;
	Laxkit::StrSliderPopup *impsel;
	Laxkit::LineEdit *lineedit;
	Laxkit::LineInput *marginl,*marginr,*margint,*marginb;
	Laxkit::LineInput *insetl,*insetr,*insett,*insetb;
	Laxkit::LineInput *saveas,*paperx,*papery,*numpages,*tilex,*tiley,*impfromfile;
	Laxkit::MessageBar *mesbar;
	Laxkit::CheckBox *defaultpage,*custompage;
 	NewDocWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,Document *ndoc=NULL);
	virtual ~NewDocWindow();
	virtual const char *whattype() { return "NewDocWindow"; }
	virtual int preinit();
	virtual int init();
//	virtual int Refresh();
//	virtual int CharInput(unsigned int ch,unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);

	void updateImposition();
};

class NewProjectWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual int sendNewProject();
	Laxkit::LineEdit *projectdir,*projectfile;
	Laxkit::CheckBox *useprojectdir;
 public:
 	NewProjectWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder);
	virtual ~NewProjectWindow();
	virtual const char *whattype() { return "NewProjectWindow"; }
	virtual int preinit();
	virtual int init();
//	virtual int Refresh();
//	virtual int CharInput(unsigned int ch,unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);

	int UpdateOkToCreate();
};

Laxkit::anXWindow *BrandNew(int which=0);

#endif

