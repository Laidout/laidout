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
#ifndef NETIMPOSITION_H
#define NETIMPOSITION_H

#include "imposition.h"
#include "nets.h"
#include "poly.h"

#define SINGLES_WITH_ADJACENT_LAYOUT 3


//----------------------------------- NetImposition ---------------------------------
StyleDef *makeNetImpositionStyleDef();

class NetImposition : public Imposition
{
	char *briefdesc;
 public:
	PageStyle *pagestyle;
	AbstractNet *abstractnet;
	double scalefromnet;
	Laxkit::RefPtrStack<Net> nets;
	int maptoabstractnet;
	int printnet;
	int netisbuiltin;

	Polyhedron *polyhedron;

	NetImposition(Net *newnet=NULL);
	virtual ~NetImposition();
	virtual const char *whattype() { return "NetImposition"; }
	static ImpositionResource **getDefaultResources();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	virtual const char *BriefDescription();
	virtual void GetDimensions(int which, double *x, double *y);
	
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual Page **CreatePages(int npages);

	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local);

	virtual Spread *Layout(int layout,int which); 
	virtual int NumLayouts();
	virtual const char *LayoutName(int layout); 
	virtual Spread *GenerateSpread(Spread *spread, Net *net, int pageoffset);
	//----------
	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *SingleLayoutWithAdjacent(int whichpage); 
	virtual Spread *PageLayout(int whichspread); 
	virtual Spread *PaperLayout(int whichpaper);

	virtual int NumSpreads(int layout); 
	virtual int NumPages();
	virtual int NumPages(int npages);
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
	
	 //new for this class:
	virtual AbstractNet *AbstractNetFromFile(const char *filename);
	virtual int SetNetFromFile(const char *file);
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Net *newnet);
	virtual const char *NetImpositionName();
	virtual void setPage();
	virtual int numActiveFaces();
	virtual int numActiveNets();
};

#endif

