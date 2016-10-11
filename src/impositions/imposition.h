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
// Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef IMPOSITION_H
#define IMPOSITION_H

#include <lax/interfaces/somedata.h>
#include <lax/lists.h>

#include "../page.h"
#include "../calculator/values.h"
#include "../papersizes.h"
#include "../dataobjects/objectcontainer.h"

#include "../document.h"



namespace Laidout {


class Document;
class Imposition;
class Spread;

//------------------------- PageLocation --------------------------------------

class PageLocation
{	
 public:
	PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *trans);
	~PageLocation();
	int info;
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

class PageLocationStack : public Laxkit::PtrStack<PageLocation>, public ObjectContainer
{
  public:
	PageLocationStack() { obj_flags|=OBJ_Unselectable|OBJ_Zone; }
	virtual ~PageLocationStack() {}
	virtual int n() { return Laxkit::PtrStack<PageLocation>::n; }
	virtual Laxkit::anObject *object_e(int i) { if (i>=0 && i<Laxkit::PtrStack<PageLocation>::n) return e[i]->page; return NULL; }
	virtual const char *object_e_name(int i) { return "page"; }
	virtual const double *object_transform(int i) {  if (i>=0 && i<Laxkit::PtrStack<PageLocation>::n) return e[i]->outline->m(); return NULL; }
};

class Spread : public ObjectContainer
{
 public:
	unsigned int mask; // which of path,min,max,pages is defined
	unsigned int style; // says what is the type of thing this spread refers to. See Imposition.
	int spreadtype;

	Document *doc;
	PaperGroup *papergroup;

	flatpoint minimum,maximum; //are in path coordinates, useful for littlespreads in Spread editor
	LaxInterfaces::SomeData *path;
	LaxInterfaces::SomeData *marks; //automatic objects for the spread
	//Group spreadobjects; // for custom objects not on a page, or other imposition specific objects
	
	PageLocationStack pagestack;

	Spread();
	virtual ~Spread();
	virtual int *pagesFromSpread();
	virtual char *pagesFromSpreadDesc(Document *doc);
	virtual int PagestackIndex(int docpage);

	virtual int n();
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);
};


//------------------------------------- ImpositionInterface --------------------------------------
class ImpositionInterface : public LaxInterfaces::anInterface
{               
  public:   
    ImpositionInterface() {}
    ImpositionInterface(LaxInterfaces::anInterface *nowner,int nid,Laxkit::Displayer *ndp);
    virtual ~ImpositionInterface() {}
    virtual const char *ImpositionType() = 0;
    virtual Imposition *GetImposition() = 0;
    virtual int SetTotalDimensions(double width, double height) = 0;
    virtual int GetDimensions(double &width, double &height) = 0;
    virtual int SetPaper(PaperStyle *paper) = 0;
    virtual int UseThisDocument(Document *doc) =0;
    virtual int UseThisImposition(Imposition *imp) =0;

    virtual int ShowThisPaperSpread(int index) = 0;
    virtual void ShowSplash(int yes) = 0;
}; 


//------------------------------------- ImpositionWindow --------------------------------------
class ImpositionWindow
{               
  public:   
    ImpositionWindow() {}
    virtual ~ImpositionWindow() {}
    virtual const char *ImpositionType() = 0;
    virtual Imposition *GetImposition() = 0;
    virtual int UseThisDocument(Document *doc) =0;
    virtual int UseThisImposition(Imposition *imp) =0;
    virtual void ShowSplash(int yes) = 0;
}; 


//----------------------------- Imposition --------------------------

class Imposition : public Value
{
  public:
	char *name;
	char *description;

	int numpapers; 
	int numpages;
	int numdocpages;
	Document *doc;
	PaperGroup *papergroup;
	PaperBox *paper;

	Imposition(const char *nsname);
	virtual ~Imposition();
	virtual const char *Name();

	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual const char *BriefDescription() = 0;
	virtual void GetDimensions(int paperorpage, double *x, double *y) = 0;

	virtual PaperStyle *GetDefaultPaper();
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetPaperGroup(PaperGroup *ngroup);
	virtual PageStyle *GetPageStyle(int pagenum,int local) = 0;
	
	virtual Page **CreatePages(int npages) = 0;
	virtual int SyncPageStyles(Document *doc,int start,int n, bool shift_within_margins);
	
	virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; }
	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local) = 0; // return outline of page in page coords
	virtual LaxInterfaces::SomeData *GetPageMarginOutline(int pagenum,int local);
	
	virtual Spread *Layout(int layout,int which); 
	virtual int NumLayoutTypes();
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
	virtual int NumPages() { return numpages; }
	virtual int NumPages(int npages);

	virtual int PaperFromPage(int pagenumber) = 0;
	virtual int SpreadFromPage(int layout, int pagenumber) = 0;
	virtual int GetPagesNeeded(int npapers) = 0;
	virtual int GetPapersNeeded(int npages) = 0;
	virtual int GetSpreadsNeeded(int npages) = 0;
	
	virtual int NumPageTypes() = 0;
	virtual const char *PageTypeName(int pagetype) = 0;
	virtual int PageType(int page) = 0;
	virtual int SpreadType(int spread) = 0;

	virtual ImpositionInterface *Interface() = 0;
};


//---------------------------------- ImpositionResource ----------------------------------

class ImpositionResource
{
 public:
	char *name; //imposition instance name, not class name
	char *impositionfile;
	char *description;
	char *objectdef;
	LaxFiles::Attribute *config;
	char configislocal;
	ImpositionResource(const char *sdef,const char *nname, const char *file, const char *desc,
					   LaxFiles::Attribute *conf,int local);
	~ImpositionResource();
	Imposition *Create();
};


//--------------------- GetBuiltinImpositionPool ------------------------------

 // These functions are defined in impositions.cc
Laxkit::PtrStack<ImpositionResource> *GetBuiltinImpositionPool(Laxkit::PtrStack<ImpositionResource> *existingpool=NULL);
int AddToImpositionPool(Laxkit::PtrStack<ImpositionResource> *existingpool, const char *directory);
Imposition *newImpositionByResource(const char *impos);
Imposition *newImpositionByType(const char *impos);
void dumpOutImpositionTypes(FILE *f,int indent);


} // namespace Laidout


#endif

