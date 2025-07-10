//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef NETIMPOSITION_H
#define NETIMPOSITION_H

#include "imposition.h"
#include "../polyptych/src/nets.h"
#include "../polyptych/src/poly.h"

#define SINGLES_WITH_ADJACENT_LAYOUT 3



namespace Laidout {


//----------------------------------- NetImposition ---------------------------------
ObjectDef *makeNetImpositionObjectDef();

class NetImposition : public Imposition
{
	char *briefdesc;
 public:
	double margin = .05;
	bool margin_is_percent = true;
	int printnet;
	
	PageStyle *pagestyle;
	Polyptych::AbstractNet *abstractnet;
	double scalefromnet;
	int maptoabstractnet;
	int netisbuiltin;
	Laxkit::RefPtrStack<Polyptych::Net> nets;

	LaxInterfaces::LineStyle *line_fold;
	LaxInterfaces::LineStyle *line_peak;
	LaxInterfaces::LineStyle *line_valley;
	LaxInterfaces::LineStyle *line_cut;
	LaxInterfaces::LineStyle *line_soft_cut;
	LaxInterfaces::LineStyle *line_debug;

	//Polyhedron *polyhedron;

	NetImposition();
	NetImposition(PaperGroup *papergroup);
	NetImposition(Polyptych::Net *newnet);
	virtual ~NetImposition();
	virtual const char *whattype() { return "NetImposition"; }
	static ImpositionResource **getDefaultResources();
	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
	virtual const char *BriefDescription();
	virtual void GetDefaultPaperDimensions(double *x, double *y);
	virtual void GetDefaultPageDimensions(double *x, double *y);
	virtual ImpositionInterface *Interface();
	
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual PageStyle *GetPageStyle(int pagenum,int local);

	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local);

	virtual Spread *Layout(int layout,int which); 
	virtual int NumLayoutTypes();
	virtual const char *LayoutName(int layout); 
	virtual Spread *GenerateSpread(Spread *spread, Polyptych::Net *net, int pageoffset);
	//----------
	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *SingleLayoutWithAdjacent(int whichpage); 
	virtual Spread *PageLayout(int whichspread); 
	virtual Spread *PaperLayout(int whichpaper);

	virtual int NumSpreads(int layout); 
	virtual int NumPageSpreads();
	virtual int NumPages(int npages);
	virtual int PaperFromPage(int pagenumber);
	virtual int SpreadFromPage(int pagenumber);
	virtual int SpreadFromPage(int layout, int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int GetNumInPaperGroupForSpread(int layout, int spread);

	virtual int NumPageTypes();
	virtual const char *PageTypeName(int pagetype);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	
	 //new for this class:
	virtual Polyptych::AbstractNet *AbstractNetFromFile(const char *filename);
	virtual int SetNetFromFile(const char *file);
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Polyptych::Net *newnet);
	virtual const char *NetImpositionName();
	virtual void setPage();
	virtual int numActiveFaces();
	virtual int numActiveNets();
};

} // namespace Laidout

#endif

