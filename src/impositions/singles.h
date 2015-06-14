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
#ifndef IMPOSITIONINST_H
#define IMPOSITIONINST_H

#include "imposition.h"



namespace Laidout {

//------------------------ Singles ---------------------------

ObjectDef *makeSinglesObjectDef();

class Singles : public Imposition
{
  public:
	double insetleft,insetright,insettop,insetbottom;
	double marginleft,marginright,margintop,marginbottom;
	double gapx,gapy;
	int tilex,tiley; 
	RectPageStyle *pagestyle;

	Singles();
	virtual ~Singles();
	virtual const char *whattype() { return "Singles"; }
	static ImpositionResource **getDefaultResources();

	// ***********TEMP!!!
    virtual int inc_count();
    virtual int dec_count();
    // ***********end TEMP!!!

	virtual void GetDimensions(int which, double *x, double *y);
	virtual const char *BriefDescription();
	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetDefaultMargins(double l,double r,double t,double b);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages(int npages);
	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local);
	virtual LaxInterfaces::SomeData *GetPageMarginOutline(int pagenum,int local);
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual int PaperFromPage(int pagenumber);
	virtual int SpreadFromPage(int pagenumber);
	virtual int SpreadFromPage(int layout, int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int NumPageTypes();
	virtual const char *PageTypeName(int pagetype);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);
	virtual ImpositionInterface *Interface();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	 //-- extra functions for this class:
	virtual void setPage();
};



} // namespace Laidout

#endif

