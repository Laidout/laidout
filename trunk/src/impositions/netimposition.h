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
#ifndef NETIMPOSITION_H
#define NETIMPOSITION_H

#include "imposition.h"
#include "nets.h"

class NetImposition : public Imposition
{
 public:
	PageStyle *pagestyle;
	Net *net;
	int netisbuiltin;
	int printnet;

	NetImposition(Net *newnet=NULL);
	virtual ~NetImposition();
	virtual Style *duplicate(Style *s=NULL);
	
	virtual int SetPaperSize(PaperStyle *npaper); // set paperstyle, and compute page size
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages();

	//virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; }

	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local); // return outline of page in page coords

	virtual Spread *GetLittleSpread(int whichpage); 
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual int *PrintingPapers(int frompage,int topage);

//	virtual int NumPapers(int npapers);
//	virtual int NumPages(int npages);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual int GetSpreadsNeeded(int npages);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
	
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Net *newnet);
	virtual void setPage();
};

#endif

