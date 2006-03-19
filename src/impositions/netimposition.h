//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
	Net *net;
	int netisbuiltin;
	int printnet;

	NetImposition(Net *newnet=NULL);
	virtual ~NetImposition();
	virtual Style *duplicate(Style *s=NULL);
	
	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
	
	//virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle

	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local); // return outline of page in page coords

	virtual Spread *GetLittleSpread(int whichpage); 
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);

//	virtual int NumPapers(int npapers);
//	virtual int NumPages(int npages);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Net *newnet);
	virtual void setPage();
};

#endif

