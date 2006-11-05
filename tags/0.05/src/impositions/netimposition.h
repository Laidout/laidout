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
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages();

	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local);

	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *SingleLayoutWithAdjacent(int whichpage); 
	virtual Spread *PageLayout(int whichspread); 
	virtual Spread *PaperLayout(int whichpaper);

	virtual int PaperFromPage(int pagenumber);
	virtual int SpreadFromPage(int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int *PrintingPapers(int frompage,int topage);

	virtual int PageType(int page);
	virtual int SpreadType(int spread);

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
	
	 //new for this class:
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Net *newnet);
	virtual void setPage();
};

#endif

