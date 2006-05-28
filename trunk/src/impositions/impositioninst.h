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
#ifndef IMPOSITIONINST_H
#define IMPOSITIONINST_H

#include "imposition.h"

//------------------------ Singles ---------------------------

class Singles : public Imposition
{
  public:
	double insetl,insetr,insett,insetb;
	int tilex,tiley; 
	RectPageStyle *pagestyle;
	Singles();
	virtual ~Singles();
	virtual StyleDef *makeStyleDef();
	
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages();
	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local);
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);
	
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

//------------------------ Double Sided Singles ---------------------------

	//***??virtual int SyncPages(Document *doc,int start,int n);
class DoubleSidedSingles : public Singles
{
 public:
	int isvertical,isleft;
	RectPageStyle *pagestyler;
	DoubleSidedSingles();
	virtual StyleDef *makeStyleDef();
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages(); // create necessary pages based on default pagestyle
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Style *duplicate(Style *s=NULL);
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual void setPage();
	virtual int GetSpreadsNeeded(int npages);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

////------------------------ Booklet ---------------------------
//
class BookletImposition : public DoubleSidedSingles
{
 public:
	double creep;
	unsigned long covercolor;
	unsigned long bodycolor;
	BookletImposition();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	
//	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; }
	virtual Spread *PaperLayout(int whichpaper);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


////------------------------ Basic Book ---------------------------
//
//class BasicBook : public Imposition
//{
// public:
//	int numsections;
//	int paperspersection; // 0 does not mean 0. 0 means infinity (or MAX_INT).
//	int creeppersection; 
//	int insetl,insetr,insett,insetb;
//	int tilex,tiley;
//	unsigned long bodycolor;
//	
//	char specifycover;
//	int spinewidth;
//	unsigned long covercolor;
//	PaperStyle coverpaper;
//	PageStyle coverpage;
//	virtual Style *duplicate(Style *s=NULL);
//	
//	virtual int GetPagesNeeded(int npapers);
//	virtual int GetPapersNeeded(int npages);
//	virtual int GetSpreadsNeeded(int npages);
//
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

////---------------------------------- CompositeImposition ----------------------------
//****
//
//

////---------------------------------- WhatupImposition ----------------------------
//****
//
//



#endif

