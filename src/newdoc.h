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
#include <lax/sliderpopup.h>
#include <lax/button.h>
#include <lax/messagebar.h>
#include <lax/colorbox.h>

#include "laidout.h"
#include "papersizes.h"


namespace Laidout {


#define NEWDOC_EDIT

class NewDocWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual void sendNewDoc();
	const char *pagesDescription(int updatetoo);
 public:
	int curorientation;
	 // the names of each, so to change Left->Inside, Top->Inside (like calender), etc
	Imposition *imp;
	Document *doc;
	PaperStyle *papertype;
	int oldimp;
	
	Laxkit::PtrStack<PaperStyle> *papersizes;
	Laxkit::SliderPopup *impsel;
	Laxkit::LineEdit *lineedit;
	Laxkit::LineInput *saveas,*paperx,*papery,*numpages,*impfromfile;
	Laxkit::MessageBar *impmesbar, *pageinfo;
	Laxkit::CheckBox *defaultpage,*custompage;

 	NewDocWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,Document *ndoc=NULL);
	virtual ~NewDocWindow();
	virtual const char *whattype() { return "NewDocWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	int UseThisImposition(Imposition *imp);
	void impositionFromFile(const char *file);
	void UpdatePaper(int dialogtoimp);
};

class NewProjectWindow : public Laxkit::RowFrame
{
	int mx,my;
	virtual int sendNewProject();
	Laxkit::LineEdit *projectdir,*projectfile;
	Laxkit::CheckBox *useprojectdir;
 public:
 	NewProjectWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder);
	virtual ~NewProjectWindow();
	virtual const char *whattype() { return "NewProjectWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	int UpdateOkToCreate();
};

Laxkit::anXWindow *BrandNew(int which=0);

} //namespace Laidout

#endif

