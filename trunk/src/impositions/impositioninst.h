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

//------------------------ Singles ---------------------------

StyleDef *makeSinglesStyleDef();

class Singles : public Imposition
{
  public:
	double insetl,insetr,insett,insetb;
	int tilex,tiley; 
	RectPageStyle *pagestyle;

	Singles();
	virtual ~Singles();
	static ImpositionResource **getDefaultResources();

	virtual const char *BriefDescription();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetDefaultMargins(double l,double r,double t,double b);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages();
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
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int NumPageTypes();
	virtual const char *PageTypeName(int pagetype);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	 //-- extra functions for this class:
	virtual void setPage();
};




#endif

