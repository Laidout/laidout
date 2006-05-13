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
#ifndef IMPOSITION_H
#define IMPOSITION_H

#include <lax/interfaces/somedata.h>
#include <lax/lists.h>

#include "../page.h"
#include "../styles.h"
#include "../papersizes.h"
#include "../objectcontainer.h"

class Imposition;
#include "../document.h"

//------------------------- PageLocation --------------------------------------

class PageLocation
{	
 public:
	PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *trans,int outlineislocal,void **natts);
	~PageLocation();
	int index;
	Page *page;
	LaxInterfaces::SomeData *outline;
	int outlineislocal;
	void **attributes;
};

//----------------------- Spread -------------------------------

#define SPREAD_PAPER            (1<<0)
#define SPREAD_PAPER_OUTLINE    (1<<1)

#define SPREAD_PAGE             (1<<2)
#define SPREAD_PAGE_OUTLINE     (1<<3)
#define SPREAD_MARGIN_OUTLINE   (1<<4)

#define SPREAD_LITTLESPREAD     (1<<5)
#define SPREAD_PRINTERMARKS     (1<<6)

#define SPREAD_PATH             (1<<7)
#define SPREAD_MINIMUM          (1<<8)
#define SPREAD_MAXIMUM          (1<<9)
#define SPREAD_PAGES            (1<<10)

class Spread : public ObjectContainer
{
 public:
	unsigned int mask; // which of path,min,max,pages is defined
	unsigned int style; // says what is the type of thing this spread refers to. See Imposition.
	LaxInterfaces::SomeData *path;
	int pathislocal;
	LaxInterfaces::SomeData *marks;
	int marksarelocal;
	flatpoint minimum,maximum; //are in path coordinates, useful for littlespreads in Spread editor
	
	Laxkit::PtrStack<PageLocation> pagestack;

	Spread();
	virtual ~Spread();
	virtual int *pagesFromSpread();
	virtual int n() { return pagestack.n; }
	virtual Laxkit::anObject *object_e(int i) { if (i>=0 && i<pagestack.n) return pagestack.e[i]->page; return NULL; }
};


//----------------------------- Imposition --------------------------

class Imposition : public Style
{
  public:
	int numpapers; 
	int numspreads;
	int numpages;
	PaperType *paperstyle; // assumed to be local
	PageStyle *pagestyle; //assumed to be local
	
	Imposition(const char *nsname);
	virtual ~Imposition() {}
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPageLikeThis(PageStyle *npage); // npage->duplicate(), doesnt transfer pointer
	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
	
//	virtual void AdjustPages(Page **pages) {} // when changing page size and atts, return bases for the new pages
	virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
	virtual Page **CreatePages(PageStyle *pagestyle=NULL) = 0; // create necessary pages based on default pagestyle
	virtual int SyncPages(Document *doc,int start,int n);
	
	virtual LaxInterfaces::SomeData *GetPaper(int papernum,int local); // return outline of paper in paper coords
	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local) = 0; // return outline of page in page coords
	
	virtual Spread *GetLittleSpread(int whichpage) = 0; 
	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *PageLayout(int whichpage) = 0; 
	virtual Spread *PaperLayout(int whichpaper) = 0;
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL) = 0;
	virtual int *PrintingPapers(int frompage,int topage) = 0;

	virtual int NumPapers(int npapers);
	//virtual int NumSpreads(int nspreads);
	virtual int NumPages(int npages);
	virtual int PaperFromPage(int pagenumber) = 0; // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers) = 0; // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages) = 0; // how many papers needed to contain n pages
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
};



//--------------------- GetBuiltinImpositionPool ------------------------------

 // These functions are defined in impositions.cc
Laxkit::PtrStack<Imposition> *GetBuiltinImpositionPool(Laxkit::PtrStack<Imposition> *existingpool=NULL);
Imposition *newImposition(const char *impos);

#endif

