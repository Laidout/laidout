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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef IMPOSITION_H
#define IMPOSITION_H

#include <lax/interfaces/somedata.h>
#include <lax/lists.h>

#include "../page.h"
#include "../styles.h"
#include "../papersizes.h"
#include "../dataobjects/objectcontainer.h"

class Imposition;
class Spread;
#include "../document.h"

//------------------------- PageLocation --------------------------------------

class PageLocation
{	
 public:
	PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *trans);
	~PageLocation();
	int index;
	Page *page;
	LaxInterfaces::SomeData *outline;
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
	int spreadtype;
	PaperGroup *papergroup;
	LaxInterfaces::SomeData *path;
	LaxInterfaces::SomeData *marks;
	flatpoint minimum,maximum; //are in path coordinates, useful for littlespreads in Spread editor
	
	Laxkit::PtrStack<PageLocation> pagestack;
	//Group spreadobjects; *** for paper objects not on a page, or other imposition specific objects

	Spread();
	virtual ~Spread();
	virtual int *pagesFromSpread();
	virtual char *pagesFromSpreadDesc(Document *doc);
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
	Document *doc;
	PaperGroup *papergroup;
	PaperBox *paper;
	
	Imposition(const char *nsname);
	virtual ~Imposition();
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetPaperGroup(PaperGroup *ngroup);
	virtual PageStyle *GetPageStyle(int pagenum,int local) = 0;
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(Laxkit::DoubleBBox *bbox=NULL);
	
//	virtual void AdjustPages(Page **pages) {} // when changing page size and atts, return bases for the new pages
	virtual Page **CreatePages() = 0;
	virtual int SyncPageStyles(Document *doc,int start,int n);
	
	virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; }
	//virtual LaxInterfaces::SomeData *GetPaper(int papernum,int local); // return outline of paper in paper coords
	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local) = 0; // return outline of page in page coords
	
	virtual Spread *Layout(int layout,int which); 
	virtual int NumLayouts();
	virtual const char *LayoutName(int layout); 
	//----*** ^^ this will ultimately replace these vv
	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *PageLayout(int whichspread) = 0; 
	virtual Spread *PaperLayout(int whichpaper) = 0;
	virtual Spread *GetLittleSpread(int whichspread) { return PageLayout(whichspread); } 
	
	virtual int NumSpreads(int layout); 
	//----*** ^^ this will ultimately replace these vv
	virtual int NumPapers() { return numpapers; } 
	virtual int NumPapers(int npapers);
	virtual int NumSpreads() { return numspreads; }
	virtual int NumPages() { return numpages; }
	virtual int NumPages(int npages);

	virtual int PaperFromPage(int pagenumber) = 0;
	virtual int SpreadFromPage(int layout, int pagenumber) = 0;
	virtual int GetPagesNeeded(int npapers) = 0;
	virtual int GetPapersNeeded(int npages) = 0;
	virtual int GetSpreadsNeeded(int npages) = 0;
	virtual int *PrintingPapers(int frompage,int topage) = 0;
	
	virtual int PageType(int page)=0;
	virtual int SpreadType(int spread)=0;

	virtual LaxInterfaces::InterfaceWithDp *Interface(int layouttype);
};



//--------------------- GetBuiltinImpositionPool ------------------------------

 // These functions are defined in impositions.cc
Laxkit::PtrStack<Imposition> *GetBuiltinImpositionPool(Laxkit::PtrStack<Imposition> *existingpool=NULL);
Imposition *newImposition(const char *impos);

#endif

